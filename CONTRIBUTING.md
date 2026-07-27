# Contributing to SysCore

Thank you for your interest in contributing to **SysCore**! We welcome contributions, bug reports, and feature requests.

## How to Contribute

1. **Fork the Repository**: Create a fork of `SysCore` on GitHub.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. **Write Clean C++20 Code**:
   - Follow standard RAII patterns.
   - Run `.clang-format` before committing.
   - Ensure CMake builds without warnings using MSVC (`/std:c++20 /W4`).
4. **Test Your Changes**: Verify that `SystemCoreApp.exe` executes cleanly.
5. **Open a Pull Request**: Submit your PR with a descriptive title and summary of changes.

## Code Standards
- Enforce strict move-only semantics for RAII resource wrappers.
- Maintain thread-safety using `std::mutex` or atomic operations.
- Avoid raw pointers unless managing explicit C-factory dynamic API boundaries.
