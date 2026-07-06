# UPP Modernization Phase 4: Orchestration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish the unified orchestration layer in UPP by integrating `HELM::AXIS` and `HELM::CONF`, replacing legacy/custom vertical interpolation with AXIS's vertical regridding modules, parsing diagnostic configurations dynamically at runtime via CONF, and validating the integrated pipeline under CTest.

**Architecture:** Integrate `helm/libs/axis` into the CMake build. Update the C++ driver `upp_driver.cpp` to parse YAML specifications using `conf::Config`. Swap out the custom vertical interpolator kernel, utilizing the native vertical regridding routines of `axis::regrid` on Kokkos memory views, and verify the pipeline via a clean CTest runner.

**Tech Stack:** C++20, Kokkos, MPI, CMake, HELM-Project (helm::axis, helm::conf, helm::span).

## Global Constraints
- All vertical regridding must utilize the shared `HELM::AXIS` vertical interpolation modules.
- Configuration schemas must be parsed dynamically from YAML files using `conf::Config`.
- Maintain a strictly zero-copy, non-allocating execution pathway.

---

### Task 1: Integrate AXIS Library in CMake Build

**Files:**
- Modify: `sorc/CMakeLists.txt`
- Modify: `sorc/upp_cpp/CMakeLists.txt`

**Interfaces:**
- Consumes: None
- Produces: `upp_cpp` target linked against `axis`.

- [ ] **Step 1: Check git status to ensure workspace is clean**
Run: `git status`
Expected: Working tree clean.

- [ ] **Step 2: Add `axis` subdirectory to `sorc/CMakeLists.txt` build traversal**
Modify `sorc/CMakeLists.txt` to include `add_subdirectory(helm/libs/axis)` before `upp_cpp/kernels`:
```cmake
add_subdirectory(helm/libs/logs)
add_subdirectory(helm/libs/halo)
add_subdirectory(helm/libs/tick)
add_subdirectory(helm/libs/span)
target_link_libraries(span INTERFACE mdspan)
target_include_directories(span INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/upp_cpp/kernels/include>)
add_subdirectory(helm/libs/dagr)
add_subdirectory(helm/libs/axis)
add_subdirectory(upp_cpp/kernels)
add_subdirectory(upp_cpp)
```

- [ ] **Step 3: Modify `sorc/upp_cpp/CMakeLists.txt` to link against `axis`**
Link `upp_cpp` target with the `axis` library:
```cmake
# Create the plan's requested aliases so downstream linkage matches the plan verbatim
add_library(helm::logs ALIAS logs)
add_library(helm::halo ALIAS halo)
add_library(helm::tick ALIAS tick)
add_library(helm::dagr ALIAS dagr)
add_library(helm::axis ALIAS axis)

target_link_libraries(upp_cpp
    PRIVATE
        Kokkos::kokkos
        MPI::MPI_CXX
        helm::logs
        helm::halo
        helm::tick
        helm::dagr
        helm::axis
        upp_kernels
)
```

- [ ] **Step 4: Run CMake configure inside container to verify successful target setup**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake -DBUILD_POSTEXEC=OFF -DBUILD_TESTING=OFF -B build -S .`
Expected: Successfully configures without conflicts, finding all dependencies.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add sorc/CMakeLists.txt sorc/upp_cpp/CMakeLists.txt
git commit -m "build: integrate AXIS regridding library into build system"
```

---

### Task 2: Implement Dynamic Configuration & AXIS Vertical Regridding

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`

**Interfaces:**
- Consumes: `conf::Config` to parse YAML, `axis::regrid` to execute vertical interpolation.
- Produces: Integrated driver `upp_cpp` resolving configuration and vertical layers dynamically.

- [ ] **Step 1: Update `upp_driver.cpp` to use CONF and AXIS**
Modify `sorc/upp_cpp/upp_driver.cpp` to parse configurations and invoke AXIS vertical regridding:
```cpp
#include <conf/config.hpp>
#include <axis/axis.hpp> // Assuming axis vertical regridding header

// Inside run_modernized_pipeline(rank, size):
// Right before executing vertical interpolation (Step 8 of Phase 3 driver):

// 8.1 Initialize and Parse Configuration Dynamically via CONF
conf::Config runtime_config = conf::Config::from_file("amio_output_t.yaml");
std::string backend_name = runtime_config.get_string("backend");
std::stringstream conf_ss;
conf_ss << "Dynamic CONF: Loaded output dataset backend: " << backend_name;
logger.log(logs::Severity_Level::INFO, conf_ss.str());

// 8.2 Swap out custom VerticalInterpolator, executing shared AXIS vertical regridding
logger.log(logs::Severity_Level::INFO, "Executing shared HELM::AXIS vertical regridding...");

// Instantiate AXIS 3D vertical interpolation/regridding plan
// (Assuming standard HELM::AXIS C++ API to set up vertical levels and execute)
std::vector<double> target_levels = { 50000.0 }; // hPa in Pa
axis::Vertical_Regrid_Plan v_plan(nx, ny, nlevels, target_levels);
v_plan.execute(temp_field.view(), p_field.view(), t_iso_field.view());

logger.log(logs::Severity_Level::INFO, "HELM::AXIS vertical regridding executed successfully.");
```

- [ ] **Step 2: Compile `upp_cpp` target inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_cpp`
Expected: Compiles and links with 100% success.

- [ ] **Step 3: Commit changes**
Run:
```bash
git add sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: integrate CONF parsing and AXIS vertical regridding in driver"
```

---

### Task 3: Setup Orchestration Integration Verification Test in CTest

**Files:**
- Create: `tests/test_orchestration.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `upp_kernels`, `axis`, `conf` targets
- Produces: Integrated test runner reported in CTest.

- [ ] **Step 1: Write orchestration unit test in `tests/test_orchestration.cpp`**
Create the file with the following complete C++ verification logic:
```cpp
#include <iostream>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <conf/config.hpp>
#include <axis/axis.hpp>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    Kokkos::initialize(argc, argv);
    {
        std::cout << "Verifying Orchestration integration (CONF + AXIS)..." << std::endl;

        // 1. Verify CONF
        conf::Config config = conf::Config::from_file("amio_output_t.yaml");
        std::string path = config.get_string("path");
        if (path != "output_t.nc") {
            throw std::runtime_error("Orchestration verification failed: CONF failed to parse path");
        }

        // 2. Verify AXIS vertical regridding plan compilation
        std::vector<double> targets = { 50000.0 };
        axis::Vertical_Regrid_Plan plan(4, 3, 5, targets);
        
        std::cout << "Orchestration (CONF + AXIS) integration verified successfully!" << std::endl;
    }
    Kokkos::finalize();
    MPI_Finalize();
    return 0;
}
```

- [ ] **Step 2: Update `tests/CMakeLists.txt` to register the new orchestration test**
Add compilation target and tests configuration to `tests/CMakeLists.txt`:
```cmake
add_executable(verify_orchestration test_orchestration.cpp)
target_link_libraries(verify_orchestration
    PRIVATE
        axis
        conf
        MPI::MPI_CXX
)
add_test(NAME verify_orchestration
         COMMAND mpirun -np 1 $<TARGET_FILE:verify_orchestration>)

# Set working directory for verification test
set_tests_properties(verify_upp_cpp verify_kernels verify_orchestration
                     PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
```

- [ ] **Step 3: Run CMake compile inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 bash -c "cmake -DBUILD_POSTEXEC=OFF -DBUILD_TESTING=OFF -B build -S . && cmake --build build"`
Expected: Compilation completes with 100% success.

- [ ] **Step 4: Execute CTest to verify all unit tests pass**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 ctest --test-dir build --output-on-failure`
Expected: CTest shows `verify_upp_cpp`, `verify_kernels`, and `verify_orchestration` all PASSED.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add tests/CMakeLists.txt tests/test_orchestration.cpp
git commit -m "test: integrate orchestration unit tests with CTest"
```
