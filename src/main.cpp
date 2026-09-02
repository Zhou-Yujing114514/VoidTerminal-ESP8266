/*
 * VoidTerminal-ESP8266 - 虚空终端 ESP8266 自研操作系统
 * 硬件: ESP-12F / 4MB Flash / DIO / 80MHz
 * 屏幕: 2.9寸 A01 墨水屏 (296x128)
 * 功能: 虚空终端聊天(九宫格输入法)、服务器监控、TXT阅读器、时钟、配网系统
 */

#include "config.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include "chat.h"
#include "monitor.h"
#include "reader.h"
#include "clock.h"
#include "wifi_config.h"

// 全局对象 display 和 u8g2Fonts 在 display.cpp 中定义

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\nVoidTerminal-ESP8266 启动中...");
    Serial.printf("版本: %s\n", FW_VERSION);
    Serial.printf("屏幕引脚: CS=%d DC=%d RST=%d BUSY=%d\n", EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
    Serial.printf("按键引脚: MENU=%d UP=%d DOWN=%d\n", KEY_MENU, KEY_UP, KEY_DOWN);
    
    // 初始化屏幕
    Serial.println("初始化屏幕...");
    disp.init();
    Serial.println("屏幕初始化完成");
    
    // 打印启动信息和剩余 RAM（方便调试）
    Serial.printf("剩余 RAM: %d 字节 (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.printf("Flash 大小: %d 字节\n", ESP.getFlashChipSize());
    
    // 初始化其他模块（app.init() 里会显示启动画面）
    input.init();
    app.init();
    chat.init();
    monitor.init();
    reader.init();
    clockMgr.init();
    wifiConfig.init();
    
    // 再次打印 RAM（初始化后）
    Serial.printf("初始化后剩余 RAM: %d 字节 (%.1f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    
    // 尝试连接预设 WiFi（后台连接，不阻塞启动）
    if (wifiConfig.isWifiConnected()) {
        Serial.println("WiFi 已连接");
    } else {
        Serial.println("尝试连接预设 WiFi...");
    }
    
    Serial.println("启动完成!");
}

void loop() {
    // 更新按键输入
    input.update();
    
    // 获取按键事件
    KeyEvent evt = input.getEvent();
    
    // 根据当前状态分发事件
    AppState state = app.getCurrentState();
    
    switch (state) {
        case STATE_MENU:
            app.handleKey(evt);
            break;
            
        case STATE_CHAT:
            chat.handleKey(evt);
            chat.update();
            break;
            
        case STATE_MONITOR:
            monitor.handleKey(evt);
            monitor.update();
            break;
            
        case STATE_READER:
            reader.handleKey(evt);
            reader.update();
            break;
            
        case STATE_CLOCK:
            clockMgr.handleKey(evt);
            clockMgr.update();
            break;
            
        case STATE_CONFIG:
            wifiConfig.handleKey(evt);
            wifiConfig.update();
            break;
            
        default:
            app.handleKey(evt);
            break;
    }
    
    // 更新应用状态（重绘等）
    app.update();
    
    // 状态切换处理
    AppState newState = app.getCurrentState();
    if (newState != state) {
        // 退出旧状态
        switch (state) {
            case STATE_CHAT: chat.exit(); break;
            case STATE_MONITOR: monitor.exit(); break;
            case STATE_READER: reader.exit(); break;
            case STATE_CLOCK: clockMgr.exit(); break;
            case STATE_CONFIG: wifiConfig.exit(); break;
            default: break;
        }
        
        // 进入新状态
        switch (newState) {
            case STATE_CHAT: chat.enter(); break;
            case STATE_MONITOR: monitor.enter(); break;
            case STATE_READER: reader.enter(); break;
            case STATE_CLOCK: clockMgr.enter(); break;
            case STATE_CONFIG: wifiConfig.enter(); break;
            default: break;
        }
    }
    
    // 短暂延时，降低CPU占用
    delay(10);
}
