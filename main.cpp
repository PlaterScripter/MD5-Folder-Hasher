#include "MD5Hasher.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MD5Hasher app;
    if (!app.initialize(hInstance)) {
        MessageBoxA(nullptr, "Failed to initialize application", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    return app.run();
}
