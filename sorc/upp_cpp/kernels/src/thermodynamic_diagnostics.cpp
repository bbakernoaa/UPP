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
