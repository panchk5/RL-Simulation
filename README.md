# RL-Simulation

Making a Distributed PPO Reinforcement Learning engine from scratch using C++.

## So far...

- defined CPU jobs
- added workload Generator for simulation
- event engine for scheduler
- simulation for CPU
- FIFO Scheduler
- Metrics
- PPO Algorithm (Simple layer (forward fetch))
- Training System (Parallel Agents, Episodes, Shared Policy)

## Next...

- Optimize PPO algorithm
- Distribute runners are run in real-time simulation for many minutes

## Training & Evaluation (Quick Commands)

Build and run the trainer (produces `ppo_model.txt`):

```bash
g++ -std=c++17 -O2 -I. \
	Scheduler/Agent/train_ppo.cpp Scheduler/Agent/ppo_agent.cpp Scheduler/Agent/agent.cpp \
	Scheduler/FIFO/fifo.cpp Environment/Simulation/simulation.cpp \
	Environment/Event/event.cpp Environment/Jobs/workloadGen.cpp -o train_ppo

./train_ppo
```

Build and run the FIFO vs PPO comparison test (loads `ppo_model.txt` if present):

```bash
g++ -std=c++17 -O2 -I. \
	Scheduler/Scheduler_Tests/test_ppo_vs_fifo.cpp Scheduler/Agent/ppo_agent.cpp Scheduler/Agent/agent.cpp \
	Scheduler/FIFO/fifo.cpp Environment/Simulation/simulation.cpp \
	Environment/Event/event.cpp Environment/Jobs/workloadGen.cpp -o test_ppo_vs_fifo

./test_ppo_vs_fifo
```

Notes:
- If `ppo_model.txt` exists it will be loaded for the "trained" run; otherwise the test runs an untrained agent for comparison.
- On macOS/linux use the commands above. On Windows use an equivalent build command (MSYS2/MinGW recommended).



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
