# FindLOGS.cmake
# Intercepts and satisfies nested find_package(LOGS) calls when LOGS is built inline.

if(TARGET HELM::LOGS)
    set(LOGS_FOUND TRUE)
    if(NOT TARGET logs)
        add_library(logs INTERFACE IMPORTED)
        target_link_libraries(logs INTERFACE HELM::LOGS)
    endif()
else()
    find_path(LOGS_INCLUDE_DIR logs/logs.hpp)
    find_library(LOGS_LIBRARY logs)
    
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LOGS
        REQUIRED_VARS LOGS_LIBRARY LOGS_INCLUDE_DIR
    )
    
    if(LOGS_FOUND)
        set(LOGS_INCLUDE_DIRS ${LOGS_INCLUDE_DIR})
        set(LOGS_LIBRARIES ${LOGS_LIBRARY})
        if(NOT TARGET HELM::LOGS)
            add_library(HELM::LOGS UNKNOWN IMPORTED)
            set_target_properties(HELM::LOGS PROPERTIES
                IMPORTED_LOCATION "${LOGS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LOGS_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()
