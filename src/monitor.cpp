#include "monitor.h"
#include <ArduinoJson.h>

MonitorManager monitor;

void MonitorManager::init() {
    _active = false;
    _firstLoad = false;
    _selectedServer = 0;
    _needRedraw = true;
    _data.valid = false;
    _lastFullRefresh = 0;
    _lastPartialRefresh = 0;
}

void MonitorManager::enter() {
    _active = true;
    _needRedraw = true;
    _data.valid = false;
    _firstLoad = true;  // 标记首次加载，在update()中异步获取
    drawLoading();
}

void MonitorManager::exit() {
    _active = false;
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
    disp.drawStatusBar("短按下:重试", "Home:返回");
    disp.refresh(true);
}

bool MonitorManager::fetchData() {
    if (WiFi.status() != WL_CONNECTED) {
        strcpy(_data.status, "WiFi未连接");
        _data.valid = false;
        return false;
    }
    
    WiFiClient client;
    HTTPClient http;
    String url = String("http://") + MONITOR_SERVER + ":" + MONITOR_PORT + "/api/status";
    http.begin(client, url);
    http.setTimeout(3000);
    
    int httpCode = http.GET();
    if (httpCode != 200) {
        strcpy(_data.status, "连接失败");
        _data.valid = false;
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // 解析 JSON
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, payload)) {
        strcpy(_data.status, "解析失败");
        _data.valid = false;
        return false;
    }
    
    // 尝试从多种可能的字段名中提取数据
    _data.cpuUsage = doc["cpu"] | doc["cpu_usage"] | doc["cpuUsage"] | 0.0f;
    _data.memoryUsage = doc["memory"] | doc["mem"] | doc["memory_usage"] | doc["mem_usage"] | 0.0f;
    _data.memoryTotal = doc["memory_total"] | doc["mem_total"] | doc["memory"]["total"] | 2.0f;
    _data.memoryUsed = doc["memory_used"] | doc["mem_used"] | doc["memory"]["used"] | 0.0f;
    if (_data.memoryUsed == 0 && _data.memoryTotal > 0 && _data.memoryUsage > 0) {
        _data.memoryUsed = _data.memoryTotal * _data.memoryUsage / 100;
    }
    _data.diskUsage = doc["disk"] | doc["disk_usage"] | doc["diskUsage"] | 0.0f;
    _data.diskTotal = doc["disk_total"] | doc["disk"]["total"] | 30.0f;
    _data.diskUsed = doc["disk_used"] | doc["disk"]["used"] | 0.0f;
    if (_data.diskUsed == 0 && _data.diskTotal > 0 && _data.diskUsage > 0) {
        _data.diskUsed = _data.diskTotal * _data.diskUsage / 100;
    }
    _data.networkIn = doc["network_in"] | doc["net_in"] | doc["network"]["in"] | 0.0f;
    _data.networkOut = doc["network_out"] | doc["net_out"] | doc["network"]["out"] | 0.0f;
    _data.processCount = doc["processes"] | doc["process_count"] | doc["procs"] | 0;
    _data.uptimeHours = doc["uptime"] | doc["uptime_hours"] | 0;
    
    strcpy(_data.status, "运行中");
    _data.valid = true;
    return true;
}

void MonitorManager::drawMonitor() {
    disp.clear();
    disp.drawTitleBar("服务器监控");
    
    if (!_data.valid) {
        drawError("无数据");
        return;
    }
    
    int y = 22;
    
    // 状态
    char line[60];
    snprintf(line, sizeof(line), "状态: %s", _data.status);
    disp.drawText(4, y, line, 1);
    y += 12;
    
    // CPU
    snprintf(line, sizeof(line), "CPU:  %.1f%%", _data.cpuUsage);
    disp.drawText(4, y, line, 1);
    disp.drawProgressBar(70, y + 2, 100, (int)_data.cpuUsage);
    y += 14;
    
    // 内存
    snprintf(line, sizeof(line), "内存: %.1f/%.1f GB (%.1f%%)", 
             _data.memoryUsed, _data.memoryTotal, _data.memoryUsage);
    disp.drawText(4, y, line, 1);
    disp.drawProgressBar(70, y + 2, 100, (int)_data.memoryUsage);
    y += 14;
    
    // 磁盘
    snprintf(line, sizeof(line), "磁盘: %.1f/%.1f GB (%.1f%%)", 
             _data.diskUsed, _data.diskTotal, _data.diskUsage);
    disp.drawText(4, y, line, 1);
    disp.drawProgressBar(70, y + 2, 100, (int)_data.diskUsage);
    y += 14;
    
    // 网络
    snprintf(line, sizeof(line), "网络: ↓%.1f MB/s  ↑%.1f MB/s", 
             _data.networkIn, _data.networkOut);
    disp.drawText(4, y, line, 1);
    y += 12;
    
    // 进程和运行时间
    snprintf(line, sizeof(line), "进程: %d  运行: %d小时", 
             _data.processCount, _data.uptimeHours);
    disp.drawText(4, y, line, 1);
    y += 12;
    
    // 服务器地址
    snprintf(line, sizeof(line), "服务器: %s", MONITOR_SERVER);
    disp.drawText(4, y, line, 1);
    
    disp.drawStatusBar("短按下:刷新  长按下:全局刷新", "Home:返回");
    disp.refresh(true);
}

void MonitorManager::drawPartialUpdate() {
    // 局部刷新：只更新变化的数值区域
    // 墨水屏局部刷新需要特殊处理，这里简化为全屏刷新
    drawMonitor();
}

void MonitorManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    
    if (evt == KEY_MENU_SHORT) {
        exit();
        app.goHome();
        return;
    }
    
    if (evt == KEY_DOWN_SHORT) {
        // 短按下 = 局部刷新（5秒间隔，避免频繁请求）
        if (millis() - _lastPartialRefresh > 5000) {
            _lastPartialRefresh = millis();
            fetchData();
            drawPartialUpdate();
        }
    } else if (evt == KEY_DOWN_LONG) {
        // 长按下 = 全局刷新（10秒间隔）
        if (millis() - _lastFullRefresh > 10000) {
            _lastFullRefresh = millis();
            fetchData();
            drawMonitor();
        }
    }
}

void MonitorManager::update() {
    if (!_active) return;
    
    // 首次加载：异步获取数据（避免enter()阻塞）
    if (_firstLoad) {
        _firstLoad = false;
        fetchData();
        if (_active) {
            drawMonitor();
        }
        return;
    }
    
    // 自动30秒局部刷新（避免频繁HTTP请求阻塞界面）
    if (millis() - _lastPartialRefresh > 30000) {
        _lastPartialRefresh = millis();
        fetchData();
        if (_active) {
            drawPartialUpdate();
        }
    }
    
    // 自动60秒全局刷新
    if (millis() - _lastFullRefresh > 60000) {
        _lastFullRefresh = millis();
        if (_active) {
            drawMonitor();
        }
    }
}
