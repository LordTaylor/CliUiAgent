# CI/CD & Automated Releases 🚀

CodeHex uses a professional automated release pipeline powered by GitHub Actions. This ensuring that every stable version is built and packaged for all major platforms (Windows, macOS, Linux) with 100% consistency.

## How it Works
When a project maintainer creates a **GitHub Release** or pushes a version tag (e.g., `v0.2.0`), the system automatically triggers a multi-platform build.

### Automated Artifacts
The following installers are automatically generated and attached to the release:
- **macOS**: `.dmg` Disk Image (with bundled frameworks and app icon).
- **Windows**: `.exe` Installer (created via NSIS).
- **Linux**: `.AppImage` (portable executable).

## Local Packaging
If you want to package the application locally, use the scripts in the `build-scripts/` directory:
- `package-macos.sh`
- `package-windows.ps1`
- `package-linux.sh`

> [!NOTE]
> These scripts now support the `QT_DIR` environment variable, allowing you to specify a custom Qt installation path for the packaging process.

## App Icon
CodeHex features a high-fidelity application icon integrated into the build process. On macOS, this is a native `.icns` bundle generated from the project's premium iconset.
