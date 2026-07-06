# UPP Modernization Phase 1: Foundation Design

## 1. Overview
This document outlines the Phase 1 Foundation for the modernization of the Unified Post Processor (UPP). The goal is to establish the C++ orchestration framework by integrating the `bbakernoaa/helm-project` and creating a new C++ entry point, transitioning away from a monolithic Fortran application.

## 2. Architecture & Components

### 2.1 Git Submodule Integration
The `helm-project` repository will be integrated into the UPP codebase as a git submodule to ensure offline availability and stability on NOAA's RDHPCS platforms.
- **Path:** `sorc/helm`
- **Source URL:** `https://github.com/bbakernoaa/helm-project`

### 2.2 C++ Main Driver
A new C++ executable will be created to serve as the modernized entry point for UPP.
- **Location:** `sorc/upp_cpp/upp_driver.cpp`
- **Responsibilities:**
  - Initialize the MPI execution environment.
  - Initialize the Kokkos hardware abstraction layer (setting up execution and memory spaces).
  - Initialize `helm::LOGS` for asynchronous, rank-aware logging.
  - **Leverage `helm-project/libs/halo` (`helm::halo`)** to initialize and execute a baseline boundary/halo exchange. This ensures that the parallel communication infrastructure is active and can coordinate across multi-dimensional grids over Kokkos memory spaces.
  - **Leverage `helm-project/libs/dagr` (`helm::dagr`)** to define a mock Directed Acyclic Graph (DAG) of diagnostics. This will demonstrate automated dependency resolution (e.g., resolving that `Reflectivity` depends on `Temperature` and `Hydrometeors`) and construct an optimal execution pathway.
  - Gracefully finalize Kokkos and MPI upon completion.

### 2.3 Build System (CMake) Modifications
The existing CMake infrastructure will be extended to support the new C++ components alongside the legacy Fortran codebase.
- **Root `CMakeLists.txt`:** Will require Kokkos (via `find_package(Kokkos REQUIRED)`) and ensure MPI is available for CXX.
- **`sorc/CMakeLists.txt`:** Will be updated to include the new `helm` subdirectory and the new C++ driver directory.
- **Target:** A new executable target named `upp_cpp` will be defined, linking against `Kokkos::kokkos`, `MPI::MPI_CXX`, and the `helm` libraries (specifically `logs`, `halo`, and `dagr`).

## 3. Data Flow and Execution
For this foundational phase, the data flow is strictly related to initialization and baseline validation:
1. The user or batch script executes `upp_cpp`.
2. The driver invokes `MPI_Init` and `Kokkos::initialize`.
3. The driver invokes `helm::LOGS` message to standard output to confirm successful startup and hardware detection.
4. The driver sets up a dummy grid allocation and executes a baseline halo exchange using **`helm::halo`** to verify that asynchronous, GPU-aware, or host-to-host boundaries can be exchanged securely across MPI ranks.
5. The driver constructs a mock diagnostic dependency graph using **`helm::dagr`** to automatically resolve execution ordering of dummy tasks. It executes this mock sequence to verify that the DAGR engine operates correctly.
6. The driver finalizes Kokkos and MPI, then exits with a success code.

## 4. Testing & Verification
- **Compilation:** The project must configure and build successfully using the provided `spack-stack` environment on standard platforms (e.g., MacOS/Linux).
- **Execution:** Running `upp_cpp` in parallel (e.g., via `mpirun`) should:
  - Succeed and output the expected initialization logs.
  - Perform the baseline halo boundary exchange.
  - Resolve and print the execution path of the mock dependency DAG via DAGR.

## 5. Scope Boundaries
This phase explicitly **excludes**:
- Porting any actual Fortran scientific algorithms (physics, interpolation).
- Setting up the `helm::CONF` YAML/JSON parser for parsing real production control files.
- Connecting the C++ driver to the legacy Fortran codebase.
Those tasks will be addressed in subsequent modernization phases.
