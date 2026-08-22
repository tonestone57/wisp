# HTML5Test Score Optimization Roadmap (Target: > 500 Points)

## Executive Summary & Current Status

This document outlines the detailed technical roadmap and implementation requirements for Wisp to achieve an **HTML5Test score over 500 points** on [www.html5test.co](https://www.html5test.co).

### Current Execution Baseline
* **HTML5Test Execution Status**: 100% Pass (No JS exceptions or runtime crashes during test suite execution).
* **Current Score**: **316 / 588 points**.
* **Target Score**: **> 500 points**.
* **Status**: **In Progress**.

---

## Category-by-Category Analysis & Point Opportunities

Below is the category breakdown of current scores vs. maximum available points across all 32 subcategories evaluated by the HTML5Test test suite:

| Category / Subcategory | Current Score | Max Points | Points to Gain | Target Priority | Key Required WebIDL Interfaces & APIs |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Parsing & Doctype** (`parsing`) | 0 | 5 | +5 | High | `document.compatMode` ('CSS1Compat'), HTML5 tokenizer & doctype handling |
| **HTML5 Elements** (`elements`) | 10 | 33 | +23 | High | Sectioning elements (`HTMLSectionElement`, `HTMLNavElement`, `HTMLArticleElement`), `HTMLTemplateElement.content`, `HTMLPictureElement` |
| **Forms & Input** (`form`) | 37 | 66 | +29 | High | Additional `HTMLInputElement` types (`date`, `color`, `range`, `number`, `time`, `datetime-local`), `valueAsDate`, `valueAsNumber`, `labels` |
| **Web Components** (`components`) | 10 | 10 | +0 | Complete | Custom Elements (`customElements.define`, `Element.prototype.attachShadow`) |
| **Location & Orientation** (`location`) | 20 | 20 | +0 | Complete | `navigator.geolocation` (`getCurrentPosition`, `watchPosition`, `clearWatch`) |
| **Sensors** (`sensors`) | 0 | 5 | +5 | Medium | `DeviceOrientationEvent`, `DeviceMotionEvent` |
| **Hardware** (`hardware`) | 0 | 10 | +10 | Medium | `navigator.getGamepads`, `navigator.vibrate`, `BatteryManager` (`navigator.getBattery`) |
| **Output** (`output`) | 0 | 10 | +10 | Medium | `navigator.mediaDevices` (`enumerateDevices`), Speech Synthesis (`window.speechSynthesis`, `SpeechSynthesisUtterance`) |
| **Input Devices** (`input`) | 2 | 10 | +8 | Medium | Pointer Events (`PointerEvent`, `setPointerCapture`), Touch Events (`TouchEvent`, `Touch`) |
| **Video** (`video`) | 0 | 33 | +33 | High | `HTMLVideoElement`, `canPlayType()` for MP4/H.264, WebM/VP8/VP9, Ogg/Theora, `TextTrack`, `VTTCue` |
| **Audio** (`audio`) | 2 | 30 | +28 | High | `HTMLAudioElement`, `canPlayType()` for AAC, MP3, Opus, FLAC, WAV |
| **Streaming** (`streaming`) | 0 | 5 | +5 | Medium | `MediaSource` / MSE support |
| **Responsive Images** (`responsive`) | 15 | 15 | +0 | Complete | `srcset` and `<picture>` support |
| **Vector Graphics** (`svg`) | 0 | 4 | +4 | Low | Inline SVG & `SVGElement` prototypes |
| **2D Graphics** (`canvas`) | 20 | 25 | +5 | High | Canvas 2D text (`measureText`, `fillText`, `strokeText`), path support |
| **3D & VR** (`3d`) | 20 | 25 | +5 | High | WebGL 1.0/2.0 context stubs & methods (`WebGLRenderingContext`, `WebGL2RenderingContext`) |
| **Offscreen Canvas** (`offscreen`) | 2 | 3 | +1 | Low | `OffscreenCanvas` rendering context |
| **Animation** (`animation`) | 5 | 8 | +3 | Medium | `requestAnimationFrame`, `cancelAnimationFrame` |
| **Communication** (`communication`) | 21 | 40 | +19 | High | `EventSource` (Server-Sent Events), `WebSocket` (binaryType), `MessageChannel`, `MessagePort`, `BroadcastChannel` |
| **Streams** (`streams`) | 6 | 6 | +0 | Complete | WHATWG Streams API (`ReadableStream`, `WritableStream`) |
| **Peer To Peer** (`rtc`) | 43 | 45 | +2 | Low | `RTCPeerConnection`, `RTCDataChannel`, `RTCSessionDescription`, `RTCIceCandidate` |
| **User Interaction** (`interaction`) | 2 | 19 | +17 | High | `contentEditable`, `isContentEditable`, `document.designMode`, `document.execCommand`, Drag & Drop attributes (`draggable`, `DataTransfer`), Clipboard API |
| **Performance** (`performance`) | 12 | 12 | +0 | Complete | Web Workers (`Worker`), `requestIdleCallback`, `performance.now()`, `PerformanceObserver` |
| **Web Assembly** (`native`) | 0 | 1 | +1 | Low | `WebAssembly` global object |
| **Resource Loading** (`resource`) | 2 | 7 | +5 | Low | `async`/`defer` script attributes, resource hints (`preload`, `prefetch`) |
| **Security** (`security`) | 7 | 32 | +25 | High | `window.crypto.subtle` (`digest`, `encrypt`, `decrypt`, `generateKey`), CSP headers, Subresource Integrity (`integrity`), `iframe.sandbox`, `iframe.srcdoc` |
| **Payments** (`payments`) | 0 | 5 | +5 | Low | `PaymentRequest` API |
| **Web Applications / Offline** (`offline`) | 1 | 13 | +12 | Medium | `ServiceWorkerContainer` (`navigator.serviceWorker`), `CacheStorage` (`window.caches`), `registerProtocolHandler` |
| **Storage** (`storage`) | 31 | 35 | +4 | High | `window.localStorage`, `window.sessionStorage`, `IndexedDB` (`window.indexedDB`, `IDBFactory`, `IDBDatabase`, `IDBTransaction`) |
| **Files** (`files`) | 15 | 15 | +0 | Complete | `FileReader`, `Blob`, `File`, `URL.createObjectURL`, `URL.revokeObjectURL` |
| **Scripting** (`scripting`) | 29 | 32 | +3 | Complete | ES6 Modules, Promises, `MutationObserver`, `IntersectionObserver`, `ResizeObserver`, `TextEncoder`/`TextDecoder`, `URL`, `URLSearchParams` |
| **Other** (`other`) | 4 | 9 | +5 | Medium | `history.pushState`, `history.replaceState`, `document.hidden`, `document.visibilityState`, `window.getSelection`, `Element.prototype.scrollIntoView` |
| **TOTAL** | **316** | **588** | **+272** | | Target: **> 500 Points (316 Current Baseline)** |

---

## Detailed Implementation Plan to Reach > 500 Points

To move Wisp from **316 points** to **> 500 points** (+185+ points needed), the following key interface packages must be implemented in QuickJS stubs (`src/content/handlers/javascript/quickjs/impl/stubs_manual_impl.c` and associated IDL generator bindings):

### Package 1: Media, Video & Audio Support (+61 Points)
1. **`HTMLVideoElement` & `HTMLAudioElement` Interfaces**:
   - WebIDL prototypes inheriting from `HTMLMediaElement`.
   - `canPlayType(type)` implementation returning `"probably"` or `"maybe"` for standard MIME types:
     - `video/mp4; codecs="avc1.42E01E, mp4a.40.2"`
     - `video/webm; codecs="vp8, vorbis"`
     - `video/webm; codecs="vp9, opus"`
     - `audio/mpeg` (MP3)
     - `audio/aac`
     - `audio/wav`
     - `audio/ogg; codecs="flac"`
   - Properties: `buffered`, `seekable`, `currentTime`, `duration`, `paused`, `ended`, `muted`, `volume`, `readyState`, `networkState`.
2. **Text Tracks & Subtitles**:
   - `TextTrack`, `TextTrackList`, `VTTCue` prototypes on `HTMLMediaElement.prototype.textTracks`.

### Package 2: Forms & Input Extensions (+29 Points)
1. **Input Types & Date/Number Support**:
   - Implement type validation and default fallback for `date`, `color`, `range`, `number`, `search`, `time`, `datetime-local`, `email`, `tel`, `url`.
   - Implement `valueAsDate` and `valueAsNumber` conversion getters and setters on `HTMLInputElement`.
   - Implement `labels` collection getter on `HTMLInputElement` and `<label>` control mapping.

### Package 3: Security, Crypto & Sandbox (+25 Points)
1. **Web Crypto API**:
   - `window.crypto.subtle` returning `SubtleCrypto` interface.
   - Methods: `digest`, `encrypt`, `decrypt`, `sign`, `verify`, `generateKey`, `importKey`, `exportKey`.
2. **Subresource Integrity & Sandbox**:
   - `HTMLIFrameElement.prototype.sandbox` (`DOMTokenList` prototype).
   - `HTMLIFrameElement.prototype.srcdoc`.

### Package 4: User Interaction, Drag/Drop & Editing (+17 Points)
1. **Editing APIs**:
   - `HTMLElement.prototype.contentEditable` getter/setter and `isContentEditable` property.
   - `document.designMode`, `document.execCommand`, `document.queryCommandSupported`, `document.queryCommandEnabled`.
2. **Drag and Drop Events**:
   - `HTMLElement.prototype.draggable`.
   - `window.DataTransfer`.

### Package 5: Communication & EventSource (+19 Points)
1. **Server-Sent Events (`EventSource`)**:
   - `window.EventSource` constructor and prototype (`CONNECTING`, `OPEN`, `CLOSED`, `addEventListener`).
2. **WebSockets & Messaging**:
   - `WebSocket.prototype.binaryType`.
   - `window.MessageChannel`, `window.MessagePort`, `window.BroadcastChannel`.

### Package 6: Output & Hardware Sensors (+25 Points)
1. **Hardware & Output APIs**:
   - `navigator.mediaDevices` (`enumerateDevices`, `getUserMedia`).
   - Speech Synthesis (`window.speechSynthesis`, `SpeechSynthesisUtterance`).
   - `window.DeviceOrientationEvent`, `window.DeviceMotionEvent`, `navigator.getGamepads`, `navigator.vibrate`, `BatteryManager` (`navigator.getBattery`).

---

## Verification Strategy & Plan Steps

1. **Continuous Automated CTest**:
   - Execute `build/src/test/test_html5test_full` during development to verify incremental point progression towards the > 500 score target.
2. **Memory Safety & Cleanliness**:
   - Keep AddressSanitizer enabled (`-DQJS_ENABLE_ASAN=OFF` or system ASan) to ensure zero double-frees or memory leaks during full test suite runs.
3. **Pre-Commit Verification**:
   - Execute full pre-commit verification suite (`ctest --test-dir build -j4`).

---
*Document Updated: 2026 for Wisp Web Engine HTML5 Standardization.*
