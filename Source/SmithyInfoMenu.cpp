#include "SmithyInfoMenu.h"
#include "DIIIIntegration.h"
#include "ItemCardHook.h"

#include "SMF/SKSEMenuFramework.hpp"

#include <windows.h>

static void WriteBool(const char* a_key, bool a_value, const std::string& a_path) {
    WritePrivateProfileStringA("SmithyInfo", a_key, a_value ? "1" : "0", a_path.c_str());
}

static void WriteString(const char* a_key, const std::string& a_value, const std::string& a_path) {
    WritePrivateProfileStringA("SmithyInfo", a_key, a_value.c_str(), a_path.c_str());
}

void __stdcall RenderSettings() {
    using namespace ImGuiMCP;

    if (CollapsingHeader("Effects Text (Item Card Descriptions)", ImGuiTreeNodeFlags_DefaultOpen)) {
        Checkbox("Player Inventory", &ItemCardHook::effectsPlayer);
        Checkbox("Container / NPC / Pickpocket", &ItemCardHook::effectsContainer);
        Checkbox("Merchant Barter", &ItemCardHook::effectsMerchant);
    }

    Separator();

    if (CollapsingHeader("List Indicators (DIII/Text Tags in Inv. List)", ImGuiTreeNodeFlags_DefaultOpen)) {
        Checkbox("Player Inventory##ind", &ItemCardHook::indicatorPlayer);
        Checkbox("Container / NPC / Pickpocket##ind", &ItemCardHook::indicatorContainer);
        Checkbox("Merchant Barter##ind", &ItemCardHook::indicatorMerchant);

        Separator();

        Checkbox("Enable DIII icon integration", &DIIIIntegration::enabled);
    }

    Separator();

    if (CollapsingHeader("Indicator Format (Text Tag Style)")) {
        static char prefixBuf[32] = {};
        static char suffixBuf[32] = {};
        static char smeltBuf[16] = {};
        static char temperBuf[16] = {};
        static char smeltLockedBuf[16] = {};
        static char temperLockedBuf[16] = {};

        if (prefixBuf[0] == '\0') {
            strncpy_s(prefixBuf, ItemCardHook::indicatorPrefix.c_str(), _TRUNCATE);
            strncpy_s(suffixBuf, ItemCardHook::indicatorSuffix.c_str(), _TRUNCATE);
            strncpy_s(smeltBuf, ItemCardHook::indicatorSmelt.c_str(), _TRUNCATE);
            strncpy_s(temperBuf, ItemCardHook::indicatorTemper.c_str(), _TRUNCATE);
            strncpy_s(smeltLockedBuf, ItemCardHook::indicatorSmeltLocked.c_str(), _TRUNCATE);
            strncpy_s(temperLockedBuf, ItemCardHook::indicatorTemperLocked.c_str(), _TRUNCATE);
        }

        InputText("Prefix", prefixBuf, 32);
        InputText("Suffix", suffixBuf, 32);
        InputText("Smelt Letter", smeltBuf, 16);
        InputText("Temper Letter", temperBuf, 16);
        InputText("Smelt Locked", smeltLockedBuf, 16);
        InputText("Temper Locked", temperLockedBuf, 16);

        ItemCardHook::indicatorPrefix = prefixBuf;
        ItemCardHook::indicatorSuffix = suffixBuf;
        ItemCardHook::indicatorSmelt = smeltBuf;
        ItemCardHook::indicatorTemper = temperBuf;
        ItemCardHook::indicatorSmeltLocked = smeltLockedBuf;
        ItemCardHook::indicatorTemperLocked = temperLockedBuf;
    }

    Separator();

    if (CollapsingHeader("Enchanted Items")) {
        Checkbox("Gate behind Arcane Blacksmith", &ItemCardHook::gateEnchantedTempering);

        static char perkBuf[16] = {};
        if (perkBuf[0] == '\0') {
            snprintf(perkBuf, sizeof(perkBuf), "%08X", ItemCardHook::arcaneBlacksmithPerkID);
        }
        InputText("Arcane Blacksmith FormID (hex)", perkBuf, 16);
        try {
            ItemCardHook::arcaneBlacksmithPerkID = static_cast<RE::FormID>(std::stoul(perkBuf, nullptr, 16));
        } catch (...) {}
    }

    Separator();

    if (CollapsingHeader("Locked Indicators")) {
        Checkbox("Hide locked indicators", &ItemCardHook::hideLockedIndicators);
    }

    Separator();

    if (CollapsingHeader("Locked Materials & Item Filter")) {
        Checkbox("Show locked materials in item card", &ItemCardHook::showLockedMaterials);
        Checkbox("Filter list is blacklist", &ItemCardHook::filterIsBlacklist);

        static char filterBuf[1024] = {};
        static bool filterBufInit = false;
        if (!filterBufInit) {
            strncpy_s(filterBuf, ItemCardHook::filterItemsRaw.c_str(), _TRUNCATE);
            filterBufInit = true;
        }

        if (InputText("Filter FormIDs", filterBuf, 1024)) {
            ItemCardHook::SetFilterItemsFromString(filterBuf);
        }
        TextDisabled("Comma-separated hex FormIDs, no 0x prefix. Empty = no filtering.");
    }

    Separator();

    if (Button("Save to INI")) {
        SmithyInfoMenu::SaveSettings();
    }

    SameLine();

    TextDisabled("(Changes are active immediately. Save persists to INI.)");
}

void SmithyInfoMenu::Register(const std::string& a_iniPath) {
    iniPath = a_iniPath;

    if (!smfEnabled) {
        logger::info("SMF: integration disabled by INI");
        return;
    }

    if (!SKSEMenuFramework::IsInstalled()) {
        logger::info("SMF: SKSE Menu Framework not installed, skipping menu registration");
        return;
    }

    SKSEMenuFramework::SetSection("SmithyInfo");
    SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);
    registered = true;
    logger::info("SMF: menu registered successfully");
}

void SmithyInfoMenu::SaveSettings() {
    if (iniPath.empty()) return;

    WriteBool("bEffectsPlayer", ItemCardHook::effectsPlayer, iniPath);
    WriteBool("bEffectsContainer", ItemCardHook::effectsContainer, iniPath);
    WriteBool("bEffectsMerchant", ItemCardHook::effectsMerchant, iniPath);
    WriteBool("bIndicatorPlayer", ItemCardHook::indicatorPlayer, iniPath);
    WriteBool("bIndicatorContainer", ItemCardHook::indicatorContainer, iniPath);
    WriteBool("bIndicatorMerchant", ItemCardHook::indicatorMerchant, iniPath);
    WriteBool("bGateEnchantedTempering", ItemCardHook::gateEnchantedTempering, iniPath);
    WriteBool("bDIIIIntegration", DIIIIntegration::enabled, iniPath);
    WriteBool("bHideLockedIndicators", ItemCardHook::hideLockedIndicators, iniPath);
    WriteBool("bShowLockedMaterials", ItemCardHook::showLockedMaterials, iniPath);
    WriteBool("bFilterIsBlacklist", ItemCardHook::filterIsBlacklist, iniPath);

    WriteString("sIndicatorPrefix", ItemCardHook::indicatorPrefix, iniPath);
    WriteString("sIndicatorSuffix", ItemCardHook::indicatorSuffix, iniPath);
    WriteString("sIndicatorSmelt", ItemCardHook::indicatorSmelt, iniPath);
    WriteString("sIndicatorTemper", ItemCardHook::indicatorTemper, iniPath);
    WriteString("sIndicatorSmeltLocked", ItemCardHook::indicatorSmeltLocked, iniPath);
    WriteString("sIndicatorTemperLocked", ItemCardHook::indicatorTemperLocked, iniPath);
    WriteString("sFilterItems", ItemCardHook::filterItemsRaw, iniPath);

    char perkBuf[16] = {};
    snprintf(perkBuf, sizeof(perkBuf), "%08X", ItemCardHook::arcaneBlacksmithPerkID);
    WriteString("sArcaneBlacksmithPerkFormID", perkBuf, iniPath);

    logger::info("SMF: settings saved to {}", iniPath);
}
