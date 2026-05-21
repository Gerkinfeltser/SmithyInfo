#include "COBJCache.h"
#include <algorithm>
#include <tuple>

namespace {

// Evaluate COBJ conditions but skip GetItemCount (func 47) checks.
// GetItemCount fails when the item isn't in inventory, but we want to
// know if the recipe is available to the player (perks, quests, skills),
// not whether they're currently holding the raw materials.
bool EvaluateConditionsSkipItemCount(const RE::TESCondition& a_conditions, RE::TESObjectREFR* a_player) {
    if (!a_conditions.head) return true;

    RE::ConditionCheckParams params(a_player, a_player);
    bool hasOR = false;

    for (auto* item = a_conditions.head; item; item = item->next) {
        auto funcId = item->data.functionData.function.underlying();
        if (funcId == 47) { // GetItemCount — skip
            continue;
        }

        bool passes = item->IsTrue(params);

        if (item->data.flags.isOR) {
            if (passes) return true;
            hasOR = true;
        } else {
            if (!passes) return false;
            if (hasOR) hasOR = false;
        }
    }

    return !hasOR;
}

}

void COBJCache::Build() {
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        logger::error("COBJCache: TESDataHandler is null");
        return;
    }

    auto& cobjArray = dataHandler->GetFormArray<RE::BGSConstructibleObject>();
    std::uint32_t smeltCount = 0;
    std::uint32_t temperCount = 0;
    std::vector<std::tuple<std::string, std::uint32_t, std::string>> smelterContributors;

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

            std::uint32_t ingredientCount = 0;
            cobj->requiredItems.ForEachContainerObject([&](RE::ContainerObject& a_co) {
                if (!a_co.obj) return RE::BSContainer::ForEachResult::kContinue;
                SmeltResult result;
                result.outputItem = created;
                result.outputCount = cobj->data.numConstructed;
                result.cobj = cobj;
                _smeltMap[a_co.obj->formID] = result;
                ingredientCount++;
                return RE::BSContainer::ForEachResult::kContinue;
            });
            if (ingredientCount >= 2) {
                auto* edid = cobj->GetFormEditorID();
                auto* outName = created->GetName();
                smelterContributors.push_back({edid ? edid : "(no EDID)", ingredientCount, outName ? outName : "(no name)"});
            }
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
                TemperEntry entry;
                entry.ingredients = std::move(ingredients);
                entry.cobj = cobj;
                _temperMap[created->formID] = std::move(entry);
                temperCount++;
            }
        }
    }

    logger::info("COBJCache built: {} smelting entries, {} tempering entries",
                 smeltCount, temperCount);

    logger::debug("COBJCache: {} smelter COBJs with 2+ ingredients, showing top 20", smelterContributors.size());

    std::sort(smelterContributors.begin(), smelterContributors.end(),
        [](const auto& a, const auto& b) { return std::get<1>(a) > std::get<1>(b); });
    std::uint32_t shown = 0;
    for (auto& [name, cnt, output] : smelterContributors) {
        if (shown >= 20) break;
        logger::debug("COBJCache: smelter '{}' → {} ({} ingredients)", name, output, cnt);
        shown++;
    }
}

const SmeltResult* COBJCache::GetSmeltResult(RE::FormID a_formID) const {
    auto it = _smeltMap.find(a_formID);
    return (it != _smeltMap.end()) ? &it->second : nullptr;
}

const TemperEntry* COBJCache::GetTemperEntry(RE::FormID a_formID) const {
    auto it = _temperMap.find(a_formID);
    return (it != _temperMap.end()) ? &it->second : nullptr;
}

bool COBJCache::IsSmeltAvailable(RE::FormID a_formID) const {
    auto* result = GetSmeltResult(a_formID);
    if (!result || !result->outputItem || !result->cobj) return false;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return false;

    bool available = EvaluateConditionsSkipItemCount(result->cobj->conditions, player);
    return available;
}

bool COBJCache::IsTemperAvailable(RE::FormID a_formID, bool a_isEnchanted, RE::FormID a_templateFormID) const {
    RE::FormID lookupID = a_formID;
    if (a_isEnchanted && a_templateFormID != 0) {
        lookupID = a_templateFormID;
    }

    auto* entry = GetTemperEntry(lookupID);
    if (!entry || entry->ingredients.empty() || !entry->cobj) return false;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return false;

    bool available = EvaluateConditionsSkipItemCount(entry->cobj->conditions, player);
    return available;
}
