#pragma once
#include <unordered_set>

struct SmeltResult {
    RE::TESBoundObject* outputItem = nullptr;
    std::uint16_t outputCount = 0;
    RE::BGSConstructibleObject* cobj = nullptr;
};

struct IngredientInfo {
    RE::TESBoundObject* item = nullptr;
    std::int32_t count = 0;
};

struct TemperEntry {
    std::vector<IngredientInfo> ingredients;
    RE::BGSConstructibleObject* cobj = nullptr;
};

class COBJCache {
public:
    static COBJCache& GetSingleton() {
        static COBJCache instance;
        return instance;
    }

    void Build();

    const SmeltResult* GetSmeltResult(RE::FormID a_formID) const;
    const TemperEntry* GetTemperEntry(RE::FormID a_formID) const;

    bool IsSmeltAvailable(RE::FormID a_formID) const;
    bool IsTemperAvailable(RE::FormID a_formID, bool a_isEnchanted, RE::FormID a_templateFormID = 0) const;

    COBJCache(const COBJCache&) = delete;
    COBJCache& operator=(const COBJCache&) = delete;
    COBJCache() = default;

private:
    std::unordered_map<RE::FormID, SmeltResult> _smeltMap;
    std::unordered_map<RE::FormID, TemperEntry> _temperMap;
};
