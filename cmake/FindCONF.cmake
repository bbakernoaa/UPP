# FindCONF.cmake
# Intercepts find_package(CONF) and returns success if the HELM::CONF target is defined.

if(TARGET HELM::CONF)
    set(CONF_FOUND TRUE)
else()
    # Fallback to standard check if not built inline
    find_path(CONF_INCLUDE_DIR conf/config.hpp)
    find_library(CONF_LIBRARY conf)
    
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(CONF
        REQUIRED_VARS CONF_LIBRARY CONF_INCLUDE_DIR
    )
    if(CONF_FOUND)
        if(NOT TARGET HELM::CONF)
            add_library(HELM::CONF UNKNOWN IMPORTED)
            set_target_properties(HELM::CONF PROPERTIES
                IMPORTED_LOCATION "${CONF_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CONF_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()
