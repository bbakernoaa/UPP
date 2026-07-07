# UPP Modernization Phase 5: Synthesis & Operational Handoff Design

## 1. Overview
This document outlines the design for Phase 5, which bridges the modernized C++ driver with the legacy Fortran codebase. We will implement a **Zero-Copy Fortran Pointer Architecture** to allow the C++ Kokkos/AMIO memory allocations to act as the primary backing store for legacy Fortran diagnostics without deep copying arrays.

## 2. Architecture & Components

### 2.1 Modifying Legacy Fortran Modules
To achieve zero-copy integration, the target global variable arrays inside `sorc/ncep_post.fd/VRBLS3D_mod.f` will be updated:
- Change declarations from `real, allocatable :: t(:,:,:)` to `real, pointer :: t(:,:,:)`.
- Do the same for `q` and `pmid` (pressure).
- This allows the variables to be safely associated with external C-pointers natively inside Fortran without allocating new memory blocks.

### 2.2 The C-Interop Bridge Layer (`upp_c_bridge.f90`)
A new Fortran module `upp_c_bridge.f90` will be introduced in the `ncep_post.fd` library.
- **Function:** `subroutine upp_handoff_to_fortran(t_ptr, q_ptr, p_ptr, nx, ny, nz) bind(C, name="upp_handoff_to_fortran")`
- **Arguments:** It accepts raw `type(c_ptr), value` arguments representing the Kokkos memory blocks allocated by the C++ driver.
- **Mapping:** It uses the intrinsic `c_f_pointer` to safely cast the 1D C memory pointers into 3D Fortran explicit-shape pointers, mapping them directly onto the global `vrbls3d` pointers (e.g., `t`, `q`, `pmid`).

### 2.3 Legacy Execution (`PROCESS`)
- After mapping the variables, the bridge routine invokes `SUBROUTINE PROCESS(kth, kpv, th, pv, iostatusD3D)` using dummy parameters or extracting the expected dimensions.
- `PROCESS` will then seamlessly run its hundreds of legacy diagnostics on the modern Kokkos memory blocks.

### 2.4 C++ Driver Integration
- In `upp_driver.cpp`, we will define `extern "C" void upp_handoff_to_fortran(const double* t, const double* q, const double* p, int nx, int ny, int nz);`.
- The driver will execute the `HELM::AXIS` vertical regridding and C++ Kokkos kernels as implemented in Phase 4.
- Following the C++ kernels, the driver will call `upp_handoff_to_fortran(...)`, passing `t_data`, `q_data`, and `p_data` directly.

## 3. Build System Modifications
- The `CMakeLists.txt` for `ncep_post.fd` must compile `upp_c_bridge.f90` into the `upp` static library.
- The driver `upp_cpp` target will link against `upp` to resolve the `upp_handoff_to_fortran` C-binding symbol.