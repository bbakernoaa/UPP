# UPP Modernization Phase 3: Data Velocity Design

## 1. Overview
This document outlines the Phase 3 Data Velocity design for the modernization of the Unified Post Processor (UPP). The goal is to integrate the asynchronous, multi-threaded I/O library `amio` from `helm-project` into the C++ driver (`upp_cpp`), demonstrating end-to-end asynchronous NetCDF reading, diagnostic calculation (using Phase 2 kernels), and asynchronous NetCDF writing.

## 2. Architecture & Components

### 2.1 Build & Traversal Setup
The `amio` library must compile and link with our driver:
- **Build traversal:** Update `sorc/CMakeLists.txt` to include `add_subdirectory(helm/libs/amio)`.
- **Linking:** Update `sorc/upp_cpp/CMakeLists.txt` to link the `upp_cpp` target against the shared `amio_core` target.

### 2.2 Manifest Configuration
The I/O behavior will be controlled by YAML manifests that map file paths and variables.
- **`input_manifest.yaml`:**
  ```yaml
  dataset:
    path: "input.nc"
    format: "netcdf"
    variables:
      - name: "t"
        dtype: "float64"
        shape: [2, 2, 3]
      - name: "q"
        dtype: "float64"
        shape: [2, 2, 3]
      - name: "p"
        dtype: "float64"
        shape: [2, 2, 3]
      - name: "sfc_g"
        dtype: "float64"
        shape: [2, 2]
  ```
- **`output_manifest.yaml`:**
  ```yaml
  dataset:
    path: "output.nc"
    format: "netcdf"
    variables:
      - name: "t_isobaric"
        dtype: "float64"
        shape: [2, 2, 1]
      - name: "gh"
        dtype: "float64"
        shape: [2, 2, 3]
      - name: "rh"
        dtype: "float64"
        shape: [2, 2, 2]
  ```

### 2.3 Integrated Asynchronous Driver
The `upp_driver.cpp` will orchestrate the data flow asynchronously:
1. **Initialize Core:** Call `amio_init("input_manifest.yaml", &core)`.
2. **Open Dataset:** Call `amio_open_dataset(core, "input_manifest.yaml", AMIO_MODE_READ, &input_dataset)`.
3. **Asynchronous Reading:** Launch async reads of variables `t`, `q`, `p`, and `sfc_g` directly into memory allocated for `span::FieldView`.
4. **Processing Barrier:** Wait for all async reads to complete successfully.
5. **Kernel Execution:** Invoke `VerticalInterpolator`, `HydrostaticIntegrator`, and `ThermodynamicDiagnostics` sequentially over Kokkos default spaces.
6. **Open Output Dataset:** Call `amio_open_dataset(core, "output_manifest.yaml", AMIO_MODE_WRITE, &output_dataset)`.
7. **Asynchronous Writing:** Enqueue async writes of variables `t_isobaric`, `gh`, and `rh` using the AMIO async API.
8. **Finalize:** Drain and close datasets (`amio_close_dataset`), finalize AMIO (`amio_finalize`), Kokkos, and MPI.

## 3. Testing & End-to-End Verification
To verify compliance and correctness:
- **`tests/generate_test_nc.cpp`:** A standalone C++ executable that creates a sample 3D input NetCDF file (`input.nc`) with realistic temperature, specific humidity, pressure, and geopotential values.
- **CTest Target `verify_data_velocity`:** Coordinates the testing cycle inside the container:
  1. Executes `generate_test_nc.x` to write `input.nc`.
  2. Executes `upp_cpp` in parallel to read inputs, calculate diagnostics, and write `output.nc`.
  3. Verifies that `output.nc` exists and that its variable values are mathematically correct.
