#include <iostream>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <conf/config.hpp>
#include <axis/axis.hpp>
#include <axis/solver/vertical_regridder.hpp>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    Kokkos::initialize(argc, argv);
    {
        std::cout << "Verifying Orchestration integration (CONF + AXIS)..." << std::endl;

        // 1. Verify CONF
        conf::Config config = conf::Config::from_file("amio_output_t.yaml");
        std::string path = config.get_string("path");
        if (path != "output_t.nc") {
            throw std::runtime_error("Orchestration verification failed: CONF failed to parse path");
        }

        // 2. Verify AXIS vertical regridding plan compilation
        Kokkos::View<double**, Kokkos::HostSpace> src("src", 12, 5);
        Kokkos::View<double**, Kokkos::HostSpace> dst("dst", 12, 1);
        Kokkos::View<double**, Kokkos::HostSpace> p_src("p_src", 12, 5);
        Kokkos::View<double**, Kokkos::HostSpace> p_dst("p_dst", 12, 1);
        
        axis::solver::VerticalRegridder<Kokkos::HostSpace>::interpolate(src, dst, p_src, p_dst);
        
        std::cout << "Orchestration (CONF + AXIS) integration verified successfully!" << std::endl;
    }
    Kokkos::finalize();
    MPI_Finalize();
    return 0;
}
