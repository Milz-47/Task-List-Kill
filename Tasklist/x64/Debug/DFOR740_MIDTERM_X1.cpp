#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")

struct ProcessInfo {
    DWORD pid;
    std::wstring exeName;
    std::wstring exePath;
    std::wstring services;
    SIZE_T memUsage;
};

std::wstring GetProcessPath(DWORD pid) {
    std::wstring path;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        wchar_t buffer[MAX_PATH];
        if (GetModuleFileNameEx(hProcess, NULL, buffer, MAX_PATH))
            path = buffer;
        CloseHandle(hProcess);
    }
    return path;
}

size_t GetProcessMemUsage(DWORD pid) {
    PROCESS_MEMORY_COUNTERS pmc;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
            CloseHandle(hProcess);
        return pmc.WorkingSetSize;
    }
    return 0;
}

std::map<DWORD, std::wstring> GetServiceProcessMapping() {
    std::map<DWORD, std::wstring> svcMap;
    DWORD bytesNeeded = 0, servicesReturned = 0, resumeHandle = 0;

    ENUM_SERVICE_STATUS_PROCESS* services = nullptr;
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!scm) return svcMap;

    EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_STATE_ALL, nullptr, 0, &bytesNeeded, &servicesReturned, &resumeHandle, nullptr);

    if (GetLastError() == ERROR_MORE_DATA) {
        services = (ENUM_SERVICE_STATUS_PROCESS*)malloc(bytesNeeded);
        if (EnumServicesStatusExW(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
            SERVICE_STATE_ALL, (LPBYTE)services, bytesNeeded,
            &bytesNeeded, &servicesReturned, &resumeHandle, nullptr)) {
            for (DWORD i = 0; i < servicesReturned; ++i) {
                DWORD pid = services[i].ServiceStatusProcess.dwProcessId;
                if (pid != 0) {
                    if (!svcMap[pid].empty()) svcMap[pid] += L", ";
                    svcMap[pid] += services[i].lpServiceName;
                }
            }
        }
    }
    if (services) free(services);
    CloseServiceHandle(scm);
    return svcMap;
}

int main() {
    std::wofstream txtFile(L"TaskList_Output.txt");
    std::wofstream csvFile(L"TaskList_Output.csv");
    csvFile << L"Image Name, PID, Services, Memory (KB), Path\n";

    auto svcMap = GetServiceProcessMapping();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            ProcessInfo pinfo;
            pinfo.pid = pe32.th32ProcessID;
            pinfo.exeName = pe32.szExeFile;
            pinfo.exePath = GetProcessPath(pinfo.pid);
            pinfo.services = svcMap.count(pinfo.pid) ? svcMap[pinfo.pid] : L"";
            pinfo.memUsage = GetProcessMemUsage(pinfo.pid) / 1024; // KB

            txtFile << L"Image Name: " << pinfo.exeName << L"\n"
                << L"PID: " << pinfo.pid << L"\n"
                << L"Services: " << pinfo.services << L"\n"
                << L"Memory: " << pinfo.memUsage << L" KB\n"
                << L"Path: " << pinfo.exePath << L"\n"
                << L"-----------------------------------------\n";

            csvFile << L"\"" << pinfo.exeName << L"\","
                << pinfo.pid << L",\""
                << pinfo.services << L"\","
                << pinfo.memUsage << L",\""
                << pinfo.exePath << L"\"\n";

        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    txtFile.close();
    csvFile.close();

    std::wcout << L"Export complete: TaskList_Output.txt and TaskList_Output.csv\n";
    return 0;
}
