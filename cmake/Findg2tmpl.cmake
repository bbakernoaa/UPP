# cmake/Findg2tmpl.cmake
# Intercepts find_package(g2tmpl) and exposes an empty interface target since mock_g2tmpl.f90 is compiled directly inside the upp target.

if(NOT TARGET g2tmpl::g2tmpl)
    add_library(g2tmpl::g2tmpl INTERFACE IMPORTED GLOBAL)
    set(g2tmpl_FOUND TRUE)
    set(G2TMPL_FOUND TRUE)
endif()
