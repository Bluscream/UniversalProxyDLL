#include <cstdint>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>
#include <sstream>
#include <iostream>
#include <tchar.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma comment(lib, "user32.lib")

#include "UniversalProxyDLL.h"

namespace fs = std::filesystem;

// Function to check if a given path is absolute
bool is_absolute_path(const std::string& path)
{
    return fs::path(path).is_absolute();
}

char* GetDllName() {
    // Allocate memory for the DLL name
    char* dllName = (char*)malloc(MAX_PATH * sizeof(char));
    if (dllName == NULL) {
        return _strdup("");
    }
    GetModuleFileName(NULL, dllName, MAX_PATH);
    char* pLastSlash = strrchr(dllName, '\\');
    if (pLastSlash != NULL) {
        pLastSlash++;
        char* pPeriod = strchr(pLastSlash, '.');
        if (pPeriod != NULL) {
            *pPeriod = '\0';
        }
    }
    return dllName;
}

std::vector<std::wstring> ParseCommandLine(const std::wstring& cmdline) {
    std::vector<std::wstring> args;
    std::wistringstream iss(cmdline);
    std::wstring token;
    while (iss >> token) {
        args.push_back(token);
    }
    return args;
}
bool FindFlagArg(const std::vector<std::wstring>& args, const std::wstring& flagName) {
    for (const auto& arg : args) {
        if (arg == flagName) {
            return true;
        }
    }
    return false;
}
std::string FindTargetNameArg(const std::vector<std::wstring>& args) {
    for (const auto& arg : args) {
        if (arg.find(L"--proxy-target=") == 0) {
            size_t pos = arg.find_first_of(L'=', 1); // Find position of '='
            if (pos != std::wstring::npos) {
                std::wstring wsValue = arg.substr(pos + 1); // Extract the value after '=' as wstring
                std::string strValue(wsValue.begin(), wsValue.end()); // Convert wstring to string
                return strValue;
            }
        }
    }
    return "ue4ss"; // Return empty string if not found
}

// Function to load a DLL, attempting to use an override path if available
HMODULE load_target_dll(HMODULE moduleHandle, std::string targetName)
{
    HMODULE hModule = nullptr;
    wchar_t moduleFilenameBuffer[1024] = { '\0' };
    GetModuleFileNameW(moduleHandle, moduleFilenameBuffer, sizeof(moduleFilenameBuffer) / sizeof(wchar_t));
    const auto currentPath = fs::path(moduleFilenameBuffer).parent_path();
    std::string ext = ".dll";
    std::string targetFileName = targetName + ext;
    const fs::path targetPath = currentPath / targetName / targetFileName;

    // Attempt to load target.dll from target directory
    hModule = LoadLibraryW(targetPath.c_str());
    if (!hModule)
    {
        // If loading from target directory fails, load from the current directory
        hModule = LoadLibraryA(targetFileName.c_str());
    }

    return hModule;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);
		try
		{
			UPD::CreateProxy(hinstDLL);

            char* dllName = GetDllName();

            wchar_t* cmdLine = GetCommandLineW();
            auto args = ParseCommandLine(cmdLine);

            bool console = FindFlagArg(args, L"--proxy-console");
            if (console) UPD::OpenDebugTerminal();

            std::string targetName = FindTargetNameArg(args);

            // Example usage
            if (!targetName.empty()) {
                std::wcout << L"UniversalProxyDLL > Target Name: " << targetName.c_str() << std::endl;
            }
            else {
                std::wcout << L"UniversalProxyDLL > No --target-name argument found." << std::endl;
            }

            bool ui = FindFlagArg(args, L"--proxy-ui");

            // Example usage
            for (const auto& arg : args) {
                std::wcout << arg << std::endl;
                if (ui) MessageBoxW(nullptr, arg.c_str(), L"ARGUMENT", MB_OK | MB_ICONERROR);
            }
			
            HMODULE htargetDll = load_target_dll(hinstDLL, targetName);
            if (!htargetDll)
            {
                std::string errMSG = "UniversalProxyDLL > Failed to load " + targetName + " from " + dllName;
                std::cerr << errMSG;
                if (ui) MessageBox(nullptr, errMSG.c_str(), "UniversalProxyDLL Error", MB_OK | MB_ICONERROR);
                bool exit = FindFlagArg(args, L"--proxy-exit");
                if (exit) ExitProcess(1);
            }

		}
		catch (std::runtime_error e)
		{
			std::cerr << e.what() << std::endl;
			return FALSE;
		}
	}
    return TRUE;
}
