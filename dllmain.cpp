#include <cstdint>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <Windows.h>
#include "UniversalProxyDLL.h"
#pragma comment(lib, "user32.lib")
namespace fs = std::filesystem;

#define LOGGING


// Function to check if a given path is absolute
bool is_absolute_path(const std::string& path) {
    return fs::path(path).is_absolute();
}

// Improved function to get the DLL name
std::string GetDllName() {
    char* dllName = new char[MAX_PATH];
    if (GetModuleFileNameA(NULL, dllName, MAX_PATH)) {
        char* pLastSlash = strrchr(dllName, '\\');
        if (pLastSlash) {
            pLastSlash++; // Move past the last slash
            char* pPeriod = strchr(pLastSlash, '.');
            if (pPeriod) {
                *pPeriod = '\0'; // Null-terminate at the period
            }
        }
        return std::string(dllName);
    }
    else {
        delete[] dllName;
        return "";
    }
}

// Parses command line arguments
std::vector<std::wstring> ParseCommandLine(const std::wstring& cmdline) {
    std::vector<std::wstring> args;
    std::wistringstream iss(cmdline);
    std::wstring token;
    while (iss >> token) {
        args.push_back(token);
    }
    return args;
}

// Finds a specific flag argument
bool FindFlagArg(const std::vector<std::wstring>& args, const std::wstring& flagName) {
    return std::find_if(args.begin(), args.end(), [&flagName](const std::wstring& arg) { return arg == flagName; }) != args.end();
}

// Finds the target name argument
std::string FindTargetNameArg(const std::vector<std::wstring>& args) {
    for (const auto& arg : args) {
        if (arg.find(L"--proxy-target=") == 0) {
            size_t pos = arg.find_first_of(L'=', 1);
            if (pos != std::wstring::npos) {
                std::wstring wsValue = arg.substr(pos + 1);
                return std::string(wsValue.begin(), wsValue.end());
            }
        }
    }
    return ""; // Return empty string if not found
}

// Loads a DLL using an override path if available
HMODULE load_target_dll(HMODULE moduleHandle, const std::string& targetName) {
    HMODULE hModule = nullptr;
    wchar_t moduleFilenameBuffer[MAX_PATH] = { '\0' };
    GetModuleFileNameW(moduleHandle, moduleFilenameBuffer, MAX_PATH);
    const auto currentPath = fs::path(moduleFilenameBuffer).parent_path();
    std::string ext = ".dll";
    std::string targetFileName = targetName + ext;
    const fs::path targetPath = currentPath / targetName / targetFileName;

    hModule = LoadLibraryW(targetPath.c_str());
    if (!hModule) {
        hModule = LoadLibraryA(targetFileName.c_str());
    }

    return hModule;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        try {
            UPD::CreateProxy(hinstDLL);
            std::string dllName = GetDllName();
            wchar_t* cmdLine = GetCommandLineW();
            auto args = ParseCommandLine(cmdLine);

            bool console = FindFlagArg(args, L"--proxy-console");
            if (console) UPD::OpenDebugTerminal();

            std::string targetName = FindTargetNameArg(args);
#ifdef LOGGING
            if (!targetName.empty()) {
                std::wcout << L"UniversalProxyDLL > Target Name: " << targetName.c_str() << std::endl;
            }
            else {
                std::wcout << L"UniversalProxyDLL > No --target-name argument found." << std::endl;
            }
#endif

            bool ui = FindFlagArg(args, L"--proxy-ui");
#ifdef LOGGING
            for (const auto& arg : args) {
                std::wcout << arg << std::endl;
            }
#endif
            HMODULE htargetDll = load_target_dll(hinstDLL, targetName);
            if (!htargetDll) {
#ifdef LOGGING
                std::string errMSG = "UniversalProxyDLL > Failed to load " + targetName + " from " + dllName;
                std::cerr << errMSG;
                if (ui) MessageBox(nullptr, errMSG.c_str(), "UniversalProxyDLL Error", MB_OK | MB_ICONERROR);
#endif
                bool exit = FindFlagArg(args, L"--proxy-exit");
                if (exit) ExitProcess(1);
            }
        }
        catch (std::runtime_error& e) {
#ifdef LOGGING
            std::cerr << e.what() << std::endl;
#endif
            return FALSE;
        }
    }
    return TRUE;
}
