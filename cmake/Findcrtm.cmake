# cmake/Findcrtm.cmake
# Intercepts find_package(crtm) and exposes an empty interface target since mock_crtm.f90 is compiled directly inside the upp target.

if(NOT TARGET crtm::crtm)
    add_library(crtm::crtm INTERFACE IMPORTED GLOBAL)
    set(crtm_FOUND TRUE)
    set(CRTM_FOUND TRUE)
endif()
