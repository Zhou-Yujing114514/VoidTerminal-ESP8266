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
    int _selectedServer;
    MonitorData _data[2];
    unsigned long _lastPartialRefresh[2];
    unsigned long _lastFullRefresh;
    bool _needFullRefresh;
    char _diag[128];  // 诊断状态（显示在屏幕上）
    
    void drawMonitor(bool fullRefresh);
    bool fetchData(int serverIndex);
    void drawLoading();
    void drawError(const char* msg);
    const char* getServerName(int index);
    const char* getServerHost(int index);
    int getServerPort(int index);
};

extern MonitorManager monitor;

#endif // MONITOR_H
