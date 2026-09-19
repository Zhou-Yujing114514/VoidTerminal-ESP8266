#include "app_state.h"
#include "wifi_config.h"

AppStateManager app;

char g_resetInfo[96] = "";

const char* menuNames[MENU_COUNT] = {
    "虚空终端",
    "配网设置"
};

void AppStateManager::init() {
    _currentState = STATE_BOOT;
    _previousState = STATE_BOOT;
    _pendingTarget = STATE_MENU;
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

void AppStateManager::enterOrSelectWifi(AppState target) {
    // WiFi 已连接则直接进入目标；否则先弹 WiFi 选择界面
    if (wifiConfig.isWifiConnected()) {
        setState(target);
    } else {
        _pendingTarget = target;
        setState(STATE_WIFI_SELECT);
    }
}

void AppStateManager::drawMenu() {
    disp.clear();
    disp.drawTitleBar("虚空终端 OS");
    
    // 竖排大卡片菜单（2 项）
    int cardX = 12;
    int cardW = SCREEN_W - 24;
    int cardH = 42;
    int gap = 12;
    int startY = 26;
    
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = startY + i * (cardH + gap);
        bool selected = (i == _menuIndex);
        
        if (selected) {
            disp.drawRect(cardX, y, cardW, cardH, true);
            // 反白文字（注意：u8g2Fonts 不受 display.setTextColor 影响，必须用 setForegroundColor）
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
            u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
        } else {
            disp.drawRect(cardX, y, cardW, cardH, false);
        }
        
        // 图标（选中项显示 > 指示箭头）+ 菜单名称居中
        char label[24];
        snprintf(label, sizeof(label), "%s%s", selected ? "> " : "  ", menuNames[i]);
        int textW = disp.getTextWidth(label);
        disp.drawText(cardX + (cardW - textW) / 2, y + cardH / 2 - 4, label, 1);
        
        if (selected) {
            u8g2Fonts.setForegroundColor(GxEPD_BLACK);
            u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        }
    }
    
    // 底部状态栏：版本 + WiFi 状态
    char status[40];
    snprintf(status, sizeof(status), "v%s  %s", FW_VERSION,
             wifiConfig.isWifiConnected() ? "WiFi:已连接" : "WiFi:未连接");
    disp.drawStatusBar(status, "上/下:选择 长按:确认");
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
                    case MENU_CHAT:
                        enterOrSelectWifi(STATE_CHAT);
                        break;
                    case MENU_CONFIG:
                        setState(STATE_CONFIG);
                        break;
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
