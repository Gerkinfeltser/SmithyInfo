# SmithyInfo

Skyrim SE SKSE plugin that shows smelting and tempering info on SkyUI item cards.

## What It Does

When you hover over a weapon or armor item in your inventory, SmithyInfo appends crafting info to the item card:

- **Smelts into:** What the item breaks down into at a smelter, including locked recipes when enabled
- **Tempers with:** What materials you need to improve it at a grindstone or workbench, including locked recipes when enabled
- **Item filter:** Hide SmithyInfo output for specific FormIDs, or flip the list into whitelist mode

**DIII Integration (Recommended):** If you have [Dynamic Inventory Icon Injector](https://www.nexusmods.com/skyrimspecialedition/mods/174136) by [JerryYOJ](https://github.com/JerryYOJ) installed, SmithyInfo registers `smithySmelt`, `smithySmeltLocked`, `smithyTemper`, and `smithyTemperLocked` conditions so you can show custom icons on smeltable/temperable items. Locked icons appear when conditions are unmet (skill level, perks, quests). This is the preferred display method — icons are cleaner than text. Without DIII, text indicators (`|TS|`, `|T|`, `|S|`) are shown on inventory list items instead (unless disabled in the INI).

Locked material text is controlled separately from locked icons. With `bShowLockedMaterials = 1`, the item card can show entries like `Tempers with (locked): 1 Steel Ingot` so you know what to collect before you meet the requirements. Tough, but useful.

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

If you have [Dynamic Inventory Icon Injector](https://www.nexusmods.com/skyrimspecialedition/mods/174136) installed, SmithyInfo automatically registers conditions and provides icon rules + SWF — no manual setup required. DIII icons will appear on smeltable/temperable items when `bDIIIIntegration = 1` (default). If DIII is not installed, text indicators (`|TS|`, `|T|`, `|S|`) are shown as a fallback.

### In-Game Settings (Optional)

If you have [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) installed, SmithyInfo adds an in-game settings panel (SmithyInfo > Settings). Changes take effect immediately; use the Save button to persist to INI. Without SMF, configure via the INI file.

## Configuration

Edit `SKSE/Plugins/SmithyInfo.ini`:

```ini
[SmithyInfo]
; Log level: trace, debug, info, warn, error, critical, off
sLogLevel = info

; Gate enchanted tempering availability behind Arcane Blacksmith
; Locked materials can still show as (locked) when bShowLockedMaterials=1
bGateEnchantedTempering = 1

; FormID of the Arcane Blacksmith perk (hex, no 0x prefix)
sArcaneBlacksmithPerkFormID = 0005218E

; DIII integration — registers smithySmelt/smithySmeltLocked/smithyTemper/smithyTemperLocked conditions
; When enabled, text indicators are suppressed in favor of DIII icons
bDIIIIntegration = 1

; Effects text on item card (right side) — per menu type (1 = show, 0 = hide)
bEffectsPlayer = 1          ; Player inventory
bEffectsContainer = 1       ; NPC inventory + chests/containers + pickpocket
bEffectsMerchant = 1        ; Merchant barter (both sides of trade screen)

; Name indicators on inventory list (left side) — per menu type (1 = show, 0 = hide)
; Controls visibility for both text indicators and DIII icons
bIndicatorPlayer = 1        ; Player inventory
bIndicatorContainer = 1     ; NPC inventory + chests/containers + pickpocket
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

; Show locked tempering/smelting materials in item card effects text
; When 1, locked recipes show materials with a (locked) marker
; This can also show enchanted tempering materials as locked when Arcane Blacksmith is missing
bShowLockedMaterials = 1

; Item filter list - comma-separated hex FormIDs (no 0x prefix)
; Empty list = no filtering regardless of mode
sFilterItems =

; Filter mode: 1 = suppress SmithyInfo for listed FormIDs, 0 = show SmithyInfo only for listed FormIDs
bFilterIsBlacklist = 1

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

Recipe availability is determined by evaluating the full COBJ condition chain (skill level, perks, quests, item counts, etc.) via the game's native `TESCondition::IsTrue()`. Items that fail conditions show locked indicators/icons, and can show locked item-card materials when `bShowLockedMaterials = 1`. Already-tempered items show as locked for tempering and can still show locked material text when enabled.

Recipe cache builds after game data loads, so newly installed mods are picked up when you start/load the game with them installed. Recipes added later by scripts during play are not reflected until the next fresh game load.

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
- Mid-session script-added recipes won't appear until the cache is rebuilt on a fresh game load
- Item card space is limited — long material lists may get clipped
- Enchanted items: smelting info is hidden (can't smelt them); available tempering requires the Arcane Blacksmith perk by default, but locked material text may still show when `bShowLockedMaterials = 1`
- DIII integration: SmithyInfo must load before DIII for condition registration to succeed
- DIII icons update on DIII's refresh cycle — there may be a brief delay when toggling indicator settings in-game before icons appear or disappear
- Tempered item detection relies on `ExtraTextDisplayData::temperFactor` — some mods may bypass this

## Q&A

**Why do DIII icons sometimes need the inventory opened twice after starting the game?**

DIII attaches its icon/SWF handling to the SkyUI inventory list when the menu first opens. Sometimes that first list has already been formatted by the time DIII is ready, so icons appear on the next open. SmithyInfo's item-card text uses a separate hook and can appear immediately.

**Why does the item card show SmithyInfo text, but the list has no icons?**

Check that DIII is installed, `bDIIIIntegration = 1`, and the relevant indicator toggle is enabled (`bIndicatorPlayer`, `bIndicatorContainer`, or `bIndicatorMerchant`). If this only happens on the first inventory open after game start, close and reopen the menu. Skyrim has had a think and remembered what icons are.

**Why does an already-tempered item show locked tempering?**

Already-tempered items are treated as locked for tempering indicators so they do not look the same as items you can improve right now. If `bShowLockedMaterials = 1`, the item card can still show the required materials as `(locked)`.

**Why do enchanted items show locked tempering materials?**

By default, available enchanted tempering requires Arcane Blacksmith. Locked material text is separate: with `bShowLockedMaterials = 1`, SmithyInfo can still show what the recipe wants before you have the perk.

**Why does an item show no SmithyInfo output?**

Most likely it has no smelting or tempering COBJ recipe, its FormID is filtered, or it is a misc-style item with no SkyUI item-card `effects` field. Recipes added by scripts during play will not appear until SmithyInfo rebuilds its cache on a fresh game load.

**Can I write true/false in the INI?**

No. Use `0` or `1` for all `b*` settings. The Windows INI parser used here cannot parse `true` or `false`.

## Source

https://github.com/Gerkinfeltser/SmithyInfo

## License

MIT
