# ⚡ ESP32 TFT Electricity Dashboard

An ESP32-based dashboard that displays **real-time electricity prices and consumption** using a **TFT touchscreen** and **Home Assistant API**.

---

## 📸 Features
- 📊 **Electricity Prices Graph**
  - Fetches data from Home Assistant (`sensor.nordpool_kwh_fi_eur_3_10_0255`).
  - Shows **today’s hourly prices** and **first hours of tomorrow** (if available).
  - Color-coded bars:  
    - **White** → low price  
    - **Red** → normal (>0.08 €/kWh)  
    - **Orange** → high (>0.20 €/kWh)  

- 🔄 **Real-Time Consumption Gauge**
  - Two circular gauges from HA sensors:
    - `sensor.sahko_tarkka_w_rounded`
    - `sensor.power_1h`
  - Scale: **0.5 kW → 6 kW**.

- ⏰ **Digital Clock**
  - Synced via **NTP**.
  - Updates hourly.

- 💡 **Adaptive Brightness**
  - Daytime → Bright  
  - Evening → Medium  
  - Night → Dim  
  - Touchscreen → temporarily max brightness.

- 👆 **Touchscreen Support**
  - Brightness override on touch.
  - Button framework included (OPEN / CLOSE buttons).

---

## 🛠️ Hardware Requirements
- **ESP32**
- **320×240 TFT Display** (ILI9341, ST7735 or similar supported by TFT_eSPI)
- **XPT2046 Touchscreen controller**
- **Wi-Fi network**
- Optional: Home Assistant with Nordpool integration

### Wiring
| Component   | Pin |
|-------------|-----|
| TFT Backlight | 27 |
| Touch IRQ   | 36 |
| Touch DIN   | 32 |
| Touch OUT   | 39 |
| Touch CLK   | 25 |
| Touch CS    | 33 |

---

## 📦 Software Dependencies
Install via Arduino IDE Library Manager or PlatformIO:

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)  
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)  
- [ArduinoJson](https://arduinojson.org/)  
- Built-in: `WiFi`, `HTTPClient`, `time`  

---

## ⚙️ Configuration
Edit Wi-Fi and Home Assistant details in `main.cpp`:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

const char* ha_server = "http://192.168.x.x:8123";
const char* entity_id = "sensor.nordpool_kwh_fi_eur_3_10_0255";
String token = "YOUR_LONG_LIVED_ACCESS_TOKEN";
```

---

## 🖥️ Display Layout
```
 -------------------------------------------------
|              Electricity Prices                 |
|                                                 |
|   Bar chart: hourly prices today + tomorrow     |
|   Color coded (low/normal/high)                 |
|   Current hour marked in green                  |
|-------------------------------------------------|
| Consumption Gauge     |         Digital Clock   |
| Two arcs (0.5–6kW)    |         HH:MM:SS        |
 -------------------------------------------------
```

---

## 📂 Project Structure
```
/src
 ├── main.cpp             # Core application
 ├── JASA_scheduler.h     # Lightweight scheduler
 └── JASA_scheduler.cpp
```

---

## 🔧 Usage
1. Flash firmware to ESP32.
2. TFT will initialize and connect to Wi-Fi.
3. Dashboard shows:
   - Current electricity prices.
   - Consumption arcs.
   - Local time.
4. Touch screen to temporarily set max brightness.

---

## 🛠️ Future Improvements
- Add MQTT support for events & sensors.
- Multiple display modes (`displaymode` variable prepared).
- Improved UI with icons & menus.
- Optimize JSON buffer (`16384` → dynamic sizing).
- Migrate to FreeRTOS tasks for smoother updates.

---

## 📜 License
MIT License – free to use, modify, and share.
