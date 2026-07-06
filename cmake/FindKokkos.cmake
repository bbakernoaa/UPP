# FindKokkos.cmake
# Intercepts and satisfies nested find_package(Kokkos) calls when Kokkos is built inline via FetchContent.

if(TARGET Kokkos::kokkos)
    set(Kokkos_FOUND TRUE)
    # Define standard outputs expected by FPHSA
    set(Kokkos_INCLUDE_DIRS "")
    set(Kokkos_LIBRARIES Kokkos::kokkos)
    
    if(NOT TARGET Kokkos)
        add_library(Kokkos INTERFACE IMPORTED)
        target_link_libraries(Kokkos INTERFACE Kokkos::kokkos)
    endif()
else()
    # Fallback to standard search if not built inline
    find_path(Kokkos_INCLUDE_DIR Kokkos_Core.hpp)
    find_library(Kokkos_LIBRARY kokkos)
    
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Kokkos
        REQUIRED_VARS Kokkos_LIBRARY Kokkos_INCLUDE_DIR
    )
    
    if(Kokkos_FOUND)
        set(Kokkos_INCLUDE_DIRS ${Kokkos_INCLUDE_DIR})
        set(Kokkos_LIBRARIES ${Kokkos_LIBRARY})
        if(NOT TARGET Kokkos::kokkos)
            add_library(Kokkos::kokkos UNKNOWN IMPORTED)
            set_target_properties(Kokkos::kokkos PROPERTIES
                IMPORTED_LOCATION "${Kokkos_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Kokkos_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()
