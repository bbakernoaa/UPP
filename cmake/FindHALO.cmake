# FindHALO.cmake
# Intercepts and satisfies nested find_package(HALO) calls when HALO is built inline.

if(TARGET HELM::HALO)
    set(HALO_FOUND TRUE)
    if(NOT TARGET halo)
        add_library(halo INTERFACE IMPORTED)
        target_link_libraries(halo INTERFACE HELM::HALO)
    endif()
else()
    find_path(HALO_INCLUDE_DIR halo/halo.hpp)
    find_library(HALO_LIBRARY halo)
    
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(HALO
        REQUIRED_VARS HALO_LIBRARY HALO_INCLUDE_DIR
    )
    
    if(HALO_FOUND)
        set(HALO_INCLUDE_DIRS ${HALO_INCLUDE_DIR})
        set(HALO_LIBRARIES ${HALO_LIBRARY})
        if(NOT TARGET HELM::HALO)
            add_library(HELM::HALO UNKNOWN IMPORTED)
            set_target_properties(HELM::HALO PROPERTIES
                IMPORTED_LOCATION "${HALO_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${HALO_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()
