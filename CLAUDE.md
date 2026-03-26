# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Remere's Map Editor (RME) — a C++20 map editor for OpenTibia (open-source Tibia MMORPG). Uses wxWidgets for GUI and OpenGL for rendering. Supports Tibia client versions 7.40 through 10.100+.

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j$(nproc)
```

Dependencies: wxWidgets (>=3.0), Boost (>=1.66: date_time, system, filesystem, iostreams), OpenGL, GLUT, ZLIB, fmt, nlohmann_json. PugiXML is bundled in `source/ext/`.

There are no automated tests.

## Code Style

Per `.editorconfig`: tabs (size 4) for C++/H files, K&R brace style, UTF-8, LF line endings, no trailing whitespace. Platform-specific code guarded by `#ifdef __WINDOWS__` / `#ifdef __VISUALC__`.

## Architecture

All source lives in `source/` (flat structure, ~200 files). Key subsystems:

- **Editor core**: `editor.h/cpp` manages maps, selections, undo/redo. `action.h/cpp` implements the undo/redo action queue.
- **Map data model**: `map.h/cpp` (container with towns, houses, spawns, waypoints), `basemap.h/cpp` (base map + tile storage), `tile.h/cpp` (ground/items/creatures per tile), `item.h/cpp` + `items.h/cpp` (item types and attributes).
- **Brush system**: `brush.h/cpp` is the base interface. Specialized brushes: `ground_brush`, `wall_brush`, `doodad_brush`, `creature_brush`, `spawn_brush`, `house_brush`, `carpet_brush`, `table_brush`, `raw_brush`, `eraser_brush`, `waypoint_brush`. `brush_tables.cpp` has auto-border lookup tables.
- **GUI**: `application.h/cpp` (wxApp + MainFrame), `gui.h/cpp` (main GUI construction), `main_menubar.h/cpp`, `main_toolbar.h/cpp`. Palettes (`palette_*.h/cpp`) provide brush selection panels. Various `*_window.h/cpp` dialogs.
- **Rendering**: `map_display.h/cpp` (viewport), `map_drawer.h/cpp` (draw logic), `graphics.h/cpp` (OpenGL rendering), `light_drawer.h/cpp`.
- **File I/O**: `iomap_otbm.h/cpp` (OTBM binary map format — primary format), `iominimap.h/cpp` (minimap export). `filehandle.h/cpp` handles DAT file reading.
- **Live editing** (collaborative): `live_server.h/cpp`, `live_client.h/cpp`, `live_socket.h/cpp`, `live_peer.h/cpp`, `live_action.h/cpp`.
- **Client versions**: `client_version.h/cpp` manages version-specific data. `data/clients.xml` defines OTB version mappings. Per-version data in `data/740/`, `data/760/`, `data/854/`, etc.
- **Extensions**: XML-based user content in `extensions/` (creatures, items, brushes). Loaded via `extension.h/cpp`.
- **Utilities**: `common.h/cpp` (string helpers, clipboard), `settings.h/cpp` (persistence), `copybuffer.h/cpp` (copy/paste), `selection.h/cpp` (marquee selection), `materials.h/cpp` (material database).

`definitions.h` has compile-time feature flags (e.g., `OTGZ_SUPPORT`). `main.h` is the common precompiled header.
