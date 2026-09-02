#ifndef CHAT_H
#define CHAT_H

#include "config.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// 会话类型
enum ConvType {
    CONV_PUBLIC = 0,   // 公共大厅
    CONV_GROUP,        // 群聊
    CONV_PRIVATE       // 私聊
};

// 会话结构
struct Conversation {
    char id[32];        // 会话ID（群ID或好友ID）
    char name[32];      // 显示名称
    ConvType type;      // 会话类型
    int unread;         // 未读消息数
    bool valid;
};

// 聊天消息结构
struct ChatMessage {
    char from[32];      // 发送者ID
    char fromName[32];  // 发送者昵称
    char content[64];   // 消息内容（从128减少到64，节省RAM）
    bool isMe;          // 是否是自己发的
    unsigned long time; // 时间戳
};

// 九宫格输入法状态
enum InputMode {
    INPUT_NONE = 0,
    INPUT_GRID,
    INPUT_LETTER,
    INPUT_CANDIDATE
};

// 聊天视图状态
enum ChatView {
    CHAT_VIEW_LIST = 0,  // 会话列表
    CHAT_VIEW_CHAT        // 聊天界面
};

#define MAX_CONVERSATIONS 5
#define MAX_MESSAGES_PER_CONV 5

class ChatManager {
public:
    void init();
    void enter();
    void exit();
    void update();
    void handleKey(KeyEvent evt);
    bool isActive() { return _active; }
    
    // WebSocket 事件处理（public，供静态回调访问）
    void handleWsEvent(WStype_t type, uint8_t* payload, size_t length);
    
private:
    bool _active;
    ChatView _view;
    int _selectedConv;
    int _convScrollOffset;
    int _messageScrollOffset;
    
    // 会话列表
    Conversation _conversations[MAX_CONVERSATIONS];
    int _convCount;
    
    // 当前会话的消息
    ChatMessage _messages[MAX_MESSAGES_PER_CONV];
    int _messageCount;
    
    // 输入法状态
    InputMode _inputMode;
    int _gridCursor;
    int _letterCursor;
    char _pinyin[32];
    int _pinyinLen;
    int _candidateIndex;
    char _inputBuffer[64];
    int _inputBufferLen;
    
    // WebSocket
    WebSocketsClient _webSocket;
    bool _wsConnected;
    bool _loggedIn;
    char _userId[32];
    char _userName[32];
    char _token[64];
    unsigned long _lastReconnectTime;
    
    // 方法
    void drawConversationList();
    void drawChatView();
    void drawInputKeyboard();
    void drawLetterSelector();
    void drawConnecting();
    void drawLoginScreen();
    
    void addConversation(const char* id, const char* name, ConvType type);
    void addMessage(const char* from, const char* fromName, const char* content, bool isMe);
    void clearCurrentMessages();
    void clearAllCache();
    
    // WebSocket
    void connectWebSocket();
    void disconnectWebSocket();
    void sendWsMessage(const char* type, const char* to, const char* content);
    void handleHelloMessage(JsonObject root);
    void handleChatMessage(JsonObject root);
    
    // 登录
    bool login(const char* username, const char* password);
    
    // 输入法
    const char* getGridLetters(int index);
    void appendPinyin(char c);
    void deletePinyin();
    void confirmCandidate();
    void loadCandidates();
    
    // 工具
    Conversation* getCurrentConversation();
    void loadMessagesForCurrentConv();
};

extern ChatManager chat;

#endif // CHAT_H
