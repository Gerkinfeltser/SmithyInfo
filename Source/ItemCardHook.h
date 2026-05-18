#pragma once

class ItemCardHook {
public:
    static void Install();
    static inline bool gateEnchantedTempering = true;
    static inline bool effectsPlayer = true;
    static inline bool effectsContainer = true;
    static inline bool effectsMerchant = true;
    static inline bool indicatorPlayer = true;
    static inline bool indicatorContainer = true;
    static inline bool indicatorMerchant = true;
    static inline RE::FormID arcaneBlacksmithPerkID = 0x0005218E;
    static inline std::string indicatorPrefix = "|";
    static inline std::string indicatorSuffix = "|";
    static inline std::string indicatorSmelt = "s";
    static inline std::string indicatorTemper = "t";

private:
    using AdvanceMovie_t = void (*)(RE::IMenu* a_this, float a_interval, std::uint32_t a_currentTime);
    static inline REL::Relocation<AdvanceMovie_t> _InvAdvanceMovie;
    static inline REL::Relocation<AdvanceMovie_t> _ContAdvanceMovie;
    static inline REL::Relocation<AdvanceMovie_t> _BarterAdvanceMovie;

    static void InvAdvanceMovie_Hook(
        RE::IMenu* a_this,
        float a_interval,
        std::uint32_t a_currentTime);

    static void ContAdvanceMovie_Hook(
        RE::IMenu* a_this,
        float a_interval,
        std::uint32_t a_currentTime);

    static void BarterAdvanceMovie_Hook(
        RE::IMenu* a_this,
        float a_interval,
        std::uint32_t a_currentTime);

    static RE::FormID _lastFormID;

    static void InjectItemCardText(RE::GFxMovieView* a_movie, RE::GFxValue& a_root, RE::ItemCard* a_itemCard, RE::ItemList* a_itemList);
};
