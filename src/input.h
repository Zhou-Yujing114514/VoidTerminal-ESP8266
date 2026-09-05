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
    unsigned long _upReleaseTime;       // 按键2释放时间（用于延迟短按）
    unsigned long _downReleaseTime;     // 按键3释放时间
    unsigned long _upFirstLowTime;      // 按键2第一次读到 LOW 的时间
    unsigned long _downFirstLowTime;    // 按键3第一次读到 LOW 的时间
    bool _menuPressed;
    bool _upPressed;
    bool _downPressed;
    bool _upLongFired;
    bool _downLongFired;
    bool _upPending;      // 按键2有待确认的短按（等待双击窗口）
    bool _downPending;    // 按键3有待确认的短按
    KeyEvent _pendingEvent;
    
    int readPinSafe(int pin);
};

extern InputManager input;

#endif // INPUT_H
