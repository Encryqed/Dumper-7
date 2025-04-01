# Using CMake with Visual Studio Code and Visual Studio

This guide covers the usage of CMake in both Visual Studio Code and Visual Studio.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Installing CMake](#installing-cmake)
- [Using CMake with Visual Studio Code](#using-cmake-with-visual-studio-code)
- [Using CMake with Visual Studio](#using-cmake-with-visual-studio)
- [Common CMake Commands](#common-cmake-commands)
- [Troubleshooting](#troubleshooting)

## Prerequisites

Before starting with CMake, ensure you have:

- A C/C++ compiler (such as MSVC, GCC, or Clang)
    - **Usually cmake is pre-installed in every Visual Studio**
- Git (optional, but recommended for version control)

## Installing CMake

### Using native installer
1. **Download CMake**:
    - Visit the [official CMake website](https://cmake.org/download/)
    - Download the appropriate installer for your platform
    - For Windows, choose the Windows x64 Installer

2. **Install CMake**:
   - Run the installer
   - Make sure to select "Add CMake to the system PATH" during installation
   - Choose either "Add for all users" or "Add for current user"

3. **Verify Installation**:
   - Open a terminal/command prompt
   - Run `cmake --version`
   - You should see the installed CMake version

### Using Visual Studio installer

1. **Open Visual Studio Installer**:
    - Launch Visual Studio Installer from your desktop or Start menu

2. **Choose your installed product**
    - Choose your installed product (Build Tools or Visual Studio) and press **Modify**
    - Go to the "Individual components" tab

3. **Find and install cmake**
    - Type "cmake" in the search box to quickly find the component
    - Check the box for "C++ CMake tools for Windows" and/or "CMake tools for Windows"
    - Click "Modify" to install the selected components

## Using CMake with Visual Studio Code

### Setup

1. **Install Required Extensions**:
   - [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
   - [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

2. **Configure a Project**:
   - Open a folder containing a CMakeLists.txt file
   - VS Code should automatically detect the CMake project
   - Click on the CMake icon in the sidebar to access CMake tools
   - Select a compiler/kit when prompted


## Using CMake with Visual Studio
1. Open folder in VS
    - Select configure preset
    - Compile (Ctrl+B)

## Troubleshooting

### Common Issues

1. **"CMake not found" Error**:
   - Make sure CMake is in your PATH
   - Restart VS Code/Visual Studio after installing CMake

2. **Compiler Not Found**:
   - Install a C/C++ compiler
   - Make sure the compiler is in your PATH
   - For Visual Studio Code, select a kit manually (F1 > CMake: Select a Kit)

3. **Build Errors**:
   - Check the CMake output panel for detailed error messages
   - Verify that all required libraries are installed
   - Check CMakeLists.txt for errors

4. **Visual Studio CMake Cache Out of Sync**:
   - Delete the build directory and reconfigure
   - Project > Clear CMake Cache

### Getting Help

- CMake Documentation: [https://cmake.org/documentation/](https://cmake.org/documentation/)
- CMake Tools Extension Documentation: [https://vector-of-bool.github.io/docs/vscode-cmake-tools/](https://vector-of-bool.github.io/docs/vscode-cmake-tools/)
- Visual Studio CMake Documentation: [https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)
