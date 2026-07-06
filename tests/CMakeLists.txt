enable_testing()

if(BUILD_POSTEXEC)
  set(VERIFY_EXE verify_yaml.x)
  add_executable(${VERIFY_EXE} verify_yaml.f90)
  target_link_libraries(${VERIFY_EXE} PRIVATE upp)

  # Since we want to run this in CI, let's add a test for a few files.
  # Note: verify_yaml.f90 needs BOTH txt and yaml to compare.
  add_test(NAME verify_hrrr_yaml
           COMMAND ${VERIFY_EXE} "${CMAKE_SOURCE_DIR}/parm/postxconfig-NT-hrrr.txt" "${CMAKE_SOURCE_DIR}/parm/postxconfig-NT-hrrr.yaml")
endif()

# Phase 2 C++ diagnostic kernels verification test
add_executable(verify_kernels test_kernels.cpp)
target_link_libraries(verify_kernels
    PRIVATE
        upp_kernels
        MPI::MPI_CXX
)
add_test(NAME verify_kernels
         COMMAND mpirun -np 1 $<TARGET_FILE:verify_kernels>)

# Phase 3 C++ dynamic I/O dataset parallel integration test
add_executable(generate_test_nc generate_test_nc.cpp)
target_link_libraries(generate_test_nc
    PRIVATE
        NetCDF::NetCDF_C
)
add_test(NAME verify_upp_cpp
         COMMAND bash -c "$<TARGET_FILE:generate_test_nc> && rm -f output_t.nc output_gh.nc output_rh.nc && mpirun -np 1 $<TARGET_FILE:upp_cpp>")

# Set the working directory for all tests to the root build folder where manifests are located
set_tests_properties(verify_upp_cpp verify_kernels
                     PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
