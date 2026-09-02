#include "app_state.h"

AppStateManager app;

const char* menuNames[MENU_COUNT] = {
    "虚空终端",
    "服务器监控",
    "TXT阅读器",
    "时钟",
    "配网设置"
};

void AppStateManager::init() {
    _currentState = STATE_BOOT;
    _previousState = STATE_BOOT;
    _menuIndex = 0;
    _needRedraw = true;
    drawBootScreen();
    delay(500);  // 短暂显示启动画面（从1500ms减少到500ms，避免阻塞过久）
    setState(STATE_MENU);
}

void AppStateManager::drawBootScreen() {
    disp.clear();
    // 绘制启动画面
    disp.drawRect(SCREEN_W/2 - 40, SCREEN_H/2 - 20, 80, 40, false);
    disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2 - 8, "VoidTerminal", 1);
    disp.drawText(SCREEN_W/2 - 20, SCREEN_H/2 + 6, "OS v" FW_VERSION, 1);
    disp.drawStatusBar("正在启动...", nullptr);
    disp.refresh(true);
}

void AppStateManager::setState(AppState state) {
    _previousState = _currentState;
    _currentState = state;
    _needRedraw = true;
}

void AppStateManager::goHome() {
    if (_currentState != STATE_MENU) {
        setState(STATE_MENU);
    }
}

void AppStateManager::drawMenu() {
    disp.clear();
    disp.drawTitleBar("虚空终端 OS");
    
    // 2x3 网格菜单
    int itemW = SCREEN_W / 2;
    int itemH = (SCREEN_H - 16 - 12) / 3;
    
    for (int i = 0; i < MENU_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = col * itemW;
        int y = 16 + row * itemH;
        
        bool selected = (i == _menuIndex);
        
        if (selected) {
            disp.drawRect(x + 2, y + 2, itemW - 4, itemH - 4, true);
            // 反白文字（注意：u8g2Fonts 不受 display.setTextColor 影响，必须用 setForegroundColor）
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
        } else {
            disp.drawRect(x + 2, y + 2, itemW - 4, itemH - 4, false);
        }
        
        // 菜单名称居中
        int textW = disp.getTextWidth(menuNames[i]);
        disp.drawText(x + (itemW - textW) / 2, y + itemH / 2 - 4, menuNames[i], 1);
        
        if (selected) {
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);
            u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        }
    }
    
    disp.drawStatusBar("上/下:选择  确认:长按下", "Home:菜单键");
    disp.refresh(true);
}

const char* AppStateManager::getMenuName(int index) {
    if (index >= 0 && index < MENU_COUNT) {
        return menuNames[index];
    }
    return "";
}

void AppStateManager::handleKey(KeyEvent evt) {
    if (evt == KEY_NONE) return;
    
    // 按键1短按 = 回主页（全局）
    if (evt == KEY_MENU_SHORT) {
        goHome();
        return;
    }
    
    switch (_currentState) {
        case STATE_MENU:
            if (evt == KEY_UP_SHORT || evt == KEY_DOWN_SHORT) {
                // 菜单导航
                if (evt == KEY_UP_SHORT) {
                    _menuIndex = (_menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
                } else {
                    _menuIndex = (_menuIndex + 1) % MENU_COUNT;
                }
                _needRedraw = true;
            } else if (evt == KEY_DOWN_LONG) {
                // 长按确认进入
                switch (_menuIndex) {
                    case MENU_CHAT: setState(STATE_CHAT); break;
                    case MENU_MONITOR: setState(STATE_MONITOR); break;
                    case MENU_READER: setState(STATE_READER); break;
                    case MENU_CLOCK: setState(STATE_CLOCK); break;
                    case MENU_CONFIG: setState(STATE_CONFIG); break;
                }
            }
            break;
            
        default:
            // 其他状态的按键处理在各自模块中
            break;
    }
}

void AppStateManager::update() {
    if (_needRedraw) {
        _needRedraw = false;
        switch (_currentState) {
            case STATE_MENU:
                drawMenu();
                break;
            case STATE_BOOT:
                drawBootScreen();
                break;
            default:
                // 其他状态由各自模块绘制
                break;
        }
    }
}
