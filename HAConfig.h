#ifndef HA_CONFIG_H
#define HA_CONFIG_H

#include <Preferences.h>
#include <WebServer.h>
#include "JASA_XPT2046_touch_helper.h"

// Stored values accessible globally
extern String g_ha_server;
extern String g_entity_id;
extern String g_token;
extern String g_el_entity_id;
extern String g_el_ma_entity_id;
extern String g_message_entity_id;

// Flags
extern bool settingsSaved;

// Initialize HAConfig (load settings and set up web server)
void HAConfig_begin(WebServer &server, String &ipStr);

void loadSettings();

// Call this continuously in loop() to handle web requests and touchscreen
void HAConfig_handleLoop(WebServer &server, JASA_XPT2046_touch_helper &touchHelper);

#endif