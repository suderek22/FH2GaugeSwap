#include "fh2mod.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace {

constexpr wchar_t kSettingsFile[] = L"ForzaGaugeSwap.ini";
constexpr wchar_t kSettingSection[] = L"ForzaGaugeSwap";
constexpr wchar_t kSettingName[] = L"CustomHUD";
constexpr char kMenuSettingKey[] = "CustomHUD";
constexpr char kMenuSettingLabel[] = "Custom HUD gauges";
constexpr char kMenuSettingDescription[] =
    "Use the custom shared HUD gauges (textures and layout). Restart the game for changes to take effect.";
constexpr char kMediaRoot[] = "ui/textures/Anthem/HUD/Speedometer/";
constexpr char kLayoutMediaPath[] = "UI/Controls/HUDSpeedometerControl.xaml";
constexpr std::size_t kNativeHeaderSize = 83;

std::filesystem::path g_settings_path;

constexpr std::array<const char*, 49> kGaugeFiles = {
    "HUD_Dial_Analogue_Needle.swatchbin",
    "HUD_Dial_Digital_Gear_1.swatchbin",
    "HUD_Dial_Digital_Gear_2.swatchbin",
    "HUD_Dial_Digital_Gear_3.swatchbin",
    "HUD_Dial_Digital_Gear_4.swatchbin",
    "HUD_Dial_Digital_Gear_5.swatchbin",
    "HUD_Dial_Digital_Gear_6.swatchbin",
    "HUD_Dial_Digital_Gear_7.swatchbin",
    "HUD_Dial_Digital_Gear_8.swatchbin",
    "HUD_Dial_Digital_Gear_B.swatchbin",
    "HUD_Dial_Digital_Gear_F.swatchbin",
    "HUD_Dial_Digital_Gear_H.swatchbin",
    "HUD_Dial_Digital_Gear_MA.swatchbin",
    "HUD_Dial_Digital_Gear_N.swatchbin",
    "HUD_Dial_Digital_Gear_PM.swatchbin",
    "HUD_Dial_Digital_Gear_R.swatchbin",
    "HUD_Dial_RevNumbers_0.swatchbin",
    "HUD_Dial_RevNumbers_1.swatchbin",
    "HUD_Dial_RevNumbers_2.swatchbin",
    "HUD_Dial_RevNumbers_3.swatchbin",
    "HUD_Dial_RevNumbers_4.swatchbin",
    "HUD_Dial_RevNumbers_5.swatchbin",
    "HUD_Dial_RevNumbers_6.swatchbin",
    "HUD_Dial_RevNumbers_7.swatchbin",
    "HUD_Dial_RevNumbers_8.swatchbin",
    "HUD_Dial_RevNumbers_9.swatchbin",
    "HUD_Dial_RevNumbers_10.swatchbin",
    "HUD_Dial_RevNumbers_11.swatchbin",
    "HUD_Dial_RevNumbers_12.swatchbin",
    "HUD_Dial_RevNumbers_13.swatchbin",
    "HUD_Dial_RevNumbers_14.swatchbin",
    "HUD_Dial_RevNumbers_15.swatchbin",
    "HUD_Dial_RevNumbers_16.swatchbin",
    "HUD_Dial_RevNumbers_17.swatchbin",
    "HUD_Dial_RevNumbers_18.swatchbin",
    "HUD_Dial_RevNumbers_19.swatchbin",
    "HUD_Dial_RevNumbers_20.swatchbin",
    "HUD_Dial_SpeedDigit_0.swatchbin",
    "HUD_Dial_SpeedDigit_1.swatchbin",
    "HUD_Dial_SpeedDigit_2.swatchbin",
    "HUD_Dial_SpeedDigit_3.swatchbin",
    "HUD_Dial_SpeedDigit_4.swatchbin",
    "HUD_Dial_SpeedDigit_5.swatchbin",
    "HUD_Dial_SpeedDigit_6.swatchbin",
    "HUD_Dial_SpeedDigit_7.swatchbin",
    "HUD_Dial_SpeedDigit_8.swatchbin",
    "HUD_Dial_SpeedDigit_9.swatchbin",
    "HUD_Dial_Unit_KPH1.swatchbin",
    "HUD_Dial_Unit_MPH1.swatchbin",
};

struct TextureSpec {
    std::uint16_t width;
    std::uint16_t height;
    std::size_t file_size;
    std::uint8_t format_marker;
    std::uint8_t profile_marker;
};

bool module_directory(std::filesystem::path& directory) {
    std::wstring module_path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(
        reinterpret_cast<HMODULE>(&__ImageBase),
        module_path.data(),
        static_cast<DWORD>(module_path.size()));
    if (length == 0 || length >= module_path.size()) {
        return false;
    }

    module_path.resize(length);
    directory = std::filesystem::path(module_path).parent_path();
    return !directory.empty();
}

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset]) |
           static_cast<std::uint16_t>(bytes[offset + 1] << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

bool has_tag(const std::vector<std::uint8_t>& bytes, std::size_t offset,
             const char (&tag)[5]) {
    return std::memcmp(bytes.data() + offset, tag, 4) == 0;
}

bool texture_spec(const char* filename, TextureSpec& spec) {
    if (std::strcmp(filename, "HUD_Dial_Analogue_Needle.swatchbin") == 0) {
        spec = {8, 64, 2131, 0x2d, 0x48};
    } else if (std::strncmp(filename, "HUD_Dial_Digital_Gear_", 22) == 0) {
        spec = {64, 128, 32851, 0x2e, 0xb8};
    } else if (std::strncmp(filename, "HUD_Dial_RevNumbers_", 20) == 0) {
        spec = {16, 16, 1107, 0x2d, 0x48};
    } else if (std::strncmp(filename, "HUD_Dial_SpeedDigit_", 20) == 0) {
        spec = {60, 100, 8275, 0x2d, 0x48};
    } else if (std::strncmp(filename, "HUD_Dial_Unit_", 14) == 0) {
        spec = {64, 32, 32851, 0x2e, 0xb8};
    } else {
        return false;
    }
    return true;
}

bool read_gauge(const std::filesystem::path& path,
                std::vector<std::uint8_t>& bytes) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    const std::streamoff size = file.tellg();
    if (size <= 0) {
        return false;
    }

    bytes.resize(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    return static_cast<bool>(
        file.read(reinterpret_cast<char*>(bytes.data()), size));
}

bool matches_native_bundle(const char* filename,
                           const std::vector<std::uint8_t>& bytes) {
    TextureSpec spec{};
    if (!texture_spec(filename, spec) || bytes.size() != spec.file_size ||
        bytes.size() < kNativeHeaderSize) {
        return false;
    }

    if (!has_tag(bytes, 0, "burG") || read_u16(bytes, 4) != 1 ||
        read_u16(bytes, 6) != 1 || read_u32(bytes, 8) != kNativeHeaderSize ||
        read_u32(bytes, 12) != bytes.size() ||
        !has_tag(bytes, 16, "BCXT") || read_u16(bytes, 20) != 2 ||
        read_u16(bytes, 22) != 1 || read_u32(bytes, 24) != 40 ||
        read_u32(bytes, 28) != kNativeHeaderSize ||
        read_u32(bytes, 32) != bytes.size() - kNativeHeaderSize ||
        read_u32(bytes, 36) != bytes.size() - kNativeHeaderSize ||
        !has_tag(bytes, 40, "HCXT") || read_u16(bytes, 44) != 0x0230 ||
        read_u16(bytes, 46) != 8) {
        return false;
    }

    for (std::size_t index = 48; index < 63; ++index) {
        if (bytes[index] != 0) {
            return false;
        }
    }
    if (bytes[63] != 1 || read_u16(bytes, 64) != spec.width ||
        read_u16(bytes, 66) != spec.height || read_u16(bytes, 68) != 1 ||
        read_u16(bytes, 70) != spec.width ||
        read_u16(bytes, 72) != spec.height || read_u16(bytes, 74) != 1 ||
        bytes[76] != 1 || bytes[77] != 1 || bytes[78] != 0 ||
        bytes[79] != spec.format_marker || bytes[80] != 1 ||
        bytes[81] != 0 || bytes[82] != spec.profile_marker) {
        return false;
    }

    return true;
}

bool read_boolean_setting(const std::filesystem::path& settings_path,
                          const wchar_t* setting_name, bool default_value,
                          bool& enabled) {
    wchar_t value[16]{};
    const DWORD length = GetPrivateProfileStringW(
        kSettingSection, setting_name, L"", value,
        static_cast<DWORD>(std::size(value)), settings_path.c_str());
    if (length == 0) {
        const wchar_t* default_text = default_value ? L"1" : L"0";
        if (!WritePrivateProfileStringW(kSettingSection, setting_name,
                                        default_text, settings_path.c_str())) {
            return false;
        }
        enabled = default_value;
        return true;
    }

    if (std::wcscmp(value, L"0") == 0) {
        enabled = false;
        return true;
    }
    if (std::wcscmp(value, L"1") == 0) {
        enabled = true;
        return true;
    }
    return false;
}

bool contains_text(const std::vector<std::uint8_t>& bytes,
                   const char* text) {
    const std::size_t length = std::strlen(text);
    return std::search(bytes.begin(), bytes.end(), text, text + length) !=
           bytes.end();
}

bool valid_hud_layout(const std::vector<std::uint8_t>& bytes) {
    constexpr std::array<const char*, 6> required_markers = {
        "x:Class=\"ForzaUI.HUDSpeedometerControl\"",
        "Binding NeedleAngle",
        "Binding GaugeFillAmount",
        "Binding GaugeRedlineAmount",
        "Binding CurrentGear",
        "Binding IsAnalog",
    };
    if (bytes.empty()) {
        return false;
    }
    for (const char* marker : required_markers) {
        if (!contains_text(bytes, marker)) {
            return false;
        }
    }
    return true;
}

void on_setting_changed(void*, const char* key, int value) {
    if (!key || std::strcmp(key, kMenuSettingKey) != 0 ||
        (value != 0 && value != 1)) {
        OutputDebugStringA(
            "ForzaGaugeSwap: ignored invalid in-game CustomHUD setting change.\n");
        return;
    }

    if (!WritePrivateProfileStringW(
            kSettingSection, kSettingName, value == 1 ? L"1" : L"0",
            g_settings_path.c_str())) {
        OutputDebugStringA(
            "ForzaGaugeSwap: failed to save the in-game setting to ForzaGaugeSwap.ini.\n");
    }
}

} // namespace

FH2MOD_EXPORT int fh2mod_init(const FH2ModApi* api) {
    if (!api || api->api_version < 2 || !api->register_mod ||
        !api->clear_file || !api->set_file) {
        return 1;
    }

    FH2ModInfo info{};
    info.size = sizeof(info);
    info.name = "ForzaGaugeSwap";
    info.version = "0.7.1";
    info.github_repo = nullptr;
    info.platform_version = "2.7.1";
    info.check_updates = 0;
    info.description =
        "Adds validated custom HUD gauges as one bundle (textures and layout).";

    FH2Mod* mod = api->register_mod(&info);
    if (!mod) {
        return 1;
    }

    std::filesystem::path mod_directory;
    if (!module_directory(mod_directory)) {
        api->clear_file(mod, nullptr);
        api->log(mod, "could not locate the mod DLL directory; HUD overrides cleared");
        return 1;
    }

    const std::filesystem::path settings_path =
        mod_directory / kSettingsFile;
    g_settings_path = settings_path;
    bool enabled = false;
    if (!read_boolean_setting(settings_path, kSettingName, false, enabled)) {
        api->clear_file(mod, nullptr);
        api->log(mod, "invalid gauge setting or could not create the INI; HUD overrides cleared");
        return 0;
    }

    if (api->api_version >= 4 && api->add_setting) {
        FH2Setting setting{};
        setting.size = sizeof(setting);
        setting.key = kMenuSettingKey;
        setting.label = kMenuSettingLabel;
        setting.description = kMenuSettingDescription;
        setting.value = enabled ? 1 : 0;
        setting.needs_restart = 1;
        setting.on_change = on_setting_changed;
        if (!api->add_setting(mod, &setting)) {
            api->log(mod, "could not add the Custom HUD gauges option to the in-game Mods menu");
        }
    } else {
        api->log(mod, "in-game Mods menu settings are unavailable; use ForzaGaugeSwap.ini");
    }

    api->clear_file(mod, nullptr);
    if (!enabled) {
        api->log(mod, "CustomHUD=0; stock HUD textures and layout are unchanged");
        return 0;
    }

    std::vector<std::uint8_t> layout;
    const std::filesystem::path layout_path =
        mod_directory / L"UI" / L"Controls" /
        L"HUDSpeedometerControl.xaml";
    if (!read_gauge(layout_path, layout) || !valid_hud_layout(layout)) {
        api->log(mod, "rejected bundled HUD layout; required FH2 bindings are missing");
        return 0;
    }

    std::vector<std::vector<std::uint8_t>> textures;
    const std::filesystem::path texture_directory =
        mod_directory / L"HUD" / L"Speedometer";
    textures.reserve(kGaugeFiles.size());
    for (const char* filename : kGaugeFiles) {
        std::vector<std::uint8_t> bytes;
        const std::filesystem::path texture_path =
            texture_directory / std::filesystem::path(filename);
        if (!read_gauge(texture_path, bytes) ||
            !matches_native_bundle(filename, bytes)) {
            std::string message = "rejected custom gauge bundle ";
            message += filename;
            message += "; expected the matching stock Durango/BC7 format; no overrides applied";
            api->log(mod, message.c_str());
            return 0;
        }
        textures.push_back(std::move(bytes));
    }

    if (!api->set_file(mod, kLayoutMediaPath, layout.data(), layout.size())) {
        api->clear_file(mod, nullptr);
        api->log(mod, "failed to install the bundled HUD layout; all mod overrides cleared");
        return 0;
    }

    for (std::size_t index = 0; index < kGaugeFiles.size(); ++index) {
        const std::string media_path =
            std::string(kMediaRoot) + kGaugeFiles[index];
        const std::vector<std::uint8_t>& bytes = textures[index];
        if (!api->set_file(mod, media_path.c_str(), bytes.data(), bytes.size())) {
            api->clear_file(mod, nullptr);
            api->log(mod, "failed to install the complete HUD texture set; all mod overrides cleared");
            return 0;
        }
    }

    api->log(mod, "installed bundled custom HUD gauges (layout and 49 validated textures)");
    return 0;
}
