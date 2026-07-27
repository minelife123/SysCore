# SysCore

C++20 system diagnostics utility and framework for Windows.

## Quick Run
```bash
npx syscore-gui
```

## Features
- **GUI Dashboard**: ImGui desktop interface (DirectX 11 backend) with English, Russian, and Chinese localization.
- **Process Audit**: Process tree viewer, working set RAM monitoring, memory protection scanner (W^X), process memory dump creator (.dmp), and loaded DLL module inspector.
- **System Diagnostics**: Windows Autoruns registry inspector (`HKCU\Run` / `HKLM\Run`), loaded kernel drivers viewer (`EnumDeviceDrivers`), active socket monitor (`GetExtendedTcpTable`), and file integrity hashing (MD5 / SHA-256).
- **Extensibility**: Embedded Lua scripting engine and dynamic C++ DLL module loader.

## Building from Source

### Requirements
- Windows 10 / 11 (x64)
- Visual Studio 2022 (C++ Desktop Workload)
- CMake 3.20+

### Build & Run
```bash
cmake -B build
cmake --build build --config Release

# Launch GUI app
.\build\Release\SysCoreGuiApp.exe

# Run unit tests
.\build\Release\SysCoreUnitTestApp.exe
```

## License
MIT
