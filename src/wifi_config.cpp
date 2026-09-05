#include "wifi_config.h"
#include <string.h>

WifiConfigManager wifiConfig;

// EEPROM 地址定义
#define EEPROM_MAGIC 0xAA
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_PRESET_BASE 1
// 聊天账号密码存储地址（预设占用 1 + 3*96 = 289 字节，从 289 开始）
#define EEPROM_CHAT_USER_ADDR 289
#define EEPROM_CHAT_PASS_ADDR (EEPROM_CHAT_USER_ADDR + CHAT_USERNAME_MAX)

void WifiConfigManager::init() {
    _active = false;
    _apMode = false;
    _wifiSelecting = false;
    _server = nullptr;
    _httpUpdater = nullptr;
    _selectedPreset = 0;
    _needRedraw = true;
    
    // 初始化 EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // 加载 WiFi 预设
    loadPresets();
}

void WifiConfigManager::loadPresets() {
    // 检查魔术字节
    if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC) {
        // 首次使用，初始化空预设
        for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
            _presets[i].valid = false;
            _presets[i].ssid[0] = 0;
            _presets[i].password[0] = 0;
        }
        return;
    }
    
    // 读取预设
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        int addr = EEPROM_PRESET_BASE + i * 96;
        _presets[i].valid = (EEPROM.read(addr) == 1);
        for (int j = 0; j < 31; j++) {
            _presets[i].ssid[j] = (char)EEPROM.read(addr + 1 + j);
        }
        _presets[i].ssid[31] = 0;
        for (int j = 0; j < 63; j++) {
            _presets[i].password[j] = (char)EEPROM.read(addr + 33 + j);
        }
        _presets[i].password[63] = 0;
    }
}

void WifiConfigManager::savePresets() {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        int addr = EEPROM_PRESET_BASE + i * 96;
        EEPROM.write(addr, _presets[i].valid ? 1 : 0);
        for (int j = 0; j < 31; j++) {
            EEPROM.write(addr + 1 + j, (uint8_t)_presets[i].ssid[j]);
        }
        for (int j = 0; j < 63; j++) {
            EEPROM.write(addr + 33 + j, (uint8_t)_presets[i].password[j]);
        }
    }
    EEPROM.commit();
}

bool WifiConfigManager::tryConnectPreset(int index) {
    if (index < 0 || index >= MAX_WIFI_PRESETS || !_presets[index].valid) {
        return false;
    }
    
    WiFi.begin(_presets[index].ssid, _presets[index].password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
    }
    
    return WiFi.status() == WL_CONNECTED;
}

bool WifiConfigManager::connectWifi() {
    // 尝试连接所有预设
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        if (_presets[i].valid && tryConnectPreset(i)) {
            return true;
        }
    }
    return false;
}

bool WifiConfigManager::ensureConnected() {
    // 已连接则直接返回
    if (WiFi.status() == WL_CONNECTED) return true;
    // 正在连接中（WiFi.begin 已发起）也返回 false，让调用方稍后再试
    // 遍历预设，用第一个有效的发起连接（WiFi.begin 非阻塞，后台完成）
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        if (_presets[i].valid) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(_presets[i].ssid, _presets[i].password);
            return false;
        }
    }
    // 无有效预设
    return false;
}

bool WifiConfigManager::saveChatAccount(const char* username, const char* password) {
    if (!username || !password || !username[0] || !password[0]) return false;
    int uLen = strlen(username);
    int pLen = strlen(password);
    if (uLen >= CHAT_USERNAME_MAX) uLen = CHAT_USERNAME_MAX - 1;
    if (pLen >= CHAT_PASSWORD_MAX) pLen = CHAT_PASSWORD_MAX - 1;
    for (int i = 0; i < uLen; i++) {
        EEPROM.write(EEPROM_CHAT_USER_ADDR + i, (uint8_t)username[i]);
    }
    EEPROM.write(EEPROM_CHAT_USER_ADDR + uLen, 0);
    for (int i = 0; i < pLen; i++) {
        EEPROM.write(EEPROM_CHAT_PASS_ADDR + i, (uint8_t)password[i]);
    }
    EEPROM.write(EEPROM_CHAT_PASS_ADDR + pLen, 0);
    EEPROM.commit();
    return true;
}

void WifiConfigManager::loadChatAccount(char* username, int uMax, char* password, int pMax) {
    username[0] = 0;
    password[0] = 0;
    for (int i = 0; i < uMax - 1 && i < CHAT_USERNAME_MAX; i++) {
        char c = (char)EEPROM.read(EEPROM_CHAT_USER_ADDR + i);
        if (c == 0 || c == 0xFF) break;
        username[i] = c;
        username[i + 1] = 0;
    }
    for (int i = 0; i < pMax - 1 && i < CHAT_PASSWORD_MAX; i++) {
        char c = (char)EEPROM.read(EEPROM_CHAT_PASS_ADDR + i);
        if (c == 0 || c == 0xFF) break;
        password[i] = c;
        password[i + 1] = 0;
    }
}

bool WifiConfigManager::hasChatAccount() {
    char u[CHAT_USERNAME_MAX];
    char p[CHAT_PASSWORD_MAX];
    loadChatAccount(u, CHAT_USERNAME_MAX, p, CHAT_PASSWORD_MAX);
    return u[0] != 0 && p[0] != 0;
}

void WifiConfigManager::enter() {
    _active = true;
    _needRedraw = true;
    drawConfigMenu();
}

void WifiConfigManager::exit() {
    stopApMode();
    _active = false;
}

void WifiConfigManager::drawConfigMenu() {
    disp.clear();
    disp.drawTitleBar("配网设置");
    
    int y = 22;
    
    // WiFi 状态
    char statusLine[64];
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(statusLine, sizeof(statusLine), "WiFi: 已连接 (%s)", WiFi.SSID().c_str());
    } else {
        snprintf(statusLine, sizeof(statusLine), "WiFi: 未连接");
    }
    disp.drawText(4, y, statusLine, 1);
    y += 16;
    
    // WiFi 预设列表
    disp.drawText(4, y, "WiFi 预设:", 1);
    y += 12;
    
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        bool selected = (i == _selectedPreset);
        if (selected) {
            disp.drawRect(0, y - 2, SCREEN_W, 12, true);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
        }
        
        char line[80];
        if (_presets[i].valid) {
            snprintf(line, sizeof(line), "  %d. %s", i + 1, _presets[i].ssid);
        } else {
            snprintf(line, sizeof(line), "  %d. (空)", i + 1);
        }
        disp.drawText(4, y, line, 1);
        
        if (selected) {
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);
            u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        }
        y += 12;
    }
    
    y += 4;
    
    // 功能选项
    disp.drawText(4, y, "功能:", 1);
    y += 12;
    disp.drawText(8, y, "- 热点配网模式 (AP)", 1);
    y += 12;
    disp.drawText(8, y, "- SD卡文件管理器", 1);
    y += 12;
    disp.drawText(8, y, "- OTA 固件更新", 1);
    
    disp.drawStatusBar("上/下:选择  长按下:进入AP模式  短按上:连接预设", "Home:返回");
    disp.refresh(true);
}

void WifiConfigManager::enterWifiSelect() {
    _wifiSelecting = true;
    _selectedPreset = 0;
    drawWifiSelect();
}

void WifiConfigManager::drawWifiSelect() {
    disp.clear();
    disp.drawTitleBar("选择 WiFi");
    
    disp.drawText(4, 28, "请选择要连接的 WiFi:", 1);
    
    int y = 44;
    int validCount = 0;
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        if (!_presets[i].valid) continue;
        validCount++;
        bool selected = (i == _selectedPreset);
        if (selected) {
            disp.drawRect(2, y - 2, SCREEN_W - 4, 16, true);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
        }
        char line[64];
        snprintf(line, sizeof(line), "  %s", _presets[i].ssid);
        disp.drawText(8, y, line, 1);
        if (selected) {
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);
            u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        }
        y += 16;
    }
    
    if (validCount == 0) {
        disp.drawText(8, 60, "无预设 WiFi，请先配网", 1);
        disp.drawText(8, 78, "长按下键进入配网设置", 1);
    }
    
    disp.drawStatusBar("上/下:选择  长按下:连接", "Home:返回");
    disp.refresh(true);
}

void WifiConfigManager::handleWifiSelectKey(KeyEvent evt) {
    if (evt == KEY_NONE) return;
    
    if (evt == KEY_MENU_SHORT) {
        // 返回主页
        _wifiSelecting = false;
        app.goHome();
        return;
    }
    
    if (evt == KEY_UP_SHORT) {
        // 上一个有效预设
        int prev = _selectedPreset - 1;
        while (prev >= 0 && !_presets[prev].valid) prev--;
        if (prev >= 0) {
            _selectedPreset = prev;
            drawWifiSelect();
        }
    } else if (evt == KEY_DOWN_SHORT) {
        // 下一个有效预设
        int next = _selectedPreset + 1;
        while (next < MAX_WIFI_PRESETS && !_presets[next].valid) next++;
        if (next < MAX_WIFI_PRESETS) {
            _selectedPreset = next;
            drawWifiSelect();
        }
    } else if (evt == KEY_DOWN_LONG) {
        // 长按连接选中的预设
        if (_presets[_selectedPreset].valid) {
            disp.clear();
            disp.drawTitleBar("连接 WiFi");
            char line[64];
            snprintf(line, sizeof(line), "正在连接 %s ...", _presets[_selectedPreset].ssid);
            disp.drawText(10, 40, line, 1);
            disp.drawProgressBar(20, 60, SCREEN_W - 40, 50);
            disp.refresh(true);
            
            if (tryConnectPreset(_selectedPreset)) {
                // 连接成功，自动进入目标工具
                _wifiSelecting = false;
                app.setState(app.getPendingTarget());
            } else {
                // 连接失败
                drawWifiSelect();
                disp.drawText(4, SCREEN_H - 28, "连接失败，请重试", 1);
                disp.refresh(true);
            }
        }
    } else if (evt == KEY_UP_LONG) {
        // 长按上键：无预设时进入配网设置
        bool hasValid = false;
        for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
            if (_presets[i].valid) { hasValid = true; break; }
        }
        if (!hasValid) {
            _wifiSelecting = false;
            app.setState(STATE_CONFIG);
        }
    }
}

void WifiConfigManager::drawApMode() {
    disp.clear();
    disp.drawTitleBar("热点配网模式");
    
    disp.drawText(SCREEN_W/2 - 40, 25, "AP 模式已开启", 2);
    
    char line[64];
    snprintf(line, sizeof(line), "热点名称: %s", AP_SSID);
    disp.drawText(10, 55, line, 1);
    
    snprintf(line, sizeof(line), "密码: %s", AP_PASSWORD);
    disp.drawText(10, 70, line, 1);
    
    disp.drawText(10, 90, "请连接热点后访问:", 1);
    disp.drawText(10, 102, "http://192.168.4.1", 1);
    
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(line, sizeof(line), "已连接: %s", WiFi.SSID().c_str());
        disp.drawText(10, SCREEN_H - 24, line, 1);
    }
    
    disp.drawStatusBar("长按下:退出AP模式", "Home:返回");
    disp.refresh(true);
}

void WifiConfigManager::startApMode() {
    if (_apMode) return;
    
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    
    _server = new ESP8266WebServer(80);
    _httpUpdater = new ESP8266HTTPUpdateServer();
    
    setupWebServer();
    _server->begin();
    
    _apMode = true;
    drawApMode();
}

void WifiConfigManager::stopApMode() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    if (_httpUpdater) {
        delete _httpUpdater;
        _httpUpdater = nullptr;
    }
    WiFi.softAPdisconnect(true);
    _apMode = false;
}

void WifiConfigManager::setupWebServer() {
    if (!_server) return;
    
    // 主页
    _server->on("/", HTTP_GET, [this]() { handleRoot(); });
    
    // WiFi 扫描
    _server->on("/scan", HTTP_GET, [this]() { handleWifiScan(); });
    
    // 保存 WiFi
    _server->on("/savewifi", HTTP_POST, [this]() { handleWifiSave(); });
    
    // 保存预设
    _server->on("/savepreset", HTTP_POST, [this]() { handlePresetSave(); });
    
    // 保存聊天账号
    _server->on("/savechat", HTTP_POST, [this]() { handleChatAccountSave(); });
    
    // SD卡文件列表
    _server->on("/files", HTTP_GET, [this]() { handleFileList(); });
    
    // 上传文件（简化版，ESP8266WebServer上传处理较复杂，这里先占位）
    _server->on("/upload", HTTP_POST, [this]() {
        _server->send(200, "text/plain", "文件上传功能开发中");
    });
    
    // 删除文件
    _server->on("/delete", HTTP_POST, [this]() { handleFileDelete(); });
    
    // 新建文件夹
    _server->on("/mkdir", HTTP_POST, [this]() { handleMkdir(); });
    
    // OTA 更新
    _httpUpdater->setup(_server, "/update", WEB_USER, WEB_PASS);
}

void WifiConfigManager::handleRoot() {
    String html = "<html><head><meta charset='utf-8'><title>虚空终端配网</title></head><body>";
    html += "<h1>虚空终端 OS 配网</h1>";
    
    // WiFi 状态
    if (WiFi.status() == WL_CONNECTED) {
        html += "<p>WiFi 已连接: " + WiFi.SSID() + "</p>";
        html += "<p>IP: " + WiFi.localIP().toString() + "</p>";
    } else {
        html += "<p>WiFi 未连接</p>";
    }
    
    // WiFi 配置表单
    html += "<h2>WiFi 配置</h2>";
    html += "<form method='post' action='/savewifi'>";
    html += "SSID: <input type='text' name='ssid'><br>";
    html += "密码: <input type='password' name='password'><br>";
    html += "<input type='submit' value='连接'>";
    html += "</form>";
    
    // WiFi 预设
    html += "<h2>WiFi 预设 (最多3个)</h2>";
    for (int i = 0; i < MAX_WIFI_PRESETS; i++) {
        html += "<h3>预设 " + String(i+1) + "</h3>";
        html += "<form method='post' action='/savepreset'>";
        html += "<input type='hidden' name='index' value='" + String(i) + "'>";
        html += "SSID: <input type='text' name='ssid' value='" + String(_presets[i].ssid) + "'><br>";
        html += "密码: <input type='password' name='password' value='" + String(_presets[i].password) + "'><br>";
        html += "<input type='checkbox' name='valid' " + String(_presets[i].valid ? "checked" : "") + "> 启用<br>";
        html += "<input type='submit' value='保存'>";
        html += "</form>";
    }
    
    // 聊天账号密码
    html += "<h2>虚空终端账号</h2>";
    char uBuf[CHAT_USERNAME_MAX];
    char pBuf[CHAT_PASSWORD_MAX];
    loadChatAccount(uBuf, CHAT_USERNAME_MAX, pBuf, CHAT_PASSWORD_MAX);
    html += "<form method='post' action='/savechat'>";
    html += "账号: <input type='text' name='username' value='" + String(uBuf) + "'><br>";
    html += "密码: <input type='password' name='password' value='" + String(pBuf) + "'><br>";
    html += "<input type='submit' value='保存账号'>";
    html += "</form>";
    html += "<p>" + String(hasChatAccount() ? "已配置账号，聊天自动登录" : "未配置账号") + "</p>";
    
    // SD卡文件管理器
    html += "<h2>SD卡文件管理器</h2>";
    html += "<p><a href='/files'>查看文件列表</a></p>";
    html += "<form method='post' action='/upload' enctype='multipart/form-data'>";
    html += "上传TXT文件: <input type='file' name='file'><br>";
    html += "<input type='submit' value='上传'>";
    html += "</form>";
    html += "<form method='post' action='/mkdir'>";
    html += "新建文件夹: <input type='text' name='dirname'><br>";
    html += "<input type='submit' value='创建'>";
    html += "</form>";
    
    // OTA
    html += "<h2>OTA 固件更新</h2>";
    html += "<p><a href='/update'>进入 OTA 更新页面</a> (用户名: admin 密码: xzmlwjh1)</p>";
    
    html += "</body></html>";
    _server->send(200, "text/html", html);
}

void WifiConfigManager::handleWifiScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    }
    json += "]";
    _server->send(200, "application/json", json);
}

void WifiConfigManager::handleWifiSave() {
    if (_server->hasArg("ssid") && _server->hasArg("password")) {
        String ssid = _server->arg("ssid");
        String password = _server->arg("password");
        WiFi.begin(ssid.c_str(), password.c_str());
        _server->send(200, "text/plain", "正在连接 WiFi...");
    } else {
        _server->send(400, "text/plain", "参数错误");
    }
}

void WifiConfigManager::handlePresetSave() {
    if (_server->hasArg("index") && _server->hasArg("ssid")) {
        int index = _server->arg("index").toInt();
        if (index >= 0 && index < MAX_WIFI_PRESETS) {
            strncpy(_presets[index].ssid, _server->arg("ssid").c_str(), 31);
            _presets[index].ssid[31] = 0;
            if (_server->hasArg("password")) {
                strncpy(_presets[index].password, _server->arg("password").c_str(), 63);
                _presets[index].password[63] = 0;
            }
            _presets[index].valid = _server->hasArg("valid");
            savePresets();
            _server->send(200, "text/plain", "预设已保存");
            return;
        }
    }
    _server->send(400, "text/plain", "参数错误");
}

void WifiConfigManager::handleChatAccountSave() {
    if (_server->hasArg("username") && _server->hasArg("password")) {
        String username = _server->arg("username");
        String password = _server->arg("password");
        if (saveChatAccount(username.c_str(), password.c_str())) {
            _server->send(200, "text/plain", "聊天账号已保存");
        } else {
            _server->send(400, "text/plain", "账号或密码为空");
        }
    } else {
        _server->send(400, "text/plain", "参数错误");
    }
}

void WifiConfigManager::handleFileList() {
    if (!SD.begin(SD_CS)) {
        _server->send(500, "text/plain", "SD卡初始化失败");
        return;
    }
    
    String path = "/";
    if (_server->hasArg("path")) {
        path = _server->arg("path");
    }
    
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        _server->send(404, "text/plain", "目录不存在");
        return;
    }
    
    String html = "<html><head><meta charset='utf-8'><title>文件列表</title></head><body>";
    html += "<h1>SD卡文件列表 - " + path + "</h1>";
    html += "<table border='1'><tr><th>名称</th><th>类型</th><th>大小</th></tr>";
    
    File entry = dir.openNextFile();
    while (entry) {
        html += "<tr><td>" + String(entry.name()) + "</td>";
        html += "<td>" + String(entry.isDirectory() ? "文件夹" : "文件") + "</td>";
        html += "<td>" + String(entry.size()) + " 字节</td></tr>";
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    
    html += "</table>";
    html += "<p><a href='/'>返回主页</a></p>";
    html += "</body></html>";
    _server->send(200, "text/html", html);
}

void WifiConfigManager::handleFileUpload() {
    // 文件上传处理（简化版）
    // 实际实现需要处理 HTTP 分片上传
    _server->send(200, "text/plain", "文件上传功能");
}

void WifiConfigManager::handleFileDelete() {
    if (_server->hasArg("path")) {
        String path = _server->arg("path");
        if (SD.remove(path.c_str())) {
            _server->send(200, "text/plain", "文件已删除");
        } else {
            _server->send(500, "text/plain", "删除失败");
        }
    } else {
        _server->send(400, "text/plain", "参数错误");
    }
}

void WifiConfigManager::handleMkdir() {
    if (_server->hasArg("dirname")) {
        String dirname = _server->arg("dirname");
        if (SD.mkdir(dirname.c_str())) {
            _server->send(200, "text/plain", "文件夹已创建");
        } else {
            _server->send(500, "text/plain", "创建失败");
        }
    } else {
        _server->send(400, "text/plain", "参数错误");
    }
}

void WifiConfigManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    
    if (evt == KEY_MENU_SHORT) {
        exit();
        app.goHome();
        return;
    }
    
    if (_apMode) {
        if (evt == KEY_DOWN_LONG) {
            stopApMode();
            drawConfigMenu();
        }
    } else {
        if (evt == KEY_UP_SHORT) {
            if (_selectedPreset > 0) {
                _selectedPreset--;
                drawConfigMenu();
            }
        } else if (evt == KEY_DOWN_SHORT) {
            if (_selectedPreset < MAX_WIFI_PRESETS - 1) {
                _selectedPreset++;
                drawConfigMenu();
            }
        } else if (evt == KEY_DOWN_LONG) {
            startApMode();
        } else if (evt == KEY_UP_LONG) {
            // 尝试连接选中的预设
            if (tryConnectPreset(_selectedPreset)) {
                drawConfigMenu();
            }
        }
    }
}

void WifiConfigManager::update() {
    if (!_active) return;
    
    if (_apMode && _server) {
        _server->handleClient();
    }
}
