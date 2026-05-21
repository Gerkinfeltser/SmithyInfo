# SmithyInfo

Skyrim SE SKSE plugin that shows smelting and tempering info on SkyUI item cards.

## What It Does

When you hover over a weapon or armor item in your inventory, SmithyInfo appends crafting info to the item card:

- **Smelts into:** What the item breaks down into at a smelter
- **Tempers with:** What materials you need to improve it at a grindstone or workbench

**DIII Integration (Recommended):** If you have [Dynamic Inventory Icon Injector](https://www.nexusmods.com/skyrimspecialedition/mods/174136) by [JerryYOJ](https://github.com/JerryYOJ) installed, SmithyInfo registers `smithySmelt`, `smithySmeltLocked`, `smithyTemper`, and `smithyTemperLocked` conditions so you can show custom icons on smeltable/temperable items. Locked icons appear when conditions are unmet (skill level, perks, quests). This is the preferred display method — icons are cleaner than text. Without DIII, text indicators (`|TS|`, `|T|`, `|S|`) are shown on inventory list items instead (unless disabled in the INI).

Optionally, it can also tag items in the inventory list with small indicators (e.g. `|TS|`, `|T|`, `|S|`) for quick filtering — on by default, fully customizable.

No more running to a crafting station just to check.

## Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)
- [Dynamic Inventory Icon Injector](https://www.nexusmods.com/skyrimspecialedition/mods/174136) (recommended — for icon display; text indicators shown as fallback without it)
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) (optional QOL — in-game settings menu; without it, configure via INI file)

## Installation

Install with your mod manager. No ESP, no Papyrus scripts, just a single DLL.

### DIII Integration (Optional)

If you use DIII, copy `SKSE/Plugins/DIII/smithyinfo-diii-rules.json` to your `Data/SKSE/Plugins/DIII/` folder. You'll need to provide your own icon SWF files and update the `source` and `label` fields in the rules.

Conditions are always registered with DIII. When `bDIIIIntegration = 1` and DIII is installed, icons are used. When disabled or DIII is absent, text indicators are used as a fallback.

### In-Game Settings (Optional)

If you have [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) installed, SmithyInfo adds an in-game settings panel (SmithyInfo > Settings). Changes take effect immediately; use the Save button to persist to INI. Without SMF, configure via the INI file.

## Configuration

Edit `SKSE/Plugins/SmithyInfo.ini`:

```ini
[SmithyInfo]
; Log level: trace, debug, info, warn, error, critical, off
sLogLevel = info

; Hide tempering info for enchanted items if player lacks Arcane Blacksmith perk
bGateEnchantedTempering = 1

; FormID of the Arcane Blacksmith perk (hex, no 0x prefix)
sArcaneBlacksmithPerkFormID = 0005218E

; DIII (Dynamic Inventory Icon Injector) integration — registers smithySmelt/smithyTemper conditions
; When enabled, text indicators are suppressed in favor of DIII icons
bDIIIIntegration = 1

; Effects text on item card (right side) — per menu type (1 = show, 0 = hide)
bEffectsPlayer = 1          ; Player inventory
bEffectsContainer = 1       ; NPC inventory + chests/containers
bEffectsMerchant = 1        ; Merchant barter (both sides of trade screen)

; Name indicators on inventory list (left side) — per menu type (1 = show, 0 = hide)
; Controls visibility for both text indicators and DIII icons
bIndicatorPlayer = 1        ; Player inventory
bIndicatorContainer = 1     ; NPC inventory + chests/containers
bIndicatorMerchant = 1      ; Merchant barter (both sides of trade screen)

; Indicator format (single characters only)
sIndicatorPrefix = |
sIndicatorSuffix = |
sIndicatorSmelt = S
sIndicatorTemper = T
sIndicatorSmeltLocked = S?    ; shown when smelting conditions are unmet
sIndicatorTemperLocked = T?   ; shown when tempering conditions are unmet

; Hide locked indicators (s?/t?) and DIII locked icons
bHideLockedIndicators = 0

; SKSE Menu Framework (optional) — in-game settings menu
bSMFIntegration = 1
```

All `b*` toggles must be integers (`0` or `1`). `GetPrivateProfileIntA` cannot parse boolean strings.

## How It Works

1. At game data load, builds a cache of all COBJ (Constructible Object) recipes — smelting and tempering
2. If DIII is installed, registers `smithySmelt`, `smithySmeltLocked`, `smithyTemper`, and `smithyTemperLocked` conditions for icon display
3. Hooks `AdvanceMovie` on `InventoryMenu`, `ContainerMenu`, and `BarterMenu` to inject text every frame
4. Appends info to the SkyUI item card's `effects` HTML field (if enabled)
5. Optionally appends indicators to inventory list item names
6. Deduplication check prevents redundant work on already-injected cards

Recipe availability is determined by evaluating the full COBJ condition chain (skill level, perks, quests, item counts, etc.) via the game's native `TESCondition::IsTrue()`. Items that fail conditions show locked indicators/icons. Already-tempered items show as locked for tempering.

Recipe cache rebuilds on every save load, so newly installed mods are picked up automatically.

## Building

Requires Visual Studio 2022, Ninja, vcpkg (`VCPKG_ROOT` set), Windows SDK.

```powershell
$vsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
cmd /c "`"$vsDevCmd`" -arch=amd64 >NUL 2>&1 & cmake --preset release"
cmd /c "`"$vsDevCmd`" -arch=amd64 >NUL 2>&1 & cmake --build `"D:\gerkgit\Skyrim_SmithyInfo\.buildenv\build\release`" --config Release"
```

DLL outputs to `SKSE/Plugins/SmithyInfo.dll`.

## Limitations

- Only weapon/armor item cards show the effects text (misc items and alchemy ingredients have no `effects` field in SkyUI — but DIII icons and list indicators still work for them)
- Mid-session script-added recipes won't appear until next save load
- Item card space is limited — long material lists may get clipped
- Enchanted items: smelting info is hidden (can't smelt them), tempering info requires the Arcane Blacksmith perk (configurable)
- DIII integration: SmithyInfo must load before DIII for condition registration to succeed
- Tempered item detection relies on `ExtraTextDisplayData::temperFactor` — some mods may bypass this

## Source

https://github.com/Gerkinfeltser/SmithyInfo

## License

MIT
