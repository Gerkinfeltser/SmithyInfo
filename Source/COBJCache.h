#pragma once

struct SmeltResult {
    RE::TESBoundObject* outputItem = nullptr;
    std::uint16_t outputCount = 0;
};

struct IngredientInfo {
    RE::TESBoundObject* item = nullptr;
    std::int32_t count = 0;
};

class COBJCache {
public:
    static COBJCache& GetSingleton() {
        static COBJCache instance;
        return instance;
    }

    void Build();

    const SmeltResult* GetSmeltResult(RE::FormID a_formID) const;
    const std::vector<IngredientInfo>* GetTemperMaterials(RE::FormID a_formID) const;

    COBJCache(const COBJCache&) = delete;
    COBJCache& operator=(const COBJCache&) = delete;

private:
    COBJCache() = default;

    std::unordered_map<RE::FormID, SmeltResult> _smeltMap;
    std::unordered_map<RE::FormID, std::vector<IngredientInfo>> _temperMap;
};
