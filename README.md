# RL-Simulation

Trying to make a Distributed PPO Reinforcement Learning engine from scratch using C++.

## So far...

- defined CPU jobs
- added workload Generator for simulation
- event engine for scheduler
- simulation for CPU
- FIFO Scheduler
- Metrics

## Next...

- add PPO algorithm



## Setup

### C++ Development Environment

This project uses C++ with MinGW-w64 (GCC) compiler installed via MSYS2.

**Installed Components:**
- GCC 15.2.0 (C++ compiler)
- CMake 4.1.2 (build system generator)
- GDB (debugger)
- Make (build tool)

**Adding MinGW-w64 to PATH:**

To use the C++ compiler from PowerShell/Command Prompt, add MinGW-w64 to your PATH:

**Option 1: Current Session Only**
```powershell
$env:PATH += ";C:\msys64\mingw64\bin"
```

**Option 2: Permanent (System-wide)**
1. Open "Edit the system environment variables"
2. Click "Environment Variables"
3. Under "System variables", select "Path" and click "Edit"
4. Click "New" and add: `C:\msys64\mingw64\bin`
5. Click OK to save

**Verify Installation:**
```powershell
g++ --version
cmake --version
```

**Using MSYS2 Terminal:**
You can also use the MSYS2 MinGW 64-bit terminal (from Start Menu) where the compiler is already in PATH.

### Building with CMake

CMake is installed and ready to use. The installer automatically adds CMake to your system PATH.

**Quick Start:**
```powershell
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

**Note:** CMake should automatically detect MinGW-w64 if it's in your PATH. If you encounter issues, you may need to specify the generator explicitly with `-G "MinGW Makefiles"`.
