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
    _pendingEvent = KEY_NONE;
    _upLastReleaseTime = 0;
    _downLastReleaseTime = 0;
    _menuLastReleaseTime = 0;
    _lastConflictReadTime = 0;
    _upFirstLowTime = 0;
    _downFirstLowTime = 0;
    
    Serial.println("InputManager: 按键初始化完成");
    Serial.printf("  KEY_MENU=GPIO%d, KEY_UP=GPIO%d, KEY_DOWN=GPIO%d\n", KEY_MENU, KEY_UP, KEY_DOWN);
    Serial.println("  仅 GPIO0(EPD_DC) 采用冲突引脚降频读取");
}

int InputManager::readPinSafe(int pin) {
    // 只有 EPD_DC(GPIO0) 与屏幕刷新冲突，需要临时切换模式
    // GPIO3(RX)/GPIO12 作为普通按键输入，无需特殊处理
    bool isConflict = (pin == EPD_DC);
    
    if (isConflict) {
        // 临时切换为输入上拉模式
        pinMode(pin, INPUT_PULLUP);
        delayMicroseconds(300); // 等待电平稳定
    }
    
    // 多次采样防抖（8次采样，超过4次为高则高）
    int sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += digitalRead(pin);
        delayMicroseconds(150);
    }
    int result = (sum > 4) ? HIGH : LOW;
    
    if (isConflict) {
        // 恢复为输出高电平（DC 空闲时是高电平）
        pinMode(pin, OUTPUT);
        digitalWrite(pin, HIGH);
        delayMicroseconds(150);
    }
    
    return result;
}

void InputManager::update() {
    unsigned long now = millis();
    
    // ===== 按键1（Menu/Home）- 无冲突，正常读取，只处理短按 =====
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
    
    // ===== 按键3（Down）- 无冲突，全速读取，二次确认机制 =====
    int downState = readPinSafe(KEY_DOWN);
    
    if (downState == LOW && !_downPressed) {
        // 第一次读到 LOW，记录时间，不立即认为按下
        if (_downFirstLowTime == 0) {
            _downFirstLowTime = now;
        } else if (now - _downFirstLowTime >= 10) {
            // 连续两次（间隔>=10ms）都读到 LOW，确认按下
            _downPressed = true;
            _downPressTime = now;
            _downLongFired = false;
            _downFirstLowTime = 0;
        }
    } else if (downState == HIGH) {
        // 读到 HIGH，重置首次 LOW 时间
        _downFirstLowTime = 0;
        
        if (_downPressed) {
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
        }
    } else if (downState == LOW && _downPressed && !_downLongFired) {
        if (now - _downPressTime > KEY_LONGPRESS_MS) {
            _downLongFired = true;
            _pendingEvent = KEY_DOWN_LONG;
        }
    }
    
    // ===== 冲突引脚（仅 GPIO0/EPD_DC）降频：每 50ms 才读取一次 =====
    if (now - _lastConflictReadTime < 50) {
        return; // 还没到读取时间，跳过
    }
    _lastConflictReadTime = now;
    
    // ===== 按键2（Up）- 二次确认机制 =====
    int upState = readPinSafe(KEY_UP);
    
    if (upState == LOW && !_upPressed) {
        // 第一次读到 LOW，记录时间，不立即认为按下
        if (_upFirstLowTime == 0) {
            _upFirstLowTime = now;
        } else if (now - _upFirstLowTime >= 10) {
            // 连续两次（间隔>=10ms）都读到 LOW，确认按下
            _upPressed = true;
            _upPressTime = now;
            _upLongFired = false;
            _upFirstLowTime = 0;
        }
    } else if (upState == HIGH) {
        // 读到 HIGH，重置首次 LOW 时间
        _upFirstLowTime = 0;
        
        if (_upPressed) {
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
        }
    } else if (upState == LOW && _upPressed && !_upLongFired) {
        if (now - _upPressTime > KEY_LONGPRESS_MS) {
            _upLongFired = true;
            _pendingEvent = KEY_UP_LONG;
        }
    }
}

KeyEvent InputManager::getEvent() {
    KeyEvent evt = _pendingEvent;
    _pendingEvent = KEY_NONE;
    return evt;
}
