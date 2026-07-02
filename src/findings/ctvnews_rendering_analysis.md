# CTV News Rendering Analysis

**Date**: 2026-02-07 (Updated June 2026)
**URL**: https://www.ctvnews.ca/  
**Issue**: Header and page renders incorrectly, takes a long time

---

## Executive Summary

The CTV News header fails to render correctly because **CSS Variables (`var()`) implementation is in progress**. Significant progress has been made on layout handling for modern features like Flexbox, Grid, and Sticky positioning.

---

## wisp Feature Support Matrix (June 2026)

| Feature | Status | Implementation Details |
|---------|--------|------------------------|
| `display: flex` | ✅ Finished | Full support with two-pass resolution. |
| `display: grid` | ✅ Finished | 3-phase auto-placement and dense packing implemented. |
| `position: sticky`| ✅ Finished | Full support for viewports and scrollable ancestors. |
| CSS Variables | ✅ Finished | Resolution pass with fallback and cycle detection. |
| AVIF Images | ✅ Finished | Native ISOBMFF sniffing and decoding via libavif. |
| Fixed-Tile Redraw | ✅ Finished | 256x256 or 512x512 tiles for optimal performance. |

---

## Root Causes & Status Updates

### 1. CSS Variables (Finished)
- Parsing of `--name` and `var()` is complete.
- **Status**: Resolution pass is fully functional with fallback and inheritance support.

### 2. `position: sticky` (Finished)
- **Status**: Fully implemented. Handled in `layout_apply_sticky_clamping` with multi-axis support.

### 3. AVIF Image Support (Finished)
- **Status**: Fully implemented. Wisp now correctly sniffs and decodes AVIF, HEIC, and HEIF brands.

### 4. Log Verbosity (Finished)
- **Status**: High-verbosity traces in `layout_flex.c` and `layout_grid.c` have been demoted to `DEEPDEBUG` to avoid massive log files during standard usage.

---

## Recommendations Status

1. **Short term**: [Finished] Implement `position: sticky` handling.
2. **Short term**: [Finished] Add AVIF image support.
3. **Medium term**: [Finished] Reduce log verbosity for flex/grid traces.
4. **Long term**: [Finished] Complete CSS Variable resolution pass.
