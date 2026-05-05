#include <chrono>
#include <windows.h>
#include <iostream>
#include <thread>
#include <TlHelp32.h>

#include "output/client_dll.hpp"
#include "output/offsets.hpp"

namespace schemas = cs2_dumper::schemas::client_dll;
namespace offsets = cs2_dumper::offsets::client_dll;

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
    std::cout << "[+] Optimized Bhop Active. [SPACE] to jump | [END] to exit" << std::endl;
    // Persistent LocalPlayer pointer to reduce unnecessary RPM calls
    uintptr_t localPlayer = 0;
    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {

        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            // Re-verify localPlayer exists (handles round restarts/map changes)
            localPlayer = Read<uintptr_t>(hProcess, clientModule + offsets::dwLocalPlayerPawn);
            if (localPlayer) {
                int32_t flags = Read<int32_t>(hProcess, localPlayer + schemas::C_BaseEntity::m_fFlags);
                // FL_ONGROUND is bit 0.
                // We only jump if the game says we are standing on something.
                if (flags & (1 << 0)) {
                    // Quick "tap" of the spacebar
                    PostMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);

                    std::this_thread::sleep_for(std::chrono::nanoseconds(10));

                    PostMessage(hwnd, WM_KEYUP, VK_SPACE, 0);
                }
            }
            // While holding space, poll faster (1ns) for frame-perfect jumps
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));

        } else {
            // While NOT holding space, poll slower (10ms) to save energy
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    CloseHandle(hProcess);

    return 0;

}
