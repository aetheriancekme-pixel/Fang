#include "AntiInjection.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>


typedef NTSTATUS(NTAPI* NtQueryInformationThread_t)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NtQueryInformationThread_t pNtQueryInformationThread = nullptr;

static void InitNtQueryInformationThread()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return;

    pNtQueryInformationThread = (NtQueryInformationThread_t)GetProcAddress(ntdll, "NtQueryInformationThread");
}

std::vector<HMODULE> AntiInjection::baselineModules;
HANDLE AntiInjection::hThread = nullptr;
bool AntiInjection::running = false;

static std::vector<HMODULE> GetModules()
{
    std::vector<HMODULE> mods;
    HMODULE buffer[1024];
    DWORD needed = 0;
    if (EnumProcessModules(GetCurrentProcess(), buffer, sizeof(buffer), &needed))
    {
        size_t count = needed / sizeof(HMODULE);
        for (size_t i = 0; i < count; ++i) mods.push_back(buffer[i]);
    }
    return mods;
}

void AntiInjection::Start()
{
    InitNtQueryInformationThread();

    if (running) return;
    baselineModules = GetModules();
    running = true;
    hThread = CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
}

void AntiInjection::Stop()
{
    running = false;
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        hThread = nullptr;
    }
}

DWORD __stdcall AntiInjection::MonitorThread(void*)
{
    while (running)
    {
        if (DetectDebugger() || DetectRemoteDebugger() || DetectNewModules() || DetectManualMapRegions() || DetectThreadStartAnomalies() || DetectNtdllHooks())
        {
            TerminateProcess(GetCurrentProcess(), 0);
        }
        Sleep(500);
    }
    return 0;
}

bool AntiInjection::DetectDebugger() { return IsDebuggerPresent(); }
bool AntiInjection::DetectRemoteDebugger() { BOOL present = FALSE; CheckRemoteDebuggerPresent(GetCurrentProcess(), &present); return present; }

bool AntiInjection::DetectNewModules()
{
    auto now = GetModules();
    for (HMODULE m : now)
    {
        bool known = false;
        for (HMODULE b : baselineModules) { if (m == b) { known = true; break; } }
        if (!known) return true;
    }
    return false;
}

bool AntiInjection::DetectManualMapRegions()
{
    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* addr = nullptr;
    while (VirtualQuery(addr, &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_COMMIT)
        {
            bool exec = (mbi.Protect & PAGE_EXECUTE) || (mbi.Protect & PAGE_EXECUTE_READ) || (mbi.Protect & PAGE_EXECUTE_READWRITE) || (mbi.Protect & PAGE_EXECUTE_WRITECOPY);
            if (exec && mbi.Type == MEM_PRIVATE) return true;
        }
        addr += mbi.RegionSize;
    }
    return false;
}

bool AntiInjection::DetectThreadStartAnomalies()
{
    if (!pNtQueryInformationThread) return false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te{ sizeof(te) };
    if (!Thread32First(snapshot, &te)) { CloseHandle(snapshot); return false; }

    DWORD pid = GetCurrentProcessId();

    do
    {
        if (te.th32OwnerProcessID != pid) continue;
        HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
        if (!hThread) continue;

        PVOID start = nullptr;
        pNtQueryInformationThread(hThread, (THREADINFOCLASS)9, &start, sizeof(start), nullptr);

        bool insideModule = false;
        for (HMODULE m : baselineModules)
        {
            MODULEINFO mi{};
            GetModuleInformation(GetCurrentProcess(), m, &mi, sizeof(mi));
            uintptr_t s = (uintptr_t)mi.lpBaseOfDll;
            uintptr_t e = s + mi.SizeOfImage;
            if ((uintptr_t)start >= s && (uintptr_t)start <= e) { insideModule = true; break; }
        }

        CloseHandle(hThread);
        if (!insideModule) { CloseHandle(snapshot); return true; }

    } while (Thread32Next(snapshot, &te));

    CloseHandle(snapshot);
    return false;
}

bool AntiInjection::DetectNtdllHooks()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;
    auto addr = (unsigned char*)GetProcAddress(ntdll, "NtOpenProcess");
    if (!addr) return false;
    if (addr[0] != 0x4C || addr[1] != 0x8B || addr[2] != 0xD1) return true;
    return false;
}
