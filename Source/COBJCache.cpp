#include "COBJCache.h"

void COBJCache::Build() {
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        logger::error("COBJCache: TESDataHandler is null");
        return;
    }

    auto& cobjArray = dataHandler->GetFormArray<RE::BGSConstructibleObject>();
    std::uint32_t smeltCount = 0;
    std::uint32_t temperCount = 0;

    for (auto* cobj : cobjArray) {
        if (!cobj || !cobj->benchKeyword) continue;

        auto* keywordEdid = cobj->benchKeyword->GetFormEditorID();
        if (!keywordEdid || !*keywordEdid) continue;

        bool isSmelter = (strcmp(keywordEdid, "CraftingSmelter") == 0);
        bool isGrindstone = (strcmp(keywordEdid, "CraftingSmithingSharpeningWheel") == 0);
        bool isArmorer = (strcmp(keywordEdid, "CraftingSmithingArmorTable") == 0);

        if (isSmelter) {
            if (!cobj->createdItem) continue;
            auto* created = cobj->createdItem->As<RE::TESBoundObject>();
            if (!created) continue;

            cobj->requiredItems.ForEachContainerObject([&](RE::ContainerObject& a_co) {
                if (!a_co.obj) return RE::BSContainer::ForEachResult::kContinue;
                SmeltResult result;
                result.outputItem = created;
                result.outputCount = cobj->data.numConstructed;
                _smeltMap[a_co.obj->formID] = result;
                return RE::BSContainer::ForEachResult::kContinue;
            });
            smeltCount++;
        } else if (isGrindstone || isArmorer) {
            if (!cobj->createdItem) continue;
            auto* created = cobj->createdItem->As<RE::TESBoundObject>();
            if (!created) continue;

            bool isWeapon = created->Is(RE::FormType::Weapon);
            bool isArmor = created->Is(RE::FormType::Armor);
            if (!isWeapon && !isArmor) continue;

            std::vector<IngredientInfo> ingredients;
            cobj->requiredItems.ForEachContainerObject([&](RE::ContainerObject& a_co) {
                if (a_co.obj) {
                    ingredients.push_back({a_co.obj, a_co.count});
                }
                return RE::BSContainer::ForEachResult::kContinue;
            });

            if (!ingredients.empty()) {
                _temperMap[created->formID] = std::move(ingredients);
                temperCount++;
            }
        }
    }

    logger::info("COBJCache built: {} smelting entries, {} tempering entries",
                 smeltCount, temperCount);
}

const SmeltResult* COBJCache::GetSmeltResult(RE::FormID a_formID) const {
    auto it = _smeltMap.find(a_formID);
    return (it != _smeltMap.end()) ? &it->second : nullptr;
}

const std::vector<IngredientInfo>* COBJCache::GetTemperMaterials(RE::FormID a_formID) const {
    auto it = _temperMap.find(a_formID);
    return (it != _temperMap.end()) ? &it->second : nullptr;
}
