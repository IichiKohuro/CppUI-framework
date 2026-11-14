#pragma once

namespace cppui {

enum class DensityMode {
    compact,
    cozy,
    comfortable,
};

struct DensityMetrics {
    float control_height;
    float control_padding;
    float control_radius;
    float control_spacing;
    float min_control_width;
};

[[nodiscard]] constexpr DensityMetrics density_metrics(DensityMode mode) noexcept {
    switch (mode) {
        case DensityMode::compact:
            return DensityMetrics{28.0f, 10.0f, 6.0f, 6.0f, 64.0f};
        case DensityMode::comfortable:
            return DensityMetrics{40.0f, 18.0f, 9.0f, 12.0f, 88.0f};
        case DensityMode::cozy:
        default:
            return DensityMetrics{34.0f, 14.0f, 7.0f, 10.0f, 76.0f};
    }
}

}  // namespace cppui
