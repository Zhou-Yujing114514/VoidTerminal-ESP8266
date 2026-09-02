#ifndef MONITOR_H
#define MONITOR_H

#include "config.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// 服务器监控数据
struct MonitorData {
    float cpuUsage;
    float memoryUsage;
    float memoryTotal;
    float memoryUsed;
    float diskUsage;
    float diskTotal;
    float diskUsed;
    float networkIn;
    float networkOut;
    int processCount;
    int uptimeHours;
    char status[32];
    bool valid;
};

class MonitorManager {
public:
    void init();
    void enter();
    void exit();
    void update();
    void handleKey(KeyEvent evt);
    bool isActive() { return _active; }
    
private:
    bool _active;
    bool _firstLoad;  // 首次加载标记（异步加载用）
    MonitorData _data;
    unsigned long _lastFullRefresh;
    unsigned long _lastPartialRefresh;
    int _selectedServer;
    bool _needRedraw;
    
    void drawMonitor();
    void drawPartialUpdate();
    bool fetchData();
    void drawLoading();
    void drawError(const char* msg);
};

extern MonitorManager monitor;

#endif // MONITOR_H
