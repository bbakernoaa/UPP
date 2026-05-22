enable_testing()

set(VERIFY_EXE verify_yaml.x)
add_executable(${VERIFY_EXE} verify_yaml.f90)
target_link_libraries(${VERIFY_EXE} PRIVATE upp)

# Since we want to run this in CI, let's add a test for a few files.
# Note: verify_yaml.f90 needs BOTH txt and yaml to compare.
# If we are deleting txt files, we can't run this comparison test in CI.
# But for now, we kept them.

add_test(NAME verify_hrrr_yaml
         COMMAND ${VERIFY_EXE} "${CMAKE_SOURCE_DIR}/parm/postxconfig-NT-hrrr.txt" "${CMAKE_SOURCE_DIR}/parm/postxconfig-NT-hrrr.yaml")
