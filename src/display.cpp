#include "display.h"

GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
DisplayManager disp;

// 字体高度（wqy14 约14像素高）
#define FONT_HEIGHT 14
#define FONT_WIDTH 8

void DisplayManager::init() {
    display.init(115200);
    display.setRotation(3); // 横屏 296x128，旋转180度修正方向
    display.setTextColor(COLOR_BLACK);
    display.setFullWindow();
    
    // 初始化 U8g2 中文字体
    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1); // 透明模式
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312a); // 文泉驿14号，一级常用汉字
    
    _needFullRefresh = true;
    _currentFontSize = 1;
    clear();
}

void DisplayManager::clear() {
    display.fillScreen(COLOR_WHITE);
}

void DisplayManager::refresh(bool full) {
    if (full || _needFullRefresh) {
        display.display();
        _needFullRefresh = false;
    } else {
        display.display(false);
    }
}

void DisplayManager::drawText(int x, int y, const char* text, int size) {
    // 用 U8g2 中文字体显示
    // u8g2 的 y 是基线位置，需要加上字体高度
    int baselineY = y + FONT_HEIGHT - 2;
    
    // size >= 2 时用更大的字体
    if (size >= 2) {
        u8g2Fonts.setFont(u8g2_font_wqy16_t_gb2312a);
        baselineY = y + 16 - 2;
    } else {
        u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312a);
    }
    
    u8g2Fonts.setCursor(x, baselineY);
    u8g2Fonts.print(text);
    
    // 恢复默认字体
    if (size >= 2) {
        u8g2Fonts.setFont(u8g2_font_wqy14_t_gb2312a);
    }
}

void DisplayManager::drawText_P(int x, int y, const char* text, int size) {
    drawText(x, y, text, size);
}

void DisplayManager::drawRect(int x, int y, int w, int h, bool filled) {
    if (filled) {
        display.fillRect(x, y, w, h, COLOR_BLACK);
    } else {
        display.drawRect(x, y, w, h, COLOR_BLACK);
    }
}

void DisplayManager::drawLine(int x1, int y1, int x2, int y2) {
    display.drawLine(x1, y1, x2, y2, COLOR_BLACK);
}

void DisplayManager::drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h) {
    display.drawBitmap(x, y, bitmap, w, h, COLOR_BLACK);
}

void DisplayManager::setFont(int size) {
    _currentFontSize = size;
}

int DisplayManager::getTextWidth(const char* text) {
    // 中文字符宽度约等于字体高度，英文约一半
    int w = 0;
    int i = 0;
    while (text[i]) {
        if ((unsigned char)text[i] >= 0x80) {
            w += FONT_HEIGHT; // 中文
            i += 3; // UTF-8 中文占3字节
        } else {
            w += FONT_WIDTH; // 英文
            i++;
        }
    }
    return w;
}

void DisplayManager::drawTitleBar(const char* title) {
    drawRect(0, 0, SCREEN_W, 20, true);
    // 标题栏文字用白色
    u8g2Fonts.setForegroundColor(GxEPD_WHITE);
    u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    drawText(4, 3, title, 1);
    u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
}

void DisplayManager::drawStatusBar(const char* left, const char* right) {
    drawLine(0, SCREEN_H - 16, SCREEN_W, SCREEN_H - 16);
    if (left) drawText(2, SCREEN_H - 14, left, 1);
    if (right) {
        int w = getTextWidth(right);
        drawText(SCREEN_W - w - 2, SCREEN_H - 14, right, 1);
    }
}

void DisplayManager::drawMenuItem(int index, const char* text, bool selected) {
    int y = 24 + index * 22;
    if (selected) {
        drawRect(2, y - 2, SCREEN_W - 4, 20, true);
        // 选中项文字用白色
        u8g2Fonts.setForegroundColor(GxEPD_WHITE);
        u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
    }
    drawText(8, y, text, 1);
    if (selected) {
        u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
    }
}

void DisplayManager::drawMessageBox(const char* title, const char* message) {
    clear();
    drawTitleBar(title);
    // 简单的自动换行
    int y = 28;
    int lineLen = 0;
    char line[80];
    int lineIdx = 0;
    int i = 0;
    while (message[i] && y < SCREEN_H - 20) {
        line[lineIdx++] = message[i];
        // 中文占3字节，宽度算2个
        if ((unsigned char)message[i] >= 0x80) {
            lineLen += 2;
            i += 3;
        } else {
            lineLen += 1;
            i++;
        }
        if (message[i-1] == '\n' || lineLen >= 32) {
            line[lineIdx] = 0;
            drawText(4, y, line, 1);
            y += 18;
            lineIdx = 0;
            lineLen = 0;
        }
    }
    if (lineIdx > 0) {
        line[lineIdx] = 0;
        drawText(4, y, line, 1);
    }
    refresh(true);
}

void DisplayManager::drawProgressBar(int x, int y, int w, int percent) {
    drawRect(x, y, w, 10, false);
    int fillW = (w - 2) * percent / 100;
    if (fillW > 0) {
        display.fillRect(x + 1, y + 1, fillW, 8, COLOR_BLACK);
    }
}

void debugPrint(const char* msg) {
    Serial.println(msg);
}

void debugPrintf(const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.println(buf);
}
