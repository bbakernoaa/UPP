# CMake configuration for the modernized UPP C++ driver

add_executable(upp_cpp upp_driver.cpp)

target_compile_features(upp_cpp PRIVATE cxx_std_20)

# Create the plan's requested aliases so downstream linkage matches the plan verbatim
add_library(helm::logs ALIAS logs)
add_library(helm::halo ALIAS halo)
add_library(helm::tick ALIAS tick)
add_library(helm::dagr ALIAS dagr)
add_library(helm::axis ALIAS axis)
add_library(helm::conf ALIAS conf)

target_link_libraries(upp_cpp
    PRIVATE
        Kokkos::kokkos
        MPI::MPI_CXX
        helm::logs
        helm::halo
        helm::tick
        helm::dagr
        helm::axis
        helm::conf
        upp_kernels
        amio_core
)
