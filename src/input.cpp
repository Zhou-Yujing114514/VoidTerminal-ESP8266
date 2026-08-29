#include "input.h"

InputManager input;

void InputManager::init() {
    pinMode(KEY_MENU, INPUT_PULLUP);
    pinMode(KEY_UP, INPUT_PULLUP);
    pinMode(KEY_DOWN, INPUT_PULLUP);
    _menuPressed = false;
    _upPressed = false;
    _downPressed = false;
    _upLongFired = false;
    _downLongFired = false;
    _pendingEvent = KEY_NONE;
    _upLastReleaseTime = 0;
    _downLastReleaseTime = 0;
    _menuLastReleaseTime = 0;
}

int InputManager::readPinSafe(int pin) {
    // 读取引脚，多次采样防抖
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += digitalRead(pin);
        delayMicroseconds(100);
    }
    return (sum > 2) ? HIGH : LOW;
}

void InputManager::update() {
    unsigned long now = millis();
    
    // 按键1（Menu/Home）- 只处理短按，禁长按
    int menuState = readPinSafe(KEY_MENU);
    if (menuState == LOW && !_menuPressed) {
        _menuPressed = true;
        _menuPressTime = now;
    } else if (menuState == HIGH && _menuPressed) {
        _menuPressed = false;
        unsigned long duration = now - _menuPressTime;
        if (duration > KEY_DEBOUNCE_MS && duration < KEY_LONGPRESS_MS) {
            // 检查是否是双击
            if (now - _menuLastReleaseTime < KEY_DOUBLECLICK_MS) {
                _pendingEvent = KEY_MENU_DOUBLE;
            } else {
                _pendingEvent = KEY_MENU_SHORT;
            }
            _menuLastReleaseTime = now;
        }
        // 长按不处理（硬件功能）
    }
    
    // 按键2（Up）- 短按/长按/双击
    int upState = readPinSafe(KEY_UP);
    if (upState == LOW && !_upPressed) {
        _upPressed = true;
        _upPressTime = now;
        _upLongFired = false;
    } else if (upState == HIGH && _upPressed) {
        _upPressed = false;
        unsigned long duration = now - _upPressTime;
        if (duration > KEY_DEBOUNCE_MS && !_upLongFired) {
            // 检查是否是双击
            if (now - _upLastReleaseTime < KEY_DOUBLECLICK_MS) {
                _pendingEvent = KEY_UP_DOUBLE;
            } else {
                _pendingEvent = KEY_UP_SHORT;
            }
            _upLastReleaseTime = now;
        }
    } else if (upState == LOW && _upPressed && !_upLongFired) {
        if (now - _upPressTime > KEY_LONGPRESS_MS) {
            _upLongFired = true;
            _pendingEvent = KEY_UP_LONG;
        }
    }
    
    // 按键3（Down）- 短按/长按/双击
    int downState = readPinSafe(KEY_DOWN);
    if (downState == LOW && !_downPressed) {
        _downPressed = true;
        _downPressTime = now;
        _downLongFired = false;
    } else if (downState == HIGH && _downPressed) {
        _downPressed = false;
        unsigned long duration = now - _downPressTime;
        if (duration > KEY_DEBOUNCE_MS && !_downLongFired) {
            if (now - _downLastReleaseTime < KEY_DOUBLECLICK_MS) {
                _pendingEvent = KEY_DOWN_DOUBLE;
            } else {
                _pendingEvent = KEY_DOWN_SHORT;
            }
            _downLastReleaseTime = now;
        }
    } else if (downState == LOW && _downPressed && !_downLongFired) {
        if (now - _downPressTime > KEY_LONGPRESS_MS) {
            _downLongFired = true;
            _pendingEvent = KEY_DOWN_LONG;
        }
    }
}

KeyEvent InputManager::getEvent() {
    KeyEvent evt = _pendingEvent;
    _pendingEvent = KEY_NONE;
    return evt;
}
