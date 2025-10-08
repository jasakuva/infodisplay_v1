#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include <math.h>
#include "JASA_scheduler.h"
#include "JASA_XPT2046_touch_helper.h"
#include <Arduino.h>
#include <FS.h>
using FS = fs::FS;   // alias old FS to new fs::FS
#include <WiFiManager.h>
#include <WebServer.h>
#include "HAConfig.h"

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

JASA_XPT2046_touch_helper touchHelper(touchscreen);

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

#define TFT_BL 27   // Backlight pin
#define LEDC_CH 0   // LEDC channel
#define LEDC_FREQ 5000
#define LEDC_RES 8  // 8-bit (0-255)

// NTP settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;      // adjust for your timezone
const int   daylightOffset_sec = 3600; // daylight saving offset


// ===== Wi-Fi & MQTT Settings =====
//const char* ssid = "jasadeko";
//const char* password = "subaru72";

//char* ha_server = "http://192.168.2.68:8123";
//char* entity_id = "sensor.nordpool_kwh_fi_eur_3_10_0255";
//String token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhMWIwMjdjOTc3NmU0YWQxYjliYzU4ZjBhNjg4ZmU3ZiIsImlhdCI6MTc1NzI1Njg0NCwiZXhwIjoyMDcyNjE2ODQ0fQ.u60X3TNzUHQKRS3QSdLu1qoS-xXNTSMqAIdpnKnEb5M";  // from HA profile

char ha_server[30];
char entity_id[60];
String token;
String el_entity_id;
String el_ma_entity_id;
String message_entity_id;



WiFiClient espClient;

WiFiManager wifiManager;

WebServer server(80);

JASA_Scheduler el_arc(15000);
JASA_Scheduler el_msg(15000);
JASA_Scheduler setBrightnessNormal(0,JASA_Scheduler::TIMER);

int currentHour;
int lastConsumptionAngle1 = 359;
int lastConsumptionAngle2 = 359;

String displaymode = "ELECTRICITY";
int displaymode_change = 0;


// ===== Button Struct =====
struct Button {
  int x, y, w, h;
  String text;
  uint16_t color;
};

Button btnResetWiFi = {180, 185, 120, 40, "Reset WiFi", TFT_RED};
Button btnResetWiFi_pressed = {110, 185, 120, 40, "Reset WiFi", TFT_BLUE};
Button btnYes = {30, 140, 120, 40, "YES", TFT_GREEN};
Button btnNo = {180, 140, 120, 40, "NO", TFT_RED};
Button btnClose = {130, 150, 60, 60, "CLOSE", TFT_RED};
Button btnClose_pressed = {130, 150, 60, 60, "CLOSE", TFT_BLUE};

void drawButton(Button btn) {
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, btn.color);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, btn.color);
  tft.drawCentreString(btn.text, btn.x + btn.w / 2, (btn.y + btn.h / 2) - 5, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawButton_pressed(Button btn) {
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, btn.color);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, btn.color);
  tft.drawCentreString(btn.text, btn.x + btn.w / 2, (btn.y + btn.h / 2) - 5, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

bool isTouchInButton(int tx, int ty, Button btn) {
  return (tx > btn.x && tx < (btn.x + btn.w) && ty > btn.y && ty < (btn.y + btn.h));
}

/*
void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}
*/


void setup() {
  Serial.begin(115200);
  delay(2000);
  // Touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  touchHelper.setLongPressTime(1600); // optional: 1.6 sec long press

  // TFT
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  //drawButton(btnOpen);
  //drawButton(btnClose);

  Serial.println("Start wifimanager");
  tft.setTextSize(1);
  tft.drawString("If this display stays owe 5 sec.,", 10, 10);
  tft.drawString("configure wifi settings.", 10, 25);
  tft.drawString("Find access point:", 10, 40);
  tft.drawString("access point: jasainfo", 10, 55);
  tft.drawString("password: 12345678", 10, 70);
  tft.drawString("and usding same phone, go to:", 10, 85);
  tft.drawString("192.168.4.1", 10, 100);
  wifiManager.autoConnect("jasainfo", "12345678");
  Serial.println("END wifimanager");

  // Wi-Fi + MQTT
  //connectWiFi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 30); // 50% brightness (0-255)

  loadSettings();
  Serial.printf("g_ha_server: %s\n", g_ha_server);
  Serial.printf("g_entity_id: %s\n", g_entity_id);
  Serial.printf("g_token: %s\n", g_token);

  tft.fillScreen(TFT_BLACK);
  
  //tft.drawString("g_ha_server: "+g_ha_server, 10, 50);
  //tft.drawString("g_entity_id: "+g_entity_id, 10, 65);
  //tft.drawString("g_token: "+g_token, 10, 80);
  tft.setTextSize(2);
  tft.drawString("WELCOME !", 10, 100);

  g_ha_server.toCharArray(ha_server, sizeof(ha_server));
  g_entity_id.toCharArray(entity_id, sizeof(entity_id));
  token = g_token;
  el_entity_id      = g_el_entity_id;
  el_ma_entity_id   = g_el_ma_entity_id;
  message_entity_id = g_message_entity_id;

  delay(5000);

  tft.fillScreen(TFT_BLACK);

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
    int graphOffset = 18;
    
    if (httpCode == 200) {
      String payload = http.getString();

      // Allocate JSON doc (increase size if needed)
      DynamicJsonDocument doc(16384);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        JsonArray rawToday = doc["attributes"]["raw_today"].as<JsonArray>();
        if (doc["attributes"].containsKey("raw_tomorrow")) {
          JsonArray rawTomorrow = doc["attributes"]["raw_tomorrow"].as<JsonArray>();
        }
        bool tomorrow_valid = doc["attributes"]["tomorrow_valid"];
        
        tft.fillRect(1, 1, 320, 119, TFT_BLACK);
        Serial.println("Today's hourly prices:");

        int today_start_index = 0;
        if (tomorrow_valid) {
          today_start_index = currentHour;
        }
        for (JsonObject entry : rawToday) {
          const char* start = entry["start"];
          float value       = entry["value"];
          int color;

          // Extract just the hour (substring of ISO8601 timestamp)
          String startStr = String(start);   // e.g. "2025-09-07T14:00:00+03:00"
          String hourStr  = startStr.substring(11, 13); // "14:00"
          String minStr   = startStr.substring(14, 16); // "14:00"

          Serial.printf("%s -> %.3f EUR/kWh\n", hourStr.c_str(), value);
          if ((hourStr.toInt()+1 >= today_start_index) || (!tomorrow_valid)) {
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
            
            tft.fillRect(h_index+graphOffset, constrain(100-h,0,99), 2, constrain(h,0,100), color);
            
            if (minStr == "00") {
              tft.setTextSize(1);
              tft.setTextDatum(TL_DATUM); 
              if(hourStr.toInt() % 2 == 0) {
                    tft.drawString(hourStr, h_index+graphOffset+1, 106);
              }
              tft.drawLine(h_index+graphOffset, 100, h_index+graphOffset, 102, color);
            }
            Serial.printf("%i -> %i **\n", h_index+graphOffset, h);
            if (hourStr.toInt()==currentHour && minStr == "00") {
              tft.drawLine(h_index+graphOffset+5, 100, h_index+graphOffset+5, 10, TFT_GREEN);
            }
            h_index = h_index+3;
          }
          index++;
        }

        if (tomorrow_valid) {
          if (!error) {
            JsonArray rawTomorrow = doc["attributes"]["raw_tomorrow"].as<JsonArray>();
            for (JsonObject entry : rawTomorrow) {
              const char* start = entry["start"];
              float value       = entry["value"];
              int color;

              // Extract just the hour (substring of ISO8601 timestamp)
              String startStr = String(start);   // e.g. "2025-09-07T14:00:00+03:00"
              String hourStr  = startStr.substring(11, 13); // "14:00"
              String minStr   = startStr.substring(14, 16); // "14:00"

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
                
                tft.fillRect(h_index+graphOffset, constrain(100-h,0,99), 2, constrain(h,0,100), color);
                
                if (minStr == "00") {
                  tft.setTextSize(1);
                  tft.setTextDatum(TL_DATUM); 
                  if(hourStr.toInt() % 2 == 0) {
                    tft.drawString(hourStr, h_index+graphOffset+1, 106);
                  }
                  tft.drawLine(h_index+graphOffset, 100, h_index+graphOffset, 102, color);
                }
                Serial.printf("%i -> %i **\n", h_index+graphOffset, h);
                if (hourStr.toInt() == 0 && minStr == "00") {
                  tft.fillRect(h_index+graphOffset, 1, 2, 100, TFT_BLUE);
                  
                }
                
                h_index = h_index+3;
              }
              index++;
            }
          }
          tft.drawString("Tomorrow", ((24-today_start_index)*3*4)+graphOffset+10, 10);
        }

        TFT_dashline(graphOffset-2,75, 315, 2, 6);
        TFT_dashline(graphOffset-2,50, 315, 2, 6);
        TFT_dashline(graphOffset-2,25, 315, 2, 6);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM); 
        tft.drawString("5", graphOffset-8, 72);
        tft.drawString("10", graphOffset-13, 47);
        tft.drawString("15", graphOffset-13, 22);

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

String getEntityStatus(String entity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(ha_server) + "/api/states/" + entity;

    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + token);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        const char* state = doc["state"];
        Serial.print("Sensor state: ");
        Serial.println(state);

        http.end();   // free connection
        return String(state);  // ✅ fixed semicolon
      } else {
        http.end();
        return  String("unavailable");
      }
    }

    http.end(); // free connection
  }

  // Default return if error or WiFi not connected
  return String("unavailable");
}

void printConsumption() {
  
  int consumptionAngle1;
  int consumptionAngle2;

  String consumption1 = getEntityStatus("sensor.sahko_tarkka_w_rounded");
  String consumption2 = getEntityStatus("sensor.power_1h");
  if ((consumption1.length() > 0 && consumption1 != "unavailable") && (consumption1.length() > 0 && consumption1 != "unavailable")) {
    float value1 = atof(consumption1.c_str());
    float value2 = atof(consumption2.c_str());
    
		if (value1 <= 1000) {
			consumptionAngle1 = mapFloat(value1,0,1000,20,180);
		} else if (value1 <= 2000) {
			consumptionAngle1 = mapFloat(value1,1000,2000,180,270);
		} else {
      consumptionAngle1 = mapFloat(value1,2000,6000,270,359);
    }

    if (value2 <= 1000) {
			consumptionAngle2 = mapFloat(value2,0,1000,20,180);
		} else if (value2 <= 2000) {
			consumptionAngle2 = mapFloat(value2,1000,2000,180,270);
		} else {
      consumptionAngle2 = mapFloat(value2,2000,6000,270,359);
    }
		
		drawArc(270, 170, 19, 27, 20, consumptionAngle2, TFT_GREEN);
    drawArc(270, 170, 19, 27, consumptionAngle2,lastConsumptionAngle2, TFT_BLACK);
    //drawArc(270, 170, 30, 37, consumptionAngle2,355, TFT_BLACK);
    
    drawArc(270, 170, 30, 37, 20, consumptionAngle1, TFT_GREEN);
    drawArc(270, 170, 30, 37, consumptionAngle1,lastConsumptionAngle1, TFT_BLACK);
    //drawArc(270, 170, 30, 37, consumptionAngle1,355, TFT_BLACK);
    drawArc(270, 170, 28, 29, 20,355, TFT_WHITE);

    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM); 
    tft.drawString("0.5", 207, 166);
    tft.drawString("1.0", 260, 119);
    tft.drawString("2", 312, 166);
    tft.drawString("6", 268, 211);
    tft.drawLine(231, 170, 240, 170, TFT_WHITE);
    tft.drawLine(270, 127, 270, 140, TFT_WHITE);
    tft.drawLine(297, 170, 305, 170, TFT_WHITE);
    tft.drawLine(270, 200, 270, 207, TFT_WHITE);
		
    lastConsumptionAngle1 = consumptionAngle1;
    lastConsumptionAngle2 = consumptionAngle2;
  }
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void drawArc(int x, int y, int r_in, int r_out, int start_angle, int end_angle, uint16_t color) {
  for (int angle = start_angle; angle < end_angle; angle++) {
    float rad1 = (angle + 90) * DEG_TO_RAD;
    float rad2 = (angle + 91) * DEG_TO_RAD;

    int x_in1 = x + cos(rad1) * r_in;
    int y_in1 = y + sin(rad1) * r_in;
    int x_out1 = x + cos(rad1) * r_out;
    int y_out1 = y + sin(rad1) * r_out;

    int x_in2 = x + cos(rad2) * r_in;
    int y_in2 = y + sin(rad2) * r_in;
    int x_out2 = x + cos(rad2) * r_out;
    int y_out2 = y + sin(rad2) * r_out;

    // Fill between two "slices"
    tft.fillTriangle(x_in1, y_in1, x_out1, y_out1, x_out2, y_out2, color);
    tft.fillTriangle(x_in1, y_in1, x_in2, y_in2, x_out2, y_out2, color);
  }
}

void setBrightnesByClock() {

int brightness;

if (currentHour >= 23 || currentHour < 8) {
    // Deep night: 23:00 → 08:00
    brightness = 5;
}
else if (currentHour >= 21 && currentHour < 23) {
    // Evening: 21:00 → 23:00
    brightness = 50;
}
else {
    // Daytime: 08:00 → 21:00
    brightness = 230;
}
  setBrightness(brightness);
  Serial.printf("Brightness set by clock: %i\n", brightness);
}

void setBrightness(int brightness) {
  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, brightness);
  Serial.printf("Brightness set: %i\n", brightness);
}

void printMessageEl() {
  String entity_message = getEntityStatus("sensor.power_1h");

  if (entity_message != "unavailable" && entity_message.length() > 0) {
    StaticJsonDocument<512> doc; // adjust if JSON is large

    DeserializationError error = deserializeJson(doc, entity_message);
    if (error) {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
      return;
    }

    JsonArray texts = doc["texts"];
    if (texts.isNull()) {
      Serial.println("No 'texts' array found in JSON");
      return;
    }

    //{"texts":[{"x":10,"y":20,"size":2,"text":"Hello"}]}
    for (JsonObject item : texts) {
      int x = item["x"];
      int y = item["y"];
      int size = item["size"];
      const char* text = item["text"];

      Serial.printf("Draw text '%s' at (%d, %d) with size %d\n", text, x, y, size);
      // drawText(x, y, size, text);
    }
  } else {
    Serial.println("Entity unavailable or empty.");
  }
}

void displayElectricity() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    delay(1000);
  }

  int event = touchHelper.checkEvent();
  if (event == 1) {
    Serial.println("Tap detected");
  } else if (event == 2) {
    Serial.println("Long press detected");
    displaymode = "SETTINGS";
  } else if (event == 3) {
    Serial.println("Double Click");
  }

  if (currentHour!=timeinfo.tm_hour || displaymode_change) {
    currentHour=timeinfo.tm_hour;
    drawCurrentDay();
    setBrightnesByClock();
    if(displaymode_change) { displaymode_change = 0; }
  }
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    TS_Point p = touchscreen.getPoint();

    setBrightness(255);
    setBrightnessNormal.setTimer(15000);

    
    int tx = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int ty = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    Serial.printf("X: %d  Y:%d\n", tx,ty);

    /* Just to show, how buttons can be placed
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
    */
    
  }
  
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Clear previous time (optional)
  //tft.fillRect(1, 120, 140, 22, TFT_BLACK);

  // Print time
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.textWidth("88:88:88"));
  //tft.setCursor(2, 120);
  tft.drawString(timeStr,2,120);
  tft.setTextPadding(0);
  if (el_arc.doTask()) { printConsumption(); }

  if (el_msg.doTask()) { printMessageEl(); }

}

int checkOK(String info_text) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(info_text,20,40);
  drawButton(btnYes);
  drawButton(btnNo);
  while (true) {
    int event = touchHelper.checkEvent();
    if (event ==1) {
      if (isTouchInButton(touchHelper.getTouchCoordinateX(), touchHelper.getTouchCoordinateY(), btnYes)) {
        tft.fillScreen(TFT_BLACK);
        return 1;
      } else if (isTouchInButton(touchHelper.getTouchCoordinateX(), touchHelper.getTouchCoordinateY(), btnNo)) {
        tft.fillScreen(TFT_BLACK);
        return 0;
      }
    }
  }
}

void displaySettings() {
  int continue_loop = 1;
  while (true) {
    tft.fillScreen(TFT_BLACK);
    String ipStr = WiFi.localIP().toString();
    
    tft.setTextSize(2);
    tft.drawString("Please configure", 5, 10);
    tft.drawString("using web browser.", 5, 25);
    tft.setTextSize(3);
    tft.drawString("Use address:", 10, 55);
    tft.setTextSize(2);
    tft.drawString("http://"+ipStr, 10, 85);

    tft.drawString("Long press display", 5, 120);
    tft.drawString("after settings configured", 5, 140);
    tft.drawString("and saved", 5, 160);

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    HAConfig_begin(server,ipStr);

    drawButton(btnResetWiFi);

    while (true) {
      HAConfig_handleLoop(server, touchHelper);
      int event = touchHelper.checkEvent();
      if (event == 2) {  // long press detected
        Serial.println("Long press detected! Continuing...");
        continue_loop = 0;
        break; // exit loop
      }
      if (event ==1) {
        if (isTouchInButton(touchHelper.getTouchCoordinateX(), touchHelper.getTouchCoordinateY(), btnResetWiFi)) {
        
        Serial.println("Reset WiFi pressed");
        if (checkOK("Reset WiFi?")) {
          continue_loop = 1;
          wifiManager.resetSettings();
          ESP.restart();
        } else {
          continue_loop == 1;
          break;
        }
        }
      }
      delay(10); // small delay to avoid busy loop
    }

    if (continue_loop == 1) { continue; }

    tft.fillScreen(TFT_BLACK);
    
    loadSettings();
    Serial.printf("g_ha_server: %s\n", g_ha_server);
    Serial.printf("g_entity_id: %s\n", g_entity_id);
    Serial.printf("g_token: %s\n", g_token);

    tft.setTextSize(2);
    tft.drawString("SAVED", 10, 10);
    tft.setTextSize(1);
    tft.drawString("g_ha_server: "+g_ha_server, 10, 40);
    tft.drawString("g_entity_id: "+g_entity_id, 10, 60);
    tft.drawString("g_token: "+g_token, 10, 80);
    tft.drawString("g_el_entity_id: "+g_el_entity_id, 10, 100);
    tft.drawString("g_el_ma_entity_id: "+g_el_ma_entity_id, 10, 120);
    tft.drawString("g_message_entity_id: "+g_message_entity_id, 10, 140);
    tft.setTextSize(2);
    tft.drawString("WAIT 5 sec.", 10, 190);


    g_ha_server.toCharArray(ha_server, sizeof(ha_server));
    g_entity_id.toCharArray(entity_id, sizeof(entity_id));
    token = g_token;
    el_entity_id      = g_el_entity_id;
    el_ma_entity_id   = g_el_ma_entity_id;
    message_entity_id = g_message_entity_id;

    delay(5000);

    tft.fillScreen(TFT_BLACK);

    displaymode = "ELECTRICITY";
    displaymode_change = 1;
    break;
  }
}

void loop() {
 
  if (displaymode == "ELECTRICITY") {
    displayElectricity();
  }

  if (displaymode == "SETTINGS") {
    displaySettings();
  }


  if (setBrightnessNormal.isTimer()) { setBrightnesByClock(); }
  
  delay(20);  // update every second

}
