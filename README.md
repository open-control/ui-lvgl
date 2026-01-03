# ui-lvgl

LVGL UI integration for Open Control Framework.

## Features

- **FontLoader**: Stateless font management optimized for embedded systems
- **FontUtils**: Low-level font loading with retry logic
- **View/Widget interfaces**: Base classes for LVGL UI components

## Installation

Add to your `platformio.ini`:

```ini
lib_deps =
    lvgl/lvgl@^9.4.0
    https://github.com/open-control/ui-lvgl.git
```

## Font Management

### Architecture

```
FLASH (compile-time, read-only)
├── font::Entry FONT_ENTRIES[] - descriptors
└── const uint8_t font_bin[] - binary data

RAM (runtime, load/unload on context switch)
└── lv_font_t* pointers - loaded fonts
```

### FontLoader API

Zero-allocation font management with compile-time entries.

```cpp
#include <oc/ui/lvgl/FontLoader.hpp>

namespace font = oc::ui::lvgl::font;

// 1. Define font storage (RAM)
struct MyFonts {
    lv_font_t* regular = nullptr;
    lv_font_t* bold = nullptr;
    lv_font_t* icons = nullptr;
};
MyFonts fonts;

// 2. Define font entries (Flash)
const font::Entry FONT_ENTRIES[] = {
    {&fonts.regular, regular_bin, regular_len, "Regular", false},
    {&fonts.bold, bold_bin, bold_len, "Bold", true},  // essential
    {&fonts.icons, icons_bin, icons_len, "Icons", false},
};

// 3. Load fonts
font::loadEssential(FONT_ENTRIES);  // Load only essential=true
font::load(FONT_ENTRIES);           // Load all (idempotent)

// 4. Use fonts
lv_obj_set_style_text_font(label, fonts.regular, 0);

// 5. Unload when done (frees RAM)
font::unload(FONT_ENTRIES);
```

#### Entry Structure

```cpp
struct Entry {
    lv_font_t** target;      // RAM: where to store loaded font
    const uint8_t* data;     // Flash: binary font data
    uint32_t size;           // Size of font data
    const char* name;        // Debug name
    bool essential;          // Load during boot/splash
};
```

#### Functions

| Function | Description |
|----------|-------------|
| `load(entries)` | Load all fonts where *target == nullptr (idempotent) |
| `loadEssential(entries)` | Load only essential fonts |
| `unload(entries)` | Unload all fonts, free RAM |
| `countLoaded(entries)` | Count loaded fonts |

### FontUtils API

Low-level font loading with retry logic for transient memory failures.

```cpp
#include <oc/ui/lvgl/FontUtils.hpp>

// Load with retry
lv_font_t* font = oc::ui::lvgl::loadBinaryFont(data, len);

// Free
oc::ui::lvgl::freeFont(font);  // Sets to nullptr
```

## Context Switching Pattern

```cpp
// Boot context - load splash fonts
void BootContext::initialize() {
    font::loadEssential(CORE_FONTS);
}

// Main context - load all fonts
void MainContext::initialize() {
    font::load(CORE_FONTS);      // No-op for already loaded
    font::load(PLUGIN_FONTS);    // Load plugin-specific
}

void MainContext::cleanup() {
    font::unload(PLUGIN_FONTS);  // Free plugin fonts
    // Core fonts stay loaded (shared)
}
```

## Embedded Considerations

- **Flash storage**: Font entries and binary data stored in flash (const)
- **RAM efficiency**: Only lv_font_t* pointers in RAM (~4 bytes each)
- **Zero heap allocation**: Font management uses no dynamic memory
- **Idempotent loading**: Safe to call load() multiple times
- **Context-aware**: Load/unload fonts on context switch to save RAM

## License

MIT
