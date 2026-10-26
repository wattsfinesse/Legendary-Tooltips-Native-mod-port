# Legendary Tooltips Native — experimental native port

**Creator:** Watts  
**Framework credit:** LeviLamroid by its original maintainers/contributors.

This is a **clean-room native prototype**, not a drop-in 1:1 port of the Java/Fabric code. It is intended for LeviLauncher and uses the LeviLamroid native framework and is pinned to the Bedrock build whose offset table reports **1.26.52.3 (Android ARM64)**.

## What this prototype attempts

- Add a configurable border, background color, rounded corners, and shadow to UI DOM elements whose ID/class/role resembles a tooltip.
- Observe newly created DOM nodes so the styling can be applied to tooltips created after the page initially loads.
- Read style values from `config/config.json`.

Default style is a muted gold frame on a dark background. Colors, shadow, and radius can be edited in the config. Restart the game after changing the config.

## Important limitations

- **Not verified against live OreUI screens.** LeviLamroid exposes a Cohtml `ExecuteScript` RVA, but the supplied source does not expose a stable item-tooltip-render event, a documented tooltip DOM selector, or a direct Java-like tooltip draw API. This prototype uses heuristic selectors (`role=tooltip`, class/id names containing `tooltip`, and a few hover-text names). If the active Bedrock screen does not expose tooltips through matching DOM elements, nothing will happen.
- **Version-pinned native hook.** It refuses to install unless the detected build string contains `1.26.52.3`. Offsets and ABI assumptions can change across Bedrock builds. A wrong function signature/RVA can crash the game; back up your profile before testing.
- The hook is experimental and cannot be safely removed with the public hook API in this source tree. Disable it logically, then restart Bedrock before replacing/unloading the `.so`.
- It does **not** implement Java Edition's per-item rarity registry, 3D item model preview, compact modifier text, max tooltip sizing, or scrollable tooltip contents. Those rely on Java GUI/item APIs and need additional Bedrock hooks or UI support.
- It has not been built or runtime-tested here because an Android NDK toolchain is not present in the current build environment. The ZIP contains source and packaging instructions, not a prebuilt `.so` or `.levipack`.

## Build in Termux / Android NDK

1. Extract both this project and `LeviLamroid-main`.
2. Ensure the compatible Android NDK and CMake are installed.
3. From this directory, set `LEVILAMROID_ROOT` to the extracted LeviLamroid source directory and run:

   ```sh
   export LEVILAMROID_ROOT=/path/to/LeviLamroid-main
   ./package.sh
   ```

   If your NDK is not found automatically, set `ANDROID_NDK_HOME` or `CMAKE_TOOLCHAIN_FILE`.
4. If the build succeeds, `dist/LegendaryTooltipsNative-0.1.0.levipack` will contain the compiled library, manifest, config, and this README. The script deliberately refuses to create a `.levipack` if the native library was not built.

## Credits

- **Creator / port author:** Watts
- **Native framework:** LeviLamroid project and its maintainers/contributors.
- **Original concept:** Legendary Tooltips for Java Edition; this project is an experimental, independent reimplementation and is not affiliated with or endorsed by the original author.

## License / provenance note

The supplied Legendary Tooltips source archive carries **CC BY-NC-ND 4.0**. This prototype does not copy its Java source or assets; it independently reimplements only a generic tooltip-frame styling idea for Bedrock's different UI stack. It is not affiliated with or endorsed by the original author. Review the original license and seek permission before distributing any original Java code/assets or a derivative that would require permission.
