#include <cstdint>
#include <fstream>
#include <string>
#include <filesystem>

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

// Function to load a DLL, attempting to use an override path if available
HMODULE load_ue4ss_dll(HMODULE moduleHandle)
{
    HMODULE hModule = nullptr;
    wchar_t moduleFilenameBuffer[1024] = { '\0' };
    GetModuleFileNameW(moduleHandle, moduleFilenameBuffer, sizeof(moduleFilenameBuffer) / sizeof(wchar_t));
    const auto currentPath = fs::path(moduleFilenameBuffer).parent_path();
    const fs::path ue4ssPath = currentPath / "ue4ss" / "UE4SS.dll";

    // Attempt to load UE4SS.dll from ue4ss directory
    hModule = LoadLibraryW(ue4ssPath.c_str());
    if (!hModule)
    {
        // If loading from ue4ss directory fails, load from the current directory
        hModule = LoadLibraryW(L"UE4SS.dll");
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
#ifdef DEBUG
			UPD::OpenDebugTerminal();
#endif
			UPD::CreateProxy(hinstDLL);
			
            HMODULE hUE4SSDll = load_ue4ss_dll(hinstDLL);
            if (!hUE4SSDll)
            {
                std::string errMSG = "Failed to load UE4SS.dll. Please see the docs on correct installation: https://docs.ue4ss.com/installation-guide";
                std::cerr << errMSG;
                //MessageBox(nullptr, errMSG.c_str(), "UE4SS Error", MB_OK | MB_ICONERROR);
                //ExitProcess(0);
            }

            //// Get the address of DllMain using the Unicode version of GetProcAddress
            //FARPROC pDllMain = GetProcAddress(hModule, L"DllMain");
            //if (pDllMain == NULL) {
            //    std::cerr << "Failed to find DllMain in DLL\n";
            //    FreeLibrary(hModule); // Clean up
            //    return 1;
            //}

            //// Call DllMain
            //// Note: Adjusting the parameters according to your requirements
            //typedef BOOL(WINAPI* DllMainFunc)(HMODULE, DWORD);
            //DllMainFunc DllMain = (DllMainFunc)pDllMain;
            //BOOL result = DllMain(NULL, DLL_PROCESS_ATTACH);

            //if (!result) {
            //    std::cerr << "DllMain failed\n";
            //}
            //else {
            //    std::cout << "DllMain succeeded\n";
            //}

		}
		catch (std::runtime_error e)
		{
			std::cout << e.what() << std::endl;
			return FALSE;
		}
	}
    return TRUE;
}
