# Implementation Plan: Multi-Process Renderer Split (Step 1)

## Overview
This plan details the architectural transition from the current hybrid model to a full Browser/Renderer split. The goal is to move HTML parsing, layout, and remaining JavaScript execution into a low-privilege Renderer process.

## 1. Objectives
- Establish a dedicated `wisp-renderer` process.
- Migrate `libhubbub` (parsing) and core layout logic (`src/content/handlers/html/layout.c`) to the renderer.
- Implement IPC brokering for DOM access and resource loading.
- Separate the "Display List" generation from the final "Plotting" pass.

## 2. Process Responsibilities

### Browser / UI Process (High Privilege)
- **Window Management**: Creating and managing OS windows.
- **Networking**: Fetching resources via `libcurl` (already in `wisp-network`).
- **File I/O**: Saving pages, downloads, cookie storage.
- **Rasterization Compositing**: Atomic blitting of tiles to the screen.
- **Input Handling**: Routing keyboard/mouse events to the correct Renderer.

### Renderer Process (Low Privilege)
- **Parsing**: Hubbub-based HTML/XML parsing.
- **DOM Tree**: Living entirely within the renderer process.
- **Layout**: CSS selection and geometric box tree calculation.
- **JavaScript**: Full QuickJS-ng execution (migrating from `wisp-js` to a unified renderer).
- **Display List Generation**: Converting the box tree into a serializable set of drawing commands.

## 3. Implementation Steps

### Phase A: Renderer Infrastructure
1. Create `src/processes/renderer/main.c`.
2. Define `WISP_IPC_MSG_RENDERER_LOAD_URL` and `WISP_IPC_MSG_RENDERER_LAYOUT_REQ`.
3. Integrate the renderer into the `wisp_subsystem` spawn logic.

### Phase B: Parsing and Layout Migration
1. Move `html_init` and `dom_hubbub_parser_create` calls to the Renderer process.
2. Proxy `hlcache` requests from Renderer to Browser via IPC.
3. Implement a serialized "Display List" format for cross-process painting.

### Phase C: UI Synchronization
1. Browser process receives dirty region notifications from Renderer.
2. Renderer sends serialized display lists for specific tiles.
3. Browser process executes the display list using local plotters (Blend2D/Direct2D).

## 4. Security Goals
- **Renderer Sandboxing**: Once the split is complete, apply OS-level sandboxing (Landlock/AppContainer) to the renderer process.
- **Process Isolation**: Ensure each browser tab corresponds to a unique Renderer process instance (Site Isolation).
