#include "ItemCardHook.h"
#include "COBJCache.h"
#include "DIIIIntegration.h"

#include <charconv>
#include <cctype>
#include <system_error>

namespace {

std::string FormatCountAndName(int a_count, const char* a_name) {
    if (a_count == 1)
        return "1 "s + a_name;
    return std::to_string(a_count) + " "s + a_name;
}

std::string_view Trim(std::string_view a_value) {
    while (!a_value.empty() && std::isspace(static_cast<unsigned char>(a_value.front()))) {
        a_value.remove_prefix(1);
    }
    while (!a_value.empty() && std::isspace(static_cast<unsigned char>(a_value.back()))) {
        a_value.remove_suffix(1);
    }
    return a_value;
}

}

bool ItemCardHook::IsIndicatorEnabled() {
    switch (activeMenu) {
    case ActiveMenu::Player:    return indicatorPlayer;
    case ActiveMenu::Container: return indicatorContainer;
    case ActiveMenu::Merchant:  return indicatorMerchant;
    default:                    return false;
    }
}

bool ItemCardHook::HasArcaneBlacksmith() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return false;
    auto* perk = RE::TESForm::LookupByID<RE::BGSPerk>(arcaneBlacksmithPerkID);
    if (!perk) return false;
    return player->HasPerk(perk);
}

RE::FormID ItemCardHook::GetTemplateFormID(RE::FormID a_formID, bool a_isEnchanted) {
    if (!a_isEnchanted) return 0;
    auto* form = RE::TESForm::LookupByID(a_formID);
    if (auto* weapon = form ? form->As<RE::TESObjectWEAP>() : nullptr) {
        if (weapon->templateWeapon) return weapon->templateWeapon->formID;
    } else if (auto* armor = form ? form->As<RE::TESObjectARMO>() : nullptr) {
        if (armor->templateArmor) return armor->templateArmor->formID;
    }
    return 0;
}

bool ItemCardHook::IsItemTempered(RE::InventoryEntryData* entry) {
    if (!entry || !entry->extraLists) return false;
    for (const auto& xList : *entry->extraLists) {
        if (!xList) continue;
        auto* textData = xList->GetByType<RE::ExtraTextDisplayData>();
        if (textData && textData->temperFactor > 0.0f) return true;
    }
    return false;
}

bool ItemCardHook::IsItemFiltered(RE::FormID a_formID) {
    if (filterItems.empty()) return false;
    bool inList = filterItems.contains(a_formID);
    return filterIsBlacklist ? inList : !inList;
}

void ItemCardHook::SetFilterItemsFromString(std::string_view a_rawItems) {
    filterItemsRaw = std::string(a_rawItems);
    filterItems.clear();

    while (!a_rawItems.empty()) {
        auto comma = a_rawItems.find(',');
        auto token = Trim(a_rawItems.substr(0, comma));
        if (!token.empty()) {
            std::uint32_t formID = 0;
            auto* begin = token.data();
            auto* end = token.data() + token.size();
            auto result = std::from_chars(begin, end, formID, 16);
            if (result.ec == std::errc{} && result.ptr == end) {
                filterItems.insert(static_cast<RE::FormID>(formID));
                logger::debug("INI: filter item {:08X}", formID);
            } else {
                logger::warn("INI: invalid sFilterItems entry '{}'", std::string(token));
            }
        }

        if (comma == std::string_view::npos) break;
        a_rawItems.remove_prefix(comma + 1);
    }
}

namespace {

bool ShouldShowTemper(bool a_isEnchanted) {
    if (!a_isEnchanted) return true;
    if (ItemCardHook::gateEnchantedTempering && !ItemCardHook::HasArcaneBlacksmith()) return false;
    return true;
}

std::string GetNameIndicator(RE::FormID a_formID, bool a_isEnchanted, bool a_isTempered) {
    if (ItemCardHook::IsItemFiltered(a_formID)) return "";

    auto& cache = COBJCache::GetSingleton();
    auto& prefix = ItemCardHook::indicatorPrefix;
    auto& suffix = ItemCardHook::indicatorSuffix;
    auto& s = ItemCardHook::indicatorSmelt;
    auto& t = ItemCardHook::indicatorTemper;
    auto& sLocked = ItemCardHook::indicatorSmeltLocked;
    auto& tLocked = ItemCardHook::indicatorTemperLocked;

    bool hasSmelt = false;
    bool smeltAvailable = false;
    bool hasTemper = false;
    bool temperAvailable = false;

    if (!a_isEnchanted) {
        auto* smeltResult = cache.GetSmeltResult(a_formID);
        hasSmelt = (smeltResult && smeltResult->outputItem);
        if (hasSmelt) {
            smeltAvailable = cache.IsSmeltAvailable(a_formID);
        }
    }

    if (ShouldShowTemper(a_isEnchanted)) {
        auto templateID = ItemCardHook::GetTemplateFormID(a_formID, a_isEnchanted);
        auto lookupID = (templateID != 0) ? templateID : a_formID;
        auto* temperEntry = cache.GetTemperEntry(lookupID);
        hasTemper = (temperEntry && !temperEntry->ingredients.empty());
        if (hasTemper) {
            temperAvailable = cache.IsTemperAvailable(a_formID, a_isEnchanted, templateID);
            if (a_isTempered) temperAvailable = false;
        }
    }

    std::string parts;

    if (hasTemper && hasSmelt) {
        bool showTemper = temperAvailable || !ItemCardHook::hideLockedIndicators;
        bool showSmelt = smeltAvailable || !ItemCardHook::hideLockedIndicators;
        if (showTemper || showSmelt) {
            parts = " "s + prefix;
            if (showTemper) parts += (temperAvailable ? t : tLocked);
            if (showSmelt) parts += (smeltAvailable ? s : sLocked);
            parts += suffix;
        }
    } else if (hasTemper) {
        if (temperAvailable) {
            parts = " "s + prefix + t + suffix;
        } else if (!ItemCardHook::hideLockedIndicators) {
            parts = " "s + prefix + tLocked + suffix;
        }
    } else if (hasSmelt) {
        if (smeltAvailable) {
            parts = " "s + prefix + s + suffix;
        } else if (!ItemCardHook::hideLockedIndicators) {
            parts = " "s + prefix + sLocked + suffix;
        }
    }

    return parts;
}

void UpdateListNames(RE::ItemList* a_itemList, RE::GFxMovieView* a_movie) {
    if (!a_itemList || !a_movie) return;

    if (DIIIIntegration::enabled && DIIIIntegration::conditionsRegistered) return;

    for (auto* entry : a_itemList->items) {
        if (!entry || !entry->data.objDesc || !entry->data.objDesc->object) continue;

        auto* objDesc = entry->data.objDesc;
        bool isEnchanted = objDesc->IsEnchanted();
        auto formID = entry->data.objDesc->object->formID;
        bool isTempered = ItemCardHook::IsItemTempered(entry->data.objDesc);

        auto indicator = GetNameIndicator(formID, isEnchanted, isTempered);
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

std::string BuildSmithyText(RE::FormID a_formID, bool a_isEnchanted, bool a_isTempered) {
    auto& cache = COBJCache::GetSingleton();
    std::string text;

    if (ItemCardHook::IsItemFiltered(a_formID)) return text;

    if (!a_isEnchanted) {
        auto* smeltResult = cache.GetSmeltResult(a_formID);
        if (smeltResult && smeltResult->outputItem) {
            auto* name = smeltResult->outputItem->GetName();
            if (name && *name) {
                bool available = cache.IsSmeltAvailable(a_formID);
                if (available || ItemCardHook::showLockedMaterials) {
                    text += "Smelts into" + (available ? ": "s : " (locked): "s) +
                            FormatCountAndName(smeltResult->outputCount, name);
                }
            }
        }
    }

    auto templateID = ItemCardHook::GetTemplateFormID(a_formID, a_isEnchanted);
    auto lookupID = (templateID != 0) ? templateID : a_formID;
    const auto* temperEntry = cache.GetTemperEntry(lookupID);
    if (temperEntry && !temperEntry->ingredients.empty()) {
        bool available = !a_isTempered && ShouldShowTemper(a_isEnchanted) && cache.IsTemperAvailable(a_formID, a_isEnchanted, templateID);
        if (available || ItemCardHook::showLockedMaterials) {
            if (!text.empty()) text += "<br>";
            text += "Tempers with" + (available ? ": "s : " (locked): "s);
            bool first = true;
            for (auto& ing : temperEntry->ingredients) {
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
    activeMenu = ActiveMenu::Player;
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
    activeMenu = ActiveMenu::Container;
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
    activeMenu = ActiveMenu::Merchant;
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
    bool isTempered = ItemCardHook::IsItemTempered(selectedItem->data.objDesc);

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

    if (currentEffects.find("Tempers with") != std::string::npos ||
        currentEffects.find("Smelts into") != std::string::npos) {
        _lastFormID = formID;
        return;
    }

    auto smithyText = BuildSmithyText(formID, isEnchanted, isTempered);
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
