#include <windows.h>

HHOOK hHook = NULL;
HANDLE hMutex = NULL;

void RunFlameshot(const char* args) {
    char exePath[MAX_PATH];
    char cmd[MAX_PATH + 50];

    // 1. Get the absolute path of the current running executable
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    
    // 2. Strip the executable name to extract the directory (find the last '\')
    int len = lstrlenA(exePath);
    while (len > 0 && exePath[len] != '\\') { len--; }
    exePath[len + 1] = '\0'; // Terminate the string right after the '\'
    
    // 3. Append "flameshot.exe" to the directory path
    lstrcatA(exePath, "flameshot.exe");

    // 4. Check if flameshot.exe actually exists in that location
    // If NOT (e.g., user placed the tool elsewhere), fallback to the default path
    if (GetFileAttributesA(exePath) == INVALID_FILE_ATTRIBUTES) {
        lstrcpyA(exePath, "C:\\Program Files\\Flameshot\\bin\\flameshot.exe");
    }

    // 5. Format the command line arguments and execute safely
    wsprintfA(cmd, "\"%s\" %s", exePath, args);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;

        // Prevent infinite loops from injected keys (e.g., macros, keybd_event)
        if (pKeyBoard->flags & LLKHF_INJECTED) {
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            
            // Intercept the Print Screen key
            if (pKeyBoard->vkCode == VK_SNAPSHOT) {
                static DWORD lastSnapshotTime = 0;
                DWORD currentTime = GetTickCount();
                
                // Debounce: 300ms cooldown to prevent rapid firing
                if (currentTime - lastSnapshotTime > 300) { 
                    lastSnapshotTime = currentTime;
                    RunFlameshot("full -c");
                }
                return 1; // Block the original key press from reaching the OS
            }

            // Intercept the Win + Shift + S combination
            if (pKeyBoard->vkCode == 0x53) {
                bool isWinDown = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
                bool isShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000);

                if (isWinDown && isShiftDown) {
                    static DWORD lastWinShiftSTime = 0;
                    DWORD currentTime = GetTickCount();
                    
                    // Debounce: 300ms cooldown
                    if (currentTime - lastWinShiftSTime > 300) {
                        lastWinShiftSTime = currentTime;

                        // Release the modifier keys virtually to prevent sticky keys
                        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
                        keybd_event(VK_RWIN, 0, KEYEVENTF_KEYUP, 0);
                        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);

                        RunFlameshot("gui");
                    }
                    return 1; // Block the original key press from reaching the OS
                }
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Ensure only one instance of this tool runs at a time
    hMutex = CreateMutexA(NULL, TRUE, "FlameshotHook_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0; 

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if (hHook == NULL) return 1;

    // Aggressive RAM optimization: Trim the working set before entering the message loop
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hHook);
    if (hMutex) CloseHandle(hMutex);
    return 0;
}
