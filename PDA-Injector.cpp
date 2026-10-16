/*
Condor 3 PDA Injector - Checklist Display Module
Copyright (C) 2024 Piotr Gertz, 2026 Refactored Edition
*/

#include "PDA-Injector.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gdiplus.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>
#include <vector>
#include <string>

#pragma comment(lib, "gdiplus.lib")

extern PDAInjectorOptions pdaOptions;

// GDI+ Lifecycle
static ULONG_PTR g_gdiplusToken = 0;
static bool g_gdiplusInitialized = false;

static void EnsureGdiPlus() {
    if (!g_gdiplusInitialized) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL) == Gdiplus::Ok) {
            g_gdiplusInitialized = true;
        }
    }
}

// Prepared Checklist Buffer State
static HDC g_hChecklistDC = NULL;
static HBITMAP g_hChecklistDIB = NULL;
static HGDIOBJ g_hOldBitmap = NULL;
static void* g_pDIBBits = nullptr;
static int g_cachedWidth = 0;
static int g_cachedHeight = 0;
static std::string g_cachedImagePath = "";
static bool g_checklistReady = false;

// Scratch buffer for Condor original PDA resize
static HDC g_hScratchDC = NULL;
static HBITMAP g_hScratchBMP = NULL;
static HGDIOBJ g_hOldScratchBMP = NULL;
static int g_scratchW = 0;
static int g_scratchH = 0;

void CleanupChecklist() {
    if (g_hChecklistDC) {
        if (g_hOldBitmap) {
            SelectObject(g_hChecklistDC, g_hOldBitmap);
            g_hOldBitmap = NULL;
        }
        DeleteDC(g_hChecklistDC);
        g_hChecklistDC = NULL;
    }

    if (g_hChecklistDIB) {
        DeleteObject(g_hChecklistDIB);
        g_hChecklistDIB = NULL;
        g_pDIBBits = nullptr;
    }

    if (g_hScratchDC) {
        if (g_hOldScratchBMP) {
            SelectObject(g_hScratchDC, g_hOldScratchBMP);
            g_hOldScratchBMP = NULL;
        }
        DeleteDC(g_hScratchDC);
        g_hScratchDC = NULL;
    }

    if (g_hScratchBMP) {
        DeleteObject(g_hScratchBMP);
        g_hScratchBMP = NULL;
    }

    g_cachedWidth = 0;
    g_cachedHeight = 0;
    g_cachedImagePath = "";
    g_checklistReady = false;
    g_scratchW = 0;
    g_scratchH = 0;

    if (g_gdiplusInitialized) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusInitialized = false;
        g_gdiplusToken = 0;
    }
}

static std::string ResolveImagePath(const std::string& path) {
    if (path.empty()) {
        return "";
    }
    // Check if path is absolute
    if (path.find(':') != std::string::npos || path.rfind("\\\\", 0) == 0) {
        return path;
    }
    std::string modDir = GetModuleDirectory();
    if (!modDir.empty()) {
        return modDir + path;
    }
    return path;
}

static bool UpdateChecklistBuffer(HDC hTargetDC, int targetWidth, int targetHeight) {
    std::string fullImagePath = ResolveImagePath(pdaOptions.checklist.image);

    // If already loaded for current resolution and path, nothing to do
    if (g_checklistReady &&
        g_cachedWidth == targetWidth &&
        g_cachedHeight == targetHeight &&
        g_cachedImagePath == fullImagePath) {
        return true;
    }

    EnsureGdiPlus();
    if (!g_gdiplusInitialized) {
        return false;
    }

    // Recreate DIBSection if resolution changed
    if (g_cachedWidth != targetWidth || g_cachedHeight != targetHeight || !g_hChecklistDC) {
        if (g_hChecklistDC) {
            if (g_hOldBitmap) {
                SelectObject(g_hChecklistDC, g_hOldBitmap);
                g_hOldBitmap = NULL;
            }
            DeleteDC(g_hChecklistDC);
            g_hChecklistDC = NULL;
        }
        if (g_hChecklistDIB) {
            DeleteObject(g_hChecklistDIB);
            g_hChecklistDIB = NULL;
            g_pDIBBits = nullptr;
        }

        g_hChecklistDC = CreateCompatibleDC(hTargetDC);
        if (!g_hChecklistDC) {
            return false;
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = targetWidth;
        bmi.bmiHeader.biHeight = -targetHeight; // top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        g_hChecklistDIB = CreateDIBSection(hTargetDC, &bmi, DIB_RGB_COLORS, &g_pDIBBits, NULL, 0);
        if (!g_hChecklistDIB || !g_pDIBBits) {
            return false;
        }

        g_hOldBitmap = SelectObject(g_hChecklistDC, g_hChecklistDIB);
        g_cachedWidth = targetWidth;
        g_cachedHeight = targetHeight;
    }

    g_cachedImagePath = fullImagePath;
    g_checklistReady = false;

    // Convert image path to UTF-16 for GDI+
    int wlen = MultiByteToWideChar(CP_ACP, 0, fullImagePath.c_str(), -1, NULL, 0);
    std::wstring widePath(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, fullImagePath.c_str(), -1, &widePath[0], wlen);

    Gdiplus::Bitmap srcBitmap(widePath.c_str());
    if (srcBitmap.GetLastStatus() != Gdiplus::Ok) {
        // Fallback: draw error message into memory DC
        RECT rc = { 0, 0, targetWidth, targetHeight };
        HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(g_hChecklistDC, &rc, blackBrush);
        DeleteObject(blackBrush);

        SetTextColor(g_hChecklistDC, RGB(255, 50, 50));
        SetBkMode(g_hChecklistDC, TRANSPARENT);
        std::string msg = "Checklist image not found:\n" + fullImagePath + "\n\nPress F10 for Condor PDA";
        DrawTextA(g_hChecklistDC, msg.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        g_checklistReady = true;
        return true;
    }

    UINT srcW = srcBitmap.GetWidth();
    UINT srcH = srcBitmap.GetHeight();
    if (srcW == 0 || srcH == 0) {
        return false;
    }

    // High quality bicubic scaling with aspect-ratio preservation (Fit / Letterbox)
    float scale = std::min(static_cast<float>(targetWidth) / static_cast<float>(srcW),
                           static_cast<float>(targetHeight) / static_cast<float>(srcH));
    int dstW = static_cast<int>(srcW * scale);
    int dstH = static_cast<int>(srcH * scale);
    int dstX = (targetWidth - dstW) / 2;
    int dstY = (targetHeight - dstH) / 2;

    {
        Gdiplus::Graphics g(g_hChecklistDC);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        // Fill background with black for letterbox margins
        g.Clear(Gdiplus::Color(255, 0, 0, 0));
        g.DrawImage(&srcBitmap, dstX, dstY, dstW, dstH);
    }

    g_checklistReady = true;
    return true;
}

static void ResizeOriginalPDA(HDC hDC, int width, int height) {
    int halfW = width / 2;
    int halfH = height / 2;

    if (!g_hScratchDC || g_scratchW != halfW || g_scratchH != halfH) {
        if (g_hScratchDC) {
            if (g_hOldScratchBMP) {
                SelectObject(g_hScratchDC, g_hOldScratchBMP);
                g_hOldScratchBMP = NULL;
            }
            DeleteDC(g_hScratchDC);
            g_hScratchDC = NULL;
        }
        if (g_hScratchBMP) {
            DeleteObject(g_hScratchBMP);
            g_hScratchBMP = NULL;
        }

        g_hScratchDC = CreateCompatibleDC(hDC);
        g_hScratchBMP = CreateCompatibleBitmap(hDC, halfW, halfH);
        g_hOldScratchBMP = SelectObject(g_hScratchDC, g_hScratchBMP);
        g_scratchW = halfW;
        g_scratchH = halfH;
    }

    BitBlt(g_hScratchDC, 0, 0, halfW, halfH, hDC, 0, 0, SRCCOPY);
    StretchBlt(hDC, 0, 0, width, height, g_hScratchDC, 0, 0, halfW, halfH, SRCCOPY);
}

bool PDAInject(HDC hPdaDC, int page) {
    if (hPdaDC == NULL) {
        return false;
    }

    // State management and hotkey edge detection
    static bool s_wasKeyDown = false;
    static bool s_initializedState = false;
    static bool g_showChecklist = true;

    if (!s_initializedState) {
        g_showChecklist = pdaOptions.checklist.show_on_start;
        s_initializedState = true;
    }

    const int toggleKey = pdaOptions.checklist.toggle_key;
    if (toggleKey > 0) {
        bool isKeyDown = (GetAsyncKeyState(toggleKey) & 0x8000) != 0;
        if (isKeyDown && !s_wasKeyDown) {
            g_showChecklist = !g_showChecklist;
        }
        s_wasKeyDown = isKeyDown;
    }

    // Determine target dimensions from hPdaDC bitmap
    int pdaWidth = PDA_DEFAULT_WIDTH;
    int pdaHeight = PDA_DEFAULT_HEIGHT;

    HBITMAP hTargetBmp = static_cast<HBITMAP>(GetCurrentObject(hPdaDC, OBJ_BITMAP));
    if (hTargetBmp) {
        BITMAP bm = {};
        if (GetObject(hTargetBmp, sizeof(BITMAP), &bm) > 0) {
            if (bm.bmWidth > 0 && bm.bmHeight > 0) {
                pdaWidth = bm.bmWidth;
                pdaHeight = bm.bmHeight;
            }
        }
    }

    if (g_showChecklist) {
        if (UpdateChecklistBuffer(hPdaDC, pdaWidth, pdaHeight) && g_checklistReady) {
            // Check if destination bitmap has accessible direct pixel memory (DIBSection)
            DIBSECTION ds = {};
            if (hTargetBmp &&
                GetObject(hTargetBmp, sizeof(DIBSECTION), &ds) == sizeof(DIBSECTION) &&
                ds.dsBm.bmBits != nullptr &&
                ds.dsBmih.biBitCount == 32 &&
                g_pDIBBits != nullptr) {
                // High performance direct memory copy
                memcpy(ds.dsBm.bmBits, g_pDIBBits, pdaWidth * pdaHeight * 4);
            } else if (g_hChecklistDC) {
                // Fast driver-accelerated GDI memory blit
                BitBlt(hPdaDC, 0, 0, pdaWidth, pdaHeight, g_hChecklistDC, 0, 0, SRCCOPY);
            }
            return true;
        }
    }

    // When checklist is inactive, stretch original Condor PDA render to fill doubled texture
    GdiFlush();
    ResizeOriginalPDA(hPdaDC, pdaWidth, pdaHeight);
    return true;
}
