# ParquetPad

A simple Parquet file viewer.

## Building with CMake Presets

This project uses CMake Presets for a portable and consistent build experience.

### Prerequisites

- A C++20 compiler (MSVC on Windows, GCC or Clang on Linux).
- CMake (3.21 or newer).
- A binary installation of the Qt6 libraries.
- A `vcpkg` installation.

### 1. Set Environment Variables

Before configuring, you **must** set the following two environment variables in your shell. This is how the build system finds your local `vcpkg` and `Qt` installations.

- `VCPKG_ROOT`: The absolute path to your vcpkg installation directory.
- `QT_DIR`: The absolute path to your Qt installation directory (e.g., `C:\Qt\6.4.0\msvc2019_64`).

**Linux / macOS:**
```sh
export VCPKG_ROOT=/path/to/your/vcpkg
export QT_DIR=/path/to/your/Qt/6.x.x/gcc_64
```

**Windows (PowerShell):**
```powershell
$env:VCPKG_ROOT = "C:\path\to\your\vcpkg"
$env:QT_DIR = "C:\path\to\your\Qt\6.x.x\msvcxxxx_xx"
```

### 2. Configure CMake

From the project root, select a preset to configure the project. The build files will be generated in the `build/<preset-name>/` directory.

**Example:**
```sh
# To configure for Linux debug
cmake --preset linux-debug
```

### 3. Build the Project

From the project root, build using the corresponding build preset.

**Example:**
```sh
# To build the configured linux-debug preset
cmake --build --preset linux-debug
```

The final executable will be located in the `build/<preset-name>/` directory.
