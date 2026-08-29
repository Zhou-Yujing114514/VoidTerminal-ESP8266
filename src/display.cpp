#include "display.h"

GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
DisplayManager disp;

void DisplayManager::init() {
    display.init(115200);
    display.setRotation(1); // 横屏 296x128
    display.setTextColor(COLOR_BLACK);
    display.setFullWindow();
    u8g2Fonts.begin(display);
    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
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
        // 局部刷新 - 墨水屏需要特殊处理
        display.display(false);
    }
}

void DisplayManager::drawText(int x, int y, const char* text, int size) {
    display.setTextSize(size);
    display.setCursor(x, y);
    display.print(text);
}

void DisplayManager::drawText_P(int x, int y, const char* text, int size) {
    display.setTextSize(size);
    display.setCursor(x, y);
    display.print(text);
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
    display.setTextSize(size);
}

int DisplayManager::getTextWidth(const char* text) {
    return strlen(text) * 6 * _currentFontSize;
}

void DisplayManager::drawTitleBar(const char* title) {
    drawRect(0, 0, SCREEN_W, 16, true);
    display.setTextColor(COLOR_WHITE);
    drawText(4, 4, title, 1);
    display.setTextColor(COLOR_BLACK);
}

void DisplayManager::drawStatusBar(const char* left, const char* right) {
    drawLine(0, SCREEN_H - 12, SCREEN_W, SCREEN_H - 12);
    if (left) drawText(2, SCREEN_H - 10, left, 1);
    if (right) {
        int w = getTextWidth(right);
        drawText(SCREEN_W - w - 2, SCREEN_H - 10, right, 1);
    }
}

void DisplayManager::drawMenuItem(int index, const char* text, bool selected) {
    int y = 20 + index * 20;
    if (selected) {
        drawRect(2, y - 2, SCREEN_W - 4, 18, true);
        display.setTextColor(COLOR_WHITE);
    }
    drawText(8, y, text, 1);
    if (selected) {
        display.setTextColor(COLOR_BLACK);
    }
}

void DisplayManager::drawMessageBox(const char* title, const char* message) {
    clear();
    drawTitleBar(title);
    // 简单的自动换行
    int y = 24;
    int lineLen = 0;
    char line[50];
    int lineIdx = 0;
    for (int i = 0; message[i] && y < SCREEN_H - 16; i++) {
        line[lineIdx++] = message[i];
        lineLen++;
        if (message[i] == '\n' || lineLen >= 40) {
            line[lineIdx] = 0;
            drawText(4, y, line, 1);
            y += 12;
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
    drawRect(x, y, w, 8, false);
    int fillW = (w - 2) * percent / 100;
    if (fillW > 0) {
        display.fillRect(x + 1, y + 1, fillW, 6, COLOR_BLACK);
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
