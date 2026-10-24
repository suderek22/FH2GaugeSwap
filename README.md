# Forza Gauge Swap

> **Experimental.** Expect rough edges; back up your game files and use at your own risk.

A mod for Forza Horizon 2 (built on the [FH2 Mod Platform](https://github.com/Trinculo54/FH2-Mod-Platform)) that replaces the shared gameplay HUD speedometer with a custom gauge.

It replaces, as one bundle:

- the speedometer layout (`Controls/HUDSpeedometerControl.xaml`): dark face, bezel and an RPM progress strip bound to the game's own gauge values
- 49 HUD textures: speed digits, RPM labels, gear glyphs, MPH/KPH units and the analogue needle

## Options

Under **Pause > Settings > Mods** there is a single **Custom HUD gauges** On/Off option (`CustomHUD` in `ForzaGaugeSwap.ini`, default `1`). It controls the layout and textures together, because mixing them can stop the game leaving the pause menu. Changing it requires a game restart; after switching to Off, one extra restart may be needed before the stock HUD returns, since the platform removes old overrides shortly after startup.

On startup the mod validates the layout and all 49 textures. If anything is missing or invalid, it installs nothing and the game keeps its stock HUD.

## Install

1. Install the FH2 Mod Platform and make sure `[Files] EnableMediaOverrides=1` is set in Extra Options.
2. Copy the contents of `dist` (after building) into the game's top-level `fh2-mods` folder, keeping the `UI\Controls` subfolder.
3. Place the `.swatchbin` files in `fh2-mods\HUD\Speedometer`.

Check `fh2-mods\platform.log` for `installed bundled custom HUD gauges (layout and 49 validated textures)` or for validation messages.

Only one mod can own each overridden file, so don't run it alongside other mods that replace the same HUD gauge files (for example FnFNitro).

## Build

From this folder in PowerShell:

```powershell
.\build.ps1
```

Requires Python 3, CMake and an x64 MSVC C++ toolchain.

## Textures

Source artwork is generated as PNGs (8x64, 16x16, 60x100, 64x32 and 64x128). Game-ready textures are native Durango BC7 `.swatchbin` files made with the reusable [SwatchbinConverter](../SwatchbinConverter/README.md) via `build_swatchbins.ps1`, which takes the path to your ForzaTech tools and your game's Anthem archive as parameters (`-ForzaTechDir`, `-AnthemArchive`). Prebuilt textures are only needed if you don't regenerate them.
