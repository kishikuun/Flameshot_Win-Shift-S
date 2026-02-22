# 🎯 Flameshot Win-Shift-S Override 

![Platform](https://img.shields.io/badge/Platform-Windows-blue.svg)
![Language](https://img.shields.io/badge/Language-C%2B%2B_Win32API-f34b7d.svg)
![RAM](https://img.shields.io/badge/RAM_Usage-<600KB-success.svg)
![Size](https://img.shields.io/badge/Binary_Size-~16KB-success.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

> A highly optimized, background C++ Win32 utility that forcibly intercepts the default Windows screenshot hotkeys (`Print Screen` and `Win + Shift + S`) and redirects them to **Flameshot**, completely bypassing the native Windows Snipping Tool.
Built with extreme minimalism in mind, this tool uses pure Windows API to achieve a virtually invisible footprint, consuming practically zero CPU and keeping memory usage aggressively low.

---

## ✨ Features
* **🧠 Smart Auto-Detect (Zero Config):** Drop the `.exe` in your Flameshot directory, and it automatically detects the correct path. Gracefully falls back to `C:\Program Files\...` if placed elsewhere.
* **🪶 Ultra-Lightweight:** Replaces heavy `ShellExecuteA` with `CreateProcessA` and uses aggressive memory trimming (`SetProcessWorkingSetSize`) to keep RAM usage under **300KB**.
* **⏱️ Debounce Protection:** A built-in 300ms cooldown prevents rapid-fire execution and system hanging if keys are accidentally held down.
* **🔒 Single Instance Lock:** Mutex protection ensures only one background process can run at a time.
* **🛡️ Anti-Looping:** Ignores injected/simulated key presses (`LLKHF_INJECTED`) to avoid conflicts with macro software or keyloggers.

---
## 🚀 Installation & Usage
1. **Download** or compile the `Flameshot_Win-Shift-S.exe` file.
2. **Move** the `.exe` into your Flameshot installation folder (e.g., `C:\Program Files\Flameshot\bin\`). *This is recommended for auto-detection.*
3. **Run** it. The tool operates silently in the background with no system tray icon or windows.
4. **Autostart (Recommended):** * Press `Win + R` to open the Run dialog.
   * Type `shell:startup` and hit Enter.
   * Place a **Shortcut** of this `.exe` into that folder to have it run automatically on boot.

---

## 🛠️ Compilation Instructions
This project does not require any heavy C++ standard libraries (No STL, No `<iostream>`). To achieve the smallest possible binary size and maximum performance, it is recommended to compile using **MinGW GCC (`g++`)**. I used MinGW GCC 15.2.
Run the following command in your terminal (PowerShell/CMD):

```powershell
g++ main.cpp -o Flameshot_Win-Shift-S.exe -mwindows -O2 -s -ffunction-sections -fdata-sections "-Wl,--gc-sections,--strip-all"
```

> 💡 **Note:** *The generated `.exe` will only be around **15KB - 20KB** on disk and consume **~200-600KB** of RAM during runtime.*
