# UPP Modernization Phase 3: Data Velocity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement an end-to-end asynchronous NetCDF I/O pipeline in `upp_cpp` using the `amio` library. It will read atmospheric inputs asynchronously, dynamically extract grid shapes at runtime using `amio_view_shape()`, execute Phase 2 diagnostic kernels over the dynamic shapes, and write isobaric results asynchronously to disk.

**Architecture:** Integrate `helm/libs/amio` into the build system. Update the C++ driver `upp_driver.cpp` to use AMIO's asynchronous API. Read variables into staging pools, retrieve dimensions dynamically from headers, process them via Kokkos kernels, and stream output fields back to disk in parallel threads.

**Tech Stack:** C++20, Kokkos, MPI, CMake, HELM-Project (helm::amio, helm::span).

## Global Constraints
- Grid shapes must not be hardcoded in the executable; they must be queried dynamically from input files using `amio_view_shape()`.
- Support parallel execution across multiple MPI ranks.
- Maintain a strictly non-allocating, zero-copy computational path.

---

### Task 1: Integrate AMIO Library in CMake Build

**Files:**
- Modify: `sorc/CMakeLists.txt`
- Modify: `sorc/upp_cpp/CMakeLists.txt`

**Interfaces:**
- Consumes: None
- Produces: `upp_cpp` linked dynamically against `amio_core`.

- [ ] **Step 1: Check git status to ensure workspace is clean**
Run: `git status`
Expected: Working tree clean.

- [ ] **Step 2: Add `amio` subdirectory to `sorc/CMakeLists.txt` build traversal**
Modify `sorc/CMakeLists.txt` to include `add_subdirectory(helm/libs/amio)` before kernels:
```cmake
add_subdirectory(helm/libs/logs)
add_subdirectory(helm/libs/halo)
add_subdirectory(helm/libs/tick)
add_subdirectory(helm/libs/span)
target_link_libraries(span INTERFACE mdspan)
target_include_directories(span INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/upp_cpp/kernels/include>)
add_subdirectory(helm/libs/amio)
add_subdirectory(helm/libs/dagr)
add_subdirectory(upp_cpp/kernels)
add_subdirectory(upp_cpp)
```

- [ ] **Step 3: Modify `sorc/upp_cpp/CMakeLists.txt` to link against `amio_core`**
Modify `sorc/upp_cpp/CMakeLists.txt` to include `amio_core`:
```cmake
target_link_libraries(upp_cpp
    PRIVATE
        Kokkos::kokkos
        MPI::MPI_CXX
        helm::logs
        helm::halo
        helm::tick
        helm::dagr
        upp_kernels
        amio_core
)
```

- [ ] **Step 4: Run CMake configuration inside container to verify targets**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake -DBUILD_POSTEXEC=OFF -DBUILD_TESTING=OFF -B build -S .`
Expected: Successful configuration, finding netcdf-c dependencies and compiling `amio_core` successfully.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add sorc/CMakeLists.txt sorc/upp_cpp/CMakeLists.txt
git commit -m "build: integrate AMIO library into build system"
```

---

### Task 2: Implement Asynchronous Input Reading & Dynamic Shape Extraction

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`
- Create: `amio_manifest.yaml`

**Interfaces:**
- Consumes: `amio` APIs (`amio_init`, `amio_read`, `amio_view_shape`, `amio_view_data`)
- Produces: Dynamically extracted grid extents (`nx`, `ny`, `nlevels`) and inputs mapped to `span::FieldView`.

- [ ] **Step 1: Create configuration dataset manifest `amio_manifest.yaml`**
Create `amio_manifest.yaml` in the repository root:
```yaml
dataset:
  path: "input.nc"
  format: "netcdf"
  variables:
    - name: "t"
      dtype: "float64"
    - name: "q"
      dtype: "float64"
    - name: "p"
      dtype: "float64"
    - name: "sfc_g"
      dtype: "float64"
```

- [ ] **Step 2: Update `upp_driver.cpp` to read input dynamically**
Modify `sorc/upp_cpp/upp_driver.cpp` to use the AMIO library to fetch inputs and determine sizes:
```cpp
#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <logs/logs.hpp>
#include <halo/halo.hpp>
#include <amio/amio.h>
#include <span/span.hpp>

// Helper to check AMIO statuses
#define AMIO_CHECK(rc) \
    if ((rc) != AMIO_OK) { \
        std::cerr << "AMIO Error at line " << __LINE__ << ": " << (rc) << std::endl; \
        MPI_Abort(MPI_COMM_WORLD, 1); \
    }

void run_modernized_pipeline(int rank, int size) {
    logs::Logger logger;
    logger.configure_communicator(MPI_COMM_WORLD);
    logger.set_threshold(logs::Severity_Level::DEBUG);

    logger.log(logs::Severity_Level::INFO, "Running Phase 3 Data Velocity pipeline...");

    // 1. Initialize AMIO Core
    amio_core_handle core = nullptr;
    AMIO_CHECK(amio_init("amio_manifest.yaml", &core));

    // 2. Open input dataset
    amio_dataset_handle input_ds = nullptr;
    AMIO_CHECK(amio_open_dataset(core, "amio_manifest.yaml", 0 /* Read mode */, &input_ds));

    // 3. Queue asynchronous reads (assume timestep 0)
    amio_view_handle t_view = nullptr, q_view = nullptr, p_view = nullptr, sfc_view = nullptr;
    AMIO_CHECK(amio_read(input_ds, "t", 0, nullptr, &t_view));
    AMIO_CHECK(amio_read(input_ds, "q", 0, nullptr, &q_view));
    AMIO_CHECK(amio_read(input_ds, "p", 0, nullptr, &p_view));
    AMIO_CHECK(amio_read(input_ds, "sfc_g", 0, nullptr, &sfc_view));

    // 4. Retrieve shapes and extract dimensions dynamically
    amio_shape_t t_shape;
    AMIO_CHECK(amio_view_shape(t_view, &t_shape));

    std::size_t nx = t_shape.extents[0];
    std::size_t ny = t_shape.extents[1];
    std::size_t nlevels = t_shape.extents[2];

    std::stringstream log_ss;
    log_ss << "Dynamically resolved input shape: [" << nx << " x " << ny << " x " << nlevels << "]";
    logger.log(logs::Severity_Level::INFO, log_ss.str());

    // 5. Get data pointers
    const void *t_data = nullptr, *q_data = nullptr, *p_data = nullptr, *sfc_data = nullptr;
    size_t size_bytes = 0;
    AMIO_CHECK(amio_view_data(t_view, &t_data, &size_bytes));
    AMIO_CHECK(amio_view_data(q_view, &q_data, &size_bytes));
    AMIO_CHECK(amio_view_data(p_view, &p_data, &size_bytes));
    AMIO_CHECK(amio_view_data(sfc_view, &sfc_data, &size_bytes));

    // 6. Wrap pointers directly in zero-copy span::FieldView
    std::array<std::size_t, 3> exts_3d = {nx, ny, nlevels};
    span::FieldView<const double, 3> temp_field(static_cast<const double*>(t_data), exts_3d);
    span::FieldView<const double, 3> q_field(static_cast<const double*>(q_data), exts_3d);
    span::FieldView<const double, 3> p_field(static_cast<const double*>(p_data), exts_3d);
    span::FieldView<const double, 2> sfc_field(static_cast<const double*>(sfc_data), {nx, ny});

    // Cleanup read views and close dataset
    AMIO_CHECK(amio_release_view(t_view));
    AMIO_CHECK(amio_release_view(q_view));
    AMIO_CHECK(amio_release_view(p_view));
    AMIO_CHECK(amio_release_view(sfc_view));
    AMIO_CHECK(amio_close_dataset(input_ds));
    AMIO_CHECK(amio_finalize(core));
}
```

- [ ] **Step 3: Compile `upp_cpp` target inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_cpp`
Expected: Compiles with 100% success.

- [ ] **Step 4: Commit changes**
Run:
```bash
git add amio_manifest.yaml sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: implement asynchronous input reading and dynamic shape query"
```

---

### Task 3: Execute Phase 2 Kernels Over Dynamic Input

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`

**Interfaces:**
- Consumes: Dynamically sized input `span::FieldView`s from Task 2.
- Produces: Output diagnostics mapped to local output buffers.

- [ ] **Step 1: Define output buffers and run calculations dynamically in `upp_driver.cpp`**
Modify `sorc/upp_cpp/upp_driver.cpp` to include kernel allocations and executions:
```cpp
#include <kernels/vertical_interpolator.hpp>
#include <kernels/hydrostatic_integrator.hpp>
#include <kernels/thermodynamic_diagnostics.hpp>

// Inside run_modernized_pipeline(rank, size):
// Right after wrapping the pointers in FieldView (Step 6 of Task 2):

// 7. Define output variables with dynamic extents
std::vector<double> out_t_raw(nx * ny * 1, 0.0); // 1 target pressure level
std::vector<double> out_gh_raw(nx * ny * nlevels, 0.0);
std::vector<double> out_rh_raw(nx * ny * nlevels, 0.0);

span::FieldView<double, 3> t_iso_field(out_t_raw.data(), {nx, ny, 1});
span::FieldView<double, 3> gh_field(out_gh_raw.data(), {nx, ny, nlevels});
span::FieldView<double, 3> rh_field(out_rh_raw.data(), {nx, ny, nlevels});

// 8. Execute vertical interpolation to 500 hPa
std::vector<double> target_levels = { 50000.0 }; // hPa in Pa
kernels::VerticalInterpolator interpolator(nx, ny, nlevels);
interpolator.execute(temp_field, p_field, target_levels, t_iso_field);

// 9. Execute geopotential integration
kernels::HydrostaticIntegrator integrator(nx, ny, nlevels);
integrator.execute(temp_field, q_field, p_field, sfc_field, gh_field);

// 10. Execute relative humidity calculation
kernels::ThermodynamicDiagnostics diagnostics(nx, ny, nlevels);
diagnostics.execute(temp_field, q_field, p_field, rh_field);

logger.log(logs::Severity_Level::INFO, "All dynamic diagnostic math kernels executed successfully.");
```

- [ ] **Step 2: Compile `upp_cpp` target inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_cpp`
Expected: Compiles with 100% success.

- [ ] **Step 3: Commit changes**
Run:
```bash
git add sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: execute diagnostic kernels dynamically inside driver"
```

---

### Task 4: Implement Asynchronous Output Writing via AMIO

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`
- Create: `amio_output_manifest.yaml`

**Interfaces:**
- Consumes: Computed diagnostics in `span::FieldView`s.
- Produces: Asynchronous writes to `output.nc` in parallel.

- [ ] **Step 1: Create configuration dataset manifest `amio_output_manifest.yaml`**
Create `amio_output_manifest.yaml` in the repository root:
```yaml
dataset:
  path: "output.nc"
  format: "netcdf"
  variables:
    - name: "t_isobaric"
      dtype: "float64"
    - name: "gh"
      dtype: "float64"
    - name: "rh"
      dtype: "float64"
```

- [ ] **Step 2: Update `upp_driver.cpp` to write out asynchronously**
Modify `sorc/upp_cpp/upp_driver.cpp` to open output dataset and launch async writes:
```cpp
// 11. Open output dataset
amio_dataset_handle output_ds = nullptr;
AMIO_CHECK(amio_open_dataset(core, "amio_output_manifest.yaml", 1 /* Write mode */, &output_ds));

// 12. Create shapes dynamically to pass to AMIO
amio_shape_t t_iso_shape = { 3, {static_cast<int64_t>(nx), static_cast<int64_t>(ny), 1}, {0, 0, 0} };
amio_shape_t gh_shape = { 3, {static_cast<int64_t>(nx), static_cast<int64_t>(ny), static_cast<int64_t>(nlevels)}, {0, 0, 0} };
amio_shape_t rh_shape = { 3, {static_cast<int64_t>(nx), static_cast<int64_t>(ny), static_cast<int64_t>(nlevels)}, {0, 0, 0} };

// 13. Launch async writes
amio_io_handle io1 = nullptr, io2 = nullptr, io3 = nullptr;
AMIO_CHECK(amio_write(output_ds, "t_isobaric", out_t_raw.data(), AMIO_DTYPE_F64, &t_iso_shape, &io1));
AMIO_CHECK(amio_write(output_ds, "gh", out_gh_raw.data(), AMIO_DTYPE_F64, &gh_shape, &io2));
AMIO_CHECK(amio_write(output_ds, "rh", out_rh_raw.data(), AMIO_DTYPE_F64, &rh_shape, &io3));

// 14. Wait for operations and close
AMIO_CHECK(amio_wait(io1, 5000));
AMIO_CHECK(amio_wait(io2, 5000));
AMIO_CHECK(amio_wait(io3, 5000));
AMIO_CHECK(amio_close_dataset(output_ds));

logger.log(logs::Severity_Level::INFO, "All isobaric diagnostic outputs written asynchronously successfully.");
```

- [ ] **Step 3: Compile `upp_cpp` target inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_cpp`
Expected: Compiles with 100% success.

- [ ] **Step 4: Commit changes**
Run:
```bash
git add amio_output_manifest.yaml sorc/upp_cpp/upp_driver.cpp
git commit -m "feat: implement asynchronous outputs writing via AMIO"
```

---

### Task 5: End-to-End Dynamic Integration Verification

**Files:**
- Create: `tests/generate_test_nc.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Produced `output.nc` from `upp_cpp`
- Produces: Full automated execution check reported in CTest.

- [ ] **Step 1: Write standalone test data generator `tests/generate_test_nc.cpp`**
Create the file to dynamically generate a dummy input NetCDF file:
```cpp
#include <iostream>
#include <vector>
#include <netcdf.h>

#define NC_CHECK(err) \
    if ((err) != NC_NOERR) { \
        std::cerr << "NetCDF Error at line " << __LINE__ << ": " << nc_strerror(err) << std::endl; \
        return 1; \
    }

int main() {
    std::cout << "Generating test meteorological data in input.nc..." << std::endl;
    int ncid, x_dim, y_dim, z_dim;
    int t_var, q_var, p_var, sfc_var;

    // Define shapes dynamically: 4 x 4 grid, 5 vertical levels
    int dims_3d[3];
    int dims_2d[2];

    NC_CHECK(nc_create("input.nc", NC_CLOBBER, &ncid));

    NC_CHECK(nc_def_dim(ncid, "x", 4, &x_dim));
    NC_CHECK(nc_def_dim(ncid, "y", 4, &y_dim));
    NC_CHECK(nc_def_dim(ncid, "z", 5, &z_dim));

    dims_3d[0] = x_dim; dims_3d[1] = y_dim; dims_3d[2] = z_dim;
    dims_2d[0] = x_dim; dims_2d[1] = y_dim;

    NC_CHECK(nc_def_var(ncid, "t", NC_DOUBLE, 3, dims_3d, &t_var));
    NC_CHECK(nc_def_var(ncid, "q", NC_DOUBLE, 3, dims_3d, &q_var));
    NC_CHECK(nc_def_var(ncid, "p", NC_DOUBLE, 3, dims_3d, &p_var));
    NC_CHECK(nc_def_var(ncid, "sfc_g", NC_DOUBLE, 2, dims_2d, &sfc_var));

    NC_CHECK(nc_enddef(ncid));

    // Populate data
    std::vector<double> t_data(4 * 4 * 5, 280.0);
    std::vector<double> q_data(4 * 4 * 5, 0.005);
    std::vector<double> p_data = {
        100000.0, 100000.0, 100000.0, 100000.0,
        80000.0,  80000.0,  80000.0,  80000.0,
        60000.0,  60000.0,  60000.0,  60000.0,
        40000.0,  40000.0,  40000.0,  40000.0,
        10000.0,  10000.0,  10000.0,  10000.0
    };
    std::vector<double> p_data_full;
    for (int k = 0; k < 16; ++k) {
        for (int l = 0; l < 5; ++l) {
            p_data_full.push_back(p_data[l]);
        }
    }
    std::vector<double> sfc_data(16, 0.0);

    NC_CHECK(nc_put_var_double(ncid, t_var, t_data.data()));
    NC_CHECK(nc_put_var_double(ncid, q_var, q_data.data()));
    NC_CHECK(nc_put_var_double(ncid, p_var, p_data_full.data()));
    NC_CHECK(nc_put_var_double(ncid, sfc_var, sfc_data.data()));

    NC_CHECK(nc_close(ncid));
    std::cout << "input.nc successfully generated." << std::endl;
    return 0;
}
```

- [ ] **Step 2: Update `tests/CMakeLists.txt` to include verification runner**
Modify `tests/CMakeLists.txt` to define the test target and register with CTest:
```cmake
add_executable(generate_test_nc generate_test_nc.cpp)
target_link_libraries(generate_test_nc PRIVATE NetCDF::NetCDF_C)

# CTest integration coordinating: generate -> process -> verify
add_test(NAME verify_data_velocity
         COMMAND bash -c "$<TARGET_FILE:generate_test_nc> && mpirun -np 2 $<TARGET_FILE:upp_cpp>")
```

- [ ] **Step 3: Run CMake compile inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 bash -c "cmake -DBUILD_POSTEXEC=OFF -DBUILD_TESTING=OFF -B build -S . && cmake --build build"`
Expected: Compiles with 100% success.

- [ ] **Step 4: Execute CTest inside container to run end-to-end check**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 ctest --test-dir build --output-on-failure -R verify_data_velocity`
Expected: CTest shows `verify_data_velocity` PASSED.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add tests/CMakeLists.txt tests/generate_test_nc.cpp
git commit -m "test: integrate data velocity verification in CTest"
```
