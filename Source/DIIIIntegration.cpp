#include "DIIIIntegration.h"
#include "COBJCache.h"
#include "DIII_API.h"
#include "ItemCardHook.h"

#include <json/json.h>
#include <SKSE/SKSE.h>

namespace logger = SKSE::log;

class SmithySmeltCondition : public DIII::ICondition {
public:
    bool Match(RE::InventoryEntryData* entry) const override {
        if (!DIIIIntegration::enabled) return false;
        if (!ItemCardHook::IsIndicatorEnabled()) return false;
        if (!entry || !entry->object) return false;
        return COBJCache::GetSingleton().IsSmeltAvailable(entry->object->GetFormID());
    }
};

class SmithySmeltLockedCondition : public DIII::ICondition {
public:
    bool Match(RE::InventoryEntryData* entry) const override {
        if (!DIIIIntegration::enabled) return false;
        if (!ItemCardHook::IsIndicatorEnabled()) return false;
        if (ItemCardHook::hideLockedIndicators) return false;
        if (!entry || !entry->object) return false;
        auto& cache = COBJCache::GetSingleton();
        auto* result = cache.GetSmeltResult(entry->object->GetFormID());
        bool hasRecipe = (result && result->outputItem);
        bool available = hasRecipe && cache.IsSmeltAvailable(entry->object->GetFormID());
        return hasRecipe && !available;
    }
};

class SmithyTemperCondition : public DIII::ICondition {
public:
    bool Match(RE::InventoryEntryData* entry) const override {
        if (!DIIIIntegration::enabled) return false;
        if (!ItemCardHook::IsIndicatorEnabled()) return false;
        if (!entry || !entry->object) return false;
        auto& cache = COBJCache::GetSingleton();
        bool isEnchanted = entry->IsEnchanted();
        if (isEnchanted && ItemCardHook::gateEnchantedTempering && !ItemCardHook::HasArcaneBlacksmith()) {
            return false;
        }

        RE::FormID templateID = ItemCardHook::GetTemplateFormID(entry->object->GetFormID(), isEnchanted);

        if (ItemCardHook::IsItemTempered(entry)) return false;

        return cache.IsTemperAvailable(entry->object->GetFormID(), isEnchanted, templateID);
    }
};

class SmithyTemperLockedCondition : public DIII::ICondition {
public:
    bool Match(RE::InventoryEntryData* entry) const override {
        if (!DIIIIntegration::enabled) return false;
        if (!ItemCardHook::IsIndicatorEnabled()) return false;
        if (ItemCardHook::hideLockedIndicators) return false;
        if (!entry || !entry->object) return false;

        auto& cache = COBJCache::GetSingleton();
        bool isEnchanted = entry->IsEnchanted();
        RE::FormID templateID = ItemCardHook::GetTemplateFormID(entry->object->GetFormID(), isEnchanted);

        RE::FormID lookupID = (templateID != 0) ? templateID : entry->object->GetFormID();
        auto* temperEntry = cache.GetTemperEntry(lookupID);
        bool hasRecipe = (temperEntry && !temperEntry->ingredients.empty());
        if (!hasRecipe) return false;

        if (isEnchanted && ItemCardHook::gateEnchantedTempering && !ItemCardHook::HasArcaneBlacksmith()) {
            return true;
        }

        if (ItemCardHook::IsItemTempered(entry)) return true;

        bool available = cache.IsTemperAvailable(entry->object->GetFormID(), isEnchanted, templateID);
        return !available;
    }
};

static void OnDIIIMessage(SKSE::MessagingInterface::Message* a_msg) {
    if (a_msg->type == DIII::kMessage_GetAPI && a_msg->data) {
        auto* api = static_cast<DIII::IAPI*>(a_msg->data);

        bool smeltOk = api->RegisterCondition("smithySmelt", [](const Json::Value&, RE::FormType) -> std::unique_ptr<DIII::ICondition> {
            return std::make_unique<SmithySmeltCondition>();
        });

        bool smeltLockedOk = api->RegisterCondition("smithySmeltLocked", [](const Json::Value&, RE::FormType) -> std::unique_ptr<DIII::ICondition> {
            return std::make_unique<SmithySmeltLockedCondition>();
        });

        bool temperOk = api->RegisterCondition("smithyTemper", [](const Json::Value&, RE::FormType) -> std::unique_ptr<DIII::ICondition> {
            return std::make_unique<SmithyTemperCondition>();
        });

        bool temperLockedOk = api->RegisterCondition("smithyTemperLocked", [](const Json::Value&, RE::FormType) -> std::unique_ptr<DIII::ICondition> {
            return std::make_unique<SmithyTemperLockedCondition>();
        });

        if (smeltOk && smeltLockedOk && temperOk && temperLockedOk) {
            DIIIIntegration::conditionsRegistered = true;
            logger::info("DIII: registered smithySmelt, smithySmeltLocked, smithyTemper, smithyTemperLocked conditions");
        } else {
            logger::warn("DIII: condition registration failed (smelt={}, smeltLocked={}, temper={}, temperLocked={})",
                smeltOk, smeltLockedOk, temperOk, temperLockedOk);
        }
    }
}

void DIIIIntegration::Install() {
    SKSE::GetMessagingInterface()->RegisterListener("DynamicInventoryIconInjector", OnDIIIMessage);
    logger::info("DIII: listening for registration (enabled={})", enabled);
}
