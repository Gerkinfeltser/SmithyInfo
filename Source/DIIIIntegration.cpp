#include "DIIIIntegration.h"
#include "COBJCache.h"
#include "DIII_API.h"

#include <json/json.h>
#include <SKSE/SKSE.h>

namespace logger = SKSE::log;

class SmithySmeltCondition : public DIII::ICondition {
public:
    bool Match(RE::InventoryEntryData* entry) const override {
        if (!entry || !entry->object) return false;
        return COBJCache::GetSingleton().GetSmeltResult(entry->object->GetFormID()) != nullptr;
    }
};

class SmithyTemperCondition : public DIII::ICondition {
public:
    bool Match(RE::InventoryEntryData* entry) const override {
        if (!entry || !entry->object) return false;
        return COBJCache::GetSingleton().GetTemperMaterials(entry->object->GetFormID()) != nullptr;
    }
};

static void OnDIIIMessage(SKSE::MessagingInterface::Message* a_msg) {
    if (a_msg->type == DIII::kMessage_GetAPI && a_msg->data) {
        auto* api = static_cast<DIII::IAPI*>(a_msg->data);

        api->RegisterCondition("smithySmelt", [](const Json::Value&, RE::FormType) -> std::unique_ptr<DIII::ICondition> {
            return std::make_unique<SmithySmeltCondition>();
        });

        api->RegisterCondition("smithyTemper", [](const Json::Value&, RE::FormType) -> std::unique_ptr<DIII::ICondition> {
            return std::make_unique<SmithyTemperCondition>();
        });

        logger::info("DIII: registered smithySmelt and smithyTemper conditions");
    }
}

void DIIIIntegration::Install() {
    if (!enabled) {
        logger::info("DIII integration disabled by INI");
        return;
    }

    SKSE::GetMessagingInterface()->RegisterListener("DynamicInventoryIconInjector", OnDIIIMessage);
    logger::info("DIII: listening for registration");
}
