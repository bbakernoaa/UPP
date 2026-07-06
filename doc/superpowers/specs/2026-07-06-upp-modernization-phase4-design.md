# UPP Modernization Phase 4: Orchestration Design

## 1. Overview
This document outlines the Phase 4 Orchestration design for the modernization of the Unified Post Processor (UPP). The goal is to fully unify the I/O, configuration parsing, and regridding/interpolation pathways using HELM shared micro-libraries—specifically `HELM::AMIO`, `HELM::AXIS`, and `HELM::CONF`—achieving a modular C++20 and Kokkos-based post-processing architecture.

## 2. Architecture & Components

### 2.1 Build Traversal & Target Linkage
The build system will be extended to compile the remaining HELM libraries:
- **Build traversal:** Update `sorc/CMakeLists.txt` to include `add_subdirectory(helm/libs/axis)`. Note: `conf` is already included from Phase 3.
- **Linking:** Update `sorc/upp_cpp/CMakeLists.txt` to link the `upp_cpp` target against `HELM::AXIS` (`axis` target) and `HELM::CONF` (`conf` target) in addition to other modules.

### 2.2 Orchestration Lifecycle & Data Flow
The driver `upp_cpp` will execute the post-processing loop according to the following modular sequence:

```
[YAML Config] ──> (CONF: parse) ──> Diagnostic Metadata List
                                             │
                                             ▼
 [input.nc]  ──> (AMIO: read)  ──>  Staging memory views
                                             │
                                             ▼
                                    (AXIS: horizontal regrid) ──> Lambert/Polar Projection
                                                                          │
                                                                          ▼
                                    (AXIS: vertical regrid)   ──> Isobaric Levels
                                                                          │
                                                                          ▼
                                                                  (Diagnostic Kernels)
                                                                          │
                                                                          ▼
 [output.nc] <── (AMIO: write) ── <──────────────────────────────── isobaric results
```

1. **Bootstrap (`HELM::CONF`):**
   - The C++ driver parses the diagnostic configuration YAML file (e.g. `parm/postxconfig-NT-hrrr.yaml`) into a `conf::Config` memory object.
   - It extracts the requested variables, interpolation levels, and mapping parameters.

2. **Ingest (`HELM::AMIO`):**
   - The driver opens the input meteorological NetCDF/GRIB2 dataset.
   - It launches asynchronous reads of raw model fields into `span::FieldView` memory blocks.

3. **Coordinate Interpolation & Regridding (`HELM::AXIS`):**
   - Axis maps the coordinate dimensions (latitude, longitude, or hybrid model surfaces) to target grids.
   - **Horizontal Regridding:** Calling `axis::regrid()` horizontal interpolations are launched inside Kokkos default spaces, interpolating raw input fields directly to the operational output grid (e.g. Lambert Conformal projection) on the fly.
   - **Vertical Regridding:** Utilizing the shared **`HELM::AXIS` vertical regridding algorithms**, model vertical layers are interpolated to target constant pressure/isobaric levels dynamically, replacing any local or duplicate vertical interpolation math kernels.

4. **Vertical Solvers (`upp_kernels`):**
   - Remaining diagnostic kernels (geopotential height, relative humidity) process the regridded and vertically interpolated arrays.

5. **Asynchronous Output (`HELM::AMIO`):**
   - Computed diagnostics are written asynchronously to the output GRIB2/NetCDF files using unified AMIO writing routines.

## 3. Testing & Verification
A dedicated automated CTest integration check `verify_orchestration` will be written at `tests/test_orchestration.cpp`:
- **Task:** Instantiates a small `conf::Config` object, reads a mock coordinate grid, invokes `axis::regrid` to execute a simple horizontal projection interpolation, executes an AXIS vertical regridding run, and writes the output via `amio`, confirming that the three modules (`CONF`, `AMIO`, and `AXIS`) link, initialize, and execute in harmony without any parallel deadlocks or compiler conflicts.
