# Supremacy Reborn

A modernized C++20 refactoring of the legacy 2018 CS:GO codebase, reorganized into clean modular subsystems for educational research and legacy build preservation.

## Project Structure

The project has been organized into modular subsystems:

- **[`core/`](core/)**: Core client orchestration, entrypoint (`entry.cpp`), global state (`csgo.cpp`), master includes (`includes.h`), and feature implementations.
- **[`sdk/`](sdk/)**: Source Engine SDK structures, interfaces, mathematical libraries (`mathlib.h`), entity definitions, and engine memory allocators.
- **[`hooks/`](hooks/)**: Virtual Method Table (VMT) hooks, Windows procedure handlers (`wndproc.cpp`), and client/engine detours.
- **[`menu/`](menu/)**: GUI engine, interactive widgets (checkboxes, sliders, dropdowns, color pickers), and configuration system.
- **[`util/`](util/)**: Memory pattern scanning, hashing, string encryption (`xorstr.h`), and third-party libraries (`json.h`, `tinyformat.h`, `base64.h`).

## Build Requirements

- **Toolchain**: Visual Studio 2022 / 2026 Build Tools (v143 or v145 toolset)
- **Language Standard**: ISO C++20 Standard (`/std:c++20`)
- **Target Configuration**: `Release | x86` (Win32 dynamic library)
- **Windows SDK**: Windows 10 / 11 SDK (10.0+)

### Building from Command Line (MSBuild)

Run from a Developer Command Prompt or PowerShell:

```bash
msbuild supremacy_csgo.sln -p:Configuration=Release -p:Platform=x86
```

The compiled binary will be placed at:
```text
output/Release/csgocheat.dll
```

## Changelog & Modernization Notes

### Version 2.0
- **Modern C++20 Compliance**:
  - Upgraded compiler language standard to `/std:c++20`.
  - Replaced legacy allocator calls with `std::allocator_traits` in third-party and container utilities.
  - Resolved C++20 keyword conflicts (renamed reserved `default` identifier and `xor` namespace).
  - Fixed CRT `<cmath>` collision by renaming project math header to `mathlib.h`.
  - Enforced two-phase template lookup compliance for `CUtlVector` / `IMemAlloc`.
  - Fixed union initialization and reference-to-temporary bugs.
- **Modular Subsystem Refactoring**:
  - Migrated 145 flat source and header files into categorized subsystem folders (`core`, `sdk`, `hooks`, `menu`, `util`).
  - Preserved complete Git tracking and rename history.
  - Configured project include paths to guarantee clean resolution across all translation units.
