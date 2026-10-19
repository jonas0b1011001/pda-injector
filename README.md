# Condor 3 PDA Checklist Injector

A lightweight, zero-latency proxy DLL for **Condor Soaring Simulator 3** that injects a custom checklist or navigation document (PNG, JPG, BMP) directly into the cockpit PDA screen. Ideal for VR pilots who need quick access to checklists, airspace charts, or task sheets without leaving the cockpit or dealing with external window overlays.

## Features
- **High Quality Bicubic Scaling:** Uses Windows GDI+ (`InterpolationModeHighQualityBicubic`) to pre-render and fit documents crisply into the PDA resolution.
- **Zero-Latency In-Game Rendering:** Prepares a 32-bit BGRA buffer in advance; inside the render hook, it executes a fast memory copy or driver-accelerated blit without frame drops or CPU stutter.
- **Seamless Hotkey Toggle:** Switch between the custom checklist and Condor's original built-in PDA using a configurable hotkey (default: **F10**).
- **Clean & Lightweight:** No external window captures (`GetDC`), no system-wide mouse hooks (`WH_MOUSE_LL`), and no window subclassing.

---

## Configuration (`pda.ini`)

Place `pda.ini` in the Condor 3 installation directory:

```ini
[checklist]
image=checklist1.png
image2=checklist2.png
toggle_key=121
page_key=123
invert_colors=1
show_on_start=1
```

### Options:
- **`image`** / **`image1`**: Primary checklist image file (relative to Condor root or absolute). Supports PNG, JPG, BMP.
- **`image2`** .. **`image9`**: (Optional) Additional checklist pages. If configured, you can cycle through all pages in-game.
- **`toggle_key`**: Virtual Keycode for toggling between the checklist and the original PDA (default: `121` for **F10**).
- **`page_key`**: Virtual Keycode for switching between checklist pages (default: `123` for **F12**).
- **`invert_colors`**: Set to `1` to invert colors ($255 - C$, e.g. for high-contrast / dark-mode reading in VR or night flights). Set to `0` for original colors.
- **`show_on_start`**: Whether the checklist is active when Condor starts (`1` = yes, `0` = no, starts with standard PDA).

---

## Installation

1. Copy `d3dx11_43.dll` into your Condor 3 directory (where `Condor.exe` is located).
2. If an original `d3dx11_43.dll` exists in your Condor directory, rename it to `d3dx11_43.original.dll`.
3. Place `pda.ini` and your checklist image (e.g. `checklist.png`) in the Condor 3 directory.
4. Launch Condor 3. Press **F10** to toggle between the checklist and Condor's original PDA.

---

## Building from Source

### Requirements
- Windows 10/11
- CMake 3.20+
- Visual Studio 2022 (MSVC with C++20 support)

### CMake Build (32-Bit / Win32)
Condor 3 is a 32-bit application and requires a 32-bit proxy DLL:

```cmd
cmake -B build -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The resulting `d3dx11_43.dll` will be located in `build/Release/`.
