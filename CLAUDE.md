# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build / configure

The build is driven by CMake presets (Ninja + MSVC) with vcpkg as the toolchain. The `vcpkg/` directory is a submodule — initialize it before the first configure.

```bash
git submodule update --init --recursive
cmake --preset windows-msvc-debug          # or windows-msvc-release / windows-msvc-relwithdebinfo
cmake --build build/windows-msvc-debug
ctest --test-dir build/windows-msvc-debug --output-on-failure
```

Run a single doctest test by name:

```bash
./build/windows-msvc-debug/bin/HyluxRuntimeTests --test-case="<name>"
./build/windows-msvc-debug/bin/HyluxRuntimeTests --list-test-cases
```

**Important conventions for dependencies:**
- Third-party libs are **prebuilt only** — never enable build-from-source in vcpkg. Custom overlay-ports live in `vcpkg-registry/overlay-ports/`, the custom triplet in `vcpkg-registry/triplets/x64-windows.cmake`.
- The triplet must whitelist any env var that prebuilt-finder cmake reads — vcpkg scrubs the env otherwise (see `VCPKG_ENV_PASSTHROUGH` in the triplet for `Qt6_DIR`, `NSIGHT_GRAPHICS_SDK_ROOT`, etc.).
- Editing `vcpkg.json` will trigger a full reinstall on the next configure — flag this to the user before changing it.
- Do **not** autonomously run `cmake`, build, or test commands after writing code. Stop after the edit and let the user drive the build.

Optional integrations (auto-detected when SDK env vars / paths exist, also gateable by CMake options): `HYLUX_RHI_VULKAN` (on by default), `HYLUX_CAPTURE_NSIGHT`, `HYLUX_CAPTURE_RENDERDOC`, `HYLUX_CAPTURE_PIX`, `HYLUX_CRASH_AFTERMATH`, `HYLUX_CRASH_DRED`. SIMD backend is picked via `HYLUX_MATH_SIMD_BACKEND` (`AUTO|SSE|NEON|SCALAR|EXT`); console toolchains set `EXT` and point `HYLUX_MATH_SIMD_EXTENSION_HEADER` at a vendor-tuned header so NDA SIMD code stays out of this tree.

## Top-level architecture

Three CMake targets fan out from `Engine/`:

- **`Hylux::Runtime`** (`Engine/Runtime/`, static lib, PCH = `pch.h`) — all engine code that ships in a game.
- **`Hylux::EditorLib`** (`Engine/Editor/EditorLib/`, static lib) — editor-only logic (currently the Slang-based shader compiler / `.hslib` builder). Depends on Runtime + `slang`.
- **`HyluxEditorApp`** (`Engine/Editor/EditorApp/`, Qt6 Quick exe) — the editor shell. Depends on EditorLib, uses KDDockWidgets, and runs `windeployqt` post-build.

Tests under `Engine/Tests/Runtime/` (`HyluxRuntimeTests`) and `Engine/Tests/Editor/` use doctest.

### Runtime composition: Engine + ISubsystem

`Hylux::Engine` (`Engine/Runtime/Engine/Engine.h`) is a **non-singleton** orchestrator — construct one per host (editor, standalone launcher, test). Do not add `Get()` / mutex / global state to it; the main loop is single-threaded.

The runtime is a graph of `ISubsystem` instances registered on the Engine. Each declares its dependencies via `GetDependencies()` (a list of `TypeId`s), and `Engine::Initialize()` resolves init order via Kahn topological sort. Subsystems are looked up at runtime with `engine.GetSubsystem<T>()`. Per-frame `ITickable`s are registered separately and ticked by `Engine::Tick`.

Major subsystems (each in its own `Engine/Runtime/<name>/` directory, each with a `<Name>Subsystem.h/.cpp`):

- **RHI** — abstract graphics interface (`IRHIDevice`, `IRHIQueue`, `IRHICommandList`, …) with a Vulkan backend (`RHI/Vulkan/`). The Vulkan subtree is compiled only when `HYLUX_RHI_VULKAN` is on, and is filtered out of the base glob in `Engine/Runtime/CMakeLists.txt` so unselected backends are never `#include`d. Capture (Nsight) and crash-reporter (Aftermath) backends follow the same opt-in pattern under `RHI/Capture/` and `RHI/Diagnostics/`.
- **RenderGraph** — pass-graph builder (`RGBuilder`, `RGPass`, `RGRasterPass`) compiled and executed against the RHI.
- **Renderer** — `RenderSubsystem` runs on a dedicated render thread. Game-thread API (`SpawnPrimitive`, `Assign*`, `UpdateTransform`, `SubmitFrame`, …) marshals work through a `StructuralCommandQueue` + `TransformDoubleBuffer` + `FrameFenceTimeline`. The render thread drains the queue, builds graphs via `RenderGraph`, and submits to GPU. All public Renderer methods are game-thread only.
- **Shader** — owns the runtime shader archive (`.hslib`), `ShaderModuleCache`, `GlobalShaderMap`, and per-material maps. Editor builds open a loose archive from app-data and support hot-reload; game builds open a cooked archive next to the exe. Shader compilation (Slang → `.hslib`) lives in EditorLib, not here.
- **Asset** — `AssetSubsystem` owns the registry, an LRU + WeakRef cache, and an `IAssetUploader`. Per-type loaders live under `Asset/Loaders/`; cooked `.hass` parsing lives under `Asset/Cooked/`. Public `Load<T>` returns a `Future<Ref<T>>`.

### Core layer (`Engine/Runtime/Core/`)

Foundation utilities used by every subsystem: math (`Mat3/4`, `Quat`, `Vec2/3/4`, `Transform`, `Frustum`) with a pluggable SIMD backend, intrusive ref-counting (`Ref`, `RefCounted`, `WeakRef`, `MakeRef`), frame allocator, containers (`SmallVector`, `RefLruCache`), reflection (`TypeId`, `TypeInfo`, `EnumInfo`, `Reflection.h`), logging (`LogSystem`, `Logger`, async/sync dispatchers, sinks), IO with a `VirtualFileSystem` over `IFileProvider`s, and a `BlobStore` layer for content-addressed cached/cooked data.

### Cross-cutting rules

- **VFS mounts only content roots** (`/Engine/`, `/Game/`, DLC). Shaders and the cook/derived-data cache must **not** mount under the VFS — they live behind their own subsystems (`ShaderSubsystem`, blob stores). Do not extend `IVirtualFileSystem` to cover them.
- **No C# scripting bindings.** The engine does not treat C# as a scripting layer; do not generate C++/C# interop. Tooling (e.g. the editor) may use it directly without bindings.
- Platform / arch defines (`HYLUX_WINDOWS`, `HYLUX_DESKTOP`, `HYLUX_ARCH_X64`, …) come from `cmake/DetectPlatform.cmake`. Use those, not `_WIN32` / `__linux__`, when branching engine code.

## Code style

- C++20, MSVC `/permissive- /W4`, 120-col, 4-space indent, brace-on-newline (`.clang-format`).
- **Naming:** types and methods `PascalCase`, fields `camelCase_` (trailing underscore on members), function parameters `camelCase`.
- **Comments:** Doxygen `///` only, in **English**, only at **file / class / function** level. Do not add comments inside function bodies, on struct fields, or on enum values. File top gets `/// @file @brief …`; each public class and function gets `/// @brief`. The code should be self-explanatory; reserve in-body `//` for non-obvious invariants only.

## Common pitfalls when contributing

- When adding a new RHI/capture/diagnostics backend, mirror the Vulkan / Nsight / Aftermath pattern in `Engine/Runtime/CMakeLists.txt`: add the subtree's regex to the `EXCLUDE REGEX` filter, then add it back inside an `if (HYLUX_<OPTION>)` block. Otherwise unselected SDK headers leak into the base build.
- Aftermath's DLL is copied next to each executable via `hylux_copy_aftermath_dll_next_to(<exe target>)` (defined in `Engine/Runtime/CMakeLists.txt`). Attaching the copy to the static `Hylux::Runtime` would break when `bin/` is cleaned but Runtime stays cached — always attach to the exe.
- Editor exe needs `windeployqt`; release builds rely on `qmldir = Engine/Editor/EditorApp/Resources/qml`.
