#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <logs/logs.hpp>
#include <halo/halo.hpp>
#include <dagr/dagr.hpp>
#include <dagr/pipeline_config.hpp>

void verify_halo_exchange(int rank, int size) {
    logs::Logger logger;
    logger.configure_communicator(MPI_COMM_WORLD);
    logger.set_threshold(logs::Severity_Level::DEBUG);

    logger.log(logs::Severity_Level::INFO, "Starting helm::halo verification...");

    // 1. Initialize HALO environment
    halo::Environment::initialize();

    // 2. Create halo communicator wrapping MPI_COMM_WORLD
    halo::Communicator comm(MPI_COMM_WORLD);

    // 3. Define 2D structured grid extents: 12x12
    std::array<std::size_t, 2> extents = {12, 12};
    std::array<std::size_t, 2> halo_widths = {1, 1};
    
    // Neighbors: west, east, south, north
    int peer = (rank + 1) % size;
    std::array<int, 4> neighbors = { peer, peer, peer, peer };

    // 4. Instantiate Structured_Halo_Plan
    halo::Structured_Halo_Plan<2> plan(extents, neighbors, halo_widths, comm);

    // 5. Create a 2D Kokkos View with size 12x12
    Kokkos::View<double**, Kokkos::LayoutLeft> grid("grid_data", 12, 12);

    // Initialize the View
    Kokkos::parallel_for("init_grid", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0,0}, {12,12}),
        KOKKOS_LAMBDA(const int i, const int j) {
            grid(i, j) = rank * 100.0 + i + j;
        }
    );

    // 6. Perform halo boundary exchange
    halo::exchange_structured_blocking(plan, grid);

    logger.log(logs::Severity_Level::INFO, "helm::halo exchange completed successfully.");
}

void verify_dagr_resolution(int rank) {
    logs::Logger logger;
    logger.configure_communicator(MPI_COMM_WORLD);
    logger.set_threshold(logs::Severity_Level::DEBUG);

    if (rank == 0) {
        logger.log(logs::Severity_Level::INFO, "Starting helm::dagr verification...");

        // 1. Define a mock Pipeline_Config
        dagr::Pipeline_Config config;
        
        // Add dummy task names
        config.task_names = { "Pressure", "Temperature", "Hydrometeors", "Reflectivity" };
        
        // Define dependencies (producer_id -> consumer_id)
        // Reflectivity (node 3) depends on Temperature (node 1) and Hydrometeors (node 2)
        // Temperature (node 1) depends on Pressure (node 0)
        config.edges = {
            { 0, 1 }, // Pressure -> Temperature
            { 1, 3 }, // Temperature -> Reflectivity
            { 2, 3 }  // Hydrometeors -> Reflectivity
        };
        
        config.max_concurrency = 4;
        config.deadlock_timeout_s = 10;
        config.shutdown_timeout_s = 10;

        // 2. Instantiate GraphOrchestrator
        halo::Communicator world(MPI_COMM_SELF);
        
        // Construct the orchestrator - this will parse and validate the graph
        dagr::GraphOrchestrator orchestrator(std::move(config), std::move(world));

        logger.log(logs::Severity_Level::INFO, "DAGR GraphOrchestrator constructed and validated successfully (acyclic, no cycles).");
    }
}

int main(int argc, char* argv[]) {
    // 1. Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // 2. Initialize Kokkos
    Kokkos::initialize(argc, argv);
    {
        // 3. Initialize and configure logs::Logger
        logs::Logger logger;
        logger.configure_communicator(MPI_COMM_WORLD);
        logger.set_threshold(logs::Severity_Level::DEBUG);

        logger.log(logs::Severity_Level::INFO, "UPP C++ Driver initialized successfully.");

        // Format and print Kokkos Configuration
        std::stringstream ss;
        Kokkos::print_configuration(ss);
        logger.log(logs::Severity_Level::INFO, ss.str());

        // 4. Execute halo exchange verification
        verify_halo_exchange(rank, size);

        // 5. Execute DAGR verification
        verify_dagr_resolution(rank);
    }
    // 6. Finalize Kokkos and MPI
    Kokkos::finalize();
    MPI_Finalize();
    return 0;
}
