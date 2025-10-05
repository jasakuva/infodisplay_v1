#include "HAConfig.h"
#include <Arduino.h>

Preferences prefs;

// Global variables
String g_ha_server;
String g_entity_id;
String g_token;
String g_el_entity_id;
String g_el_ma_entity_id;
String g_message_entity_id;



bool settingsSaved = false;

// Default values
const char* default_server = "http://192.168.2.68:8123";
const char* default_entity = "sensor.nordpool_kwh_fi_eur_3_10_0255";
const char* default_token  = "";

// Save settings to Preferences
void saveSettings(const String& serverVal, const String& entityVal, const String& tokenVal, const String& v_el_entity_id, const String& v_el_ma_entity_id, const String& v_message_entity_id ) {
    prefs.begin("ha", false);
    prefs.putString("g_server", serverVal);
    prefs.putString("g_entity", entityVal);
    prefs.putString("g_token", tokenVal);
    prefs.putString("el_ent_id", v_el_entity_id);
    prefs.putString("el_ma_ent_id", v_el_ma_entity_id);
    prefs.putString("message_ent_id", v_message_entity_id);
    prefs.end();

    Serial.println("Settings saved:");
    Serial.println("Server: " + serverVal);
    Serial.println("Entity: " + entityVal);
    Serial.println("g_el_ma_entity_id: " + v_el_ma_entity_id);
}

// Load settings from Preferences
void loadSettings() {
    prefs.begin("ha", true);
    g_ha_server           = prefs.getString("g_server", default_server);
    g_entity_id           = prefs.getString("g_entity", default_entity);
    g_token               = prefs.getString("g_token", default_token);
    g_el_entity_id        = prefs.getString("el_ent_id", default_token);
    g_el_ma_entity_id     = prefs.getString("el_ma_ent_id", default_token);
    g_message_entity_id   = prefs.getString("message_ent_id", default_token);
    prefs.end();
}

// Build HTML page for web config


String pageHTML(String &ipStr) {
    
    String html = "<!DOCTYPE html><html><body>";
    html += "<h2>HA Config</h2>";
    html += "IP address: " + ipStr + "<br>";
    html += "<form method='POST' action='/save'>";
    html += "HA Server: <input name='server' value='" + g_ha_server + "'><br>";
    html += "Token: <input name='token' size='140' value='" + g_token + "'><br>";
    html += "Nordpool Entity ID: <input name='entity' size='60' value='" + g_entity_id + "'><br>";
    html += "Electricty consumption entity, current: <input name='g_el_entity_id' size='60' value='" + g_el_entity_id + "'><br>";
    html += "Electricty consumption entity, moving average: <input name='g_el_ma_entity_id' size='60' value='" + g_el_ma_entity_id + "'><br>";
    html += "Message entity: <input name='g_message_entity_id' size='60' value='" + g_message_entity_id + "'><br>";
    html += "<button type='submit'>Save</button>";
    html += "</form>";
    html += "</body></html>";
    return html;
}


// Setup web server and load saved settings
void HAConfig_begin(WebServer &server, String &ipStr) {
    loadSettings();

    server.on("/", [&]() {
        server.send(200, "text/html", pageHTML(ipStr));
    });

    server.on("/save", [&server]() {
        if (server.hasArg("server") && server.hasArg("entity") && server.hasArg("token")) {
            g_ha_server         = server.arg("server");
            g_entity_id         = server.arg("entity");
            g_token             = server.arg("token");
            g_el_entity_id      = server.arg("g_el_entity_id");
            g_el_ma_entity_id   = server.arg("g_el_ma_entity_id");
            g_message_entity_id = server.arg("g_message_entity_id");
            saveSettings(g_ha_server, g_entity_id, g_token,g_el_entity_id, g_el_ma_entity_id, g_message_entity_id);
            settingsSaved = true;
            server.send(200, "text/html", "<h3>Saved!</h3>");
        } else {
            server.send(400, "text/plain", "Missing parameters!");
        }
    });
    server.begin();
    Serial.println("Web server started");
  }

// Non-blocking loop: handle web requests and wait for long press or save
void HAConfig_handleLoop(WebServer &server, JASA_XPT2046_touch_helper &touchHelper) {
    server.handleClient();
    /*
    int event = touchHelper.checkEvent();
    if (event == 2 || settingsSaved) { // long press detected or settings saved
        Serial.println("Long press detected or settings saved. Continue...");
        settingsSaved = false;
        // You can now update displaymode or continue program
    }
    */
}