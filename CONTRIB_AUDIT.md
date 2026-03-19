# Contrib Libraries Audit and Modernization Assessment

This document provides a full audit, analysis, and assessment of the third-party libraries bundled within the `contrib/` directory of the Wisp browser codebase. The primary goal is to evaluate the feasibility and effort required to modernize these libraries by upgrading them to their latest upstream releases.

## Executive Summary

Wisp relies on several external libraries, predominantly originating from the NetSurf project (`libcss`, `libdom`, `libhubbub`, `libnsbmp`, `libnsgif`, `libnsutils`, `libparserutils`, `libsvgtiny`), as well as `quickjs-ng` for JavaScript execution.

**Assessment:** Upgrading the NetSurf libraries to upstream is **highly complex and generally not recommended** without a massive refactoring effort. Wisp has heavily modified and extended these libraries. Simply replacing them with upstream versions would result in significant feature regressions (e.g., losing CSS Grid, `calc()` support, and `libwapcaplet` integration).

`quickjs-ng` has a smaller modification footprint and is an easier candidate for upstream syncs, although its build system integration and custom bindings must be carefully maintained.

---

## Detailed Library Analysis

Below is an analysis of each library, detailing the extent of divergences between the Wisp fork and the current upstream repositories.

### `libcss` (CSS parser and selection engine)
*   **Upstream (NetSurf):** ~Jan 2026 (104d87f docs: Convert to markdown)
*   **Wisp Modifications:** Extremely heavy.
    *   **Modified files:** 252
    *   **Added by Wisp:** 52
*   **Key Divergences:** Wisp has extended `libcss` far beyond the upstream NetSurf implementation. Wisp added full parsers and data structures for **CSS Grid**, `calc()`, and newer pseudo-elements. The `properties.toml` system implies a heavily modified or entirely custom code generation pipeline for CSS properties.
*   **Upgrade Feasibility:** **Low**. An upstream sync would overwrite CSS Grid and `calc()` support. Any modernization must be done by porting upstream bug fixes into the Wisp tree manually, rather than replacing the Wisp tree with upstream.

### `libdom` (Document Object Model)
*   **Upstream (NetSurf):** ~Jan 2026 (f69781e ci: actions: Add cross compilation jobs)
*   **Wisp Modifications:** Extremely heavy.
    *   **Modified files:** 725
    *   **Added by Wisp:** 3
*   **Key Divergences:** Almost every file in the source tree has been touched. Wisp has altered internal data structures, `libxml` bindings, and integration with `libhubbub` (`dom_hubbub_parser`). Wisp uses a unified CMake build rather than the NetSurf build system.
*   **Upgrade Feasibility:** **Low**. The DOM implementation is heavily intertwined with Wisp's specific memory management, error handling, and parser integration.

### `libhubbub` (HTML5 compliant parser)
*   **Upstream (NetSurf):** ~Jan 2026 (6651b8c ci: actions: Install JSON-C for tests)
*   **Wisp Modifications:** Moderate.
    *   **Modified files:** 59
    *   **Added by Wisp:** 2
*   **Key Divergences:** Altered error codes and API signatures to bridge with Wisp's modified `libdom`. Modifications to treebuilder and tokenizer logic.
*   **Upgrade Feasibility:** **Low to Medium**. While fewer files are modified than `libcss`, the tightly coupled nature of `libhubbub` and `libdom` means upgrading one practically requires upgrading the other.

### `libnsbmp` & `libnsgif` (Image decoders)
*   **Upstream (NetSurf):** ~Jan 2026
*   **Wisp Modifications:** Light.
    *   **Modified files:** ~8 each
*   **Key Divergences:** Minor type signature changes, macro adjustments (e.g., `UNUSED` macros), and formatting/header changes to integrate with Wisp's build system.
*   **Upgrade Feasibility:** **High**. These libraries are relatively self-contained and could be re-synced with upstream with minimal effort, provided the CMake build files and Wisp-specific headers are preserved.

### `libnsutils` (Utility library)
*   **Upstream (NetSurf):** ~Jan 2026
*   **Wisp Modifications:** Significant structural change.
    *   **Modified files:** 9
    *   **Added by Wisp:** 4
*   **Key Divergences:** Wisp merged `libwapcaplet` (NetSurf's string internment library) directly into `libnsutils`. Upstream NetSurf maintains `libwapcaplet` as a separate project.
*   **Upgrade Feasibility:** **Medium**. Upgrading requires manually merging upstream `libnsutils` and `libwapcaplet` changes into this unified directory.

### `libparserutils` (Parser building library)
*   **Upstream (NetSurf):** ~Jan 2026
*   **Wisp Modifications:** Moderate.
    *   **Modified files:** 47
*   **Key Divergences:** Character set and encoding (UTF-8, UTF-16) parsing adjustments. Modifications to error handling macros.
*   **Upgrade Feasibility:** **Medium**. Re-syncing is possible but requires careful manual review of the charset and codec handling code.

### `libsvgtiny` (SVG Tiny parser)
*   **Upstream (NetSurf):** ~Jan 2026
*   **Wisp Modifications:** Heavy.
    *   **Modified files:** 23
    *   **Added by Wisp:** 23
*   **Key Divergences:** Wisp heavily uses `libdom` integration to parse SVG DOMs natively. Many new internal structures and rendering path modifications have been introduced.
*   **Upgrade Feasibility:** **Low**. Similar to `libcss`, Wisp has essentially forked this library to add specific rendering and parsing paths.

### `quickjs-ng` (JavaScript Engine)
*   **Upstream (quickjs-ng):** Mar 2026 (v0.12+)
*   **Wisp Current Version:** v0.11.0 (QJS_VERSION_MINOR 11)
*   **Wisp Modifications:** Moderate.
    *   **Modified files:** 47
*   **Key Divergences:** CMake integration, `#include` path adjustments, custom C-API wrappers, and specific memory usage adaptations for the Wisp subsystem.
*   **Upgrade Feasibility:** **Medium to High**. QuickJS-ng can be upgraded to the latest release (e.g., v0.12.1), provided the Wisp CMake file (`CMakeLists.txt`) and specific memory/subsystem hooks are re-applied. Since it's an actively developed engine, upgrading is highly recommended for ES6+ compliance and performance improvements.

---

## Conclusion and Recommendations

1. **Do not perform blanket "drop-in" replacements** for NetSurf libraries (`libcss`, `libdom`, `libhubbub`, `libsvgtiny`). The Wisp fork contains critical browser functionality (Grid, flexbox, calc, customized SVG handling) that upstream NetSurf does not have.
2. **Modernize via Cherry-Picking:** For the core NetSurf libraries, track upstream security patches and critical bug fixes, and manually cherry-pick them into the Wisp tree.
3. **Image Decoders (`libnsbmp`, `libnsgif`):** These are good candidates for periodic manual syncing since the API surface and modifications are minimal.
4. **QuickJS-ng:** Prioritize upgrading `quickjs-ng` to the latest upstream release (0.12.x+) to benefit from JavaScript language features and bug fixes. Maintain the CMake build structure and custom subsystem integration code.
