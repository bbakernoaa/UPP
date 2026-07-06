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
