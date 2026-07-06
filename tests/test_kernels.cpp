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

    span::FieldView<const double, 3> in_field(in_raw.data(), exts);
    span::FieldView<const double, 3> pres_field(pres_raw.data(), exts);
    span::FieldView<double, 3> out_field(out_raw.data(), {2, 2, 1});

    // We want to interpolate to target pressure level 300 hPa
    std::vector<double> targets = { 300.0 };

    kernels::VerticalInterpolator interpolator(2, 2, 3);
    interpolator.execute(
        in_field,
        pres_field,
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

    span::FieldView<const double, 3> temp_field(temp_raw.data(), exts);
    span::FieldView<const double, 3> q_field(q_raw.data(), exts);
    span::FieldView<const double, 3> pres_field(pres_raw.data(), exts);
    span::FieldView<const double, 2> sfc_field(sfc_raw.data(), {2, 2});
    span::FieldView<double, 3> z_field(z_raw.data(), exts);

    kernels::HydrostaticIntegrator integrator(2, 2, 3);
    integrator.execute(
        temp_field,
        q_field,
        pres_field,
        sfc_field,
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

    span::FieldView<const double, 3> temp_field(temp_raw.data(), exts);
    span::FieldView<const double, 3> q_field(q_raw.data(), exts);
    span::FieldView<const double, 3> pres_field(pres_raw.data(), exts);
    span::FieldView<double, 3> rh_field(rh_raw.data(), exts);

    kernels::ThermodynamicDiagnostics diagnostics(2, 2, 2);
    diagnostics.execute(
        temp_field,
        q_field,
        pres_field,
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
