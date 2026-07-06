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

# Modernized C++ driver parallel verification test
add_test(NAME verify_upp_cpp
         COMMAND mpirun -np 2 $<TARGET_FILE:upp_cpp>)

# Phase 2 C++ diagnostic kernels verification test
add_executable(verify_kernels test_kernels.cpp)
target_link_libraries(verify_kernels
    PRIVATE
        upp_kernels
        MPI::MPI_CXX
)
add_test(NAME verify_kernels
         COMMAND mpirun -np 1 $<TARGET_FILE:verify_kernels>)
