#include <chrono>
#include <windows.h>
#include <iostream>
#include <thread>
#include <TlHelp32.h>

#include "output/client_dll.hpp"
#include "output/offsets.hpp"

namespace schemas = cs2_dumper::schemas::client_dll;
namespace offsets = cs2_dumper::offsets::client_dll;

constexpr int FL_ONGROUND = 1 << 0;

template <typename T>
inline T Read(HANDLE hProcess, uintptr_t address) {
    T buffer = T();
    ReadProcessMemory(hProcess, (LPCVOID)address, &buffer, sizeof(T), NULL);
    return buffer;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const char* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_stricmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
        CloseHandle(hSnap);
    }
    return modBaseAddr;
}

int main() {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    HWND hwnd = FindWindowA(NULL, "Counter-Strike 2");
    if (!hwnd) return 1;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);

    uintptr_t clientModule = GetModuleBaseAddress(pid, "client.dll");
    if (!clientModule) return 1;

    std::cout << "[+] BHOP Active | Hold SPACE | P to Toggle Pause | END to exit" << std::endl;

    uintptr_t localPlayer = 0;
    auto lastJump = std::chrono::steady_clock::now();
    bool paused = false;

    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {

        // Simple toggle for 'P'
        if (GetAsyncKeyState('P') & 0x8000) {
            paused = !paused;
            std::cout << (paused ? "[!] Paused (Chat Mode)" : "[+] Resumed (BHOP Mode)") << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        // Your original logic, only runs if not paused
        if (!paused && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
            localPlayer = Read<uintptr_t>(hProcess, clientModule + offsets::dwLocalPlayerPawn);

            if (localPlayer) {
                int32_t flags = Read<int32_t>(hProcess, localPlayer + schemas::C_BaseEntity::m_fFlags);

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastJump).count();

                if ((flags & FL_ONGROUND) && elapsed >= 15) {
                    PostMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);
                    PostMessage(hwnd, WM_KEYUP, VK_SPACE, 0);
                    lastJump = now;
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        } else {
            // Idle sleep to keep CPU usage low when paused or space isn't held
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    CloseHandle(hProcess);
    return 0;
}
