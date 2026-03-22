# Specification: Elite Multi-Platform Toolset

This document outlines the detailed functionality and implementation strategy for the next generation of CodeHex tools, transitioning the agent from a C++ specialist to a Full-Stack Elite engineer.

---

## 1. Core Engineering Tools

### 1.1 `LspTool` (Language Server Integration)
- **Goal**: Provide the agent with deep structural context (symbols, types, definitions).
- **Functionality**:
    - `find_definition(symbol)`: Navigates to the source code of any class/function.
    - `find_references(symbol)`: Lists all call sites or usages.
    - `semantic_rename(old, new)`: Safe renaming that handles all includes and namespaces.
- **Implementation**: Connects via JSON-RPC to `clangd` (C++), `ts-server` (TS/JS), or `gopls` (Go).

### 1.2 `QualityGateTool` (Proactive Lints)
- **Goal**: Prevent bugs before build time.
- **Functionality**:
    - `audit_file(path)`: Runs `clang-tidy`, `cppcheck`, or `eslint` on the file.
    - `suggest_fixes()`: Returns a list of automated code corrections.
- **Implementation**: Wraps static analysis CLIs and parses output into JSON results the agent can immediately apply via `SearchReplace`.

---

## 2. Web Development (Elite WebDev)

### 2.1 `BrowserControlTool`
- **Goal**: Allow the agent to "see" and "interact" with the running application.
- **Functionality**:
    - `navigate(url)`: Opens a headless instance.
    - `screenshot(selector)`: Captures specific UI components for visual verification.
    - `inspect_element(selector)`: Returns CSS computed styles and DOM properties.
    - `click/type/scroll`: Full interaction for end-to-end testing.
- **Implementation**: Shared bridge with Playwright/Puppeteer running in a background node process.

### 2.2 `AssetForgeTool`
- **Goal**: Rapid UI prototyping.
- **Functionality**:
    - `generate_icon(description, style)`: Creates SVG or high-res PNG icons.
    - `optimize_asset(path)`: Compresses images or converts to WebP.
- **Implementation**: Integration with DALL-E 3 or local Stable Diffusion nodes.

---

## 3. Game Development (Elite GameDev)

### 3.1 `SceneInspectorTool` (Godot/Unreal Focus)
- **Goal**: Understand the 3D/2D node hierarchy.
- **Functionality**:
    - `dump_tree(scene_path)`: Returns JSON representation of the scene graph.
    - `get_properties(node_id)`: Inspects transforms, signals, and materials.
    - `fix_relationship(parent, child)`: Programmatically adjusts the scene structure.
- **Implementation**: Custom parser for `.tscn` / `.uasset` or a runtime bridge plugin for the engine.

### 3.2 `MathPhysTool`
- **Goal**: Assist with game-specific logic.
- **Functionality**:
    - `calculate_look_at(from, to)`: Returns rotation vectors.
    - `solve_trajectory(velocity, gravity)`: Predicts physics paths for projectile logic.

---

## 4. Mobile Development (Elite Mobile)

### 4.1 `MobileControlTool`
- **Goal**: Automation of the device loop.
- **Functionality**:
    - `boot_emulator(id)`: Launches Android/iOS environments.
    - `deploy_build(path)`: Installs APK/IPA.
    - `set_device_state(rotation, battery, network)`: Simulates edge cases.
- **Implementation**: Wrappers for `adb` and `simctl`.

---

## Conclusion
These tools transform CodeHex from a **text-editor agent** into a **platform-aware engineer**. The first priority for implementation is the **`LspTool`**, as it provides the foundation for all other structural work.
