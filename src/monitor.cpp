#include "monitor.h"
#include "wifi_config.h"
#include <ArduinoJson.h>

MonitorManager monitor;

void MonitorManager::init() {
    _active = false;
    _selectedServer = 0;
    _lastFullRefresh = 0;
    _needFullRefresh = false;
    for (int i = 0; i < 2; i++) {
        _data[i].valid = false;
        _lastPartialRefresh[i] = 0;
    }
}

void MonitorManager::enter() {
    _active = true;
    _selectedServer = 0;
    _needFullRefresh = true;
    wifiConfig.ensureConnected();  // 自动尝试连接 WiFi
    drawLoading();
}

void MonitorManager::exit() {
    _active = false;
}

const char* MonitorManager::getServerName(int index) {
    return (index == 0) ? MONITOR_NAME1 : MONITOR_NAME2;
}

const char* MonitorManager::getServerHost(int index) {
    return (index == 0) ? MONITOR_SERVER1 : MONITOR_SERVER2;
}

int MonitorManager::getServerPort(int index) {
    return (index == 0) ? MONITOR_PORT1 : MONITOR_PORT2;
}

void MonitorManager::drawLoading() {
    disp.clear();
    disp.drawTitleBar("服务器监控");
    disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2 - 10, "正在加载...", 1);
    disp.drawProgressBar(20, SCREEN_H/2 + 10, SCREEN_W - 40, 50);
    disp.refresh(true);
}

void MonitorManager::drawError(const char* msg) {
    disp.clear();
    disp.drawTitleBar("服务器监控");
    disp.drawText(10, 30, "获取数据失败", 2);
    disp.drawText(10, 60, msg, 1);
    disp.drawStatusBar("短按上/下:切换  长按下:刷新", "Home:返回");
    disp.refresh(true);
}

bool MonitorManager::fetchData(int serverIndex) {
    if (WiFi.status() != WL_CONNECTED) {
        strcpy(_data[serverIndex].status, "WiFi未连接");
        _data[serverIndex].valid = false;
        return false;
    }
    
    WiFiClient client;
    client.setTimeout(3000);
    HTTPClient http;
    http.setTimeout(3000);
    String url = String("http://") + getServerHost(serverIndex) + ":" + getServerPort(serverIndex) + "/api/status";
    http.begin(client, url);
    
    int httpCode = http.GET();
    if (httpCode != 200) {
        strcpy(_data[serverIndex].status, "连接失败");
        _data[serverIndex].valid = false;
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, payload)) {
        strcpy(_data[serverIndex].status, "解析失败");
        _data[serverIndex].valid = false;
        return false;
    }
    
    MonitorData& d = _data[serverIndex];
    d.cpuUsage = doc["cpu"] | doc["cpu_usage"] | doc["cpuUsage"] | 0.0f;
    d.memoryUsage = doc["memory"] | doc["mem"] | doc["memory_usage"] | doc["mem_usage"] | 0.0f;
    d.memoryTotal = doc["memory_total"] | doc["mem_total"] | doc["memory"]["total"] | 2.0f;
    d.memoryUsed = doc["memory_used"] | doc["mem_used"] | doc["memory"]["used"] | 0.0f;
    if (d.memoryUsed == 0 && d.memoryTotal > 0 && d.memoryUsage > 0) {
        d.memoryUsed = d.memoryTotal * d.memoryUsage / 100;
    }
    d.diskUsage = doc["disk"] | doc["disk_usage"] | doc["diskUsage"] | 0.0f;
    d.diskTotal = doc["disk_total"] | doc["disk"]["total"] | 30.0f;
    d.diskUsed = doc["disk_used"] | doc["disk"]["used"] | 0.0f;
    if (d.diskUsed == 0 && d.diskTotal > 0 && d.diskUsage > 0) {
        d.diskUsed = d.diskTotal * d.diskUsage / 100;
    }
    d.networkIn = doc["network_in"] | doc["net_in"] | doc["network"]["in"] | 0.0f;
    d.networkOut = doc["network_out"] | doc["net_out"] | doc["network"]["out"] | 0.0f;
    d.processCount = doc["processes"] | doc["process_count"] | doc["procs"] | 0;
    d.uptimeHours = doc["uptime"] | doc["uptime_hours"] | 0;
    
    strcpy(d.status, "运行中");
    d.valid = true;
    return true;
}

void MonitorManager::drawMonitor(bool fullRefresh) {
    disp.clear();
    
    // 标题栏：左侧服务器名，右侧序号
    char title[40];
    snprintf(title, sizeof(title), "服务器监控 - %s", getServerName(_selectedServer));
    disp.drawTitleBar(title);
    // 右侧序号（标题栏黑底白字）
    char idx[16];
    snprintf(idx, sizeof(idx), "%d/2", _selectedServer + 1);
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    int idxW = disp.getTextWidth(idx);
    disp.drawText(SCREEN_W - idxW - 4, 3, idx, 1);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    
    MonitorData& d = _data[_selectedServer];
    
    if (!d.valid) {
        disp.drawText(10, 40, "获取数据失败", 2);
        disp.drawText(10, 70, d.status, 1);
        disp.drawStatusBar("短按上/下:切换  长按下:刷新", "Home:返回");
        disp.refresh(fullRefresh);
        return;
    }
    
    int y = 24;
    char line[60];
    
    // CPU
    snprintf(line, sizeof(line), "CPU:  %.1f%%", d.cpuUsage);
    disp.drawText(4, y, line, 1);
    disp.drawProgressBar(70, y + 2, 120, (int)d.cpuUsage);
    y += 16;
    
    // 内存
    snprintf(line, sizeof(line), "内存: %.2f/%.2f GB (%.1f%%)", 
             d.memoryUsed, d.memoryTotal, d.memoryUsage);
    disp.drawText(4, y, line, 1);
    disp.drawProgressBar(70, y + 2, 120, (int)d.memoryUsage);
    y += 16;
    
    // 磁盘
    snprintf(line, sizeof(line), "磁盘: %.1f/%.1f GB (%.1f%%)", 
             d.diskUsed, d.diskTotal, d.diskUsage);
    disp.drawText(4, y, line, 1);
    disp.drawProgressBar(70, y + 2, 120, (int)d.diskUsage);
    y += 16;
    
    // 网络
    snprintf(line, sizeof(line), "网络: ↓%.2f MB/s  ↑%.2f MB/s", 
             d.networkIn, d.networkOut);
    disp.drawText(4, y, line, 1);
    y += 14;
    
    // 进程和运行时间
    snprintf(line, sizeof(line), "进程: %d  运行: %d小时", 
             d.processCount, d.uptimeHours);
    disp.drawText(4, y, line, 1);
    
    disp.drawStatusBar("短按上/下:切换服务器  长按下:刷新", "Home:返回");
    disp.refresh(fullRefresh);
}

void MonitorManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    
    if (evt == KEY_MENU_SHORT) {
        exit();
        app.goHome();
        return;
    }
    
    if (evt == KEY_UP_SHORT) {
        // 切换到上一个服务器
        _selectedServer = (_selectedServer - 1 + 2) % 2;
        _needFullRefresh = true;
    } else if (evt == KEY_DOWN_SHORT) {
        // 切换到下一个服务器
        _selectedServer = (_selectedServer + 1) % 2;
        _needFullRefresh = true;
    } else if (evt == KEY_DOWN_LONG) {
        // 长按手动刷新（3秒间隔限制）
        if (millis() - _lastPartialRefresh[_selectedServer] > 3000) {
            _lastPartialRefresh[_selectedServer] = millis();
            fetchData(_selectedServer);
            _needFullRefresh = true;
        }
    }
}

void MonitorManager::update() {
    if (!_active) return;
    
    // WiFi 未连接时持续尝试（后台连接）
    if (WiFi.status() != WL_CONNECTED) {
        wifiConfig.ensureConnected();
    }
    
    unsigned long now = millis();
    
    // 当前服务器：1秒自动局部刷新
    if (now - _lastPartialRefresh[_selectedServer] > MONITOR_PARTIAL_REFRESH_MS) {
        _lastPartialRefresh[_selectedServer] = now;
        fetchData(_selectedServer);
        drawMonitor(false);  // 局部刷新
        _needFullRefresh = false;
    }
    
    // 10秒全局刷新消除残影
    if (now - _lastFullRefresh > MONITOR_FULL_REFRESH_MS) {
        _lastFullRefresh = now;
        drawMonitor(true);
        _needFullRefresh = false;
        return;
    }
    
    // 切换服务器后立即全屏刷新
    if (_needFullRefresh) {
        _needFullRefresh = false;
        drawMonitor(true);
    }
}
