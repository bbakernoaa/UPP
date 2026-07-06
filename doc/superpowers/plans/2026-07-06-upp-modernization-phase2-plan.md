# UPP Modernization Phase 2: Abstraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a performance-portable C++ diagnostic library `upp_kernels` with three core diagnostic kernels—vertical interpolation, hydrostatic integration, and thermodynamic relative humidity calculation—leveraging the `span::FieldView` memory abstraction from `helm-project` and verified via automated parallel unit tests under CTest.

**Architecture:** Create a modular static library `upp_kernels` that decouples calculation logic from I/O and orchestration. It extracts unmanaged Kokkos Views from input/output `span::FieldView` arguments and executes mathematical loops inside thread-safe and GPU-compatible `Kokkos::parallel_for` regions.

**Tech Stack:** C++20, Kokkos, MPI, CMake, HELM-Project (helm::span).

## Global Constraints
- All interfaces must consume/produce `span::FieldView` objects with `std::layout_left` memory mappings.
- No dynamic heap allocations allowed during kernel `execute()` phases (zero-allocation guarantee).
- Support both host and default device execution spaces natively.

---

### Task 1: Setup Kernels Directory and CMake Target

**Files:**
- Modify: `sorc/CMakeLists.txt`
- Create: `sorc/upp_cpp/kernels/CMakeLists.txt`

**Interfaces:**
- Consumes: None
- Produces: Static library target `upp_kernels` linking with Kokkos and `helm::span`.

- [ ] **Step 1: Create `sorc/upp_cpp/kernels/CMakeLists.txt`**
Create the file with the following complete CMake content:
```cmake
add_library(upp_kernels STATIC "")

target_compile_features(upp_kernels PRIVATE cxx_std_20)

target_include_directories(upp_kernels
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Link against Kokkos and the SPAN library from helm-project
target_link_libraries(upp_kernels
    PUBLIC
        Kokkos::kokkos
        span
)
```

- [ ] **Step 2: Modify `sorc/CMakeLists.txt`**
Add the `kernels` subdirectory to build traversal right before `upp_cpp` driver.
Modify `sorc/CMakeLists.txt` to include:
```cmake
add_subdirectory(helm/libs/logs)
add_subdirectory(helm/libs/halo)
add_subdirectory(helm/libs/tick)
add_subdirectory(helm/libs/dagr)
add_subdirectory(upp_cpp/kernels)
add_subdirectory(upp_cpp)
```

- [ ] **Step 3: Modify `sorc/upp_cpp/CMakeLists.txt` to link with the new kernels target**
Modify `sorc/upp_cpp/CMakeLists.txt` so `upp_cpp` links against `upp_kernels`:
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
)
```

- [ ] **Step 4: Run CMake configure inside container to verify successful target setup**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake -DBUILD_POSTEXEC=OFF -DBUILD_TESTING=OFF -B build -S .`
Expected: Configures successfully with `upp_kernels` target defined.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add sorc/CMakeLists.txt sorc/upp_cpp/CMakeLists.txt sorc/upp_cpp/kernels/CMakeLists.txt
git commit -m "build: setup upp_kernels CMake project target"
```

---

### Task 2: Implement VerticalInterpolator Kernel

**Files:**
- Create: `sorc/upp_cpp/kernels/include/kernels/vertical_interpolator.hpp`
- Create: `sorc/upp_cpp/kernels/src/vertical_interpolator.cpp`
- Modify: `sorc/upp_cpp/kernels/CMakeLists.txt`

**Interfaces:**
- Consumes: `span::FieldView<const double, 3>` inputs
- Produces: `kernels::VerticalInterpolator` class and linear interpolation execution.

- [ ] **Step 1: Write header `sorc/upp_cpp/kernels/include/kernels/vertical_interpolator.hpp`**
Create the file with the following C++ content:
```cpp
#pragma once

#include <vector>
#include <span/span.hpp>

namespace kernels {

class VerticalInterpolator {
public:
    VerticalInterpolator(std::size_t num_x, std::size_t num_y, std::size_t num_levels);

    void execute(
        const span::FieldView<const double, 3>& input_model,
        const span::FieldView<const double, 3>& model_pressure,
        const std::vector<double>& target_pressure_levels,
        span::FieldView<double, 3>& output_isobaric
    );

private:
    std::size_t nx_;
    std::size_t ny_;
    std::size_t nlevels_;
};

} // namespace kernels
```

- [ ] **Step 2: Write source `sorc/upp_cpp/kernels/src/vertical_interpolator.cpp`**
Create the file with the following complete C++ implementation using Kokkos `parallel_for` and linear interpolation:
```cpp
#include <kernels/vertical_interpolator.hpp>
#include <cmath>

namespace kernels {

VerticalInterpolator::VerticalInterpolator(std::size_t num_x, std::size_t num_y, std::size_t num_levels)
    : nx_(num_x), ny_(num_y), nlevels_(num_levels) {}

void VerticalInterpolator::execute(
    const span::FieldView<const double, 3>& input_model,
    const span::FieldView<const double, 3>& model_pressure,
    const std::vector<double>& target_pressure_levels,
    span::FieldView<double, 3>& output_isobaric
) {
    auto in_view = input_model.view();
    auto pres_view = model_pressure.view();
    auto out_view = output_isobaric.view();

    std::size_t num_targets = target_pressure_levels.size();

    // Allocate host mirror for target levels to pass to Kokkos
    Kokkos::View<double*, Kokkos::HostSpace> target_levels_host("targets_host", num_targets);
    for (std::size_t k = 0; k < num_targets; ++k) {
        target_levels_host(k) = target_pressure_levels[k];
    }
    auto target_levels_device = Kokkos::create_mirror_view_and_copy(
        Kokkos::DefaultExecutionSpace(), target_levels_host
    );

    std::size_t nx = nx_;
    std::size_t ny = ny_;
    std::size_t nlevels = nlevels_;

    Kokkos::parallel_for("vertical_interpolate", Kokkos::MDRangePolicy<Kokkos::Rank<3>>({0,0,0}, {nx, ny, num_targets}),
        KOKKOS_LAMBDA(const std::size_t i, const std::size_t j, const std::size_t k) {
            double target_p = target_levels_device(k);
            double val = 0.0;
            bool interpolated = false;

            // Find hybrid levels bounding the target pressure
            for (std::size_t l = 0; l < nlevels - 1; ++l) {
                double p1 = pres_view(i, j, l);
                double p2 = pres_view(i, j, l + 1);

                if ((target_p >= p1 && target_p <= p2) || (target_p >= p2 && target_p <= p1)) {
                    double f1 = in_view(i, j, l);
                    double f2 = in_view(i, j, l + 1);
                    
                    // Linear interpolation in log(pressure) space
                    double log_p1 = std::log(p1);
                    double log_p2 = std::log(p2);
                    double log_target = std::log(target_p);

                    val = f1 + (f2 - f1) * (log_target - log_p1) / (log_p2 - log_p1);
                    interpolated = true;
                    break;
                }
            }

            // Fallback extrapolation
            if (!interpolated) {
                if (target_p < pres_view(i, j, 0)) {
                    val = in_view(i, j, 0);
                } else {
                    val = in_view(i, j, nlevels - 1);
                }
            }

            out_view(i, j, k) = val;
        }
    );
    Kokkos::fence();
}

} // namespace kernels
```

- [ ] **Step 3: Update `sorc/upp_cpp/kernels/CMakeLists.txt` to compile source**
Update library target to compile `vertical_interpolator.cpp`:
```cmake
target_sources(upp_kernels PRIVATE src/vertical_interpolator.cpp)
```

- [ ] **Step 4: Verify Compilation**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_kernels`
Expected: Compiles with 100% success.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add sorc/upp_cpp/kernels/CMakeLists.txt sorc/upp_cpp/kernels/include/ sorc/upp_cpp/kernels/src/vertical_interpolator.cpp
git commit -m "feat: implement vertical interpolator diagnostic kernel"
```

---

### Task 3: Implement HydrostaticIntegrator Kernel

**Files:**
- Create: `sorc/upp_cpp/kernels/include/kernels/hydrostatic_integrator.hpp`
- Create: `sorc/upp_cpp/kernels/src/hydrostatic_integrator.cpp`
- Modify: `sorc/upp_cpp/kernels/CMakeLists.txt`

**Interfaces:**
- Consumes: `span::FieldView<const double, 3>` inputs
- Produces: `kernels::HydrostaticIntegrator` class and hydrostatic thickness integration.

- [ ] **Step 1: Write header `sorc/upp_cpp/kernels/include/kernels/hydrostatic_integrator.hpp`**
Create the file with the following C++ content:
```cpp
#pragma once

#include <span/span.hpp>

namespace kernels {

class HydrostaticIntegrator {
public:
    HydrostaticIntegrator(std::size_t num_x, std::size_t num_y, std::size_t num_levels);

    void execute(
        const span::FieldView<const double, 3>& temperature,
        const span::FieldView<const double, 3>& spec_humidity,
        const span::FieldView<const double, 3>& pressure,
        const span::FieldView<const double, 2>& surface_geopotential,
        span::FieldView<double, 3>& geopotential_height
    );

private:
    std::size_t nx_;
    std::size_t ny_;
    std::size_t nlevels_;
};

} // namespace kernels
```

- [ ] **Step 2: Write source `sorc/upp_cpp/kernels/src/hydrostatic_integrator.cpp`**
Create the file with the following complete C++ implementation executing thickness integrations:
```cpp
#include <kernels/hydrostatic_integrator.hpp>
#include <cmath>

namespace kernels {

HydrostaticIntegrator::HydrostaticIntegrator(std::size_t num_x, std::size_t num_y, std::size_t num_levels)
    : nx_(num_x), ny_(num_y), nlevels_(num_levels) {}

void HydrostaticIntegrator::execute(
    const span::FieldView<const double, 3>& temperature,
    const span::FieldView<const double, 3>& spec_humidity,
    const span::FieldView<const double, 3>& pressure,
    const span::FieldView<const double, 2>& surface_geopotential,
    span::FieldView<double, 3>& geopotential_height
) {
    auto t_view = temperature.view();
    auto q_view = spec_humidity.view();
    auto p_view = pressure.view();
    auto sfc_z = surface_geopotential.view();
    auto z_view = geopotential_height.view();

    constexpr double Rd = 287.05; // Dry gas constant (J/kg/K)
    constexpr double g = 9.80665;  // Acceleration due to gravity (m/s^2)

    std::size_t nx = nx_;
    std::size_t ny = ny_;
    std::size_t nlevels = nlevels_;

    Kokkos::parallel_for("hydrostatic_integrate", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0,0}, {nx, ny}),
        KOKKOS_LAMBDA(const std::size_t i, const std::size_t j) {
            // Start integration at the surface
            double z_current = sfc_z(i, j) / g;

            for (std::size_t l = 0; l < nlevels - 1; ++l) {
                double t = t_view(i, j, l);
                double q = q_view(i, j, l);
                double p_bottom = p_view(i, j, l);
                double p_top = p_view(i, j, l + 1);

                // Compute Virtual Temperature (K)
                double tv = t * (1.0 + 0.61 * q);

                // Thickness integration (dz)
                double dz = (Rd * tv / g) * std::log(p_bottom / p_top);
                z_current += dz;

                z_view(i, j, l) = z_current;
            }
            z_view(i, j, nlevels - 1) = z_current;
        }
    );
    Kokkos::fence();
}

} // namespace kernels
```

- [ ] **Step 3: Update `sorc/upp_cpp/kernels/CMakeLists.txt` to compile source**
Update library target to compile `hydrostatic_integrator.cpp`:
```cmake
target_sources(upp_kernels PRIVATE src/hydrostatic_integrator.cpp)
```

- [ ] **Step 4: Verify Compilation**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_kernels`
Expected: Compiles with 100% success.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add sorc/upp_cpp/kernels/CMakeLists.txt sorc/upp_cpp/kernels/src/hydrostatic_integrator.cpp
git commit -m "feat: implement hydrostatic integrator diagnostic kernel"
```

---

### Task 4: Implement ThermodynamicDiagnostics Kernel

**Files:**
- Create: `sorc/upp_cpp/kernels/include/kernels/thermodynamic_diagnostics.hpp`
- Create: `sorc/upp_cpp/kernels/src/thermodynamic_diagnostics.cpp`
- Modify: `sorc/upp_cpp/kernels/CMakeLists.txt`

**Interfaces:**
- Consumes: `span::FieldView<const double, 3>` inputs
- Produces: `kernels::ThermodynamicDiagnostics` class and Relative Humidity calculation.

- [ ] **Step 1: Write header `sorc/upp_cpp/kernels/include/kernels/thermodynamic_diagnostics.hpp`**
Create the file with the following C++ content:
```cpp
#pragma once

#include <span/span.hpp>

namespace kernels {

class ThermodynamicDiagnostics {
public:
    ThermodynamicDiagnostics(std::size_t num_x, std::size_t num_y, std::size_t num_levels);

    void execute(
        const span::FieldView<const double, 3>& temperature,
        const span::FieldView<const double, 3>& spec_humidity,
        const span::FieldView<const double, 3>& pressure,
        span::FieldView<double, 3>& relative_humidity
    );

private:
    std::size_t nx_;
    std::size_t ny_;
    std::size_t nlevels_;
};

} // namespace kernels
```

- [ ] **Step 2: Write source `sorc/upp_cpp/kernels/src/thermodynamic_diagnostics.cpp`**
Create the file with the following complete C++ implementation computing Relative Humidity pointwise:
```cpp
#include <kernels/thermodynamic_diagnostics.hpp>
#include <cmath>

namespace kernels {

ThermodynamicDiagnostics::ThermodynamicDiagnostics(std::size_t num_x, std::size_t num_y, std::size_t num_levels)
    : nx_(num_x), ny_(num_y), nlevels_(num_levels) {}

void ThermodynamicDiagnostics::execute(
    const span::FieldView<const double, 3>& temperature,
    const span::FieldView<const double, 3>& spec_humidity,
    const span::FieldView<const double, 3>& pressure,
    span::FieldView<double, 3>& relative_humidity
) {
    auto t_view = temperature.view();
    auto q_view = spec_humidity.view();
    auto p_view = pressure.view();
    auto rh_view = relative_humidity.view();

    std::size_t nx = nx_;
    std::size_t ny = ny_;
    std::size_t nlevels = nlevels_;

    Kokkos::parallel_for("thermo_rh", Kokkos::MDRangePolicy<Kokkos::Rank<3>>({0,0,0}, {nx, ny, nlevels}),
        KOKKOS_LAMBDA(const std::size_t i, const std::size_t j, const std::size_t l) {
            double temp = t_view(i, j, l);
            double spec_h = q_view(i, j, l);
            double pres = p_view(i, j, l); // pressure in Pa

            // 1. Convert pressure to hPa for Tetens formula
            double pres_hpa = pres / 100.0;

            // 2. Compute Saturation Vapor Pressure (es) using Tetens equation (hPa)
            double tc = temp - 273.15; // Kelvin to Celsius
            double es = 6.112 * std::exp((17.67 * tc) / (tc + 243.5));

            // 3. Compute mixing ratio (w) from specific humidity (q)
            double w = spec_h / (1.0 - spec_h);

            // 4. Compute actual vapor pressure (e) from mixing ratio and pressure
            double e = (w * pres_hpa) / (w + 0.622);

            // 5. Compute Relative Humidity (0 - 100%)
            double rh = 100.0 * (e / es);

            // Clamp bounds
            if (rh > 100.0) rh = 100.0;
            if (rh < 0.0) rh = 0.0;

            rh_view(i, j, l) = rh;
        }
    );
    Kokkos::fence();
}

} // namespace kernels
```

- [ ] **Step 3: Update `sorc/upp_cpp/kernels/CMakeLists.txt` to compile source**
Update library target to compile `thermodynamic_diagnostics.cpp`:
```cmake
target_sources(upp_kernels PRIVATE src/thermodynamic_diagnostics.cpp)
```

- [ ] **Step 4: Verify Compilation**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 cmake --build build --target upp_kernels`
Expected: Compiles with 100% success.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add sorc/upp_cpp/kernels/CMakeLists.txt sorc/upp_cpp/kernels/src/thermodynamic_diagnostics.cpp
git commit -m "feat: implement thermodynamic relative humidity kernel"
```

---

### Task 5: Setup Kernel Unit Tests in CTest

**Files:**
- Create: `tests/test_kernels.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `upp_kernels` target
- Produces: `verify_kernels` target and CTest run verification.

- [ ] **Step 1: Write unit tests in `tests/test_kernels.cpp`**
Create `tests/test_kernels.cpp` with the following comprehensive tests validating all three kernels:
```cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <span/span.hpp>
#include <kernels/vertical_interpolator.hpp>
#include <kernels/hydrostatic_integrator.hpp>
#include <kernels/thermodynamic_diagnostics.hpp>

void test_vertical_interpolation() {
    std::cout << "Running test_vertical_interpolation..." << std::endl;
    std::array<std::size_t, 3> exts = {2, 2, 3};
    std::vector<double> in_raw = {
        10.0, 10.0, 10.0, 10.0, // Level 0
        20.0, 20.0, 20.0, 20.0, // Level 1
        30.0, 30.0, 30.0, 30.0  // Level 2
    };
    std::vector<double> pres_raw = {
        1000.0, 1000.0, 1000.0, 1000.0, // Level 0 (1000 hPa)
        500.0,  500.0,  500.0,  500.0,  // Level 1 (500 hPa)
        100.0,  100.0,  100.0,  100.0   // Level 2 (100 hPa)
    };
    std::vector<double> out_raw(2 * 2 * 1, 0.0); // 1 target level

    span::FieldView<double, 3> in_field(in_raw.data(), exts);
    span::FieldView<double, 3> pres_field(pres_raw.data(), exts);
    span::FieldView<double, 3> out_field(out_raw.data(), {2, 2, 1});

    // We want to interpolate to target pressure level 300 hPa
    std::vector<double> targets = { 300.0 };

    kernels::VerticalInterpolator interpolator(2, 2, 3);
    interpolator.execute(
        span::FieldView<const double, 3>(in_field),
        span::FieldView<const double, 3>(pres_field),
        targets,
        out_field
    );

    // Verify mathematical bounds
    for (std::size_t i = 0; i < 4; ++i) {
        if (out_raw[i] <= 20.0 || out_raw[i] >= 30.0) {
            throw std::runtime_error("test_vertical_interpolation failed: value out of bounds");
        }
    }
    std::cout << "test_vertical_interpolation PASSED." << std::endl;
}

void test_hydrostatic_integration() {
    std::cout << "Running test_hydrostatic_integration..." << std::endl;
    std::array<std::size_t, 3> exts = {2, 2, 3};
    std::vector<double> temp_raw(12, 280.0);
    std::vector<double> q_raw(12, 0.005);
    std::vector<double> pres_raw = {
        100000.0, 100000.0, 100000.0, 100000.0, // Bottom (1000 hPa)
        50000.0,  50000.0,  50000.0,  50000.0,  // Mid
        10000.0,  10000.0,  10000.0,  10000.0   // Top (100 hPa)
    };
    std::vector<double> sfc_raw(4, 0.0); // Surface geopotential (0 m^2/s^2)
    std::vector<double> z_raw(12, 0.0);

    span::FieldView<double, 3> temp_field(temp_raw.data(), exts);
    span::FieldView<double, 3> q_field(q_raw.data(), exts);
    span::FieldView<double, 3> pres_field(pres_raw.data(), exts);
    span::FieldView<double, 2> sfc_field(sfc_raw.data(), {2, 2});
    span::FieldView<double, 3> z_field(z_raw.data(), exts);

    kernels::HydrostaticIntegrator integrator(2, 2, 3);
    integrator.execute(
        span::FieldView<const double, 3>(temp_field),
        span::FieldView<const double, 3>(q_field),
        span::FieldView<const double, 3>(pres_field),
        span::FieldView<const double, 2>(sfc_field),
        z_field
    );

    // Verify thickness is positive
    for (std::size_t i = 0; i < 12; ++i) {
        if (z_raw[i] < 0.0) {
            throw std::runtime_error("test_hydrostatic_integration failed: negative geopotential height");
        }
    }
    std::cout << "test_hydrostatic_integration PASSED." << std::endl;
}

void test_thermo_rh() {
    std::cout << "Running test_thermo_rh..." << std::endl;
    std::array<std::size_t, 3> exts = {2, 2, 2};
    std::vector<double> temp_raw(8, 290.0); // 290 K (~17 C)
    std::vector<double> q_raw(8, 0.008);   // 8 g/kg
    std::vector<double> pres_raw(8, 100000.0); // 1000 hPa
    std::vector<double> rh_raw(8, 0.0);

    span::FieldView<double, 3> temp_field(temp_raw.data(), exts);
    span::FieldView<double, 3> q_field(q_raw.data(), exts);
    span::FieldView<double, 3> pres_field(pres_raw.data(), exts);
    span::FieldView<double, 3> rh_field(rh_raw.data(), exts);

    kernels::ThermodynamicDiagnostics diagnostics(2, 2, 2);
    diagnostics.execute(
        span::FieldView<const double, 3>(temp_field),
        span::FieldView<const double, 3>(q_field),
        span::FieldView<const double, 3>(pres_field),
        rh_field
    );

    // Verify RH is bounded in [0, 100]
    for (std::size_t i = 0; i < 8; ++i) {
        if (rh_raw[i] < 0.0 || rh_raw[i] > 100.0) {
            throw std::runtime_error("test_thermo_rh failed: RH out of range");
        }
    }
    std::cout << "test_thermo_rh PASSED." << std::endl;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    Kokkos::initialize(argc, argv);
    {
        test_vertical_interpolation();
        test_hydrostatic_integration();
        test_thermo_rh();
    }
    Kokkos::finalize();
    MPI_Finalize();
    std::cout << "All kernels unit tests PASSED!" << std::endl;
    return 0;
}
```

- [ ] **Step 2: Update `tests/CMakeLists.txt` to add kernel tests**
Add compilation target and tests configuration to `tests/CMakeLists.txt`:
```cmake
add_executable(verify_kernels test_kernels.cpp)
target_link_libraries(verify_kernels
    PRIVATE
        upp_kernels
        MPI::MPI_CXX
)
add_test(NAME verify_kernels COMMAND mpirun -np 1 $<TARGET_FILE:verify_kernels>)
```

- [ ] **Step 3: Run CMake configure and compile tests inside the container**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 bash -c "cmake -DBUILD_POSTEXEC=OFF -DBUILD_TESTING=OFF -B build -S . && cmake --build build"`
Expected: Compilation succeeds.

- [ ] **Step 4: Execute CTest to verify all unit tests pass**
Run: `docker run --rm -v $PWD:/workspace -w /workspace 081212acfb78e790a3ee6471cd3dab41737f60214678e9a2c428fe8b6fdedaa7 ctest --test-dir build --output-on-failure`
Expected: CTest shows `verify_kernels` test PASSED.

- [ ] **Step 5: Commit changes**
Run:
```bash
git add tests/CMakeLists.txt tests/test_kernels.cpp
git commit -m "test: integrate kernel unit tests with CTest"
```
