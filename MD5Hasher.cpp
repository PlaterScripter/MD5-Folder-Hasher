#include "MD5Hasher.h"
#include <sstream>
#include <iomanip>

// Link with required libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")  // For CryptoAPI

// MD5Hasher implementation
MD5Hasher::MD5Hasher() 
    : m_hWnd(nullptr), m_hWndProgress(nullptr), m_hWndStatus(nullptr), m_hWndEditFolder(nullptr),
      m_isProcessing(FALSE), m_totalFiles(0), m_currentFile(0), m_startTime(0) {
}

MD5Hasher::~MD5Hasher() {
    if (m_isProcessing) {
        m_isProcessing = FALSE;
        Sleep(1000);
    }
}

bool MD5Hasher::initialize(HINSTANCE hInstance) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // Register window class
    WNDCLASSEXA wcex;
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MD5Hasher::staticWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(MD5Hasher*);
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = "MD5HasherClass";
    wcex.hIconSm = LoadIconA(nullptr, IDI_APPLICATION);

    if (!RegisterClassExA(&wcex)) {
        return false;
    }

    // Create window
    m_hWnd = CreateWindowExA(0, "MD5HasherClass", "MD5 File Hasher", 
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, 620, 160,
                            nullptr, nullptr, hInstance, this);

    return m_hWnd != nullptr;
}

int MD5Hasher::run() {
    if (!m_hWnd) return 1;

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MD5Hasher::staticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    MD5Hasher* pThis = nullptr;

    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MD5Hasher*>(pCreate->lpCreateParams);
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hWnd = hWnd;
    } else {
        pThis = reinterpret_cast<MD5Hasher*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->wndProc(hWnd, message, wParam, lParam);
    }

    return DefWindowProcA(hWnd, message, wParam, lParam);
}

LRESULT MD5Hasher::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            CreateWindowA("BUTTON", "Browse", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         10, 10, 80, 30, hWnd, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
            m_hWndEditFolder = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
                                           100, 10, 400, 30, hWnd, reinterpret_cast<HMENU>(1002), nullptr, nullptr);
            CreateWindowA("BUTTON", "Start", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         510, 10, 80, 30, hWnd, reinterpret_cast<HMENU>(1003), nullptr, nullptr);
            m_hWndProgress = CreateWindowA(PROGRESS_CLASSA, nullptr, WS_VISIBLE | WS_CHILD,
                                         10, 50, 580, 30, hWnd, reinterpret_cast<HMENU>(1004), nullptr, nullptr);
            m_hWndStatus = CreateWindowA("STATIC", "", WS_VISIBLE | WS_CHILD,
                                       10, 90, 580, 30, hWnd, reinterpret_cast<HMENU>(1005), nullptr, nullptr);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1001: // Browse button
                    browseFolder();
                    break;
                case 1003: // Start button
                    startProcessing();
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hWnd, message, wParam, lParam);
    }
    return 0;
}

void MD5Hasher::browseFolder() {
    BROWSEINFOA bi = {0};
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = "Select folder to hash";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != nullptr) {
        char folderPath[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, folderPath)) {
            m_folder = folderPath;
            SetWindowTextA(m_hWndEditFolder, m_folder.c_str());
        }
        CoTaskMemFree(pidl);
    }
}

void MD5Hasher::startProcessing() {
    if (m_isProcessing) {
        m_isProcessing = FALSE;
        SetWindowTextA(GetDlgItem(m_hWnd, 1003), "Start");
        return;
    }

    char folderBuffer[MAX_PATH];
    GetWindowTextA(m_hWndEditFolder, folderBuffer, MAX_PATH);
    m_folder = folderBuffer;

    if (m_folder.empty()) {
        MessageBoxA(m_hWnd, "Please select a folder.", "Error", MB_OK);
        return;
    }

    m_isProcessing = TRUE;
    SetWindowTextA(GetDlgItem(m_hWnd, 1003), "Cancel");

    m_totalFiles = 0;
    m_currentFile = 0;
    countFiles(m_folder);

    if (m_totalFiles == 0) {
        MessageBoxA(m_hWnd, "No files to hash.", "Info", MB_OK);
        m_isProcessing = FALSE;
        SetWindowTextA(GetDlgItem(m_hWnd, 1003), "Start");
        return;
    }

    SendMessageA(m_hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessageA(m_hWndProgress, PBM_SETPOS, 0, 0);
    SetWindowTextA(m_hWndStatus, "Counting complete. Starting hashing...");

    m_output = m_folder + "\\files.md5";
    std::ofstream out(m_output);
    if (!out.is_open()) {
        MessageBoxA(m_hWnd, "Error opening output file.", "Error", MB_OK);
        m_isProcessing = FALSE;
        SetWindowTextA(GetDlgItem(m_hWnd, 1003), "Start");
        return;
    }

    m_startTime = time(nullptr);
    processFiles(m_folder, m_folder, out);
    out.close();

    if (m_isProcessing) {
        std::string msg = "Processing complete. Output saved to " + m_output;
        MessageBoxA(m_hWnd, msg.c_str(), "Done", MB_OK);
    } else {
        MessageBoxA(m_hWnd, "Processing cancelled.", "Info", MB_OK);
    }

    m_isProcessing = FALSE;
    SetWindowTextA(GetDlgItem(m_hWnd, 1003), "Start");
}

void MD5Hasher::countFiles(const std::string& path) {
    WIN32_FIND_DATAA find;
    HANDLE hFind;
    std::string searchPath = path + "\\*";

    hFind = FindFirstFileA(searchPath.c_str(), &find);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (strcmp(find.cFileName, ".") == 0 || strcmp(find.cFileName, "..") == 0) continue;
        if (_stricmp(find.cFileName, "files.md5") == 0) continue;

        std::string subpath = path + "\\" + find.cFileName;
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            countFiles(subpath);
        } else {
            m_totalFiles++;
        }
    } while (FindNextFileA(hFind, &find));

    FindClose(hFind);
}

void MD5Hasher::processFiles(const std::string& path, const std::string& base, std::ofstream& out) {
    WIN32_FIND_DATAA find;
    HANDLE hFind;
    std::string searchPath = path + "\\*";

    hFind = FindFirstFileA(searchPath.c_str(), &find);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!m_isProcessing) return;

        if (strcmp(find.cFileName, ".") == 0 || strcmp(find.cFileName, "..") == 0) continue;
        if (_stricmp(find.cFileName, "files.md5") == 0) continue;

        std::string subpath = path + "\\" + find.cFileName;
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            processFiles(subpath, base, out);
        } else {
            std::string relpath = subpath.substr(base.length() + 1);
            for (char& c : relpath) {
                if (c == '\\') c = '/';
            }

            std::string hash = computeMD5(subpath);
            if (!hash.empty()) {
                out << hash << " *../" << relpath << "\n";

                m_currentFile++;
                updateStatus();

                if (m_currentFile % 10 == 0) {
                    pumpMessages();
                }
            }
        }
    } while (FindNextFileA(hFind, &find) && m_isProcessing);

    FindClose(hFind);
}

std::string MD5Hasher::computeMD5(const std::string& filepath) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BYTE rgbFile[4096];
    DWORD cbRead = 0;
    BYTE rgbHash[16];  // MD5 is always 16 bytes
    DWORD cbHash = 16;
    
    std::string result;

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return "";
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "";
    }

    hFile = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    // Read file and update hash
    while (ReadFile(hFile, rgbFile, sizeof(rgbFile), &cbRead, NULL) && cbRead > 0) {
        if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
            CloseHandle(hFile);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return "";
        }
    }

    if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_HANDLE_EOF) {
        CloseHandle(hFile);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return "";
    }

    cbHash = 16;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        // Convert to hex string
        char hashStr[33];
        for (DWORD i = 0; i < cbHash; i++) {
            sprintf_s(hashStr + i * 2, 3, "%02x", rgbHash[i]);
        }
        hashStr[32] = 0;
        result = hashStr;
    }

    CloseHandle(hFile);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    
    return result;
}

void MD5Hasher::pumpMessages() {
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void MD5Hasher::updateStatus() {
    SendMessageA(m_hWndProgress, PBM_SETPOS, static_cast<WPARAM>(m_currentFile * 100 / m_totalFiles), 0);

    time_t now = time(nullptr);
    double elapsed = difftime(now, m_startTime);
    double eta = 0;
    if (m_currentFile > 0) {
        double timePerFile = elapsed / m_currentFile;
        eta = timePerFile * (m_totalFiles - m_currentFile);
    }

    char status[256];
    sprintf_s(status, sizeof(status), "Processing: %d/%d (%.2f%%) ETA: %.0f s", m_currentFile, m_totalFiles, 
              (m_currentFile * 100.0 / m_totalFiles), eta);
    SetWindowTextA(m_hWndStatus, status);
}