# SmithyInfo

Skyrim SE SKSE plugin that shows smelting and tempering info on SkyUI item cards.

## What It Does

When you hover over a weapon or armor item in your inventory, SmithyInfo appends crafting info to the item card:

- **Smelts into:** What the item breaks down into at a smelter
- **Tempers with:** What materials you need to improve it at a grindstone or workbench

Optionally, it can also tag items in the inventory list with small indicators (e.g. `|ts|`, `|t|`, `|s|`) for quick filtering — off by default, fully customizable.

No more running to a crafting station just to check.

## Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)

## Installation

Install with your mod manager. No ESP, no Papyrus scripts, just a single DLL.

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

; Effects text on item card (right side) — per menu type (1 = show, 0 = hide)
bEffectsPlayer = 1          ; Player inventory
bEffectsContainer = 1       ; NPC inventory + chests/containers
bEffectsMerchant = 1        ; Merchant barter (both sides of trade screen)

; Name indicators on inventory list (left side) — per menu type (1 = show, 0 = hide)
bIndicatorPlayer = 0        ; Player inventory
bIndicatorContainer = 0     ; NPC inventory + chests/containers
bIndicatorMerchant = 0      ; Merchant barter (both sides of trade screen)

; Indicator format (single characters only)
sIndicatorPrefix = |
sIndicatorSuffix = |
sIndicatorSmelt = s
sIndicatorTemper = t
```

All `b*` toggles must be integers (`0` or `1`). `GetPrivateProfileIntA` cannot parse boolean strings.

## How It Works

1. At game data load, builds a cache of all COBJ (Constructible Object) recipes — smelting and tempering
2. Hooks `AdvanceMovie` on `InventoryMenu`, `ContainerMenu`, and `BarterMenu` to inject text every frame
3. Appends info to the SkyUI item card's `effects` HTML field
4. Optionally appends indicators to inventory list item names
5. Deduplication check prevents redundant work on already-injected cards

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

- Only weapon/armor item cards show the info (misc items have no `effects` field in SkyUI — sorry all that Dwemer junk, you'll just have to flail around at the smelter like old times)
- Mid-session script-added recipes won't appear until next save load
- Item card space is limited — long material lists may get clipped
- Enchanted items: smelting info is hidden (can't smelt them), tempering info requires the Arcane Blacksmith perk (configurable)

## License

MIT
