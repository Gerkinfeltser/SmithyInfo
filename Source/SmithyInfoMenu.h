#pragma once

#include <string>

class SmithyInfoMenu {
public:
    static void Register(const std::string& a_iniPath);
    static void SaveSettings();

    static inline bool registered = false;
    static inline bool smfEnabled = true;
    static inline std::string iniPath;
};
