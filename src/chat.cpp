#include "chat.h"

ChatManager chat;

const char* gridLetters[9] = {
    ".,!?", "abc", "def",
    "ghi", "jkl", "mno",
    "pqrs", "tuv", "wxyz"
};

struct PinyinCandidate {
    const char* pinyin;
    const char* candidates[8];
};

const PinyinCandidate candidateTable[] = {
    {"ni", {"你", "尼", "泥", "逆", "拟", "呢", "匿", "腻"}},
    {"hao", {"好", "号", "浩", "耗", "豪", "毫", "郝", "嚎"}},
    {"wo", {"我", "握", "窝", "蜗", "卧", "沃", "斡", "幄"}},
    {"zai", {"在", "再", "载", "灾", "栽", "宰", "哉", "崽"}},
    {"ma", {"吗", "妈", "马", "麻", "骂", "嘛", "码", "蚂"}},
    {"de", {"的", "得", "德", "地", "底", "低", "滴", "笛"}},
    {"shi", {"是", "时", "事", "市", "试", "室", "式", "示"}},
    {"bu", {"不", "步", "部", "布", "怖", "捕", "哺", "埠"}},
    {"yao", {"要", "药", "摇", "咬", "腰", "邀", "耀", "尧"}},
    {"you", {"有", "又", "右", "友", "优", "游", "油", "由"}},
    {"he", {"和", "河", "喝", "何", "合", "盒", "贺", "赫"}},
    {"ta", {"他", "她", "它", "塔", "踏", "塌", "榻", "蹋"}},
    {"men", {"们", "门", "闷", "焖", "扪", "懑", "钔", "们"}},
    {"xie", {"谢", "写", "些", "鞋", "斜", "血", "歇", "蝎"}},
    {"le", {"了", "乐", "勒", "雷", "泪", "类", "累", "垒"}},
    {"zhen", {"真", "针", "侦", "珍", "斟", "甄", "箴", "臻"}},
};

#define CANDIDATE_TABLE_SIZE (sizeof(candidateTable) / sizeof(candidateTable[0]))
const char* currentCandidates[8];
int currentCandidateCount = 0;

static void wsEventCallback(WStype_t type, uint8_t* payload, size_t length) {
    chat.handleWsEvent(type, payload, length);
}

void ChatManager::init() {
    _active = false;
    _view = CHAT_VIEW_LIST;
    _selectedConv = 0;
    _convScrollOffset = 0;
    _messageScrollOffset = 0;
    _convCount = 0;
    _messageCount = 0;
    _inputMode = INPUT_NONE;
    _gridCursor = 4;
    _letterCursor = 0;
    _pinyinLen = 0;
    _pinyin[0] = 0;
    _candidateIndex = 0;
    _inputBufferLen = 0;
    _inputBuffer[0] = 0;
    _wsConnected = false;
    _loggedIn = false;
    _userId[0] = 0;
    _userName[0] = 0;
    _token[0] = 0;
    _lastReconnectTime = 0;
    addConversation("public", "网站问题反馈区", CONV_PUBLIC);
}

void ChatManager::enter() {
    _active = true;
    _view = CHAT_VIEW_LIST;
    _selectedConv = 0;
    if (!_wsConnected) {
        drawConnecting();
        connectWebSocket();
    } else {
        drawConversationList();
    }
}

void ChatManager::exit() {
    _active = false;
    _inputMode = INPUT_NONE;
}

void ChatManager::addConversation(const char* id, const char* name, ConvType type) {
    if (_convCount >= MAX_CONVERSATIONS) return;
    for (int i = 0; i < _convCount; i++) {
        if (strcmp(_conversations[i].id, id) == 0) {
            strncpy(_conversations[i].name, name, 31);
            return;
        }
    }
    strncpy(_conversations[_convCount].id, id, 31);
    _conversations[_convCount].id[31] = 0;
    strncpy(_conversations[_convCount].name, name, 31);
    _conversations[_convCount].name[31] = 0;
    _conversations[_convCount].type = type;
    _conversations[_convCount].unread = 0;
    _conversations[_convCount].valid = true;
    _convCount++;
}

Conversation* ChatManager::getCurrentConversation() {
    if (_selectedConv >= 0 && _selectedConv < _convCount) {
        return &_conversations[_selectedConv];
    }
    return nullptr;
}

void ChatManager::addMessage(const char* from, const char* fromName, const char* content, bool isMe) {
    if (_messageCount >= MAX_MESSAGES_PER_CONV) {
        for (int i = 1; i < MAX_MESSAGES_PER_CONV; i++) {
            _messages[i-1] = _messages[i];
        }
        _messageCount--;
    }
    strncpy(_messages[_messageCount].from, from, 31);
    _messages[_messageCount].from[31] = 0;
    strncpy(_messages[_messageCount].fromName, fromName, 31);
    _messages[_messageCount].fromName[31] = 0;
    strncpy(_messages[_messageCount].content, content, 255);
    _messages[_messageCount].content[255] = 0;
    _messages[_messageCount].isMe = isMe;
    _messages[_messageCount].time = millis();
    _messageCount++;
}

void ChatManager::clearCurrentMessages() {
    _messageCount = 0;
    _messageScrollOffset = 0;
}

void ChatManager::clearAllCache() {
    clearCurrentMessages();
    for (int i = 0; i < _convCount; i++) {
        _conversations[i].unread = 0;
    }
}

void ChatManager::drawConnecting() {
    disp.clear();
    disp.drawTitleBar("虚空终端");
    disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2 - 10, "连接中...", 2);
    disp.drawProgressBar(20, SCREEN_H/2 + 15, SCREEN_W - 40, 30);
    disp.drawStatusBar("正在连接服务器", "Home:返回");
    disp.refresh(true);
}

void ChatManager::drawConversationList() {
    disp.clear();
    disp.drawTitleBar("虚空终端 - 消息");
    if (_convCount == 0) {
        disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2, "暂无会话", 2);
    } else {
        int y = 20;
        int visibleCount = 0;
        int maxVisible = 7;
        for (int i = _convScrollOffset; i < _convCount && visibleCount < maxVisible; i++) {
            bool selected = (i == _selectedConv);
            if (selected) {
                disp.drawRect(0, y - 2, SCREEN_W, 16, true);
                display.setTextColor(COLOR_WHITE);
            }
            const char* typeIcon = "";
            switch (_conversations[i].type) {
                case CONV_PUBLIC: typeIcon = "[公]"; break;
                case CONV_GROUP: typeIcon = "[群]"; break;
                case CONV_PRIVATE: typeIcon = "[私]"; break;
            }
            char line[80];
            if (_conversations[i].unread > 0) {
                snprintf(line, sizeof(line), "%s %s (%d)", typeIcon, _conversations[i].name, _conversations[i].unread);
            } else {
                snprintf(line, sizeof(line), "%s %s", typeIcon, _conversations[i].name);
            }
            disp.drawText(4, y, line, 1);
            if (selected) display.setTextColor(COLOR_BLACK);
            y += 14;
            visibleCount++;
        }
    }
    char statusLine[64];
    if (_wsConnected) {
        snprintf(statusLine, sizeof(statusLine), "已连接 | %s", _loggedIn ? _userName : "未登录");
    } else {
        snprintf(statusLine, sizeof(statusLine), "未连接");
    }
    disp.drawStatusBar(statusLine, "上/下:选择 长按下:进入 Home:返回");
    disp.refresh(true);
}

void ChatManager::drawChatView() {
    disp.clear();
    Conversation* conv = getCurrentConversation();
    char title[64];
    if (conv) {
        const char* typeStr = "";
        switch (conv->type) {
            case CONV_PUBLIC: typeStr = "公共大厅"; break;
            case CONV_GROUP: typeStr = "群聊"; break;
            case CONV_PRIVATE: typeStr = "私聊"; break;
        }
        snprintf(title, sizeof(title), "%s - %s", typeStr, conv->name);
    } else {
        snprintf(title, sizeof(title), "聊天");
    }
    disp.drawTitleBar(title);
    int y = SCREEN_H - 14;
    int visibleCount = 0;
    int maxVisible = 6;
    for (int i = _messageCount - 1 - _messageScrollOffset; i >= 0 && visibleCount < maxVisible; i--) {
        ChatMessage& msg = _messages[i];
        char senderLine[64];
        snprintf(senderLine, sizeof(senderLine), "%s:", msg.fromName);
        y -= 10;
        if (y < 18) break;
        if (msg.isMe) {
            int w = disp.getTextWidth(senderLine);
            disp.drawText(SCREEN_W - w - 4, y, senderLine, 1);
        } else {
            disp.drawText(4, y, senderLine, 1);
        }
        char line[40];
        int lineIdx = 0;
        int contentLen = strlen(msg.content);
        for (int j = 0; j < contentLen; j++) {
            line[lineIdx++] = msg.content[j];
            if (lineIdx >= 28 || msg.content[j] == '\n') {
                line[lineIdx] = 0;
                y -= 10;
                if (y < 18) break;
                if (msg.isMe) {
                    int w = disp.getTextWidth(line);
                    disp.drawText(SCREEN_W - w - 8, y, line, 1);
                } else {
                    disp.drawText(8, y, line, 1);
                }
                lineIdx = 0;
            }
        }
        if (lineIdx > 0 && y >= 18) {
            line[lineIdx] = 0;
            y -= 10;
            if (y >= 18) {
                if (msg.isMe) {
                    int w = disp.getTextWidth(line);
                    disp.drawText(SCREEN_W - w - 8, y, line, 1);
                } else {
                    disp.drawText(8, y, line, 1);
                }
            }
        }
        visibleCount++;
        y -= 2;
    }
    if (_inputMode != INPUT_NONE) {
        disp.drawRect(0, SCREEN_H - 14, SCREEN_W, 14, true);
        display.setTextColor(COLOR_WHITE);
        char inputLine[100];
        snprintf(inputLine, sizeof(inputLine), "%s%s", _inputBuffer, _pinyin);
        disp.drawText(2, SCREEN_H - 12, inputLine, 1);
        display.setTextColor(COLOR_BLACK);
    } else {
        disp.drawStatusBar("上/下:翻页 双击下:输入 短按上:返回列表", "Home:主页");
    }
    disp.refresh(true);
}

void ChatManager::drawInputKeyboard() {
    disp.clear();
    disp.drawTitleBar("输入消息");
    if (_pinyinLen > 0 && currentCandidateCount > 0) {
        disp.drawRect(0, 16, SCREEN_W, 14, false);
        char candLine[100];
        int pos = 0;
        for (int i = 0; i < currentCandidateCount && pos < 90; i++) {
            if (i == _candidateIndex) candLine[pos++] = '[';
            const char* cand = currentCandidates[i];
            while (*cand && pos < 85) candLine[pos++] = *cand++;
            if (i == _candidateIndex) candLine[pos++] = ']';
            candLine[pos++] = ' ';
        }
        candLine[pos] = 0;
        disp.drawText(2, 18, candLine, 1);
    }
    int keyW = SCREEN_W / 3;
    int keyH = (SCREEN_H - 30 - 14 - 16) / 3;
    int startY = 32;
    for (int i = 0; i < 9; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = col * keyW;
        int y = startY + row * keyH;
        bool selected = (i == _gridCursor);
        if (selected) {
            disp.drawRect(x + 2, y + 2, keyW - 4, keyH - 4, true);
            display.setTextColor(COLOR_WHITE);
        } else {
            disp.drawRect(x + 2, y + 2, keyW - 4, keyH - 4, false);
        }
        char numStr[2] = {(char)('1' + i), 0};
        disp.drawText(x + 4, y + 4, numStr, 1);
        disp.drawText(x + 4, y + 16, gridLetters[i], 1);
        if (selected) display.setTextColor(COLOR_BLACK);
    }
    int funcY = SCREEN_H - 14;
    disp.drawRect(0, funcY, SCREEN_W / 3, 14, false);
    disp.drawText(4, funcY + 2, "删除", 1);
    disp.drawRect(SCREEN_W / 3, funcY, SCREEN_W / 3, 14, false);
    disp.drawText(SCREEN_W / 3 + 4, funcY + 2, "空格", 1);
    disp.drawRect(SCREEN_W * 2 / 3, funcY, SCREEN_W / 3, 14, false);
    disp.drawText(SCREEN_W * 2 / 3 + 4, funcY + 2, "退出", 1);
    disp.refresh(true);
}

void ChatManager::drawLetterSelector() {
    disp.clear();
    disp.drawTitleBar("选择字母");
    const char* letters = gridLetters[_gridCursor];
    int count = strlen(letters);
    int boxW = SCREEN_W / count;
    for (int i = 0; i < count; i++) {
        int x = i * boxW;
        bool selected = (i == _letterCursor);
        if (selected) {
            disp.drawRect(x + 4, 40, boxW - 8, 40, true);
            display.setTextColor(COLOR_WHITE);
        } else {
            disp.drawRect(x + 4, 40, boxW - 8, 40, false);
        }
        char letter[2] = {letters[i], 0};
        disp.drawText(x + boxW/2 - 4, 52, letter, 2);
        if (selected) display.setTextColor(COLOR_BLACK);
    }
    disp.drawStatusBar("上/下:选择 双击下:确认 短按上:取消", nullptr);
    disp.refresh(true);
}

void ChatManager::connectWebSocket() {
    if (WiFi.status() != WL_CONNECTED) return;
    _webSocket.begin(CHAT_SERVER, CHAT_PORT, "/ws");
    _webSocket.onEvent(wsEventCallback);
    _webSocket.setReconnectInterval(5000);
    _wsConnected = true;
}

void ChatManager::disconnectWebSocket() {
    _webSocket.disconnect();
    _wsConnected = false;
    _loggedIn = false;
}

void ChatManager::sendWsMessage(const char* type, const char* to, const char* content) {
    if (!_wsConnected) return;
    StaticJsonDocument<512> doc;
    doc["type"] = type;
    if (to) doc["to"] = to;
    if (content) doc["content"] = content;
    if (_token[0]) doc["token"] = _token;
    String json;
    serializeJson(doc, json);
    _webSocket.sendTXT(json);
}

void ChatManager::handleWsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            _wsConnected = false;
            _loggedIn = false;
            break;
        case WStype_CONNECTED:
            _wsConnected = true;
            if (_token[0]) sendWsMessage("auth", nullptr, nullptr);
            break;
        case WStype_TEXT: {
            StaticJsonDocument<1024> doc;
            if (deserializeJson(doc, payload, length)) break;
            const char* msgType = doc["type"] | "";
            if (strcmp(msgType, "hello") == 0) {
                handleHelloMessage(doc.as<JsonObject>());
            } else if (strcmp(msgType, "message") == 0) {
                handleChatMessage(doc.as<JsonObject>());
            } else if (strcmp(msgType, "auth_ok") == 0) {
                _loggedIn = true;
                const char* uid = doc["user"]["id"] | "";
                const char* uname = doc["user"]["name"] | "";
                strncpy(_userId, uid, 31);
                strncpy(_userName, uname, 31);
            }
            break;
        }
        default: break;
    }
}

void ChatManager::handleHelloMessage(JsonObject root) {
    JsonArray friends = root["friends"].as<JsonArray>();
    for (JsonObject friendObj : friends) {
        const char* fid = friendObj["id"] | "";
        const char* fname = friendObj["name"] | "";
        addConversation(fid, fname, CONV_PRIVATE);
    }
    JsonArray groups = root["groups"].as<JsonArray>();
    for (JsonObject groupObj : groups) {
        const char* gid = groupObj["id"] | "";
        const char* gname = groupObj["name"] | "";
        addConversation(gid, gname, CONV_GROUP);
    }
    JsonArray globalMsgs = root["globalMsgs"].as<JsonArray>();
    for (JsonObject msgObj : globalMsgs) {
        const char* from = msgObj["from"] | "";
        const char* fromName = msgObj["fromName"] | from;
        const char* content = msgObj["content"] | "";
        bool isMe = (strcmp(from, _userId) == 0);
        if (_selectedConv == 0 && strcmp(_conversations[0].id, "public") == 0) {
            addMessage(from, fromName, content, isMe);
        }
    }
    if (_active && _view == CHAT_VIEW_LIST) drawConversationList();
}

void ChatManager::handleChatMessage(JsonObject root) {
    const char* from = root["from"] | "";
    const char* fromName = root["fromName"] | from;
    const char* content = root["content"] | "";
    const char* to = root["to"] | "";
    bool isMe = (strcmp(from, _userId) == 0);
    Conversation* conv = getCurrentConversation();
    if (conv) {
        bool belongsToCurrent = false;
        if (conv->type == CONV_PUBLIC && strcmp(to, "public") == 0) {
            belongsToCurrent = true;
        } else if (conv->type == CONV_GROUP && strcmp(to, conv->id) == 0) {
            belongsToCurrent = true;
        } else if (conv->type == CONV_PRIVATE && 
                   (strcmp(from, conv->id) == 0 || strcmp(to, conv->id) == 0)) {
            belongsToCurrent = true;
        }
        if (belongsToCurrent && _view == CHAT_VIEW_CHAT) {
            addMessage(from, fromName, content, isMe);
            drawChatView();
        }
    }
}

bool ChatManager::login(const char* username, const char* password) {
    if (WiFi.status() != WL_CONNECTED) return false;
    WiFiClient client;
    HTTPClient http;
    String url = String("http://") + CHAT_SERVER + "/api/login";
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<256> doc;
    doc["username"] = username;
    doc["password"] = password;
    String body;
    serializeJson(doc, body);
    int httpCode = http.POST(body);
    if (httpCode == 200) {
        String payload = http.getString();
        StaticJsonDocument<512> resp;
        if (!deserializeJson(resp, payload)) {
            const char* token = resp["token"] | "";
            if (token[0]) {
                strncpy(_token, token, 63);
                _loggedIn = true;
                http.end();
                return true;
            }
        }
    }
    http.end();
    return false;
}

const char* ChatManager::getGridLetters(int index) {
    if (index >= 0 && index < 9) return gridLetters[index];
    return "";
}

void ChatManager::appendPinyin(char c) {
    if (_pinyinLen < 30) {
        _pinyin[_pinyinLen++] = c;
        _pinyin[_pinyinLen] = 0;
        loadCandidates();
    }
}

void ChatManager::deletePinyin() {
    if (_pinyinLen > 0) {
        _pinyinLen--;
        _pinyin[_pinyinLen] = 0;
        loadCandidates();
    }
}

void ChatManager::loadCandidates() {
    currentCandidateCount = 0;
    _candidateIndex = 0;
    for (unsigned int i = 0; i < CANDIDATE_TABLE_SIZE; i++) {
        if (strcmp(candidateTable[i].pinyin, _pinyin) == 0) {
            for (int j = 0; j < 8 && candidateTable[i].candidates[j]; j++) {
                currentCandidates[currentCandidateCount++] = candidateTable[i].candidates[j];
            }
            break;
        }
    }
    if (currentCandidateCount == 0 && _pinyinLen > 0) {
        currentCandidates[0] = _pinyin;
        currentCandidateCount = 1;
    }
}

void ChatManager::confirmCandidate() {
    if (_candidateIndex < currentCandidateCount) {
        const char* cand = currentCandidates[_candidateIndex];
        int candLen = strlen(cand);
        if (_inputBufferLen + candLen < 250) {
            strcpy(_inputBuffer + _inputBufferLen, cand);
            _inputBufferLen += candLen;
        }
    }
    _pinyinLen = 0;
    _pinyin[0] = 0;
    currentCandidateCount = 0;
    _candidateIndex = 0;
    if (_inputBufferLen > 0) {
        Conversation* conv = getCurrentConversation();
        if (conv) {
            sendWsMessage("message", conv->id, _inputBuffer);
            addMessage(_userId, _userName, _inputBuffer, true);
        }
        _inputBufferLen = 0;
        _inputBuffer[0] = 0;
        _inputMode = INPUT_NONE;
        drawChatView();
    } else {
        drawInputKeyboard();
    }
}

void ChatManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    if (evt == KEY_MENU_SHORT) {
        exit();
        app.goHome();
        return;
    }
    if (_view == CHAT_VIEW_LIST) {
        if (evt == KEY_UP_SHORT) {
            if (_selectedConv > 0) {
                _selectedConv--;
                if (_selectedConv < _convScrollOffset) _convScrollOffset = _selectedConv;
                drawConversationList();
            }
        } else if (evt == KEY_DOWN_SHORT) {
            if (_selectedConv < _convCount - 1) {
                _selectedConv++;
                if (_selectedConv >= _convScrollOffset + 7) _convScrollOffset = _selectedConv - 6;
                drawConversationList();
            }
        } else if (evt == KEY_DOWN_LONG) {
            clearCurrentMessages();
            _view = CHAT_VIEW_CHAT;
            _messageScrollOffset = 0;
            drawChatView();
        } else if (evt == KEY_UP_LONG) {
            clearAllCache();
            drawConversationList();
        }
    } else if (_view == CHAT_VIEW_CHAT) {
        if (_inputMode == INPUT_NONE) {
            if (evt == KEY_UP_SHORT) {
                if (_messageScrollOffset < _messageCount - 1) {
                    _messageScrollOffset++;
                    drawChatView();
                }
            } else if (evt == KEY_DOWN_SHORT) {
                if (_messageScrollOffset > 0) {
                    _messageScrollOffset--;
                    drawChatView();
                }
            } else if (evt == KEY_DOWN_DOUBLE) {
                _inputMode = INPUT_GRID;
                _gridCursor = 4;
                _pinyinLen = 0;
                _pinyin[0] = 0;
                _candidateIndex = 0;
                currentCandidateCount = 0;
                drawInputKeyboard();
            } else if (evt == KEY_UP_LONG) {
                _view = CHAT_VIEW_LIST;
                _inputMode = INPUT_NONE;
                drawConversationList();
            }
        } else if (_inputMode == INPUT_GRID) {
            if (evt == KEY_UP_SHORT) {
                if (_gridCursor >= 3) _gridCursor -= 3;
                drawInputKeyboard();
            } else if (evt == KEY_DOWN_SHORT) {
                if (_gridCursor < 6) _gridCursor += 3;
                drawInputKeyboard();
            } else if (evt == KEY_UP_LONG) {
                if (currentCandidateCount > 0) {
                    _candidateIndex = (_candidateIndex + 1) % currentCandidateCount;
                    drawInputKeyboard();
                }
            } else if (evt == KEY_DOWN_LONG) {
                if (currentCandidateCount > 0 && _pinyinLen > 0) {
                    confirmCandidate();
                } else {
                    _inputMode = INPUT_LETTER;
                    _letterCursor = 0;
                    drawLetterSelector();
                }
            } else if (evt == KEY_DOWN_DOUBLE) {
                _inputMode = INPUT_LETTER;
                _letterCursor = 0;
                drawLetterSelector();
            }
        } else if (_inputMode == INPUT_LETTER) {
            const char* letters = gridLetters[_gridCursor];
            int count = strlen(letters);
            if (evt == KEY_UP_SHORT) {
                _inputMode = INPUT_GRID;
                drawInputKeyboard();
            } else if (evt == KEY_DOWN_SHORT) {
                _letterCursor = (_letterCursor + 1) % count;
                drawLetterSelector();
            } else if (evt == KEY_DOWN_LONG || evt == KEY_DOWN_DOUBLE) {
                appendPinyin(letters[_letterCursor]);
                _inputMode = INPUT_GRID;
                drawInputKeyboard();
            }
        }
    }
}

void ChatManager::update() {
    if (!_active) return;
    if (_wsConnected) {
        _webSocket.loop();
    } else if (millis() - _lastReconnectTime > 5000) {
        _lastReconnectTime = millis();
        if (WiFi.status() == WL_CONNECTED) {
            connectWebSocket();
        }
    }
}
