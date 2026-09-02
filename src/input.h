#ifndef INPUT_H
#define INPUT_H

#include "config.h"

// 按键事件类型
enum KeyEvent {
    KEY_NONE = 0,
    KEY_MENU_SHORT,    // 按键1短按 - 回主页/删除
    KEY_MENU_DOUBLE,   // 按键1双击 - 退出输入
    KEY_UP_SHORT,      // 按键2短按
    KEY_UP_LONG,       // 按键2长按
    KEY_UP_DOUBLE,     // 按键2双击
    KEY_DOWN_SHORT,    // 按键3短按
    KEY_DOWN_LONG,     // 按键3长按
    KEY_DOWN_DOUBLE    // 按键3双击
};

// 按键管理类
class InputManager {
public:
    void init();
    KeyEvent getEvent();  // 非阻塞获取按键事件
    void update();        // 在主循环中调用，更新按键状态
    
private:
    unsigned long _menuPressTime;
    unsigned long _upPressTime;
    unsigned long _downPressTime;
    unsigned long _upLastReleaseTime;
    unsigned long _downLastReleaseTime;
    unsigned long _menuLastReleaseTime;
    unsigned long _lastConflictReadTime;  // 上次读取冲突引脚的时间（用于降频）
    unsigned long _upFirstLowTime;        // 按键2第一次读到 LOW 的时间（用于二次确认）
    unsigned long _downFirstLowTime;      // 按键3第一次读到 LOW 的时间
    bool _menuPressed;
    bool _upPressed;
    bool _downPressed;
    bool _upLongFired;
    bool _downLongFired;
    bool _upConfirmed;   // 按键2是否已通过二次确认
    bool _downConfirmed; // 按键3是否已通过二次确认
    KeyEvent _pendingEvent;
    
    int readPinSafe(int pin);
};

extern InputManager input;

#endif // INPUT_H
