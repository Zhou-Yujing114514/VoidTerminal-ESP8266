#ifndef CLOCK_H
#define CLOCK_H

#include "config.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include <ESP8266WiFi.h>
#include <time.h>

class ClockManager {
public:
    void init();
    void enter();
    void exit();
    void update();
    void handleKey(KeyEvent evt);
    bool isActive() { return _active; }
    
private:
    bool _active;
    bool _timeSynced;
    unsigned long _lastUpdate;
    unsigned long _lastSyncAttempt;
    
    void drawClock();
    void syncTime();
    const char* getWeekday(int wday);
};

extern ClockManager clockMgr;

#endif // CLOCK_H
