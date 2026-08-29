#ifndef READER_H
#define READER_H

#include "config.h"
#include "display.h"
#include "input.h"
#include "app_state.h"
#include <SD.h>

// 文件条目
struct FileEntry {
    char name[64];
    bool isDirectory;
    long size;
};

class ReaderManager {
public:
    void init();
    void enter();
    void exit();
    void update();
    void handleKey(KeyEvent evt);
    bool isActive() { return _active; }
    
private:
    bool _active;
    bool _sdReady;
    char _currentPath[128];
    FileEntry _files[50];
    int _fileCount;
    int _fileIndex;
    int _scrollOffset;
    
    // 阅读状态
    bool _reading;
    File _currentFile;
    long _filePosition;
    char _readBuffer[MAX_READER_BUFFER];
    int _bufferLen;
    int _pageIndex;
    
    void drawFileList();
    void drawReader();
    void drawSDCardError();
    bool listFiles(const char* path);
    void openFile(const char* filename);
    void closeFile();
    void loadPage();
    void nextPage();
    void prevPage();
    int countLines(const char* text);
};

extern ReaderManager reader;

#endif // READER_H
