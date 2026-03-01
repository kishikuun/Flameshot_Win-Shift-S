#include <windows.h>
#include <tlhelp32.h>

HHOOK hHook = NULL;
HANDLE hMutex = NULL;

// Global cache for the executable path to prevent Disk I/O latency inside the hook
char g_flameshotPath[MAX_PATH] = {0}; 

// Called ONLY ONCE at startup to resolve the target executable path
void InitFlameshotPath() {
    char exePath[MAX_PATH];
    
    // 1. Get the current daemon's directory
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    int len = lstrlenA(exePath);
    while (len > 0 && exePath[len] != '\\') { len--; }
    exePath[len + 1] = '\0'; 
    
    // 2. Append the target executable name
    lstrcatA(exePath, "flameshot.exe");

    // 3. Verify existence, otherwise fallback to default installation path
    if (GetFileAttributesA(exePath) != INVALID_FILE_ATTRIBUTES) {
        lstrcpyA(g_flameshotPath, exePath);
    } else {
        lstrcpyA(g_flameshotPath, "C:\\Program Files\\Flameshot\\bin\\flameshot.exe");
    }
}

// REAL-TIME CHECK: Scans the RAM for the process only when hotkeys are physically pressed (~1ms execution time)
bool IsFlameshotRunning() {
    bool exists = false;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (Process32First(hSnapshot, &pe32)) {
            do {
                // Case-insensitive comparison of the process name
                if (lstrcmpiA(pe32.szExeFile, "flameshot.exe") == 0) {
                    exists = true;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return exists;
}

// Executes the cached path directly
void RunFlameshot(const char* args) {
    char cmd[MAX_PATH + 50];
    wsprintfA(cmd, "\"%s\" %s", g_flameshotPath, args);

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

        // Prevent infinite loops from simulated key presses (e.g., keybd_event)
        if (pKeyBoard->flags & LLKHF_INJECTED) {
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            
            // --- Intercept the Print Screen key ---
            if (pKeyBoard->vkCode == VK_SNAPSHOT) {
                
                // Graceful Fallback: Real-time check. If Flameshot is dead, let Windows handle the key.
                if (!IsFlameshotRunning()) {
                    return CallNextHookEx(hHook, nCode, wParam, lParam); 
                }

                static DWORD lastSnapshotTime = 0;
                DWORD currentTime = GetTickCount();
                
                // Debounce: 300ms cooldown to prevent spam
                if (currentTime - lastSnapshotTime > 300) { 
                    lastSnapshotTime = currentTime;
                    RunFlameshot("full -c");
                }
                return 1; // Consume the key
            }

            // --- Intercept the Win + Shift + S combination ---
            if (pKeyBoard->vkCode == 0x53) {
                bool isWinDown = (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
                bool isShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000);

                if (isWinDown && isShiftDown) {
                    
                    // Graceful Fallback: Real-time check. If Flameshot is dead, let Windows Snipping Tool take over.
                    if (!IsFlameshotRunning()) {
                        return CallNextHookEx(hHook, nCode, wParam, lParam);
                    }

                    static DWORD lastWinShiftSTime = 0;
                    DWORD currentTime = GetTickCount();
                    
                    if (currentTime - lastWinShiftSTime > 300) {
                        lastWinShiftSTime = currentTime;

                        // Virtually release modifier keys to prevent 'sticky keys' OS bug
                        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
                        keybd_event(VK_RWIN, 0, KEYEVENTF_KEYUP, 0);
                        keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);

                        RunFlameshot("gui");
                    }
                    return 1; // Consume the key
                }
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. Ensure single instance
    hMutex = CreateMutexA(NULL, TRUE, "FlameshotHook_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0; 

    // 2. Cache the executable path strictly ONCE before setting the hook
    InitFlameshotPath();

    // 3. Install the low-level keyboard hook
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    if (hHook == NULL) return 1;

    // 4. Aggressive RAM optimization: Trim the working set memory
    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);

    // 5. Message loop to keep the daemon alive
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 6. Cleanup
    UnhookWindowsHookEx(hHook);
    if (hMutex) CloseHandle(hMutex);
    return 0;
}
