#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <string>

#define TIMER_ID 1

const wchar_t* TARGET_PROCESS_NAME = L"dontstarve_steam_x64.exe";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ApplyRestrictions(HWND hWnd);
DWORD GetProcessIdByName(const wchar_t* processName);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t szClassName[] = L"DontStarveLimiterClass";

    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = szClassName;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Error in reg class of window", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hWnd = CreateWindowExW(
        0, szClassName, L"DSTIdle",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 150,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        MessageBoxW(NULL, L"Error to create window!", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hStatusLabel;

    switch (message) {
        case WM_CREATE: {
            hStatusLabel = CreateWindowExW(
                0, L"STATIC", L"Searching dontstarve_steam_x64.exe...",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 40, 360, 30, hWnd, NULL, NULL, NULL
            );

            SetTimer(hWnd, TIMER_ID, 1000, NULL);

            ApplyRestrictions(hWnd);
            break;
        }

        case WM_TIMER: {
            if (wParam == TIMER_ID) {
                ApplyRestrictions(hWnd);
            }
            break;
        }

        case WM_DESTROY: {
            KillTimer(hWnd, TIMER_ID);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (wcscmp(pe.szExeFile, processName) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return pid;
}

void ApplyRestrictions(HWND hWnd) {
    HWND hStatusLabel = GetWindow(hWnd, GW_CHILD);
    DWORD pid = GetProcessIdByName(TARGET_PROCESS_NAME);

    if (pid == 0) {
        SetWindowTextW(hStatusLabel, L"Waiting for launch game...");
        return;
    }

    HANDLE hProcess = OpenProcess(
        PROCESS_SET_INFORMATION | PROCESS_QUERY_INFORMATION,
        FALSE, pid
    );

    if (hProcess == NULL) {
        SetWindowTextW(hStatusLabel, L"Error in access to process of game");
        return;
    }

    bool success = true;

    if (!SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS)) {
        success = false;
    }

    DWORD_PTR processAffinityMask = 1;
    if (!SetProcessAffinityMask(hProcess, processAffinityMask)) {
        success = false;
    }

    if (!SetPriorityClass(hProcess, PROCESS_MODE_BACKGROUND_BEGIN)) {

    }

    CloseHandle(hProcess);

    if (success) {
        SetWindowTextW(hStatusLabel, L"Success, keep this program in background!");
    } else {
        SetWindowTextW(hStatusLabel, L"Error, no working.");
    }
}
