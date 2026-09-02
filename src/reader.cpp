#include "reader.h"

ReaderManager reader;

void ReaderManager::init() {
    _active = false;
    _sdReady = false;
    _reading = false;
    _fileCount = 0;
    _fileIndex = 0;
    _scrollOffset = 0;
    _filePosition = 0;
    _pageIndex = 0;
    strcpy(_currentPath, "/");
}

void ReaderManager::enter() {
    _active = true;
    _reading = false;
    strcpy(_currentPath, "/");
    
    // 初始化SD卡
    if (!SD.begin(SD_CS)) {
        _sdReady = false;
        drawSDCardError();
        return;
    }
    _sdReady = true;
    listFiles(_currentPath);
    drawFileList();
}

void ReaderManager::exit() {
    if (_reading) {
        closeFile();
    }
    _active = false;
}

void ReaderManager::drawSDCardError() {
    disp.clear();
    disp.drawTitleBar("TXT阅读器");
    disp.drawText(SCREEN_W/2 - 40, SCREEN_H/2 - 20, "SD卡未插入", 2);
    disp.drawText(SCREEN_W/2 - 50, SCREEN_H/2 + 10, "请插入SD卡后重试", 1);
    disp.drawStatusBar("短按下:重试", "Home:返回");
    disp.refresh(true);
}

bool ReaderManager::listFiles(const char* path) {
    _fileCount = 0;
    _fileIndex = 0;
    _scrollOffset = 0;
    
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        return false;
    }
    
    File entry = dir.openNextFile();
    while (entry && _fileCount < 50) {
        strncpy(_files[_fileCount].name, entry.name(), 63);
        _files[_fileCount].name[63] = 0;
        _files[_fileCount].isDirectory = entry.isDirectory();
        _files[_fileCount].size = entry.size();
        _fileCount++;
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
    
    return _fileCount > 0;
}

void ReaderManager::drawFileList() {
    disp.clear();
    disp.drawTitleBar("TXT阅读器 - 文件列表");
    
    if (!_sdReady) {
        drawSDCardError();
        return;
    }
    
    if (_fileCount == 0) {
        disp.drawText(SCREEN_W/2 - 30, SCREEN_H/2 - 10, "无文件", 2);
    } else {
        int y = 20;
        int visibleCount = 0;
        int maxVisible = 8;
        
        for (int i = _scrollOffset; i < _fileCount && visibleCount < maxVisible; i++) {
            bool selected = (i == _fileIndex);
            if (selected) {
                disp.drawRect(0, y - 2, SCREEN_W, 14, true);
                u8g2Fonts.setForegroundColor(GxEPD_WHITE);
                u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
            }
            
            char line[80];
            if (_files[i].isDirectory) {
                snprintf(line, sizeof(line), "[DIR] %s", _files[i].name);
            } else {
                float sizeKB = _files[i].size / 1024.0;
                snprintf(line, sizeof(line), "  %s (%.1fKB)", _files[i].name, sizeKB);
            }
            disp.drawText(4, y, line, 1);
            
            if (selected) {
                u8g2Fonts.setForegroundColor(GxEPD_BLACK);
                u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
            }
            
            y += 12;
            visibleCount++;
        }
    }
    
    // 当前路径
    char pathLine[80];
    snprintf(pathLine, sizeof(pathLine), "路径: %s", _currentPath);
    disp.drawText(2, SCREEN_H - 24, pathLine, 1);
    
    disp.drawStatusBar("上/下:选择  长按下:打开  短按上:上级目录", "Home:返回");
    disp.refresh(true);
}

void ReaderManager::openFile(const char* filename) {
    char fullPath[160];
    if (strcmp(_currentPath, "/") == 0) {
        snprintf(fullPath, sizeof(fullPath), "/%s", filename);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", _currentPath, filename);
    }
    
    _currentFile = SD.open(fullPath, FILE_READ);
    if (!_currentFile) {
        return;
    }
    
    _reading = true;
    _filePosition = 0;
    _pageIndex = 0;
    loadPage();
    drawReader();
}

void ReaderManager::closeFile() {
    if (_currentFile) {
        _currentFile.close();
    }
    _reading = false;
}

void ReaderManager::loadPage() {
    if (!_currentFile) return;
    
    _currentFile.seek(_filePosition);
    _bufferLen = _currentFile.read((uint8_t*)_readBuffer, MAX_READER_BUFFER - 1);
    _readBuffer[_bufferLen] = 0;
    
    // UTF8 简单处理（不完整，实际应该用完整的UTF8解析）
    // 这里简化处理，直接显示
}

void ReaderManager::drawReader() {
    disp.clear();
    
    // 标题栏显示文件名和页码
    char title[80];
    snprintf(title, sizeof(title), "阅读: %s  页:%d", 
             _files[_fileIndex].name, _pageIndex + 1);
    disp.drawTitleBar(title);
    
    if (_bufferLen == 0) {
        disp.drawText(SCREEN_W/2 - 20, SCREEN_H/2, "文件结束", 2);
    } else {
        // 简单的自动换行显示
        int y = 22;
        int lineIdx = 0;
        char line[50];
        
        for (int i = 0; i < _bufferLen && y < SCREEN_H - 14; i++) {
            line[lineIdx++] = _readBuffer[i];
            
            // 换行条件：遇到换行符、行满、或UTF8字符边界
            if (_readBuffer[i] == '\n' || lineIdx >= 35) {
                line[lineIdx] = 0;
                disp.drawText(4, y, line, 1);
                y += 12;
                lineIdx = 0;
            }
        }
        if (lineIdx > 0 && y < SCREEN_H - 14) {
            line[lineIdx] = 0;
            disp.drawText(4, y, line, 1);
        }
    }
    
    // 进度条
    if (_currentFile && _currentFile.size() > 0) {
        int percent = (int)(_filePosition * 100 / _currentFile.size());
        disp.drawProgressBar(4, SCREEN_H - 14, SCREEN_W - 8, percent);
    }
    
    disp.drawStatusBar("短按下:下一页  短按上:上一页  长按下:返回列表", "Home:返回");
    disp.refresh(true);
}

void ReaderManager::nextPage() {
    if (!_currentFile) return;
    _filePosition += _bufferLen;
    _pageIndex++;
    if (_filePosition >= _currentFile.size()) {
        _filePosition = _currentFile.size();
    }
    loadPage();
    drawReader();
}

void ReaderManager::prevPage() {
    if (!_currentFile) return;
    _filePosition -= _bufferLen;
    if (_filePosition < 0) _filePosition = 0;
    _pageIndex = (_pageIndex > 0) ? _pageIndex - 1 : 0;
    loadPage();
    drawReader();
}

void ReaderManager::handleKey(KeyEvent evt) {
    if (!_active || evt == KEY_NONE) return;
    
    if (evt == KEY_MENU_SHORT) {
        exit();
        app.goHome();
        return;
    }
    
    if (_reading) {
        // 阅读模式
        if (evt == KEY_DOWN_SHORT) {
            nextPage();
        } else if (evt == KEY_UP_SHORT) {
            prevPage();
        } else if (evt == KEY_DOWN_LONG) {
            closeFile();
            listFiles(_currentPath);
            drawFileList();
        }
    } else {
        // 文件列表模式
        if (!_sdReady) {
            if (evt == KEY_DOWN_SHORT) {
                if (SD.begin(SD_CS)) {
                    _sdReady = true;
                    listFiles(_currentPath);
                }
                drawFileList();
            }
            return;
        }
        
        if (evt == KEY_UP_SHORT) {
            if (_fileIndex > 0) {
                _fileIndex--;
                if (_fileIndex < _scrollOffset) _scrollOffset = _fileIndex;
                drawFileList();
            }
        } else if (evt == KEY_DOWN_SHORT) {
            if (_fileIndex < _fileCount - 1) {
                _fileIndex++;
                if (_fileIndex >= _scrollOffset + 8) _scrollOffset = _fileIndex - 7;
                drawFileList();
            }
        } else if (evt == KEY_DOWN_LONG) {
            // 打开文件或进入目录
            if (_files[_fileIndex].isDirectory) {
                char newPath[160];
                if (strcmp(_currentPath, "/") == 0) {
                    snprintf(newPath, sizeof(newPath), "/%s", _files[_fileIndex].name);
                } else {
                    snprintf(newPath, sizeof(newPath), "%s/%s", _currentPath, _files[_fileIndex].name);
                }
                strcpy(_currentPath, newPath);
                listFiles(_currentPath);
                drawFileList();
            } else {
                openFile(_files[_fileIndex].name);
            }
        } else if (evt == KEY_UP_LONG) {
            // 返回上级目录
            if (strcmp(_currentPath, "/") != 0) {
                // 简化处理：直接返回根目录
                strcpy(_currentPath, "/");
                listFiles(_currentPath);
                drawFileList();
            }
        }
    }
}

void ReaderManager::update() {
    // 阅读器不需要定时更新
}

int ReaderManager::countLines(const char* text) {
    int count = 1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') count++;
    }
    return count;
}
