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
