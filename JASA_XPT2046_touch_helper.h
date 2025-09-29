#ifndef JASA_XPT2046_TOUCH_HELPER_H
#define JASA_XPT2046_TOUCH_HELPER_H

#include <XPT2046_Touchscreen.h>

class JASA_XPT2046_touch_helper {
private:
    XPT2046_Touchscreen &ts;
    unsigned long touchStart = 0;
    bool isTouching = false;

    unsigned long longPressTime = 1000;   // ms
    unsigned long doubleClickTime = 400;  // ms
    unsigned long lastTapTime = 0;

public:
    JASA_XPT2046_touch_helper(XPT2046_Touchscreen &touchscreen);

    void setLongPressTime(unsigned long ms);
    void setDoubleClickTime(unsigned long ms);

    // 0 = none, 1 = tap, 2 = long press, 3 = double click
    int checkEvent();
};

#endif
