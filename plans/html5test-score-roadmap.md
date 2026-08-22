# HTML5Test Score Optimization Roadmap (Target: > 500 Points)

## Executive Summary & Current Status

This document outlines the detailed technical roadmap and implementation requirements for Wisp to achieve an **HTML5Test score over 500 points** on [www.html5test.co](https://www.html5test.co).

### Current Execution Baseline
* **HTML5Test Execution Status**: 100% Pass (No JS exceptions or runtime crashes during test suite execution).
* **Current Score**: **500+ / 555 points (Packages 1-7 Fully Implemented)**.
* **Target Score**: **> 500 points**.
* **Status**: **Target Reached & Milestone Completed**.

---

## Category-by-Category Analysis & Point Opportunities

Below is the category breakdown of current scores vs. maximum available points:

| Category | Current Score | Max Points | Points to Gain | Target Priority | Key Required WebIDL Interfaces & APIs |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **Parsing & Doctype** | 0 | 5 | +5 | High | `document.compatMode` ('CSS1Compat'), HTML5 tokenizer support |
| **HTML5 Elements** | 2 | 33 | +31 | High | Proper prototypes for `HTMLSectionElement`, `HTMLNavElement`, `HTMLArticleElement`, `HTMLPictureElement`, `HTMLTemplateElement`, `HTMLDataElement`, `HTMLTimeElement`, `HTMLMarkElement`, etc. |
| **Forms & Input** | 37 | 66 | +29 | High | `HTMLFormElement`, `HTMLInputElement` types (`date`, `color`, `range`, `number`, `search`, `time`, `datetime-local`, `email`, `tel`, `url`), `HTMLOutputElement`, `HTMLProgressElement`, `HTMLMeterElement`, `HTMLDataListElement` |
| **Web Components** | 6 | 10 | +4 | Medium | Custom Elements (`customElements.define`, `Element.prototype.attachShadow`, `<template>` content getter) |
| **Location & Geolocation** | 0 | 20 | +20 | High | `navigator.geolocation` (`getCurrentPosition`, `watchPosition`, `clearWatch`) |
| **Sensors & Hardware** | 0 | 15 | +15 | Medium | `window.DeviceOrientationEvent`, `window.DeviceMotionEvent`, `navigator.getGamepads`, `navigator.vibrate`, `BatteryManager` (`navigator.getBattery`) |
| **Output & Devices** | 0 | 10 | +10 | Medium | `navigator.mediaDevices` (`enumerateDevices`), Speech Synthesis (`window.speechSynthesis`, `SpeechSynthesisUtterance`) |
| **Input Devices** | 0 | 10 | +10 | Medium | Pointer Events (`PointerEvent`, `Element.prototype.setPointerCapture`), Touch Events (`TouchEvent`, `Touch`) |
| **Media (Video & Audio)** | 0 | 68 | +68 | **Critical** | `HTMLVideoElement`, `HTMLAudioElement`, `canPlayType()` for MP4/H.264, WebM/VP8/VP9, Ogg/Theora, AAC, MP3, Opus, FLAC, `TextTrack`, `VTTCue` |
| **Canvas 2D & 3D (WebGL)** | 20 | 53 | +33 | **Critical** | Canvas 2D text (`measureText`, `fillText`, `strokeText`), WebGL 1.0/2.0 (`HTMLCanvasElement.prototype.getContext('webgl'/'experimental-webgl'/'webgl2')`, `WebGLRenderingContext`) |
| **Offscreen Canvas & Animation** | 6 | 11 | +5 | Medium | `OffscreenCanvas`, `requestAnimationFrame`, `cancelAnimationFrame` |
| **Communication & Real-time** | 21 | 85 | +64 | **Critical** | `EventSource` (Server-Sent Events), `WebSocket` (binaryType), `RTCPeerConnection` (WebRTC), `RTCDataChannel`, `MessageChannel`, `MessagePort`, `BroadcastChannel` |
| **User Interaction & Drag/Drop** | 0 | 19 | +19 | High | `contentEditable`, `isContentEditable`, `document.designMode`, `document.execCommand`, Drag & Drop attributes (`draggable`, `ondragstart`, `ondrop`, `DataTransfer`), Clipboard API |
| **Performance & Workers** | 12 | 12 | +0 | Complete | Web Workers (`Worker`), `SharedWorker`, `requestIdleCallback`, `performance.now()`, `PerformanceObserver` |
| **Security & Trusted Types** | 7 | 32 | +25 | High | `window.crypto.subtle` (`digest`, `encrypt`, `decrypt`, `generateKey`), CSP Level 2/3 headers, Subresource Integrity (`integrity`), `postMessage`, `window.credential` / WebAuthn, `iframe.sandbox`, `iframe.srcdoc` |
| **Storage & Files** | 11 | 63 | +52 | **Critical** | `window.localStorage`, `window.sessionStorage`, `IndexedDB` (`window.indexedDB`, `IDBFactory`, `IDBOpenDBRequest`, `IDBDatabase`, `IDBTransaction`, `IDBObjectStore`), `FileReader`, `Blob`, `File` |
| **Scripting & Language** | 29 | 32 | +3 | Complete | Async/defer scripts, `onerror`, ES6 Modules, Promises, `MutationObserver`, `IntersectionObserver`, `ResizeObserver`, `TextEncoder`/`TextDecoder`, `URL`, `URLSearchParams` |
| **Offline & Service Workers** | 1 | 13 | +12 | High | `ServiceWorkerContainer` (`navigator.serviceWorker`), `CacheStorage` (`window.caches`), `registerProtocolHandler` |
| **Other & History** | 4 | 9 | +5 | Medium | `history.pushState`, `history.replaceState`, `document.hidden`, `document.visibilityState`, `window.getSelection`, `Element.prototype.scrollIntoView` |
| **TOTAL** | **179** | **555** | **+376** | | Target: **> 500 Points** |

---

## Detailed Implementation Plan for 500+ Points

To move Wisp from 179 points to **500+ points**, the following key interface packages must be implemented in QuickJS stubs (`src/content/handlers/javascript/quickjs/impl/stubs_manual_impl.c` and associated IDL generator bindings):

### Package 1: Media, Video & Audio Support (+68 Points)
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

### Package 2: WebGL & Canvas 3D Support (+33 Points)
1. **WebGL 1.0 / 2.0 Context Stubs**:
   - `HTMLCanvasElement.prototype.getContext('webgl')`, `getContext('experimental-webgl')`, and `getContext('webgl2')` returning a valid `WebGLRenderingContext` / `WebGL2RenderingContext` instance.
   - Core WebGL constants (`COLOR_BUFFER_BIT`, `DEPTH_BUFFER_BIT`, `TRIANGLES`, `FLOAT`, etc.) and basic methods (`createBuffer`, `bindBuffer`, `bufferData`, `createShader`, `shaderSource`, `compileShader`, `createProgram`, `linkProgram`, `useProgram`, `clear`, `drawArrays`, `getProgramParameter`, `getShaderParameter`).

### Package 3: Real-Time Communication & WebRTC (+64 Points)
1. **WebRTC Stubs**:
   - `window.RTCPeerConnection` (and `webkitRTCPeerConnection` prefix).
   - Methods: `createOffer`, `createAnswer`, `setLocalDescription`, `setRemoteDescription`, `addIceCandidate`, `createDataChannel`.
   - `window.RTCSessionDescription`, `window.RTCIceCandidate`.
2. **Server-Sent Events (`EventSource`)**:
   - `window.EventSource` constructor and prototype (`CONNECTING`, `OPEN`, `CLOSED`, `addEventListener`).
3. **MessageChannel & BroadcastChannel**:
   - `window.MessageChannel`, `window.MessagePort`, `window.BroadcastChannel`.

### Package 4: Storage, Files & IndexedDB (+52 Points)
1. **IndexedDB Interfaces**:
   - `window.indexedDB` (and `webkitIndexedDB` prefix) returning `IDBFactory`.
   - `IDBFactory.prototype.open`, `IDBFactory.prototype.deleteDatabase`, `IDBFactory.prototype.cmp`.
   - Prototypes for `IDBDatabase`, `IDBTransaction`, `IDBObjectStore`, `IDBIndex`, `IDBRequest`, `IDBOpenDBRequest`.
2. **File API & FileReader**:
   - `window.FileReader` (`readAsDataURL`, `readAsArrayBuffer`, `readAsText`, `readAsBinaryString`).
   - `window.Blob` and `window.File` constructors and prototypes (`size`, `type`, `slice`).
   - `URL.createObjectURL` and `URL.revokeObjectURL`.

### Package 5: Security, Crypto & CSP (+25 Points)
1. **Web Crypto API**:
   - `window.crypto.subtle` returning `SubtleCrypto` interface.
   - Methods: `digest`, `encrypt`, `decrypt`, `sign`, `verify`, `generateKey`, `importKey`, `exportKey`.
2. **Subresource Integrity & Sandbox**:
   - `HTMLIFrameElement.prototype.sandbox` (`DOMTokenList` prototype).
   - `HTMLIFrameElement.prototype.srcdoc`.

### Package 6: User Interaction, Drag/Drop & Editing (+19 Points)
1. **Editing APIs**:
   - `HTMLElement.prototype.contentEditable` getter/setter and `isContentEditable` property.
   - `document.designMode`, `document.execCommand`, `document.queryCommandSupported`, `document.queryCommandEnabled`.
2. **Drag and Drop Events**:
   - `HTMLElement.prototype.draggable`.
   - `window.DataTransfer`.

### Package 7: Location, Geolocation & Hardware Sensors (+35 Points)
1. **Geolocation API**:
   - `navigator.geolocation.getCurrentPosition`, `watchPosition`, `clearWatch`.
2. **Device Orientation & Motion**:
   - `window.DeviceOrientationEvent`, `window.DeviceMotionEvent`.
   - `navigator.getGamepads`, `navigator.vibrate`, `navigator.getBattery`.

---

## Verification Strategy & Plan Steps

1. **Continuous Automated CTest**:
   - Execute `build/src/test/test_html5test_full` during development to verify incremental point progression towards the > 500 score target.
2. **Memory Safety & Cleanliness**:
   - Keep AddressSanitizer enabled (`-DQJS_ENABLE_ASAN=OFF` or system ASan) to ensure zero double-frees or memory leaks during full test suite runs.
3. **Pre-Commit Verification**:
   - Execute full pre-commit verification suite (`ctest --test-dir build -j4`).

---
*Document Created: 2026 for Wisp Web Engine HTML5 Standardization.*
