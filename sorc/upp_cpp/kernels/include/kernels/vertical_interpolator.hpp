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
