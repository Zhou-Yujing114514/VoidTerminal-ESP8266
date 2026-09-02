#include "clock.h"

ClockManager clockMgr;

// NTP 服务器配置
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600; // 北京时间 UTC+8
const int daylightOffset_sec = 0;

void ClockManager::init() {
    _active = false;
    _timeSynced = false;
    _lastUpdate = 0;
    _lastSyncAttempt = 0;
}

void ClockManager::enter() {
    _active = true;
    _lastUpdate = 0;
    if (!_timeSynced) {
        syncTime();
    }
    drawClock();
}

void ClockManager::exit() {
    _active = false;
}

void ClockManager::syncTime() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    _lastSyncAttempt = millis();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // 等待时间同步（最多5次×500ms=2.5秒，避免看门狗超时）
    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 100000 && attempts < 5) {
        delay(500);
        yield();  // 喂狗，防止看门狗超时
        now = time(nullptr);
        attempts++;
    }
    
    if (now > 100000) {
        _timeSynced = true;
    }
}

const char* ClockManager::getWeekday(int wday) {
    switch (wday) {
        case 0: return "周日";
        case 1: return "周一";
        case 2: return "周二";
        case 3: return "周三";
        case 4: return "周四";
        case 5: return "周五";
        case 6: return "周六";
        default: return "";
    }
}

void ClockManager::drawClock() {
    disp.clear();
    disp.drawTitleBar("时钟");
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // 大字体显示时间
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    // 居中显示大时间
    int timeWidth = strlen(timeStr) * 18; // 3倍字体大约宽度
    disp.drawText((SCREEN_W - timeWidth) / 2, 35, timeStr, 3);
    
    // 日期
    char dateStr[32];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d  %s",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             getWeekday(timeinfo.tm_wday));
    int dateWidth = disp.getTextWidth(dateStr);
    disp.drawText((SCREEN_W - dateWidth) / 2, 75, dateStr, 1);
    
    // 同步状态
    if (_timeSynced) {
        disp.drawText(4, SCREEN_H - 24, "已同步", 1);
    } else {
        disp.drawText(4, SCREEN_H - 24, "未同步", 1);
    }
    
    disp.drawStatusBar("短按下:刷新  长按下:同步时间", "Home:返回");
    disp.refresh(true);
}

void ClockManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    
    if (evt == KEY_MENU_SHORT) {
        exit();
        app.goHome();
        return;
    }
    
    if (evt == KEY_DOWN_SHORT) {
        drawClock();
    } else if (evt == KEY_DOWN_LONG) {
        syncTime();
        drawClock();
    }
}

void ClockManager::update() {
    if (!_active) return;
    
    // 每秒更新一次显示
    if (millis() - _lastUpdate > 1000) {
        _lastUpdate = millis();
        drawClock();
    }
    
    // 每5分钟尝试同步一次时间
    if (!_timeSynced && millis() - _lastSyncAttempt > 300000) {
        syncTime();
    }
}
