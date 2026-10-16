#include <Windows.h>
#include "PDA-Injector.h"
#include "inih/cpp/INIReader.h"

PDAInjectorOptions pdaOptions;
static HMODULE g_hModule = NULL;

#ifdef DX11PROXY
extern HMODULE hTrueDX11_43Module;
#endif

std::string GetModuleDirectory() {
    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(g_hModule, path, MAX_PATH) > 0) {
        char* lastSlash = strrchr(path, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
            return std::string(path);
        }
    }
    return "";
}

BOOL LoadSettings(PDAInjectorOptions& settings, const std::string& ini_path) {
    std::string resolvedPath = ini_path;
    // If path is relative, resolve relative to the DLL directory
    if (ini_path.find(':') == std::string::npos && ini_path.rfind("\\\\", 0) != 0) {
        std::string modDir = GetModuleDirectory();
        if (!modDir.empty()) {
            resolvedPath = modDir + ini_path;
        }
    }

    INIReader reader(resolvedPath);
    if (reader.ParseError() < 0) {
        return FALSE;
    }

    settings.checklist.image = reader.GetString("checklist", "image", "checklist.png");
    settings.checklist.toggle_key = reader.GetInteger("checklist", "toggle_key", 121);
    settings.checklist.show_on_start = reader.GetBoolean("checklist", "show_on_start", true);

    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
#ifdef DX11PROXY
        hTrueDX11_43Module = LoadLibraryA("d3dx11_43.original.dll");
        if (!hTrueDX11_43Module) {
            // Condor might not find original DLL if not renamed or missing
        }
#endif
        LoadSettings(pdaOptions, "pda.ini");
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        CleanupChecklist();
#ifdef DX11PROXY
        if (hTrueDX11_43Module) {
            FreeLibrary(hTrueDX11_43Module);
            hTrueDX11_43Module = NULL;
        }
#endif
        break;
    }
    return TRUE;
}
