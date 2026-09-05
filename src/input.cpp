#include "input.h"

InputManager input;

void InputManager::init() {
    // 按键1（Menu/Home）- GPIO12，无冲突，正常初始化
    pinMode(KEY_MENU, INPUT_PULLUP);
    
    // 按键3（Down）- GPIO3(RX)：SD卡版硬件，按键3改到GPIO3(RX引脚)。
    // 仅作按键输入，不读取串口，可安全设为输入上拉（不影响TX调试输出）。
    pinMode(KEY_DOWN, INPUT_PULLUP);
    
    // 按键2（Up）- GPIO0，与 EPD_DC 冲突，不在此初始化
    // 在 readPinSafe 中临时切换模式
    _menuPressed = false;
    _upPressed = false;
    _downPressed = false;
    _upLongFired = false;
    _downLongFired = false;
    _upPending = false;
    _downPending = false;
    _pendingEvent = KEY_NONE;
    _upReleaseTime = 0;
    _downReleaseTime = 0;
    _upFirstLowTime = 0;
    _downFirstLowTime = 0;
    
    Serial.println("InputManager: 按键初始化完成");
    Serial.printf("  KEY_MENU=GPIO%d, KEY_UP=GPIO%d, KEY_DOWN=GPIO%d\n", KEY_MENU, KEY_UP, KEY_DOWN);
    Serial.println("  双击/短按采用延迟确认，长按立即触发");
}

int InputManager::readPinSafe(int pin) {
    // 只有 EPD_DC(GPIO0) 与屏幕刷新冲突，需要临时切换模式
    bool isConflict = (pin == EPD_DC);
    
    if (isConflict) {
        pinMode(pin, INPUT_PULLUP);
        delayMicroseconds(200);
    }
    
    int sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += digitalRead(pin);
        delayMicroseconds(100);
    }
    int result = (sum > 4) ? HIGH : LOW;
    
    if (isConflict) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
        delayMicroseconds(100);
    }
    
    return result;
}

// 处理一个按键的按下/释放状态机，返回产生的按键码（无事件返回 KEY_NONE）
// 采用延迟短按：释放后等待双击窗口，若窗口内再次按下则双击，否则短按
static KeyEvent processKey(
    int pinState,
    bool& pressed, bool& longFired, bool& pending,
    unsigned long& pressTime, unsigned long& releaseTime, unsigned long& firstLowTime,
    unsigned long now,
    KeyEvent shortEvt, KeyEvent doubleEvt, KeyEvent longEvt)
{
    if (pinState == LOW && !pressed) {
        // 检测到按下
        if (pending) {
            // 双击窗口内再次按下 → 双击
            pending = false;
            pressed = true;
            pressTime = now;
            longFired = false;
            return doubleEvt;
        }
        // 第一次按下，用二次确认避免抖动
        if (firstLowTime == 0) {
            firstLowTime = now;
        } else if (now - firstLowTime >= 10) {
            pressed = true;
            pressTime = now;
            longFired = false;
            firstLowTime = 0;
        }
    } else if (pinState == HIGH) {
        firstLowTime = 0;
        if (pressed) {
            pressed = false;
            unsigned long duration = now - pressTime;
            if (duration > KEY_DEBOUNCE_MS && !longFired) {
                // 释放：不立即发短按，进入待确认（等双击窗口）
                pending = true;
                releaseTime = now;
            }
        }
    } else if (pinState == LOW && pressed && !longFired) {
        if (now - pressTime > KEY_LONGPRESS_MS) {
            longFired = true;
            return longEvt;
        }
    }
    return KEY_NONE;
}

void InputManager::update() {
    unsigned long now = millis();
    
    // ===== 按键1（Menu/Home）- 短按立即触发（回主页，无需双击延迟）=====
    int menuState = readPinSafe(KEY_MENU);
    if (menuState == LOW && !_menuPressed) {
        _menuPressed = true;
        _menuPressTime = now;
    } else if (menuState == HIGH && _menuPressed) {
        _menuPressed = false;
        unsigned long duration = now - _menuPressTime;
        if (duration > KEY_DEBOUNCE_MS && duration < KEY_LONGPRESS_MS) {
            _pendingEvent = KEY_MENU_SHORT;
        }
    }
    
    // ===== 按键3（Down）- 延迟短按 =====
    int downState = readPinSafe(KEY_DOWN);
    KeyEvent downEvt = processKey(downState, _downPressed, _downLongFired, _downPending,
                                  _downPressTime, _downReleaseTime, _downFirstLowTime, now,
                                  KEY_DOWN_SHORT, KEY_DOWN_DOUBLE, KEY_DOWN_LONG);
    if (downEvt != KEY_NONE) _pendingEvent = downEvt;
    
    // 按键3待确认短按超时 → 确认为短按
    if (_downPending && now - _downReleaseTime > KEY_DOUBLECLICK_MS) {
        _downPending = false;
        _pendingEvent = KEY_DOWN_SHORT;
    }
    
    // ===== 按键2（Up）- 延迟短按 =====
    int upState = readPinSafe(KEY_UP);
    KeyEvent upEvt = processKey(upState, _upPressed, _upLongFired, _upPending,
                                _upPressTime, _upReleaseTime, _upFirstLowTime, now,
                                KEY_UP_SHORT, KEY_UP_DOUBLE, KEY_UP_LONG);
    if (upEvt != KEY_NONE) _pendingEvent = upEvt;
    
    // 按键2待确认短按超时 → 确认为短按
    if (_upPending && now - _upReleaseTime > KEY_DOUBLECLICK_MS) {
        _upPending = false;
        _pendingEvent = KEY_UP_SHORT;
    }
}

KeyEvent InputManager::getEvent() {
    KeyEvent evt = _pendingEvent;
    _pendingEvent = KEY_NONE;
    return evt;
}
