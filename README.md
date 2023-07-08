# PB2E

Plazma Burst 2 Expanded Extension ANE (WIP)
Current featues:

- Input fix for Adobe AIR
- Lua scripting support (version 5.4.6)

## Prerequisites

1. make (on Windows, you can get it using [Chocolatey](https://chocolatey.org)

## How to build

1. Set the environment variable **AIR_SDK_HOME** to the root directory of the AIR SDK (the one with bin, lib, etc.)
2. Set the environment variable **PB2EX_DIR** to the location of the PB2_MP-Client repo
3. Open the solution (PB2E.sln) in Visual Studio 2022
4. Compile in Release mode
5. Ctrl + B