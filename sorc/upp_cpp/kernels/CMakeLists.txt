add_library(upp_kernels STATIC
    dummy.cpp
    src/vertical_interpolator.cpp
    src/hydrostatic_integrator.cpp
    src/thermodynamic_diagnostics.cpp
)

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
