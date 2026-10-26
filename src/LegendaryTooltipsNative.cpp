#include "LegendaryTooltipsNative.h"

#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_set>

namespace legendary_tooltips_native {
namespace {

using CohtmlExecuteScriptFn = void (*)(void* view, const char* script);
CohtmlExecuteScriptFn gOriginalExecuteScript = nullptr;
std::atomic<bool> gHookActive{false};
std::mutex gViewMutex;
std::unordered_set<void*> gInjectedViews;
std::mutex gScriptMutex;
std::string gInstallerScript;

pl::mod::ModInfo makeModInfo() {
    pl::mod::ModInfo info;
    info.id = "legendary_tooltips_native";
    info.displayName = "Legendary Tooltips Native";
    info.author = "Community port prototype";
    info.version = "0.1.0";
    info.entryFileName = "liblegendary_tooltips_native.so";
    return info;
}

std::string sanitizeCssColor(const std::string& input, const std::string& fallback) {
    // Restrict config values to simple CSS color characters; reject declarations or URLs.
    if (input.empty() || input.size() > 48) return fallback;
    for (unsigned char ch : input) {
        if (!(std::isalnum(ch) || ch == '#' || ch == '(' || ch == ')' ||
              ch == ',' || ch == '.' || ch == '%' || ch == ' ' || ch == '-')) {
            return fallback;
        }
    }
    const auto lowered = [&]() {
        std::string out = input;
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return out;
    }();
    if (lowered.find("url(") != std::string::npos || lowered.find("expression(") != std::string::npos) {
        return fallback;
    }
    return input;
}

void cohtmlExecuteScriptDetour(void* view, const char* script) {
    auto original = gOriginalExecuteScript;
    if (!original) return;

    // Preserve the game's requested script and return immediately if styling is inactive.
    original(view, script);
    if (!gHookActive.load(std::memory_order_relaxed) || !view) return;

    bool shouldInject = false;
    {
        std::lock_guard<std::mutex> lock(gViewMutex);
        shouldInject = gInjectedViews.insert(view).second;
    }
    if (!shouldInject) return;

    std::string installer;
    {
        std::lock_guard<std::mutex> lock(gScriptMutex);
        installer = gInstallerScript;
    }
    if (!installer.empty()) {
        // Call the trampoline directly so our own injection does not recursively re-enter the detour.
        original(view, installer.c_str());
    }
}

} // namespace

LegendaryTooltipsNative::LegendaryTooltipsNative()
    : pl::mod::NativeMod(nullptr, makeModInfo()) {}

LegendaryTooltipsNative& LegendaryTooltipsNative::getInstance() {
    static LegendaryTooltipsNative instance;
    return instance;
}

bool LegendaryTooltipsNative::load(pl::mod::ModContext& context) {
    mConfigDir = context.configDir();
    if (!readConfig(mConfigDir)) {
        context.logger().warn("Could not fully load tooltip style config; using defaults");
    }
    buildInstallerScript();
    context.logger().info("Legendary Tooltips Native source prototype loaded; Bedrock={}",
                          levilamroid::mcpe::getDetectedVersion());
    return true;
}

bool LegendaryTooltipsNative::enable(pl::mod::ModContext& context) {
    if (!mConfigEnabled) {
        context.logger().info("Tooltip styling is disabled in config/config.json");
        gHookActive.store(false, std::memory_order_relaxed);
        return true;
    }

    if (!installCohtmlHook()) {
        context.logger().warn("Tooltip DOM hook was not installed. No tooltip styling will be applied.");
        return false;
    }

    gHookActive.store(true, std::memory_order_relaxed);
    context.logger().info("Experimental tooltip frame styling enabled");
    return true;
}

bool LegendaryTooltipsNative::disable(pl::mod::ModContext& context) {
    gHookActive.store(false, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(gViewMutex);
        gInjectedViews.clear();
    }
    context.logger().info("Tooltip styling disabled for new UI injections. A currently open OreUI view may retain styling until it is recreated.");
    return true;
}

bool LegendaryTooltipsNative::unload(pl::mod::ModContext& context) {
    // LeviLamroid's public hook API does not expose a reliable unhook operation here.
    // Keep callbacks inert. Restart Bedrock before removing/replacing this library.
    gHookActive.store(false, std::memory_order_relaxed);
    context.logger().warn("Mod unloaded logically; restart the game before unloading/replacing the .so because the installed engine detour cannot be safely removed through this API.");
    return true;
}

bool LegendaryTooltipsNative::readConfig(const std::filesystem::path& configDir) {
    try {
        std::error_code ec;
        std::filesystem::create_directories(configDir, ec);
        const auto path = configDir / "config.json";
        nlohmann::json config;
        if (std::filesystem::exists(path, ec)) {
            std::ifstream input(path);
            if (!input) return false;
            input >> config;
        } else {
            config = {
                {"version", "0.1.0"},
                {"enabled", true},
                {"border_color", "#d8b65c"},
                {"background_color", "#17191f"},
                {"shadow", true},
                {"border_radius", 5}
            };
            std::ofstream output(path);
            if (!output) return false;
            output << config.dump(4) << '\n';
        }

        mConfigEnabled = config.value("enabled", true);
        mBorderColor = sanitizeCssColor(config.value("border_color", std::string("#d8b65c")), "#d8b65c");
        mBackgroundColor = sanitizeCssColor(config.value("background_color", std::string("#17191f")), "#17191f");
        mShadow = config.value("shadow", true);
        mRadius = std::clamp(config.value("border_radius", 5), 0, 18);
        return true;
    } catch (const std::exception& e) {
        getLogger().warn("Config parse error: {}", e.what());
        return false;
    }
}

void LegendaryTooltipsNative::buildInstallerScript() {
    nlohmann::json style = {
        {"enabled", mConfigEnabled},
        {"borderColor", mBorderColor},
        {"backgroundColor", mBackgroundColor},
        {"shadow", mShadow},
        {"radius", mRadius}
    };

    const std::string script = std::string("(()=>{const cfg=") + style.dump() + R"JS(;
if(!cfg.enabled || window.__legendaryTooltipsNativeInstalled) return;
window.__legendaryTooltipsNativeInstalled=true;
const selector='[role="tooltip"],[class*="tooltip"],[id*="tooltip"],[class*="Tooltip"],[id*="Tooltip"],[class*="hover_text"],[class*="hover-text"],[data-tooltip]';
function paint(el){
  try {
    if(!(el instanceof Element) || !el.matches(selector)) return;
    const txt=String(el.innerText||el.textContent||'').trim();
    if(!txt || txt.length>1600) return;
    const r=el.getBoundingClientRect();
    if(r.width<28 || r.height<8 || r.width>760 || r.height>500) return;
    el.style.setProperty('border','1px solid '+cfg.borderColor,'important');
    el.style.setProperty('background-color',cfg.backgroundColor,'important');
    el.style.setProperty('border-radius',String(cfg.radius)+'px','important');
    el.style.setProperty('box-sizing','border-box','important');
    if(cfg.shadow) el.style.setProperty('box-shadow','0 5px 18px rgba(0,0,0,.38)','important');
    else el.style.removeProperty('box-shadow');
    el.setAttribute('data-ltn-styled','1');
  } catch(_e){}
}
function scan(root){
  if(root instanceof Element) paint(root);
  if(root && root.querySelectorAll) root.querySelectorAll(selector).forEach(paint);
}
function start(){
  try {
    scan(document);
    if(!document.documentElement || window.__legendaryTooltipsNativeObserver) return;
    const observer=new MutationObserver(records=>records.forEach(record=>record.addedNodes.forEach(node=>scan(node))));
    observer.observe(document.documentElement,{childList:true,subtree:true});
    window.__legendaryTooltipsNativeObserver=observer;
  } catch(_e){}
}
if(document.readyState==='loading') document.addEventListener('DOMContentLoaded',start,{once:true});
else start();
})();)JS";

    std::lock_guard<std::mutex> lock(gScriptMutex);
    gInstallerScript = script;
}

bool LegendaryTooltipsNative::installCohtmlHook() {
    const std::string version = levilamroid::mcpe::getDetectedVersion();
    // The offsets supplied with the uploaded source are only verified for this Bedrock build.
    if (version.find("1.26.52.3") == std::string::npos) {
        getLogger().error("Refusing to hook Cohtml ExecuteScript for unsupported build '{}'; this prototype is pinned to Bedrock 1.26.52.3", version);
        return false;
    }

    if (mHookInstalled) return true;
    const auto rva = levilamroid::mcpe::getOffsets().cohtmlExecuteScript;
    if (rva == 0) return false;

    if (!ll::memory::hookRva<CohtmlExecuteScriptFn>(rva, &cohtmlExecuteScriptDetour, &gOriginalExecuteScript)) {
        getLogger().error("hookRva failed for cohtmlExecuteScript at RVA 0x{:x}", rva);
        return false;
    }
    mHookInstalled = true;
    return true;
}

} // namespace legendary_tooltips_native

PL_REGISTER_MOD(legendary_tooltips_native::LegendaryTooltipsNative,
                legendary_tooltips_native::LegendaryTooltipsNative::getInstance());
