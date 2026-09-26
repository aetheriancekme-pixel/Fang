#include "Driver.h"

std::uint32_t Driver_t::Find_Process(const std::string& Process_Name)
{
    std::uint64_t Local_Process = 0;
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        return Local_Process;
    }

    PROCESSENTRY32 Process_Entry{};
    Process_Entry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(Snapshot, &Process_Entry))
    {
        do
        {
            if (!_stricmp(Process_Name.c_str(), Process_Entry.szExeFile))
            {
                Local_Process = Process_Entry.th32ProcessID;
                Process_ID = Local_Process;
                break;
            }
        } while (Process32Next(Snapshot, &Process_Entry));
    }

    CloseHandle(Snapshot);
    return Local_Process;
}

std::uint64_t Driver_t::Find_Module(const std::string& Module_Name)
{
    std::uint64_t Module_Address = 0;

    if (!Process_Handle)
    {
        return Module_Address;
    }

    DWORD Process_ID = GetProcessId(Process_Handle);
    HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, Process_ID);

    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        return Module_Address;
    }

    MODULEENTRY32 Module_Entry{};
    Module_Entry.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(Snapshot, &Module_Entry))
    {
        do
        {
            if (!_stricmp(Module_Name.c_str(), Module_Entry.szModule))
            {
                Module_Address = reinterpret_cast<uint64_t>(Module_Entry.modBaseAddr);
                Base_Address = Module_Address;
                break;
            }
        } while (Module32Next(Snapshot, &Module_Entry));
    }

    CloseHandle(Snapshot);
    return Module_Address;
}

bool Driver_t::Attach_Process(const std::string& Process_Name)
{
    HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, false, Find_Process(Process_Name));

    if (Process == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    Process_Handle = Process;

    return true;
}

std::string Driver_t::Read_String(std::uint64_t Address)
{
    std::int32_t String_Length = Read<std::int32_t>(Address + 0x10);
    std::uint64_t String_Address = (String_Length >= 16) ? Read<std::uint64_t>(Address) : Address;

    if (String_Length == 0 || String_Length > 255)
    {
        return "Unknown";
    }

    std::vector<char> Buffer(String_Length + 1, 0);
    Driver_ReadVirtualMemory(Process_Handle, reinterpret_cast<void*>(String_Address), Buffer.data(), Buffer.size(), nullptr);

    return std::string(Buffer.data(), String_Length);
}

void Driver_t::Write_String(std::uint64_t Address, const std::string& Value)
{
    auto Str = Read<RbxString>(Address);

    if (Value.length() > Str.Capacity)
    {
        while (Value.length() > Str.Capacity)
            Str.Capacity = Str.Capacity * 2 + 1;

        Str.Data.Pointer = reinterpret_cast<std::uint64_t>(
            VirtualAllocEx(
                Process_Handle,
                nullptr,
                Str.Capacity,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
            )
            );
    }

    Str.Length = Value.length();

    if (Str.Length > 15)
    {
        Write<RbxString>(Address, Str);
        Driver_WriteVirtualMemory(
            Process_Handle,
            reinterpret_cast<void*>(Str.Data.Pointer),
            (void*)Value.data(),
            Value.length(),
            nullptr
        );
    }
    else
    {
        Str.Capacity = 15;
        Write<RbxString>(Address, Str);
        Driver_WriteVirtualMemory(
            Process_Handle,
            reinterpret_cast<void*>(Address),
            (void*)Value.data(),
            Value.length(),
            nullptr
        );
    }
}

std::uint32_t Driver_t::Get_Process()
{
    return Process_ID;
}

std::uint64_t Driver_t::Get_Module()
{
    return Base_Address;
}

HANDLE Driver_t::Get_Handle()
{
    return Process_Handle;
}