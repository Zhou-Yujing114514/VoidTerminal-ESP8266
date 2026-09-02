#ifndef APP_STATE_H
#define APP_STATE_H

#include "config.h"
#include "display.h"
#include "input.h"

// 应用状态枚举
enum AppState {
    STATE_BOOT = 0,
    STATE_MENU,         // 主菜单
    STATE_CHAT,         // 虚空终端聊天
    STATE_MONITOR,      // 服务器监控
    STATE_CLOCK,        // 时钟
    STATE_CONFIG,       // 配网设置
    STATE_SHUTDOWN      // 关机/休眠
};

// 主菜单项目
enum MenuItem {
    MENU_CHAT = 0,
    MENU_MONITOR,
    MENU_CLOCK,
    MENU_CONFIG,
    MENU_COUNT
};

// 应用状态管理类
class AppStateManager {
public:
    void init();
    void update();
    void handleKey(KeyEvent evt);
    AppState getCurrentState() { return _currentState; }
    void setState(AppState state);
    void goHome();  // 回主页
    
private:
    AppState _currentState;
    AppState _previousState;
    int _menuIndex;
    bool _needRedraw;
    
    void drawMenu();
    void drawBootScreen();
    const char* getMenuName(int index);
};

extern AppStateManager app;

#endif // APP_STATE_H
