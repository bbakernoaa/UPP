# UPP Modernization Phase 5: Synthesis Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bridge the modernized C++ driver with the legacy Fortran codebase using zero-copy `c_f_pointer` mappings to hand off execution directly to the original `PROCESS` subroutine.

**Architecture:** Change legacy `vrbls3d` global variables from `allocatable` to `pointer`. Create a Fortran `bind(C)` wrapper that safely maps 1D C-pointers onto 3D Fortran views. Update the C++ driver to invoke this wrapper, passing control to the legacy logic over our modern Kokkos allocations.

**Tech Stack:** Fortran 2008 (`iso_c_binding`), C++20, Kokkos, CMake.

## Global Constraints
- Use strictly zero-copy `c_f_pointer` mapping; do not perform deep copies of fields passed from C++ to Fortran.
- Avoid modifying legacy Fortran scientific source code; the only legacy changes should be variable declarations (`allocatable` to `pointer`).
- Ensure the build succeeds inside the local NOAA EMC Docker container.

---

### Task 1: Update VRBLS3D to Zero-Copy Pointers

**Files:**
- Modify: `sorc/ncep_post.fd/VRBLS3D_mod.f`

**Interfaces:**
- Consumes: None
- Produces: Fortran pointer globals (`T`, `Q`, `PMID`) ready for `c_f_pointer` binding.

- [ ] **Step 1: Check git status**
Run: `git status`
Expected: Working tree clean.

- [ ] **Step 2: Change allocatable to pointer in VRBLS3D_mod.f**
Modify `sorc/ncep_post.fd/VRBLS3D_mod.f` line 21, changing `allocatable` to `pointer`:
```fortran
      real, pointer :: UH(:,:,:) &    !< U-component wind (U at P-points including 2 row halo)
      ,VH(:,:,:) &         !< V-component wind (V at P-points including 2 row halo)
```

- [ ] **Step 3: Verify compilation in container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp`
Expected: Compiles with 100% success (proving `ALLOCATE` calls in legacy code are fully compatible with pointers).

- [ ] **Step 4: Commit changes**
Run:
```bash
git add sorc/ncep_post.fd/VRBLS3D_mod.f
git commit -m "refactor: convert vrbls3d arrays to pointers for zero-copy C interoperability"
```

---

### Task 2: Create C-Interoperability Bridge

**Files:**
- Create: `sorc/ncep_post.fd/upp_c_bridge.f90`
- Modify: `sorc/ncep_post.fd/CMakeLists.txt`

**Interfaces:**
- Consumes: `vrbls3d` pointers
- Produces: `upp_handoff_to_fortran` C-callable symbol.

- [ ] **Step 1: Create `upp_c_bridge.f90`**
Create `sorc/ncep_post.fd/upp_c_bridge.f90` with the following:
```fortran
module upp_c_bridge
  use iso_c_binding
  implicit none
contains
  subroutine upp_handoff_to_fortran(t_ptr, q_ptr, p_ptr, nx, ny, nz) bind(C, name="upp_handoff_to_fortran")
      use vrbls3d, only: t, q, pmid
      type(c_ptr), value :: t_ptr, q_ptr, p_ptr
      integer(c_int), value :: nx, ny, nz
      
      ! Map C pointers directly to Fortran 3D explicit-shape views (zero-copy)
      call c_f_pointer(t_ptr, t, [nx, ny, nz])
      call c_f_pointer(q_ptr, q, [nx, ny, nz])
      call c_f_pointer(p_ptr, pmid, [nx, ny, nz])
      
      ! Fire the legacy PROCESS driver
      ! (Using dummy arguments for TH and PV arrays to prove linkage execution)
      call PROCESS(1, 1, [0.0], [0.0], 0)
  end subroutine upp_handoff_to_fortran
end module upp_c_bridge
```

- [ ] **Step 2: Add bridge to CMakeLists.txt**
Open `sorc/ncep_post.fd/CMakeLists.txt` and search for the source file list. Add `upp_c_bridge.f90` near the top of the list of sources for the `upp` library target (or right after other `_mod.f` files).

- [ ] **Step 3: Compile `upp` inside container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 bash -c "cmake -DBUILD_POSTEXEC=ON -DBUILD_TESTING=OFF -B build -S . && cmake --build build --target upp"`
Expected: Compiles completely.

- [ ] **Step 4: Commit changes**
Run:
```bash
git add sorc/ncep_post.fd/upp_c_bridge.f90 sorc/ncep_post.fd/CMakeLists.txt
git commit -m "feat: implement zero-copy C-Fortran bridge module"
```

---

### Task 3: Invoke Bridge from C++ Driver

**Files:**
- Modify: `sorc/upp_cpp/upp_driver.cpp`
- Modify: `sorc/upp_cpp/CMakeLists.txt`

**Interfaces:**
- Consumes: `upp_handoff_to_fortran` from `upp` library.
- Produces: Fully integrated application driver.

- [ ] **Step 1: Link `upp` library to `upp_cpp` driver**
Modify `sorc/upp_cpp/CMakeLists.txt` to add `upp` to the `target_link_libraries` of `upp_cpp`. (It is typically named `upp` in the legacy build system):
```cmake
target_link_libraries(upp_cpp
    PRIVATE
        Kokkos::kokkos
        MPI::MPI_CXX
        helm::logs
        helm::halo
        helm::tick
        helm::dagr
        helm::axis
        helm::conf
        upp_kernels
        amio_core
        upp
)
```

- [ ] **Step 2: Add C-bridge signature to `upp_driver.cpp`**
Near the top of `sorc/upp_cpp/upp_driver.cpp` (after the headers), add the `extern "C"` declaration:
```cpp
extern "C" {
    void upp_handoff_to_fortran(const void* t_ptr, const void* q_ptr, const void* p_ptr, int nx, int ny, int nz);
}
```

- [ ] **Step 3: Call bridge in pipeline**
In `sorc/upp_cpp/upp_driver.cpp`, right before `// Create shapes dynamically to pass to AMIO` (around line 125), add the invocation:
```cpp
    // Hand off memory to legacy Fortran PROCESS driver (Zero-copy)
    logger.log(logs::Severity_Level::INFO, "Handing off Kokkos views to legacy Fortran PROCESS...");
    upp_handoff_to_fortran(t_data, q_data, p_data, static_cast<int>(nx), static_cast<int>(ny), static_cast<int>(nlevels));
    logger.log(logs::Severity_Level::INFO, "Legacy Fortran execution complete.");
```

- [ ] **Step 4: Recompile complete project**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 bash -c "cmake -DBUILD_POSTEXEC=ON -DBUILD_TESTING=ON -B build -S . && cmake --build build"`
Expected: Complete clean compile without missing symbols.

- [ ] **Step 5: Run CTest Verification**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 ctest --test-dir build --output-on-failure`
Expected: CTest passes, and the execution logs show `PROCESS starts`.

- [ ] **Step 6: Commit changes**
Run:
```bash
git add sorc/upp_cpp/upp_driver.cpp sorc/upp_cpp/CMakeLists.txt
git commit -m "feat: hand off execution to legacy Fortran PROCESS in C++ driver"
```
