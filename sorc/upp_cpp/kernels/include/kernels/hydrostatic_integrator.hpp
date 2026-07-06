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
