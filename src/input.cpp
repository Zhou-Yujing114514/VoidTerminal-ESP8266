#include "input.h"

InputManager input;

// 跟踪冲突引脚的原始状态
static bool epdDcOriginalLevel = true;  // DC 默认高电平
static bool sdCsOriginalLevel = true;   // CS 默认高电平（未选中）

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
}

int InputManager::readPinSafe(int pin) {
    // 判断是否是冲突引脚
    bool isEpdDc = (pin == EPD_DC);
    bool isSdCs = (pin == SD_CS);
    bool isConflict = isEpdDc || isSdCs;
    
    if (isConflict) {
        // 保存当前引脚电平（在切换为输入前读取）
        // 注意：digitalRead 在输出模式下也能读取到当前输出电平
        int originalLevel = digitalRead(pin);
        if (isEpdDc) epdDcOriginalLevel = (originalLevel == HIGH);
        if (isSdCs) sdCsOriginalLevel = (originalLevel == HIGH);
        
        // 临时切换为输入上拉模式
        pinMode(pin, INPUT_PULLUP);
        // 等待电平稳定
        delayMicroseconds(50);
    }
    
    // 多次采样防抖
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += digitalRead(pin);
        delayMicroseconds(100);
    }
    int result = (sum > 2) ? HIGH : LOW;
    
    if (isConflict) {
        // 恢复为输出模式
        pinMode(pin, OUTPUT);
        // 恢复原来的电平
        if (isEpdDc) {
            digitalWrite(pin, epdDcOriginalLevel ? HIGH : LOW);
        }
        if (isSdCs) {
            digitalWrite(pin, sdCsOriginalLevel ? HIGH : LOW);
        }
        // 等待电平稳定
        delayMicroseconds(20);
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
