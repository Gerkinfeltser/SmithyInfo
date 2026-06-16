#include "COBJCache.h"
#include "DIIIIntegration.h"
#include "ItemCardHook.h"
#include "SmithyInfoMenu.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <windows.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

static spdlog::level::level_enum ParseLogLevel(const char* a_str) {
    if (_stricmp(a_str, "trace") == 0) return spdlog::level::trace;
    if (_stricmp(a_str, "debug") == 0) return spdlog::level::debug;
    if (_stricmp(a_str, "info") == 0) return spdlog::level::info;
    if (_stricmp(a_str, "warn") == 0) return spdlog::level::warn;
    if (_stricmp(a_str, "error") == 0) return spdlog::level::err;
    if (_stricmp(a_str, "critical") == 0) return spdlog::level::critical;
    if (_stricmp(a_str, "off") == 0) return spdlog::level::off;
    return spdlog::level::info;
}

static std::string GetINIPath() {
    char dllPath[MAX_PATH] = {};
    GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), dllPath, MAX_PATH);
    std::filesystem::path p(dllPath);
    return (p.parent_path() / "SmithyInfo.ini").string();
}

static std::string iniPath;

static void OnDataLoaded(SKSE::MessagingInterface::Message* a_msg) {
    if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
        COBJCache::GetSingleton().Build();
        SmithyInfoMenu::Register(iniPath);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);

    iniPath = GetINIPath();
    char levelBuf[32] = {};
    GetPrivateProfileStringA("SmithyInfo", "sLogLevel", "info", levelBuf, 32, iniPath.c_str());
    auto logLevel = ParseLogLevel(levelBuf);

    auto logPath = SKSE::log::log_directory();
    if (logPath) {
        auto fullPath = *logPath / "SmithyInfo.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fullPath.string(), true);
        auto logger = std::make_shared<spdlog::logger>("SmithyInfo", sink);
        logger->set_pattern("[%H:%M:%S.%e] [%l] %v");
        logger->set_level(logLevel);
        logger->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(logger));
    }

    logger::info("SmithyInfo loading");
    logger::info("INI path: {}", iniPath);
    logger::info("INI: sLogLevel = {} (mapped to spdlog level {})", levelBuf, static_cast<int>(logLevel));

    ItemCardHook::gateEnchantedTempering = GetPrivateProfileIntA("SmithyInfo", "bGateEnchantedTempering", 1, iniPath.c_str()) != 0;
    logger::info("INI: bGateEnchantedTempering = {}", ItemCardHook::gateEnchantedTempering);

    ItemCardHook::effectsPlayer = GetPrivateProfileIntA("SmithyInfo", "bEffectsPlayer", 1, iniPath.c_str()) != 0;
    ItemCardHook::effectsContainer = GetPrivateProfileIntA("SmithyInfo", "bEffectsContainer", 1, iniPath.c_str()) != 0;
    ItemCardHook::effectsMerchant = GetPrivateProfileIntA("SmithyInfo", "bEffectsMerchant", 1, iniPath.c_str()) != 0;
    logger::info("INI: effects player={}, container={}, merchant={}",
        ItemCardHook::effectsPlayer, ItemCardHook::effectsContainer, ItemCardHook::effectsMerchant);

    ItemCardHook::indicatorPlayer = GetPrivateProfileIntA("SmithyInfo", "bIndicatorPlayer", 1, iniPath.c_str()) != 0;
    ItemCardHook::indicatorContainer = GetPrivateProfileIntA("SmithyInfo", "bIndicatorContainer", 1, iniPath.c_str()) != 0;
    ItemCardHook::indicatorMerchant = GetPrivateProfileIntA("SmithyInfo", "bIndicatorMerchant", 1, iniPath.c_str()) != 0;
    logger::info("INI: indicators player={}, container={}, merchant={}",
        ItemCardHook::indicatorPlayer, ItemCardHook::indicatorContainer, ItemCardHook::indicatorMerchant);

    char buf[16] = {};
    GetPrivateProfileStringA("SmithyInfo", "sArcaneBlacksmithPerkFormID", "0005218E", buf, 16, iniPath.c_str());
    try {
        ItemCardHook::arcaneBlacksmithPerkID = static_cast<RE::FormID>(std::stoul(buf, nullptr, 16));
    } catch (...) {
        logger::warn("INI: invalid sArcaneBlacksmithPerkFormID '{}', using default 0005218E", buf);
    }
    logger::info("INI: sArcaneBlacksmithPerkFormID = {:08X}", ItemCardHook::arcaneBlacksmithPerkID);

    char indicatorBuf[32] = {};
    GetPrivateProfileStringA("SmithyInfo", "sIndicatorPrefix", "|", indicatorBuf, 32, iniPath.c_str());
    ItemCardHook::indicatorPrefix = indicatorBuf;

    char suffixBuf[32] = {};
    GetPrivateProfileStringA("SmithyInfo", "sIndicatorSuffix", "|", suffixBuf, 32, iniPath.c_str());
    ItemCardHook::indicatorSuffix = suffixBuf;

    char smeltBuf[16] = {};
    GetPrivateProfileStringA("SmithyInfo", "sIndicatorSmelt", "s", smeltBuf, 16, iniPath.c_str());
    ItemCardHook::indicatorSmelt = smeltBuf;

    char temperBuf[16] = {};
    GetPrivateProfileStringA("SmithyInfo", "sIndicatorTemper", "t", temperBuf, 16, iniPath.c_str());
    ItemCardHook::indicatorTemper = temperBuf;

    char smeltLockedBuf[16] = {};
    GetPrivateProfileStringA("SmithyInfo", "sIndicatorSmeltLocked", "s?", smeltLockedBuf, 16, iniPath.c_str());
    ItemCardHook::indicatorSmeltLocked = smeltLockedBuf;

    char temperLockedBuf[16] = {};
    GetPrivateProfileStringA("SmithyInfo", "sIndicatorTemperLocked", "t?", temperLockedBuf, 16, iniPath.c_str());
    ItemCardHook::indicatorTemperLocked = temperLockedBuf;
    logger::info("INI: indicator format = '{}{}{}{}' (locked: '{}{}')",
        ItemCardHook::indicatorPrefix, ItemCardHook::indicatorTemper, ItemCardHook::indicatorSmelt, ItemCardHook::indicatorSuffix,
        ItemCardHook::indicatorTemperLocked, ItemCardHook::indicatorSmeltLocked);

    DIIIIntegration::enabled = GetPrivateProfileIntA("SmithyInfo", "bDIIIIntegration", 1, iniPath.c_str()) != 0;
    logger::info("INI: bDIIIIntegration = {}", DIIIIntegration::enabled);

    ItemCardHook::hideLockedIndicators = GetPrivateProfileIntA("SmithyInfo", "bHideLockedIndicators", 0, iniPath.c_str()) != 0;
    logger::info("INI: bHideLockedIndicators = {}", ItemCardHook::hideLockedIndicators);

    ItemCardHook::showLockedMaterials = GetPrivateProfileIntA("SmithyInfo", "bShowLockedMaterials", 1, iniPath.c_str()) != 0;
    logger::info("INI: bShowLockedMaterials = {}", ItemCardHook::showLockedMaterials);

    ItemCardHook::filterIsBlacklist = GetPrivateProfileIntA("SmithyInfo", "bFilterIsBlacklist", 1, iniPath.c_str()) != 0;
    logger::info("INI: bFilterIsBlacklist = {}", ItemCardHook::filterIsBlacklist);

    char filterBuf[1024] = {};
    GetPrivateProfileStringA("SmithyInfo", "sFilterItems", "", filterBuf, 1024, iniPath.c_str());
    ItemCardHook::SetFilterItemsFromString(filterBuf);
    logger::info("INI: sFilterItems = {} entries (blacklist={})", ItemCardHook::filterItems.size(), ItemCardHook::filterIsBlacklist);

    SmithyInfoMenu::smfEnabled = GetPrivateProfileIntA("SmithyInfo", "bSMFIntegration", 1, iniPath.c_str()) != 0;
    logger::info("INI: bSMFIntegration = {}", SmithyInfoMenu::smfEnabled);

    DIIIIntegration::Install();
    ItemCardHook::Install();

    SKSE::GetMessagingInterface()->RegisterListener(OnDataLoaded);

    return true;
}
