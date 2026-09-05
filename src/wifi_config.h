#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "config.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <SD.h>
#include <EEPROM.h>

// WiFi 预设结构
struct WifiPreset {
    char ssid[32];
    char password[64];
    bool valid;
};

class WifiConfigManager {
public:
    void init();
    void enter();
    void exit();
    void update();
    void handleKey(KeyEvent evt);
    bool isActive() { return _active; }
    bool connectWifi();
    bool isWifiConnected() { return WiFi.status() == WL_CONNECTED; }
    // 非阻塞确保已连接：已连接直接返回true；否则发起连接（后台完成）并返回false
    bool ensureConnected();
    // 聊天账号密码（存储在EEPROM，配网网页配置）
    bool saveChatAccount(const char* username, const char* password);
    void loadChatAccount(char* username, int uMax, char* password, int pMax);
    bool hasChatAccount();
    
private:
    bool _active;
    bool _apMode;
    ESP8266WebServer* _server;
    ESP8266HTTPUpdateServer* _httpUpdater;
    WifiPreset _presets[MAX_WIFI_PRESETS];
    int _selectedPreset;
    bool _needRedraw;
    
    void drawConfigMenu();
    void drawWifiStatus();
    void drawApMode();
    void startApMode();
    void stopApMode();
    void setupWebServer();
    void handleRoot();
    void handleWifiScan();
    void handleWifiSave();
    void handlePresetSave();
    void handleChatAccountSave();
    void handleFileList();
    void handleFileUpload();
    void handleFileDelete();
    void handleMkdir();
    void loadPresets();
    void savePresets();
    bool tryConnectPreset(int index);
};

extern WifiConfigManager wifiConfig;

#endif // WIFI_CONFIG_H
