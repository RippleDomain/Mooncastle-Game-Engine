# Mooncastle Engine & Editor

A modern C++ game engine with a WPF-powered editor, a user friendly asset pipeline, and a DirectX 12 renderer featuring Forward+, PBR/IBL, tiled light culling, and a fully scriptable index based entity-component system.

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Rendering](#rendering)
- [Entity Component System (ECS)](#entity-component-system-ecs)
- [Materials, Textures & PBR/IBL](#materials-textures--pbribl)
- [Custom Asset Pipeline & Importing](#custom-asset-pipeline--importing)
- [Lighting](#lighting)
- [Geometry & Mesh Tools](#geometry--mesh-tools)
- [Scripting](#scripting)
- [Editor](#editor)
- [Project System](#project-system)
- [Input](#input)
- [Undo/Redo, Logging & Multiselection Support](#undoredo-logging--multiselection-support)
- [Testing](#testing)
- [Roadmap](#roadmap)

---

## Overview

Mooncastle is a hands-on engine + editor that emphasizes:
- **Fast Iteration:** Project templates, .sln/.vcxproj generation, DXC shader compilation, hot DLL workflows.
- **Modern Eendering:** DirectX 12, swap chain, GPU synchronization, descriptor heap allocation, Forward+, tiled light culling, PBR/IBL, post-processing.
- **Solid and User Friendly Tooling:** WPF editor, content browser, geometry/texture tools, import settings UI, statistics, progress, and an undo/redo system.
- **Scriptable Gameplay and Entities:** A script component system with editor visibility and DLL-based game code integration.
- **Scalable Asset Pipeline:** FBX and texture pipeline (LODs, coalescing meshes), texture compression, channel inspection, cubemap prefiltering.

---

## Key Features

- **DX12 Renderer:** Pipelines, root signatures, constant/structured buffers, render/depth textures, G-Prepass, resource state barriers, post FX.
- **Physically Based Rendering:** Image-based lighting, BRDF LUT/prefiltering, MikkTSpace tangents, textured models, default assets.
- **Forward+ & Tiled Light Culling:** Cullable and non-cullable lights (low/high level), optimized light lists, GPU data paths.
- **Index Based ECS:** Optimzied and intuitive entity-component system where entities are kept track of by their indices.
- **Editor:** Content browser, geometry/texture editors, scene authoring, multi-entity selection/editing, hotkeys, logging, history.
- **Project System:** Create/open/swap projects, templates, validation, persistent scenes, saving/loading, game binary execution.
- **Custom Asset Pipeline:** FBX import (vertices/LODs), texture import/compression, cubemap tools, import configuration & progress tracking.
- **Scripting & ECS:** Script component with editor exposure.

---

## Rendering

- **DX12 Core:** Device/adapter setup, swap chain, command queues/lists, fences, descriptor heap allocator.
- **Pipelines:** Helpers for root signatures/PSOs, **G-Pass** and **depth pre-pass**.
- **Resources:** D3D12 texture class, render/depth textures, vertex/index/constant buffers, correct barriers.
- **Camera:** API-independent camera with a DX12 path, fully tested.
- **Foundations for Forward+:** Clustering/tiling groundwork and debug-ready layout.

---

## Entity Component System (ECS)

- **Index-Based ECS:** Entities identified by indices; efficient storage/lookup.
- **Core Components:** Fully implemented Transform; Vector properties with dedicated editor view.
- **Math & Types:** Unified primitive/math types and entity headers.
- **Tests:** ECS unit tests implemented and **passing**.

---

## Materials, Textures & PBR/IBL

- **PBR Shading:** Validated on multiple models; normal/roughness/metalness/emissive workflows.
- **IBL:** Default sky assets, BRDF LUT, **specular/diffuse prefiltering** with in-editor inspection/toggling.
- **Textures:** Import, save/load, (de)compression (BCn), channel view (R/G/B/A), pan/zoom, block-compressed viewing.
- **Shader Compilation:** DXC-based, supports user shader variations; editor-side material compilation.

---

## Custom Asset Pipeline & Importing

- **FBX SDK Integration:** Vertex data extraction, normals/tangents/UVs/materials, **LODs**, and the ability to **coalesce meshes**.
- **Import UX:** Drag-and-drop FBX and textures, pre-import configurator, reimport with different settings.
- **Progress & Stats:** Per-asset progress tracker, import summaries, and a robust **Asset Registry**.
- **Cubemaps:** Import, inspect faces/mips, prefilter, and toggle between specular/diffuse views with a single click of a button.

---

## Lighting

- **Low- & High-Level Lights:** Authoring layer over GPU data structures.
- **Phong & PBR Paths:** Separate lighting implementations: Phong for debugging; PBR for production.
- **Tiled/Forward+ Culling:** Efficient per-tile/cluster assignment and optimized GPU submissions.
- **Cone-shaped Frustum Culling:** Utilize a cone shaped frustum to maximize performance in well-lit environments.
- **Post-Processing:** Framework ready for effects (bloom/tone-mapping, etc.).

---

## Geometry & Mesh Tools

- **Primitives:** The ability to create UV-mapped **UV sphere**, **Cube** and **Plane** primitives with a dedicated dialog.
- **Geometry Assets:** Serialization + memory packing; highlight/isolate meshes in the **Geometry Detail** view.
- **Export & Upload:** Save mesh files; verified CPU→GPU upload paths and vertex packing improvements.

---

## Scripting

- **Script Component:** Attach/enable/disable scripts per entity.
- **Discovery & UI:** Available scripts listed in the editor for quick authoring.
- **DLL Workflow:** Engine ↔ GameCode DLL infrastructure; build, hot-swap, and run from the editor.

---

## Editor

- **WPF UI Foundation:** reusable controls, consistent styling (dark mode WIP).
- **Content Browser:** folders/thumbnails/search, drag-and-drop importing, stats and progress indicators.
- **Property & Component Views:** vector and transform editors; bug-fixed component panels.
- **Scene Tools:** multi-select & batch edit, entity renaming, add/remove scenes, camera controls in 3D previews.
- **Hotkeys:** undo/redo, save, and common authoring operations.

---

## Project System

- **Create / Open / Swap Projects:** new/existing project dialogs; swap between projects without restarting.
- **Project Templates & Validation:** predefined layouts with project name/path checks.
- **Persistence & Scenes:** load/save projects, initialize active scene on open.
- **Build & Run:** editor can generate `.sln/.vcxproj`, build engine/game DLLs, and run the game binary with the active scene.

---

## Input

- **Input Handling & Binding:** Engine-level abstraction and editor usage; tested in full-scene lighting scenarios with cameras and items.

---

## Undo/Redo, Logging & Multiselection Support

- **Undo/Redo:** Comprehensive history with hotkeys; editor-wide coverage.
- **Logger:** Message panel with **filters**; persistent **history & error log**.
- **Multiselection:** Allows multiselection of items and multiselected item editing.

---

## Testing

- **ECS Tests:** Automated and passing.
- **Renderer Validation:** Model/texture uploads, shader variation rebuilds, startup I/O optimizations confirmed.
- **Hosting Native Windows:** Tested and passing.

---

## Roadmap

- [ ] Add OpenGL and Vulkan support.
- [ ] Add a light mode to the editor.
- [ ] Automate texturing of 3D models.
- [ ] Add Linux support.

---

## Screenshots / Demos

_coming soon_
