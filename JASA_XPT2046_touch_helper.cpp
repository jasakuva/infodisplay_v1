#include "JASA_XPT2046_touch_helper.h"
#include <Arduino.h>

JASA_XPT2046_touch_helper::JASA_XPT2046_touch_helper(XPT2046_Touchscreen &touchscreen)
    : ts(touchscreen) {}

void JASA_XPT2046_touch_helper::setLongPressTime(unsigned long ms) {
    longPressTime = ms;
}

void JASA_XPT2046_touch_helper::setDoubleClickTime(unsigned long ms) {
    doubleClickTime = ms;
}

int JASA_XPT2046_touch_helper::checkEvent() {
    bool touchingNow = ts.tirqTouched() && ts.touched();

    if (touchingNow) {
        if (!isTouching) {
            // New press
            isTouching = true;
            touchStart = millis();
        }
    } else {
        if (isTouching) {
            // Released
            unsigned long duration = millis() - touchStart;
            isTouching = false;

            if (duration >= longPressTime) {
                return 2; // long press
            } else {
                // Check double click
                unsigned long now = millis();
                if (now - lastTapTime <= doubleClickTime) {
                    lastTapTime = 0; // reset
                    return 3; // double click
                } else {
                    lastTapTime = now;
                    return 1; // single tap
                }
            }
        }
    }

    return 0; // no event
}