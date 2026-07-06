# UPP Modernization Phase 1: Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish the foundational C++ orchestration environment by integrating the `bbakernoaa/helm-project` submodule, extending the CMake build system, and creating a modern C++ driver executable (`upp_cpp`) that validates initialization (MPI, Kokkos), asynchronous logging (`helm::LOGS`), boundary/halo operations (`helm::halo`), and dependency resolution (`helm::dagr`).

**Architecture:** Create a new modern C++ build pathway. The entry point is a new driver `upp_driver.cpp` that links against Kokkos, MPI, and the HELM micro-libraries. It executes a verification sequence across these libraries to prove successful system bootstrap.

**Tech Stack:** C++20, Kokkos, MPI, CMake, HELM-Project (helm::logs, helm::halo, helm::dagr).

## Global Constraints
- Target standard C++20 standards.
- Build system must integrate with existing CMake.
- Avoid modifying Fortran files in this phase.
- Ensure strict zero-allocation/zero-copy paths.

---

### Task 1: Git Submodule Integration

**Files:**
- Modify: `.gitmodules`
- Create: `sorc/helm/` (submodule destination)

**Interfaces:**
- Consumes: None
- Produces: Downstream CMake configuration for `helm-project`.

- [ ] **Step 1: Check git status to ensure workspace is clean**
Run: `git status`
Expected: Working tree clean.

- [ ] **Step 2: Add the `helm-project` git submodule**
Run: `git submodule add https://github.com/bbakernoaa/helm-project sorc/helm`
Expected: Successfully added submodule at `sorc/helm`.

- [ ] **Step 3: Initialize and update the submodule**
Run: `git submodule update --init --recursive`
Expected: Submodule files fetched.

- [ ] **Step 4: Verify the directory contains the helm-project code**
Run: `ls -la sorc/helm` (or custom check)
Expected: Directory list shows `CMakeLists.txt`, `libs`, etc.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add .gitmodules sorc/helm
git commit -m "feat: add helm-project git submodule"
```

---

### Task 2: CMake Infrastructure Setup

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `sorc/CMakeLists.txt`
- Create: `sorc/upp_cpp/CMakeLists.txt`

**Interfaces:**
- Consumes: `helm-project` (via `add_subdirectory(helm)`)
- Produces: CMake targets for `upp_cpp` and tests.

- [ ] **Step 1: Modify root `CMakeLists.txt` to find Kokkos and MPI**
Add Kokkos and MPI dependencies. Find line 1 of `/Users/barry/Documents/UPP/CMakeLists.txt` and ensure CXX is enabled, plus find packages.
Modify root `CMakeLists.txt`:
```cmake
# Add CXX to project languages if not already present
project(UPP LANGUAGES Fortran C CXX)

find_package(MPI REQUIRED COMPONENTS CXX)
find_package(Kokkos REQUIRED)
```

- [ ] **Step 2: Modify `sorc/CMakeLists.txt` to traverse into `helm` and `upp_cpp`**
Modify `sorc/CMakeLists.txt` to include:
```cmake
add_subdirectory(helm)
add_subdirectory(upp_cpp)
```

- [ ] **Step 3: Create `sorc/upp_cpp/CMakeLists.txt`**
Create the file with the following content:
```cmake
add_executable(upp_cpp upp_driver.cpp)

target_compile_features(upp_cpp PRIVATE cxx_std_20)

target_link_libraries(upp_cpp
    PRIVATE
        Kokkos::kokkos
        MPI::MPI_CXX
        helm::logs
        helm::halo
        helm::dagr
)
```

- [ ] **Step 4: Run CMake configuration to verify successful configuration**
Run: `cmake -B build -S .`
Expected: Configures successfully, finding Kokkos, MPI, and HELM libraries.

- [ ] **Step 5: Commit CMake changes**
Run:
```bash
git add CMakeLists.txt sorc/CMakeLists.txt sorc/upp_cpp/CMakeLists.txt
git commit -m "build: configure CMake for C++ and HELM"
```

---

### Task 3: C++ Main Driver Skeleton & Logs

**Files:**
- Create: `sorc/upp_cpp/upp_driver.cpp`

**Interfaces:**
- Consumes: `Kokkos::initialize`, `MPI_Init`, `helm::LOGS` API
- Produces: Standalone executable `upp_cpp` with asynchronous logging.

- [ ] **Step 1: Write initial `upp_driver.cpp` skeleton**
Create `sorc/upp_cpp/upp_driver.cpp` with the following code:
```cpp
#include <iostream>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <helm/logs.hpp> // Assuming helm's log header path

int main(int argc, char* argv[]) {
    // 1. Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 2. Initialize Kokkos
    Kokkos::initialize(argc, argv);
    {
        // 3. Initialize helm::LOGS
        helm::logs::initialize();

        helm::logs::info("UPP C++ Driver initialized successfully on rank {} of {}", rank, size);

        // Print Kokkos Configuration
        std::stringstream ss;
        Kokkos::print_configuration(ss);
        helm::logs::info("Kokkos Config:\n{}", ss.str());

        helm::logs::finalize();
    }
    // 4. Finalize Kokkos and MPI
    Kokkos::finalize();
    MPI_Finalize();
    return 0;
}
```

- [ ] **Step 2: Build the executable**
Run: `cmake --build build --target upp_cpp`
Expected: Build succeeds.

- [ ] **Step 3: Run the executable to verify initialization and logging**
Run: `mpirun -np 2 ./build/sorc/upp_cpp/upp_cpp`
Expected: Logs print correctly showing ranks 0 and 1, with the Kokkos configuration details.

- [ ] **Step 4: Commit the driver skeleton**
Run:
```bash
git add sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: implement C++ driver skeleton with Kokkos, MPI, and helm::LOGS"
```

---

### Task 4: Integrate `helm::halo` Boundary Exchange

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`

**Interfaces:**
- Consumes: `helm::halo` library calls for MPI communication over Kokkos memory views.
- Produces: Baseline boundary/halo exchange execution.

- [ ] **Step 1: Add a halo test function to `upp_driver.cpp`**
Modify `sorc/upp_cpp/upp_driver.cpp` to include a mock halo verification step:
```cpp
// Add appropriate include
#include <helm/halo.hpp>

void verify_halo_exchange(int rank) {
    helm::logs::info("[Rank {}] Starting helm::halo verification...", rank);

    // Create a 2D Kokkos View representing local grid domain with halo boundaries
    // local domain: 10x10, with 1-cell halo boundaries on all sides -> 12x12
    Kokkos::View<double**, Kokkos::LayoutLeft> grid("grid_data", 12, 12);

    // Initialize boundaries with dummy values
    Kokkos::parallel_for("init_grid", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0,0}, {12,12}),
        KOKKOS_LAMBDA(const int i, const int j) {
            grid(i, j) = rank * 100.0 + i + j;
        }
    );

    // Set up helm::halo boundary communicator
    helm::halo::Communicator halo_comm;
    // (Assuming standard helm::halo API to define neighbors, shapes, and execute)
    // For Phase 1 validation, initialize and perform a mock boundary synchronize
    halo_comm.setup(grid.extent(0), grid.extent(1), MPI_COMM_WORLD);
    halo_comm.exchange(grid);

    helm::logs::info("[Rank {}] helm::halo exchange completed successfully.", rank);
}
```
*Note: Integrate `verify_halo_exchange(rank);` inside the Kokkos execution block in `main()`.*

- [ ] **Step 2: Rebuild `upp_cpp`**
Run: `cmake --build build --target upp_cpp`
Expected: Compilation succeeds.

- [ ] **Step 3: Execute the parallel driver to verify halo boundaries**
Run: `mpirun -np 2 ./build/sorc/upp_cpp/upp_cpp`
Expected: Output prints the successful execution of `helm::halo exchange`.

- [ ] **Step 4: Commit changes**
Run:
```bash
git add sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: integrate helm::halo verification in C++ driver"
```

---

### Task 5: Integrate `helm::dagr` Dependency Resolution

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`

**Interfaces:**
- Consumes: `helm::dagr` library calls to build a Directed Acyclic Graph.
- Produces: Ordered execution path for diagnostic products.

- [ ] **Step 1: Add a DAGR verification function to `upp_driver.cpp`**
Modify `sorc/upp_cpp/upp_driver.cpp` to define and traverse a mock dependency graph:
```cpp
#include <helm/dagr.hpp>

void verify_dagr_resolution() {
    helm::logs::info("Starting helm::dagr verification...");

    helm::dagr::Graph graph;

    // Define mock diagnostics and their dependencies
    // Reflectivity depends on Temperature and Hydrometeors
    // Temperature depends on Pressure
    graph.add_node("Reflectivity", {"Temperature", "Hydrometeors"});
    graph.add_node("Temperature", {"Pressure"});
    graph.add_node("Hydrometeors", {});
    graph.add_node("Pressure", {});

    // Resolve optimal execution path
    std::vector<std::string> execution_path = graph.resolve();

    helm::logs::info("DAGR Resolved Execution Pathway:");
    for (const auto& node : execution_path) {
        helm::logs::info("  - Execute: {}", node);
    }
}
```
*Note: Call `verify_dagr_resolution();` from the rank-0 block inside `main()`.*

- [ ] **Step 2: Rebuild `upp_cpp`**
Run: `cmake --build build --target upp_cpp`
Expected: Compilation succeeds.

- [ ] **Step 3: Execute driver to verify DAGR output**
Run: `mpirun -np 1 ./build/sorc/upp_cpp/upp_cpp`
Expected: Logs output:
```
DAGR Resolved Execution Pathway:
  - Execute: Pressure
  - Execute: Temperature
  - Execute: Hydrometeors
  - Execute: Reflectivity
```
*(Exact path may vary depending on DAGR internal topological sorting algorithm, but must satisfy dependencies)*

- [ ] **Step 4: Commit changes**
Run:
```bash
git add sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: integrate helm::dagr dependency resolution in driver"
```

---

### Task 6: CTest Automated Integration Test

**Files:**
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `upp_cpp` executable
- Produces: Automated test status reported via CTest.

- [ ] **Step 1: Add the modern C++ driver verification test to CTest**
Modify `tests/CMakeLists.txt` to add:
```cmake
add_test(NAME verify_upp_cpp
         COMMAND mpirun -np 2 $<TARGET_FILE:upp_cpp>)
```

- [ ] **Step 2: Run CMake configure and build**
Run: `cmake -B build -S . && cmake --build build`
Expected: Configuration and build succeed.

- [ ] **Step 3: Execute CTest to run tests**
Run: `ctest --test-dir build --output-on-failure`
Expected: `verify_upp_cpp` test PASSES.

- [ ] **Step 4: Commit tests configuration**
Run:
```bash
git add tests/CMakeLists.txt
git commit -m "test: integrate modern C++ driver test with CTest"
```
