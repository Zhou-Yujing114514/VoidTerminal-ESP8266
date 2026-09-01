#include "input.h"

InputManager input;

void InputManager::init() {
    // 按键1（Menu）- GPIO12，无冲突，正常初始化
    pinMode(KEY_MENU, INPUT_PULLUP);
    
    // 按键2（Up）- GPIO0，与 EPD_DC 冲突，不在此初始化
    // 按键3（Down）- GPIO5，与 SD_CS 冲突，不在此初始化
    // 这两个引脚在 readPinSafe 中临时切换模式
    
    _menuPressed = false;
    _upPressed = false;
    _downPressed = false;
    _upLongFired = false;
    _downLongFired = false;
    _pendingEvent = KEY_NONE;
    _upLastReleaseTime = 0;
    _downLastReleaseTime = 0;
    _menuLastReleaseTime = 0;
    
    Serial.println("InputManager: 按键初始化完成");
    Serial.printf("  KEY_MENU=GPIO%d, KEY_UP=GPIO%d, KEY_DOWN=GPIO%d\n", KEY_MENU, KEY_UP, KEY_DOWN);
}

int InputManager::readPinSafe(int pin) {
    // 判断是否是冲突引脚
    bool isConflict = (pin == EPD_DC) || (pin == SD_CS);
    
    if (isConflict) {
        // 临时切换为输入上拉模式
        pinMode(pin, INPUT_PULLUP);
        delayMicroseconds(200); // 等待电平稳定
    }
    
    // 多次采样防抖（10次采样，超过5次为高则高）
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += digitalRead(pin);
        delayMicroseconds(200);
    }
    int result = (sum > 5) ? HIGH : LOW;
    
    if (isConflict) {
        // 恢复为输出高电平（CS 和 DC 空闲时都是高电平）
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
        delayMicroseconds(100);
    }
    
    return result;
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
            if (now - _menuLastReleaseTime < KEY_DOUBLECLICK_MS) {
                _pendingEvent = KEY_MENU_DOUBLE;
            } else {
                _pendingEvent = KEY_MENU_SHORT;
            }
            _menuLastReleaseTime = now;
        }
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
        Serial.println("KEY_DOWN 按下!");
    } else if (downState == HIGH && _downPressed) {
        _downPressed = false;
        unsigned long duration = now - _downPressTime;
        Serial.printf("KEY_DOWN 释放, 时长=%lu\n", duration);
        if (duration > KEY_DEBOUNCE_MS && !_downLongFired) {
            if (now - _downLastReleaseTime < KEY_DOUBLECLICK_MS) {
                _pendingEvent = KEY_DOWN_DOUBLE;
                Serial.println("KEY_DOWN 双击事件!");
            } else {
                _pendingEvent = KEY_DOWN_SHORT;
                Serial.println("KEY_DOWN 短按事件!");
            }
            _downLastReleaseTime = now;
        }
    } else if (downState == LOW && _downPressed && !_downLongFired) {
        if (now - _downPressTime > KEY_LONGPRESS_MS) {
            _downLongFired = true;
            _pendingEvent = KEY_DOWN_LONG;
            Serial.println("KEY_DOWN 长按事件!");
        }
    }
}

KeyEvent InputManager::getEvent() {
    KeyEvent evt = _pendingEvent;
    _pendingEvent = KEY_NONE;
    return evt;
}
