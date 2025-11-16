#pragma once

#include <windows.h>
#include <shlobj.h>
#include <commctrl.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>

class MD5Hasher {
private:
    HWND m_hWnd;
    HWND m_hWndProgress;
    HWND m_hWndStatus;
    HWND m_hWndEditFolder;
    
    volatile BOOL m_isProcessing;
    std::string m_folder;
    std::string m_output;
    int m_totalFiles;
    int m_currentFile;
    time_t m_startTime;

public:
    MD5Hasher();
    ~MD5Hasher();

    bool initialize(HINSTANCE hInstance);
    int run();

    // Window procedure
    LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    void browseFolder();
    void startProcessing();
    void countFiles(const std::string& path);
    void processFiles(const std::string& path, const std::string& base, std::ofstream& out);
    std::string computeMD5(const std::string& filepath);
    void pumpMessages();
    void updateStatus();

    static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};