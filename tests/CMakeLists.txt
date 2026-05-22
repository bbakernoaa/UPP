set(VERIFY_EXE verify_yaml.x)
add_executable(${VERIFY_EXE} verify_yaml.f90)
target_link_libraries(${VERIFY_EXE} PRIVATE upp)
