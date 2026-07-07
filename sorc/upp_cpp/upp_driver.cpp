#include <iostream>
#include <sstream>
#include <array>
#include <vector>
#include <mpi.h>
#include <Kokkos_Core.hpp>
#include <logs/logs.hpp>
#include <halo/halo.hpp>
#include <amio/amio.h>
#include <span/span.hpp>
#include <conf/config.hpp>
#include <axis/axis.hpp>
#include <axis/solver/vertical_regridder.hpp>
#include <kernels/hydrostatic_integrator.hpp>
#include <kernels/thermodynamic_diagnostics.hpp>

// Helper to check AMIO statuses
#define AMIO_CHECK(rc) \
    if ((rc) != AMIO_OK) { \
        std::cerr << "AMIO Error at line " << __LINE__ << ": " << (rc) << " (" << amio_strerror(rc) << ")" << std::endl; \
        MPI_Abort(MPI_COMM_WORLD, 1); \
    }

extern "C" {
    void upp_handoff_to_fortran(const void* t_ptr, const void* q_ptr, const void* p_ptr, int nx, int ny, int nz);
}

void run_modernized_pipeline(int rank, int size) {
    logs::Logger logger;
    logger.configure_communicator(MPI_COMM_WORLD);
    logger.set_threshold(logs::Severity_Level::DEBUG);

    logger.log(logs::Severity_Level::INFO, "Running Phase 4 Orchestration pipeline...");

    // 1. Initialize AMIO Core
    amio_core_handle core = nullptr;
    AMIO_CHECK(amio_init("amio_manifest.yaml", &core));

    // 2. Open input dataset in READ mode (AMIO_MODE_READ = 1)
    amio_dataset_handle input_ds = nullptr;
    AMIO_CHECK(amio_open_dataset(core, "amio_manifest.yaml", AMIO_MODE_READ, &input_ds));

    // 3. Queue asynchronous reads (timestep 0)
    amio_view_handle t_view = nullptr, q_view = nullptr, p_view = nullptr, sfc_view = nullptr;
    AMIO_CHECK(amio_read(input_ds, "t", 0, nullptr, &t_view));
    AMIO_CHECK(amio_read(input_ds, "q", 0, nullptr, &q_view));
    AMIO_CHECK(amio_read(input_ds, "p", 0, nullptr, &p_view));
    AMIO_CHECK(amio_read(input_ds, "sfc_g", 0, nullptr, &sfc_view));

    // 4. Retrieve shapes and extract dimensions dynamically
    amio_shape_t t_shape;
    std::memset(&t_shape, 0, sizeof(t_shape));
    AMIO_CHECK(amio_view_shape(t_view, &t_shape));

    std::size_t nx = t_shape.extents[0];
    std::size_t ny = t_shape.extents[1];
    std::size_t nlevels = t_shape.extents[2];
    std::size_t nx_ny = nx * ny;

    std::stringstream log_ss;
    log_ss << "Dynamically resolved input shape: [" << nx << " x " << ny << " x " << nlevels << "]";
    logger.log(logs::Severity_Level::INFO, log_ss.str());

    // 5. Get data pointers
    const void *t_data = nullptr, *q_data = nullptr, *p_data = nullptr, *sfc_data = nullptr;
    size_t size_bytes = 0;
    AMIO_CHECK(amio_view_data(t_view, &t_data, &size_bytes));
    AMIO_CHECK(amio_view_data(q_view, &q_data, &size_bytes));
    AMIO_CHECK(amio_view_data(p_view, &p_data, &size_bytes));
    AMIO_CHECK(amio_view_data(sfc_view, &sfc_data, &size_bytes));

    // 6. Wrap pointers directly in zero-copy flat 2D span::FieldView (nx_ny columns, nlevels vertical layers)
    span::FieldView<const double, 2> temp_field(static_cast<const double*>(t_data), {nx_ny, nlevels});
    span::FieldView<const double, 2> q_field(static_cast<const double*>(q_data), {nx_ny, nlevels});
    span::FieldView<const double, 2> p_field(static_cast<const double*>(p_data), {nx_ny, nlevels});
    span::FieldView<const double, 2> sfc_field(static_cast<const double*>(sfc_data), {nx, ny});

    // 7. Define output variables with dynamic extents
    std::vector<double> out_t_raw(nx_ny * 1, 0.0); // 1 target pressure level
    std::vector<double> out_gh_raw(nx_ny * nlevels, 0.0);
    std::vector<double> out_rh_raw(nx_ny * nlevels, 0.0);

    span::FieldView<double, 2> t_iso_field(out_t_raw.data(), {nx_ny, 1});
    span::FieldView<double, 2> gh_field(out_gh_raw.data(), {nx_ny, nlevels});
    span::FieldView<double, 2> rh_field(out_rh_raw.data(), {nx_ny, nlevels});

    // 8. Dynamic CONF Parsing: Load output dataset configurations
    conf::Config runtime_config = conf::Config::from_file("amio_output_t.yaml");
    std::string backend_name = runtime_config.get_string("backend");
    std::stringstream conf_ss;
    conf_ss << "Dynamic CONF: Loaded output dataset backend: " << backend_name;
    logger.log(logs::Severity_Level::INFO, conf_ss.str());

    // 9. Execute shared HELM::AXIS vertical regridding to 500 hPa
    logger.log(logs::Severity_Level::INFO, "Executing shared HELM::AXIS vertical regridding...");
    
    // Allocate temporary standard LayoutRight (default HostSpace) views for AXIS vertical regridding
    Kokkos::View<double**, Kokkos::HostSpace> src_temp_axis("src_temp_axis", nx_ny, nlevels);
    Kokkos::View<double**, Kokkos::HostSpace> src_pres_axis("src_pres_axis", nx_ny, nlevels);
    Kokkos::View<double**, Kokkos::HostSpace> dst_temp_axis("dst_temp_axis", nx_ny, 1);

    // Copy from our LayoutLeft (temp_field, p_field) views to AXIS views
    Kokkos::parallel_for("copy_to_axis", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({0,0}, {nx_ny, nlevels}),
        KOKKOS_LAMBDA(const std::size_t i, const std::size_t j) {
            src_temp_axis(i, j) = temp_field.view()(i, j);
            src_pres_axis(i, j) = p_field.view()(i, j);
        }
    );
    Kokkos::fence();

    // Allocate 2D target pressure levels view: (N_col, 1)
    Kokkos::View<double**, Kokkos::HostSpace> dst_levels("dst_levels", nx_ny, 1);
    Kokkos::parallel_for("init_dst_levels", nx_ny, KOKKOS_LAMBDA(const std::size_t i) {
        dst_levels(i, 0) = 50000.0; // 500 hPa in Pa
    });
    Kokkos::fence();

    // Define explicit const view references to perfectly match templated overloads
    Kokkos::View<const double**, Kokkos::HostSpace> src_temp_const = src_temp_axis;
    Kokkos::View<const double**, Kokkos::HostSpace> src_pres_const = src_pres_axis;
    Kokkos::View<const double**, Kokkos::HostSpace> dst_levels_const = dst_levels;

    // Run AXIS vertical regridder
    axis::solver::VerticalRegridder<Kokkos::HostSpace>::interpolate(
        src_temp_const,
        dst_temp_axis,
        src_pres_const,
        dst_levels_const,
        0.0 // Tension parameter (cubic spline fallback)
    );

    // Copy the results back to t_iso_field (using .view() operator)
    Kokkos::parallel_for("copy_dst_levels", nx_ny, KOKKOS_LAMBDA(const std::size_t i) {
        t_iso_field.view()(i, 0) = dst_temp_axis(i, 0);
    });
    Kokkos::fence();

    logger.log(logs::Severity_Level::INFO, "HELM::AXIS vertical regridding executed successfully.");

    // 10. Execute geopotential integration
    // Adapting 3D math kernels to flat 2D layers [nx_ny, nlevels]
    kernels::HydrostaticIntegrator integrator(nx, ny, nlevels);
    
    // Wrap 2D views back to 3D representation expected by kernels
    span::FieldView<double, 3> gh_field_3d(out_gh_raw.data(), {nx, ny, nlevels});
    span::FieldView<const double, 3> temp_field_3d(static_cast<const double*>(t_data), {nx, ny, nlevels});
    span::FieldView<const double, 3> q_field_3d(static_cast<const double*>(q_data), {nx, ny, nlevels});
    span::FieldView<const double, 3> p_field_3d(static_cast<const double*>(p_data), {nx, ny, nlevels});
    integrator.execute(temp_field_3d, q_field_3d, p_field_3d, sfc_field, gh_field_3d);

    // 11. Execute relative humidity calculation
    kernels::ThermodynamicDiagnostics diagnostics(nx, ny, nlevels);
    span::FieldView<double, 3> rh_field_3d(out_rh_raw.data(), {nx, ny, nlevels});
    diagnostics.execute(temp_field_3d, q_field_3d, p_field_3d, rh_field_3d);

    logger.log(logs::Severity_Level::INFO, "All dynamic diagnostic math kernels executed successfully.");

    // Hand off memory to legacy Fortran PROCESS driver (Zero-copy)
    logger.log(logs::Severity_Level::INFO, "Handing off Kokkos views to legacy Fortran PROCESS...");
    std::cout << "C++ Sample Temp at [0,0]: " << temp_field.view()(0, 0) << std::endl;
    std::cout << "C++ Sample Humid at [0,0]: " << q_field.view()(0, 0) << std::endl;
    std::cout << "C++ Sample Pres at [0,0]: " << p_field.view()(0, 0) << std::endl;
    upp_handoff_to_fortran(t_data, q_data, p_data, static_cast<int>(nx), static_cast<int>(ny), static_cast<int>(nlevels));
    logger.log(logs::Severity_Level::INFO, "Legacy Fortran execution complete.");

    // Create shapes dynamically to pass to AMIO
    amio_shape_t t_iso_shape = { 3, {static_cast<int64_t>(nx), static_cast<int64_t>(ny), 1}, {0, 0, 0} };
    amio_shape_t gh_shape = { 3, {static_cast<int64_t>(nx), static_cast<int64_t>(ny), static_cast<int64_t>(nlevels)}, {0, 0, 0} };
    amio_shape_t rh_shape = { 3, {static_cast<int64_t>(nx), static_cast<int64_t>(ny), static_cast<int64_t>(nlevels)}, {0, 0, 0} };

    // 12. Write t_isobaric parameter to its own file output_t.nc
    {
        amio_dataset_handle t_ds = nullptr;
        AMIO_CHECK(amio_open_dataset(core, "amio_output_t.yaml", AMIO_MODE_WRITE, &t_ds));
        amio_io_handle io = nullptr;
        AMIO_CHECK(amio_write(t_ds, "t_isobaric", out_t_raw.data(), AMIO_DTYPE_F64, &t_iso_shape, &io));
        AMIO_CHECK(amio_wait(io, 10000));
        AMIO_CHECK(amio_close_dataset(t_ds));
        logger.log(logs::Severity_Level::INFO, "t_isobaric written asynchronously successfully to output_t.nc.");
    }

    // 13. Write gh parameter to its own file output_gh.nc
    {
        amio_dataset_handle gh_ds = nullptr;
        AMIO_CHECK(amio_open_dataset(core, "amio_output_gh.yaml", AMIO_MODE_WRITE, &gh_ds));
        amio_io_handle io = nullptr;
        AMIO_CHECK(amio_write(gh_ds, "gh", out_gh_raw.data(), AMIO_DTYPE_F64, &gh_shape, &io));
        AMIO_CHECK(amio_wait(io, 10000));
        AMIO_CHECK(amio_close_dataset(gh_ds));
        logger.log(logs::Severity_Level::INFO, "Geopotential Height written asynchronously successfully to output_gh.nc.");
    }

    // 14. Write rh parameter to its own file output_rh.nc
    {
        amio_dataset_handle rh_ds = nullptr;
        AMIO_CHECK(amio_open_dataset(core, "amio_output_rh.yaml", AMIO_MODE_WRITE, &rh_ds));
        amio_io_handle io = nullptr;
        AMIO_CHECK(amio_write(rh_ds, "rh", out_rh_raw.data(), AMIO_DTYPE_F64, &rh_shape, &io));
        AMIO_CHECK(amio_wait(io, 10000));
        AMIO_CHECK(amio_close_dataset(rh_ds));
        logger.log(logs::Severity_Level::INFO, "Relative Humidity written asynchronously successfully to output_rh.nc.");
    }

    // 15. Cleanup read views and close dataset
    AMIO_CHECK(amio_release_view(t_view));
    AMIO_CHECK(amio_release_view(q_view));
    AMIO_CHECK(amio_release_view(p_view));
    AMIO_CHECK(amio_release_view(sfc_view));
    AMIO_CHECK(amio_close_dataset(input_ds));

    // 16. Finalize AMIO Core
    AMIO_CHECK(amio_finalize(core));
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

        // 4. Execute asynchronous end-to-end processing pipeline
        run_modernized_pipeline(rank, size);
    }
    // 5. Finalize Kokkos and MPI
    Kokkos::finalize();
    MPI_Finalize();
    return 0;
}
