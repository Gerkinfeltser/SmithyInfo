// DIII_API.h — vendored from Dynamic Inventory Icon Injector SKSE
// https://github.com/JerryYOJ/Dynamic-Inventory-Icon-Injector-SKSE
// Other plugins #include this to register custom conditions with DIII.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace RE {
    class InventoryEntryData;
    enum class FormType;
}

// Forward declare Json::Value to avoid requiring jsoncpp in every TU
namespace Json {
    class Value;
}

namespace DIII {
    // Base class other plugins inherit from to create custom conditions
    class ICondition {
    public:
        virtual ~ICondition() = default;
        virtual bool Match(RE::InventoryEntryData* entry) const = 0;
    };

    // Builder signature: takes JSON value, returns a condition (or nullptr)
    using ConditionBuilder = std::function<
        std::unique_ptr<ICondition>(const Json::Value& value, RE::FormType type)
    >;

    // The API interface — stable ABI via pure virtual
    class IAPI {
    public:
        virtual ~IAPI() = default;

        // Register a custom match field builder
        // name = JSON key (e.g. "smithySmelt")
        // builder = factory that creates the condition from JSON
        virtual bool RegisterCondition(
            const char* name,
            ConditionBuilder builder
        ) = 0;

        // API version for compatibility checks
        virtual uint32_t GetVersion() const = 0;
    };

    // Message type for SKSE messaging
    constexpr uint32_t kMessage_GetAPI = 0xD111;
}
