#pragma once

#include <windows.h>
#include <string>
#include <fstream>
#include <ctime>

// Well-named class and methods, using only Win32 and CRT
class MD5Hasher {
private:
    HWND m_hWnd;
    HWND m_hWndProgress;
    HWND m_hWndStatus;
    HWND m_hWndEditFolder;
    HWND m_hWndListPaths;

    volatile BOOL m_isProcessing;
    std::string m_folder;
    std::string m_output;
    int m_totalFiles;
    int m_currentFile;
    time_t m_startTime;
    bool m_listPaths;

public:
    MD5Hasher();
    ~MD5Hasher();

    bool initialize(HINSTANCE hInstance);
    int run();
    LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    void browseFolder();
    void startProcessing();
    void countFiles(const std::string& path);
    void countPaths(const std::string& path);
    void processFiles(const std::string& path, const std::string& base, std::ofstream& out);
    void processPaths(const std::string& path, const std::string& base, std::ofstream& out);
    std::string computeMD5(const std::string& filepath);
    void pumpMessages();
    void updateStatus();

    static LRESULT CALLBACK staticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
