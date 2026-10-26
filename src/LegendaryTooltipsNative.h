#pragma once

#include "pl/Mod.hpp"

#include <filesystem>
#include <string>

namespace legendary_tooltips_native {

class LegendaryTooltipsNative final : public pl::mod::NativeMod {
public:
    LegendaryTooltipsNative();

    static LegendaryTooltipsNative& getInstance();

    bool load(pl::mod::ModContext& context);
    bool enable(pl::mod::ModContext& context);
    bool disable(pl::mod::ModContext& context);
    bool unload(pl::mod::ModContext& context);

private:
    bool readConfig(const std::filesystem::path& configDir);
    bool installCohtmlHook();
    void buildInstallerScript();

    std::filesystem::path mConfigDir;
    bool mConfigEnabled{true};
    std::string mBorderColor{"#d8b65c"};
    std::string mBackgroundColor{"#17191f"};
    bool mShadow{true};
    int mRadius{5};
    bool mHookInstalled{false};
};

} // namespace legendary_tooltips_native
