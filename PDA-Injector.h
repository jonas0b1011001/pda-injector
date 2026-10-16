#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

// Condor PDA doubled resolution dimensions
constexpr long PDA_DEFAULT_WIDTH = (298 * 2);
constexpr long PDA_DEFAULT_HEIGHT = (498 * 2);

struct ChecklistOptions {
    std::string image = "checklist.png";
    int toggle_key = 121; // VK_F10
    bool show_on_start = true;
};

struct PDAInjectorOptions {
    ChecklistOptions checklist;
};

bool PDAInject(HDC hPdaDC, int page);
void CleanupChecklist();
std::string GetModuleDirectory();