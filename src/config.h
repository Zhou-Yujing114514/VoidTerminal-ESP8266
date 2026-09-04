#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>

// ===== 版本信息 =====
#define FW_VERSION "2.3.3"
#define FW_NAME "VoidTerminal-ESP8266"

// ===== 引脚配置 (A01 墨水屏 + ESP-12F) =====
#define EPD_CS    15
#define EPD_DC    0
#define EPD_RST   2
#define EPD_BUSY  4
#define SD_CS     5
#define KEY_MENU  12   // 按键1（左/Home）- 注意：避免与其他功能冲突
#define KEY_UP    0    // 按键2（中/上一页）- 与 EPD_DC 共用，读取时注意
#define KEY_DOWN  3    // 按键3（右/下一页）- SD卡版改为GPIO3(RX引脚)，不与SD_CS冲突

// ===== 屏幕参数 =====
#define SCREEN_W 296
#define SCREEN_H 128

// ===== 服务器配置 =====
#define CHAT_SERVER "buer.kdns.fr"
#define CHAT_PORT 443          // HTTPS 端口（80端口已被服务器关闭）
// 服务器 TLS 证书 SHA1 指纹（十六进制），用于 beginSSL 与 setFingerprint
#define CHAT_SSL_FINGERPRINT_HEX "1C:86:71:D8:C7:8C:C4:BA:58:43:B6:12:FF:36:4E:63:7E:51:FA:E1"
#define MONITOR_SERVER "cn-fj-qz-1.server.zakocloud.com"
#define MONITOR_PORT 18080

// ===== 配网配置 =====
#define AP_SSID "CMCC-Admin"
#define AP_PASSWORD "xzmlwjh1"
#define WEB_USER "admin"
#define WEB_PASS "xzmlwjh1"
#define MAX_WIFI_PRESETS 3

// ===== 存储配置 =====
#define EEPROM_SIZE 512

// ===== 按键时长 =====
#define KEY_DEBOUNCE_MS 50
#define KEY_LONGPRESS_MS 800
#define KEY_DOUBLECLICK_MS 300

// ===== 全局对象声明 =====
extern GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display;
extern U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// ===== 颜色定义 =====
#define COLOR_WHITE GxEPD_WHITE
#define COLOR_BLACK GxEPD_BLACK

// ===== 工具函数 =====
void debugPrint(const char* msg);
void debugPrintf(const char* fmt, ...);

#endif // CONFIG_H
