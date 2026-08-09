# WISP Engine Audit & Implementation Tracker (2026/2027)

## Executive Status
- **WebIDL Coverage:** 2,979 / 2,979 stubs mapped (100% strong C symbol overrides under `src/content/handlers/javascript/quickjs/impl/`)
- **Test Suite Status:** 114 / 114 passing (0 regressions)
- **Leak Prevention:** LSan clean on CSS Node Selection Data (`free_style_snapshot` reclamation)

## Directives Ledger

### 1. WebIDL Stubs & Overrides
- [x] Audit weak stubs vs manual C implementations (`stubs_manual_impl.c`)
- [x] Upgrade functional no-op stubs to full spec implementations:
  - [x] `wisp_canvasrenderingcontext2d_putImageData_1_impl` (Dirty bounds support)

### 2. JavaScript Engine / Web APIs
- [ ] Canvas 2D API parity check (ImageData dirty rects, ImageData scaling)
- [ ] Web Workers / EventLoop task queue audits

### 3. CSS3 Implementation & Parsing
- [ ] CSSOM property setter/getter synchronization
- [ ] CSS Flexbox / Grid layout pass-throughs

### 4. HTML5 Standard Elements
- [ ] Custom Element / Web Component lifecycle stubs
- [ ] HTML5 Form validation / Canvas element event integration
