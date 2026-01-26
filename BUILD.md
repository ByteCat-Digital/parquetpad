# Building ParquetPad

This guide provides instructions for building the ParquetPad application from source.

## Prerequisites

Before you begin, ensure you have the following software installed:

-   [CMake](https://cmake.org/download/) (version 3.21 or newer)
-   [Conan](https://conan.io/downloads.html) (version 2.x)
-   A C++20 compatible compiler (e.g., Visual Studio on Windows, GCC or Clang on Linux)
-   [Qt6](https://www.qt.io/download-qt-installer) (The open-source version is sufficient)

## 1. Set Environment Variables

You must set the `QT_DIR` environment variable to point to your Qt installation directory. This allows CMake to find the required Qt libraries.

### Windows (PowerShell)

```powershell
$env:QT_DIR = "C:\Qt\6.7.0\msvc2019_64" # Replace with your actual Qt path
```

### Linux / macOS

```sh
export QT_DIR=/path/to/your/Qt/6.x.x/gcc_64
```

## 2. Install Dependencies using Conan

The next step is to download and install the project's dependencies using Conan. We have provided a helper script `conanrun.ps1` to simplify this process.

Alternatively, you can run the `conan install` command manually.

### Using the `conanrun.ps1` script

This script will run `conan install` with the correct parameters for the specified build type.

**Debug Build:**
```powershell
./conanrun.ps1 -BuildType Debug
```

**Release Build:**
```powershell
./conanrun.ps1 -BuildType Release
```

### Manual `conan install`

If you prefer to run the command manually, you can use the following commands.

**Debug Build:**
```powershell
conan install . --output-folder=build/windows-msvc-debug --build=missing -s build_type=Debug
```

**Release Build:**
```powershell
conan install . --output-folder=build/windows-msvc-release --build=missing -s build_type=Release
```

## 3. Configure and Build the Project

Once the dependencies are installed, you can configure and build the project using CMake. We have provided a helper script `conanbuild.ps1` to simplify this process.

### Using the `conanbuild.ps1` script

This script will run `cmake --preset` and `cmake --build` for the specified build type.

**Debug Build:**
```powershell
./conanbuild.ps1 -BuildType Debug
```

**Release Build:**
```powershell
./conanbuild.ps1 -BuildType Release
```

### Manual CMake Commands

If you prefer to run the commands manually, you can use the following commands.

**Debug Build:**
```powershell
# Configure
cmake --preset windows-msvc-debug

# Build
cmake --build --preset windows-msvc-debug
```

**Release Build:**
```powershell
# Configure
cmake --preset windows-msvc-release

# Build
cmake --build --preset windows-msvc-release
```

The final executable will be located in the `build/<preset-name>/` directory (e.g., `build/windows-msvc-debug/`).
