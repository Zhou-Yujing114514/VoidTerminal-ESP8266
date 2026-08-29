#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

// 显示管理类
class DisplayManager {
public:
    void init();
    void clear();
    void refresh(bool full = false);
    void drawText(int x, int y, const char* text, int size = 1);
    void drawText_P(int x, int y, const char* text, int size = 1);
    void drawRect(int x, int y, int w, int h, bool filled = false);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h);
    void setFont(int size);
    int getTextWidth(const char* text);
    
    // UI 组件
    void drawTitleBar(const char* title);
    void drawStatusBar(const char* left, const char* right);
    void drawMenuItem(int index, const char* text, bool selected);
    void drawMessageBox(const char* title, const char* message);
    void drawProgressBar(int x, int y, int w, int percent);
    
private:
    bool _needFullRefresh;
    int _currentFontSize;
};

extern DisplayManager disp;

#endif // DISPLAY_H
