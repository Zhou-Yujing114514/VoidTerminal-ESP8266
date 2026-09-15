#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>

// ===== 版本信息 =====
// 代码审查修复版：修复 token 拷贝未终止的潜在越界读；其余功能逻辑未改动。
// 注：上一版本为 2.3.3，按语义化版本单调递增原则取 2.4.0，而非回退到 2.1.0。
#define FW_VERSION "2.4.0"
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
// 监控服务器地址/端口：不再在源码中硬编码生产基础设施信息。
// 出厂默认为空（监控功能默认关闭），首次使用通过配网网页写入 EEPROM 运行时配置。
// 安全说明：监控探针为 HTTP 明文，仅应在内网/加密隧道（如 WireGuard/frp TLS 隧道）中使用，
// 不要把公网可达的监控端点直接暴露在固件中。
#define MONITOR_SERVER1 ""
#define MONITOR_PORT1 0
#define MONITOR_NAME1 "聊天站"
#define MONITOR_SERVER2 ""
#define MONITOR_PORT2 0
#define MONITOR_NAME2 "小说站"
#define MONITOR_PARTIAL_REFRESH_MS 1000
#define MONITOR_FULL_REFRESH_MS 10000

// ===== 配网配置 =====
// AP 口令与 Web 配网/OTA 口令：不再硬编码。
// 首次上电时由设备用芯片ID+硬件随机数生成，写入 EEPROM，并在墨水屏配网界面上显示。
// AP_SSID 为非敏感广播名，保留；WEB_USER 为 OTA 用户名，非敏感，保留。
#define AP_SSID "CMCC-Admin"
#define WEB_USER "admin"
#define MAX_WIFI_PRESETS 3

// ===== 存储配置 =====
#define EEPROM_SIZE 512
#define CHAT_USERNAME_MAX 32
#define CHAT_PASSWORD_MAX 64

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
