#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

// Condor PDA doubled resolution dimensions
constexpr long PDA_DEFAULT_WIDTH = (298 * 2);
constexpr long PDA_DEFAULT_HEIGHT = (498 * 2);

struct ChecklistOptions {
    std::vector<std::string> images;
    int toggle_key = 121; // VK_F10
    int page_key = 123;   // VK_F12
    bool show_on_start = true;
    bool swap_colors = true;
    bool invert_colors = false;
};

struct PDAInjectorOptions {
    ChecklistOptions checklist;
};

bool PDAInject(HDC hPdaDC, int page);
void CleanupChecklist();
std::string GetModuleDirectory();