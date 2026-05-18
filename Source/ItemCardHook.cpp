#include "ItemCardHook.h"
#include "COBJCache.h"

namespace {

std::string FormatCountAndName(int a_count, const char* a_name) {
    if (a_count == 1)
        return "1 "s + a_name;
    return std::to_string(a_count) + " "s + a_name;
}

static bool PlayerHasArcaneBlacksmith() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return false;
    auto* perk = RE::TESForm::LookupByID<RE::BGSPerk>(ItemCardHook::arcaneBlacksmithPerkID);
    if (!perk) return false;
    return player->HasPerk(perk);
}

// Returns the FormID to use for cache lookup.
// For enchanted items, falls back to templateWeapon/templateArmor if no direct match.
RE::FormID GetEffectiveFormID(RE::FormID a_formID, bool a_isEnchanted) {
    if (!a_isEnchanted) return a_formID;

    auto* form = RE::TESForm::LookupByID(a_formID);
    if (auto* weapon = form ? form->As<RE::TESObjectWEAP>() : nullptr) {
        if (weapon->templateWeapon) return weapon->templateWeapon->formID;
    } else if (auto* armor = form ? form->As<RE::TESObjectARMO>() : nullptr) {
        if (armor->templateArmor) return armor->templateArmor->formID;
    }
    return a_formID;
}

bool ShouldShowTemper(bool a_isEnchanted) {
    if (!a_isEnchanted) return true;
    if (ItemCardHook::gateEnchantedTempering && !PlayerHasArcaneBlacksmith()) return false;
    return true;
}

std::string GetNameIndicator(RE::FormID a_formID, bool a_isEnchanted) {
    auto& cache = COBJCache::GetSingleton();
    bool hasSmelt = false;
    bool hasTemper = false;

    if (!a_isEnchanted) {
        auto* smeltResult = cache.GetSmeltResult(a_formID);
        hasSmelt = (smeltResult && smeltResult->outputItem);
    }

    if (ShouldShowTemper(a_isEnchanted)) {
        auto lookupID = GetEffectiveFormID(a_formID, a_isEnchanted);
        const auto* temperMats = cache.GetTemperMaterials(lookupID);
        hasTemper = (temperMats && !temperMats->empty());
    }

    auto& prefix = ItemCardHook::indicatorPrefix;
    auto& suffix = ItemCardHook::indicatorSuffix;
    auto& s = ItemCardHook::indicatorSmelt;
    auto& t = ItemCardHook::indicatorTemper;

    if (hasSmelt && hasTemper) return " "s + prefix + t + s + suffix;
    if (hasTemper) return " "s + prefix + t + suffix;
    if (hasSmelt) return " "s + prefix + s + suffix;
    return "";
}

void UpdateListNames(RE::ItemList* a_itemList, RE::GFxMovieView* a_movie) {
    if (!a_itemList || !a_movie) return;

    for (auto* entry : a_itemList->items) {
        if (!entry || !entry->data.objDesc || !entry->data.objDesc->object) continue;

        auto* objDesc = entry->data.objDesc;
        bool isEnchanted = objDesc->IsEnchanted();
        auto formID = entry->data.objDesc->object->formID;

        auto indicator = GetNameIndicator(formID, isEnchanted);
        if (indicator.empty()) continue;

        RE::GFxValue textVal;
        if (entry->obj.GetMember("text", &textVal) && textVal.IsString()) {
            std::string currentText = textVal.GetString();
            if (currentText.find(indicator) == std::string::npos) {
                RE::GFxValue newTextVal;
                a_movie->CreateString(&newTextVal, (currentText + indicator).c_str());
                entry->obj.SetMember("text", newTextVal);
            }
        }
    }
}

std::string BuildSmithyText(RE::FormID a_formID, bool a_isEnchanted) {
    auto& cache = COBJCache::GetSingleton();
    std::string text;

    if (!a_isEnchanted) {
        auto* smeltResult = cache.GetSmeltResult(a_formID);
        if (smeltResult && smeltResult->outputItem) {
            auto* name = smeltResult->outputItem->GetName();
            if (name && *name) {
                text += "Smelts into: " + FormatCountAndName(smeltResult->outputCount, name);
            }
        }
    }

    if (ShouldShowTemper(a_isEnchanted)) {
        auto lookupID = GetEffectiveFormID(a_formID, a_isEnchanted);
        const auto* temperMats = cache.GetTemperMaterials(lookupID);
        if (temperMats && !temperMats->empty()) {
            if (!text.empty()) text += "<br>";
            text += "Tempers with: ";
            bool first = true;
            for (auto& ing : *temperMats) {
                if (!ing.item) continue;
                auto* name = ing.item->GetName();
                if (!name || !*name) continue;
                if (!first) text += ", ";
                text += FormatCountAndName(ing.count, name);
                first = false;
            }
        }
    }

    return text;
}

}

RE::FormID ItemCardHook::_lastFormID = 0;

void ItemCardHook::Install() {
    {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_InventoryMenu[0]);
        _InvAdvanceMovie = vTable.write_vfunc(0x5, &InvAdvanceMovie_Hook);
        logger::info("ItemCardHook: vtable hook installed on InventoryMenu::AdvanceMovie (index 0x5)");
    }
    {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_ContainerMenu[0]);
        _ContAdvanceMovie = vTable.write_vfunc(0x5, &ContAdvanceMovie_Hook);
        logger::info("ItemCardHook: vtable hook installed on ContainerMenu::AdvanceMovie (index 0x5)");
    }
    {
        REL::Relocation<std::uintptr_t> vTable(RE::VTABLE_BarterMenu[0]);
        _BarterAdvanceMovie = vTable.write_vfunc(0x5, &BarterAdvanceMovie_Hook);
        logger::info("ItemCardHook: vtable hook installed on BarterMenu::AdvanceMovie (index 0x5)");
    }
}

void ItemCardHook::InvAdvanceMovie_Hook(
    RE::IMenu* a_this,
    float a_interval,
    std::uint32_t a_currentTime)
{
    _InvAdvanceMovie(a_this, a_interval, a_currentTime);

    auto* menu = static_cast<RE::InventoryMenu*>(a_this);
    auto& rt = menu->GetRuntimeData();
    if (ItemCardHook::indicatorPlayer) {
        UpdateListNames(rt.itemList, menu->uiMovie.get());
    }
    if (ItemCardHook::effectsPlayer) {
        InjectItemCardText(menu->uiMovie.get(), rt.root, rt.itemCard, rt.itemList);
    }
}

void ItemCardHook::ContAdvanceMovie_Hook(
    RE::IMenu* a_this,
    float a_interval,
    std::uint32_t a_currentTime)
{
    _ContAdvanceMovie(a_this, a_interval, a_currentTime);

    auto* menu = static_cast<RE::ContainerMenu*>(a_this);
    auto& rt = menu->GetRuntimeData();
    if (ItemCardHook::indicatorContainer) {
        UpdateListNames(rt.itemList, menu->uiMovie.get());
    }
    if (ItemCardHook::effectsContainer) {
        InjectItemCardText(menu->uiMovie.get(), rt.root, rt.itemCard, rt.itemList);
    }
}

void ItemCardHook::BarterAdvanceMovie_Hook(
    RE::IMenu* a_this,
    float a_interval,
    std::uint32_t a_currentTime)
{
    _BarterAdvanceMovie(a_this, a_interval, a_currentTime);

    auto* menu = static_cast<RE::BarterMenu*>(a_this);
    auto& rt = menu->GetRuntimeData();
    if (ItemCardHook::indicatorMerchant) {
        UpdateListNames(rt.itemList, menu->uiMovie.get());
    }
    if (ItemCardHook::effectsMerchant) {
        InjectItemCardText(menu->uiMovie.get(), rt.root, rt.itemCard, rt.itemList);
    }
}

void ItemCardHook::InjectItemCardText(
    RE::GFxMovieView* a_movie,
    RE::GFxValue& a_root,
    RE::ItemCard* a_itemCard,
    RE::ItemList* a_itemList)
{
    if (!a_movie || !a_itemList || !a_itemCard) return;

    auto* selectedItem = a_itemList->GetSelectedItem();
    if (!selectedItem || !selectedItem->data.objDesc || !selectedItem->data.objDesc->object) return;

    auto formID = selectedItem->data.objDesc->object->formID;
    auto isNewSelection = (formID != _lastFormID);

    auto* objDesc = selectedItem->data.objDesc;
    bool isEnchanted = objDesc->IsEnchanted();

    if (isNewSelection) {
        logger::debug("SmithyInfo: form {:08X}, enchanted={}, gateEnabled={}", formID, isEnchanted, ItemCardHook::gateEnchantedTempering);
    }

    auto& cardObj = a_itemCard->obj;
    if (!cardObj.IsObject()) return;

    RE::GFxValue effectsVal;
    bool hasEffects = cardObj.GetMember("effects", &effectsVal);

    std::string currentEffects;
    if (hasEffects && effectsVal.IsString()) {
        currentEffects = effectsVal.GetString();
    }

    if (currentEffects.find("Tempers with:") != std::string::npos ||
        currentEffects.find("Smelts into:") != std::string::npos) {
        _lastFormID = formID;
        return;
    }

    auto smithyText = BuildSmithyText(formID, isEnchanted);
    if (smithyText.empty()) {
        if (isNewSelection) {
            logger::debug("SmithyInfo: no smithy text for {:08X}", formID);
        }
        _lastFormID = formID;
        return;
    }

    std::string newEffects;
    if (!currentEffects.empty()) {
        newEffects = currentEffects + "<br>" + smithyText;
    } else {
        newEffects = smithyText;
    }

    RE::GFxValue newEffectsVal;
    a_movie->CreateString(&newEffectsVal, newEffects.c_str());
    cardObj.SetMember("effects", newEffectsVal);

    RE::GFxValue args[]{cardObj};
    a_root.Invoke("UpdateItemCardInfo", nullptr, args, 1);

    if (isNewSelection) {
        logger::debug("SmithyInfo: injected text for {:08X}: '{}'", formID, smithyText);
    }
    _lastFormID = formID;
}
