#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include <math.h>
#include "JASA_scheduler.h"

// ===== TFT Setup =====
TFT_eSPI tft = TFT_eSPI();

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;      // adjust for your timezone
const int   daylightOffset_sec = 3600; // daylight saving offset



// ===== Wi-Fi & MQTT Settings =====
const char* ssid = "jasadeko";
const char* password = "subaru72";
const char* mqtt_server = "192.168.2.219";

const char* ha_server = "http://192.168.2.68:8123";
const char* entity_id = "sensor.nordpool_kwh_fi_eur_3_10_0255";
String token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhMWIwMjdjOTc3NmU0YWQxYjliYzU4ZjBhNjg4ZmU3ZiIsImlhdCI6MTc1NzI1Njg0NCwiZXhwIjoyMDcyNjE2ODQ0fQ.u60X3TNzUHQKRS3QSdLu1qoS-xXNTSMqAIdpnKnEb5M";  // from HA profile

WiFiClient espClient;

JASA_Scheduler el_arc(10000);

int currentHour;


// ===== Button Struct =====
struct Button {
  int x, y, w, h;
  String text;
  uint16_t color;
};

Button btnOpen = {20, 150, 90, 60, "OPEN", TFT_GREEN};
Button btnOpen_pressed = {20, 150, 90, 60, "OPEN", TFT_BLUE};
Button btnClose = {130, 150, 90, 60, "CLOSE", TFT_RED};
Button btnClose_pressed = {130, 150, 90, 60, "CLOSE", TFT_BLUE};

void drawButton(Button btn) {
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, btn.color);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, btn.color);
  tft.drawCentreString(btn.text, btn.x + btn.w / 2, btn.y + 20, 2);
}

void drawButton_pressed(Button btn) {
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, btn.color);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, btn.color);
  tft.drawCentreString(btn.text, btn.x + btn.w / 2, btn.y + 20, 2);
}

bool isTouchInButton(int tx, int ty, Button btn) {
  return (tx > btn.x && tx < (btn.x + btn.w) && ty > btn.y && ty < (btn.y + btn.h));
}

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}



void setup() {
  Serial.begin(115200);

  // Touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  // TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  drawButton(btnOpen);
  drawButton(btnClose);

  // Wi-Fi + MQTT
  connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
}

void waitUntilNotTouch() {
  while (touchscreen.tirqTouched() && touchscreen.touched()) {
    delay(20);
  }
}

void TFT_dashline(int x0,int y0, int x1, int dashLength, int gapLength) {
  for (int x = x0; x < x1; x += dashLength + gapLength) {
            int xEnd = x + dashLength;
            if (xEnd > x1) xEnd = x1;
            tft.drawLine(x, y0, xEnd, y0, TFT_GREEN);
  }
}

void drawCurrentDay() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(ha_server) + "/api/states/" + entity_id;
    http.begin(url);
    http.addHeader("Authorization", "Bearer " + token);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();

    int h_index = 0;
    int index = 0;
    int tomorrow_exists = 0;
    
    if (httpCode == 200) {
      String payload = http.getString();

      // Allocate JSON doc (increase size if needed)
      DynamicJsonDocument doc(16384);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        JsonArray rawToday = doc["attributes"]["raw_today"].as<JsonArray>();
        if (doc["attributes"].containsKey("raw_tomorrow")) {
          JsonArray rawTomorrow = doc["attributes"]["raw_tomorrow"].as<JsonArray>();
          tomorrow_exists = 1;
        }
        tft.fillRect(1, 1, 320, 119, TFT_BLACK);
        Serial.println("Today's hourly prices:");

        int today_start_index = 0;
        if (tomorrow_exists == 1) {
          today_start_index = currentHour;
        }
        for (JsonObject entry : rawToday) {
          const char* start = entry["start"];
          float value       = entry["value"];
          int color;

          // Extract just the hour (substring of ISO8601 timestamp)
          String startStr = String(start);   // e.g. "2025-09-07T14:00:00+03:00"
          String hourStr  = startStr.substring(11, 13); // "14:00"

          Serial.printf("%s -> %.3f EUR/kWh\n", hourStr.c_str(), value);
          if ((hourStr.toInt() >= today_start_index) || (tomorrow_exists == 0)) {
            float scaled = lroundf(value * 500);
            int h = (int)scaled;
            if(value>0.08) {
              color = TFT_RED;
              } else {
              color = TFT_WHITE;
              }

            if(value>0.20) {
              color = TFT_ORANGE;
            }
            
            tft.fillRect(h_index+30, constrain(100-h,0,99), 10, constrain(h,0,100), color);
            tft.drawLine(h_index+30, 100, h_index+30, 101, color);
            if (index % 2 == 0) {
              tft.setTextSize(1);
              tft.setTextDatum(TL_DATUM); 
              tft.drawString(String(index), h_index+30, 106);
            }
            Serial.printf("%i -> %i **\n", h_index+30, h);
            if (index==currentHour) {
              tft.drawLine(h_index+35, 100, h_index+35, 10, TFT_GREEN);
            }
            h_index = h_index+10;
          }
          index++;
        }

        if (tomorrow_exists==1) {
          if (!error) {
            JsonArray rawTomorrow = doc["attributes"]["raw_tomorrow"].as<JsonArray>();
            for (JsonObject entry : rawTomorrow) {
              const char* start = entry["start"];
              float value       = entry["value"];
              int color;

              // Extract just the hour (substring of ISO8601 timestamp)
              String startStr = String(start);   // e.g. "2025-09-07T14:00:00+03:00"
              String hourStr  = startStr.substring(11, 13); // "14:00"

              Serial.printf("%s -> %.3f EUR/kWh\n", hourStr.c_str(), value);
              if (hourStr.toInt() < today_start_index+12) {
                float scaled = lroundf(value * 500);
                int h = (int)scaled;
                if(value>0.08) {
                  color = TFT_RED;
                  } else {
                  color = TFT_WHITE;
                  }

                if(value>0.20) {
                  color = TFT_ORANGE;
                }
                
                tft.fillRect(h_index+30, constrain(100-h,0,99), 10, constrain(h,0,100), color);
                tft.drawLine(h_index+30, 100, h_index+30, 101, color);
                if (index % 2 == 0) {
                  tft.setTextSize(1);
                  tft.setTextDatum(TL_DATUM); 
                  tft.drawString(hourStr, h_index+30, 106);
                }
                Serial.printf("%i -> %i **\n", h_index+30, h);
                if (hourStr.toInt() == 0) {
                  tft.fillRect(h_index+30, 1, 2, 100, TFT_BLUE);
                  
                }
                
                h_index = h_index+10;
              }
              index++;
            }
          }
          tft.drawString("Tomorrow", ((24-today_start_index)*10)+40, 10);
        }

        TFT_dashline(28,75, 315, 2, 6);
        TFT_dashline(28,50, 315, 2, 6);
        TFT_dashline(28,25, 315, 2, 6);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM); 
        tft.drawString("5", 15, 72);
        tft.drawString("10", 8, 47);
        tft.drawString("15", 8, 22);

      } else {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    }

    http.end();
  }
}

void drawThickArc(int x0, int y0, int r, int thickness, int startAngle, int endAngle, uint16_t color) {
  for (int radius = r; radius < r + thickness; radius++) {       // loop over radii
    for (int angle = startAngle; angle <= endAngle; angle++) {   // loop over angles
      float rad = angle * 3.14159 / 180.0;
      int x = x0 + radius * cos(rad);
      int y = y0 + radius * sin(rad);
      tft.drawPixel(x, y, color);
    }
  }
}

void printConsumption() {
  
  if ((WiFi.status() == WL_CONNECTED)) {
    
    HTTPClient http;
    String url = String(ha_server) + "/api/states/sensor.sahko_tarkka_w_rounded";

    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + token);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();
	int consumptionAngle;

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        const char* state = doc["state"]; // get state
        Serial.print("Sensor state: ");
        Serial.println(state);

        float value = atof(state);
        Serial.print("Sensor value as float: ");
        Serial.println(value);
		if (value <= 2000) {
			consumptionAngle = mapFloat(value,0,2000,20,180);
		} else {
			consumptionAngle = mapFloat(value,2000,6000,180,340);
		}
		void drawArc(180, 160, 30, 35, 20, consumptionAngle, TFT_GREEN);
		
      }
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpCode);
    }

    http.end();
  }
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void drawArc(int x, int y, int r_in, int r_out, int start_angle, int end_angle, uint16_t color) {
  for (int angle = start_angle; angle <= end_angle; angle++) {
    float rad = (angle - 90) * DEG_TO_RAD; // Rotate angle system
    int x_in = x + cos(rad) * r_in;
    int y_in = y + sin(rad) * r_in;
    int x_out = x + cos(rad) * r_out;
    int y_out = y + sin(rad) * r_out;
    tft.drawLine(x_in, y_in, x_out, y_out, color);
  }
}

void loop() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    delay(1000);
  }

  if (currentHour!=timeinfo.tm_hour) {
    currentHour=timeinfo.tm_hour;
    drawCurrentDay();
  }

  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();
    
    int tx = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int ty = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    Serial.printf("X: %d  Y:%d\n", tx,ty);

    if (isTouchInButton(tx, ty, btnOpen)) {
      drawButton(btnOpen_pressed);
      Serial.println("OPEN pressed - sending MQTT message");
      drawCurrentDay();
      delay(300); // debounce

      waitUntilNotTouch();
      drawButton(btnOpen);
    }
    else if (isTouchInButton(tx, ty, btnClose)) {
      drawButton(btnClose_pressed);
      Serial.println("CLOSE pressed - sending MQTT message");
      
      delay(300); // debounce
      waitUntilNotTouch();
      drawButton(btnClose);
    }

    
  }
  
  


  

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Clear previous time (optional)
  //tft.fillRect(1, 120, 140, 22, TFT_BLACK);

  // Print time
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("88:88:88"));
  //tft.setCursor(2, 120);
  tft.drawString(timeStr,2,120);
  tft.setTextPadding(0);
  if (el_arc.doTask()) { printConsumption(); }
  delay(20);  // update every second

}
