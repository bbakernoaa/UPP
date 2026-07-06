# UPP Modernization Phase 2: Abstraction Design

## 1. Overview
This document outlines the Phase 2 Abstraction design for the modernization of the Unified Post Processor (UPP). The primary goal is to establish a performance-portable C++ diagnostic library (`upp_kernels`) leveraging `std::mdspan`, Kokkos execution spaces, and the `span::FieldView` memory abstraction from `helm-project`.

## 2. Architecture & Components

### 2.1 File & Module Layout
A new subproject will be introduced for the diagnostic calculations:
- **Directory:** `sorc/upp_cpp/kernels/`
- **Build Target:** `upp_kernels` (static library)
- **Dependencies:** `HELM::SPAN` (`sorc/helm/libs/span`), `Kokkos::kokkos`, `MPI::MPI_CXX`.

### 2.2 Core Kernel Interfaces
The library will expose three main diagnostic classes inside the `kernels` namespace:

#### 1. `kernels::VerticalInterpolator`
- **Purpose:** Interpolates atmospheric fields (e.g., Temperature, Wind) from model hybrid-sigma coordinate levels to target constant pressure surfaces.
- **Formulation:** Linear interpolation in logarithm of pressure:
  $$F_p = F_1 + (F_2 - F_1) \cdot \frac{\ln(P_{target}) - \ln(P_1)}{\ln(P_2) - \ln(P_1)}$$
- **Interface:**
  ```cpp
  class VerticalInterpolator {
  public:
      VerticalInterpolator(std::size_t num_x, std::size_t num_y, std::size_t num_levels);
      
      void execute(
          const span::FieldView<const double, 3>& input_model,
          const span::FieldView<const double, 3>& model_pressure,
          const std::vector<double>& target_pressure_levels,
          span::FieldView<double, 3>& output_isobaric
      );
  };
  ```

#### 2. `kernels::HydrostaticIntegrator`
- **Purpose:** Integrates the hydrostatic equation to calculate geopotential height ($Z$) on model levels and constant pressure levels.
- **Formulation:** Hydrostatic thickness integration:
  $$\Delta Z = \frac{R_d \cdot T_v}{g} \cdot \ln\left(\frac{P_{bottom}}{P_{top}}\right)$$
  where $T_v = T \cdot (1 + 0.61 \cdot q)$ is the virtual temperature.
- **Interface:**
  ```cpp
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
  };
  ```

#### 3. `kernels::ThermodynamicDiagnostics`
- **Purpose:** Performs pointwise calculation of Relative Humidity ($RH$) from Temperature ($T$), Specific Humidity ($q$), and Pressure ($P$).
- **Formulation:**
  $$e_s = 6.112 \cdot \exp\left(\frac{17.67 \cdot (T - 273.15)}{T - 29.65}\right)$$
  $$w = q / (1 - q)$$
  $$e = \frac{w \cdot P}{w + 0.622}$$
  $$RH = \min(100.0, \max(0.0, 100.0 \cdot \frac{e}{e_s}))$$
- **Interface:**
  ```cpp
  class ThermodynamicDiagnostics {
  public:
      ThermodynamicDiagnostics(std::size_t num_x, std::size_t num_y, std::size_t num_levels);
      
      void execute(
          const span::FieldView<const double, 3>& temperature,
          const span::FieldView<const double, 3>& spec_humidity,
          const span::FieldView<const double, 3>& pressure,
          span::FieldView<double, 3>& relative_humidity
      );
  };
  ```

### 2.3 Memory Mapping & Execution Strategy
- Input and output buffers are managed as `span::FieldView` objects using `std::layout_left` (column-major) to ensure binary compatibility with Fortran memory layouts.
- Within each kernel's execution, data is mapped directly to `Kokkos::View` (via `FieldView::kokkos_host_view` or `kokkos_device_view`) to execute optimal `Kokkos::parallel_for` grids.
- Calculations are fully GPU-aware and thread-safe.

## 3. Data Flow
1. Memory slices are allocated by the host application (or legacy Fortran).
2. Pointers are wrapped into non-allocating `span::FieldView<double, Rank>` wrappers.
3. The host application invokes the kernel's `execute` method, passing input and output FieldViews.
4. The kernel launches a parallel execution region on the default execution space.
5. Outputs are written directly to the target output memory buffers.

## 4. Testing & Verification
A dedicated CTest executable `verify_kernels` will be created at `tests/test_kernels.cpp`.
- **Vertical Interpolation Test:** Verifies that a linear 3D temperature profile is interpolated correctly to constant pressure levels.
- **Geopotential Integration Test:** Verifies hydrostatic height integration against known analytical solutions.
- **Thermodynamic Test:** Verifies Relative Humidity calculations against standard meteorological look-up tables.
