# This is the CMake build file for EMC_post (also known as UPP).
#
cmake_minimum_required(VERSION 3.15)

file(STRINGS "VERSION" pVersion LIMIT_COUNT 1)

project(
  upp
  VERSION ${pVersion}
  LANGUAGES Fortran C CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Handle user options.
option(OPENMP "use OpenMP threading" ON)
option(BUILD_POSTEXEC "Build NCEPpost executable" ON)
option(BUILD_WITH_NEMSIO "Build NCEPpost with nemsio library" ON)
option(BUILD_WITH_WRFIO "Build NCEPpost with WRF-IO library" OFF)
option(BUILD_WITH_IFI "Build NCEPpost with In-Flight Icing (IFI) library if present" OFF)
option(REQUIRE_IFI "Abort if libIFI is not found ; enables BUILD_WITH_IFI=ON" OFF)
option(BUILD_WITH_GTG   "Build NCEPpost with NCAR/GTG" OFF)
option(ENABLE_DOCS "Enable generation of doxygen-based documentation." OFF)
option(BUILD_IFI_EXECUTABLES "Build libIFI FIP2-lookalike and test programs." OFF)

if(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release|RelWithDebInfo|MinSizeRel)$")
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()

if(NOT CMAKE_Fortran_COMPILER_ID MATCHES "^(Intel|IntelLLVM|GNU|Clang|AppleClang)$")
  message(WARNING "Compiler not officially supported: ${CMAKE_Fortran_COMPILER_ID}")
endif()

find_package(MPI REQUIRED COMPONENTS Fortran CXX)
find_package(Kokkos QUIET)
if(NOT Kokkos_FOUND)
  message(STATUS "Kokkos not found locally. Fetching via FetchContent...")
  include(FetchContent)
  FetchContent_Declare(
      kokkos
      GIT_REPOSITORY https://github.com/kokkos/kokkos.git
      GIT_TAG 5.1.1
  )
  FetchContent_MakeAvailable(kokkos)
  
  # Generate a dummy KokkosConfig.cmake for inline subprojects (ArborX, etc.)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cmake/Kokkos")
  file(WRITE "${CMAKE_BINARY_DIR}/cmake/Kokkos/KokkosConfig.cmake" "
      set(Kokkos_FOUND TRUE)
      set(Kokkos_VERSION \"5.1.1\")
      if(NOT TARGET Kokkos::kokkos)
          add_library(Kokkos::kokkos ALIAS kokkos)
      endif()
  ")
  file(WRITE "${CMAKE_BINARY_DIR}/cmake/Kokkos/KokkosConfigVersion.cmake" "
      set(PACKAGE_VERSION \"5.1.1\")
      set(PACKAGE_VERSION_COMPATIBLE TRUE)
  ")
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/cmake/Kokkos")
endif()

# Fetch kokkos/mdspan reference implementation to support C++20 standard compiler fallbacks
message(STATUS "Fetching mdspan reference implementation via FetchContent...")
FetchContent_Declare(
    mdspan
    GIT_REPOSITORY https://github.com/kokkos/mdspan.git
    GIT_TAG mdspan-0.6.0
)
FetchContent_MakeAvailable(mdspan)

find_package(NetCDF REQUIRED COMPONENTS Fortran)

if(OPENMP)
  find_package(OpenMP REQUIRED COMPONENTS Fortran)
endif()

if(APPLE)
  # The linker on macOS does not include `common symbols` by default
  # Passing the -c flag includes them and fixes an error with undefined symbols
  set(CMAKE_Fortran_ARCHIVE_FINISH "<CMAKE_RANLIB> -c <TARGET>")
endif()

if(BUILD_POSTEXEC)
  find_package(bacio REQUIRED)
  find_package(crtm REQUIRED)
  if(TARGET crtm AND NOT TARGET crtm::crtm)
    add_library(crtm::crtm INTERFACE IMPORTED GLOBAL)
    target_link_libraries(crtm::crtm INTERFACE crtm)
  endif()
  find_package(g2 REQUIRED)
  find_package(g2tmpl REQUIRED)
  if(BUILD_WITH_GTG)
    find_package(ip REQUIRED)
  endif()
endif()

if(INTERNAL_IFI)
  if(BUILD_WITH_IFI)
    message(SEND_ERROR "Cannot use INTERNAL_IFI with either BUILD_WITH_IFI or REQUIRE_IFI.")
  endif()
  if(IFI_DIR)
    message(SEND_ERROR "When INTERNAL_IFI is specified, do not set IFI_DIR.")
  endif()
  set(REQUIRE_IFI ON)
  set(BUILD_WITH_IFI ON)
  set(IFI_FOUND ON)
  set(IFI_DIR ${CMAKE_INSTALL_DIR})
  message(NOTICE "Building IFI inside the post.")
else()
  if(REQUIRE_IFI)
    set(BUILD_WITH_IFI ON)
  endif()

  if(BUILD_WITH_IFI)
    if(REQUIRE_IFI)
      find_package(IFI REQUIRED)
    else()
      find_package(IFI)
    endif()
  endif()
  if(IFI_FOUND)
    message(NOTICE "Linking to IFI external to the post.")
  endif()
endif()

if(BUILD_IFI_EXECUTABLES)
  if(NOT INTERNAL_IFI)
    message(SEND_ERROR "Can only BUILD_IFI_EXECUTABLES when INTERNAL_IFI is enabled.")
  else()
    message(NOTICE "Building libIFI FIP2-lookalike and test programs.")
  endif()
endif()

if(BUILD_POSTEXEC)
  if(BUILD_WITH_NEMSIO)
    find_package(nemsio REQUIRED)
    find_package(sigio REQUIRED)
  endif()
  find_package(sp QUIET)
  find_package(ip 5.0 QUIET)
  if((NOT ip_VERSION VERSION_GREATER_EQUAL 5.0) AND (NOT sp_FOUND))
    message(SEND_ERROR "UPP requires sp or ip v5+.")
  endif()
  find_package(w3emc REQUIRED)
  if(BUILD_WITH_WRFIO)
    find_package(wrf_io REQUIRED)
  endif()
  if(CMAKE_Fortran_COMPILER_ID MATCHES "^Intel$")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -cxxlib")
  elseif(CMAKE_Fortran_COMPILER_ID MATCHES "^(IntelLLVM|GNU)$")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++")
  elseif(CMAKE_Fortran_COMPILER_ID MATCHES "^(Clang|AppleClang)$")
    # This one is a wild guess. I haven't tried linking a Fortran
    # executable with C++ libraries in clang.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
  endif()
endif()

if(IFI_FOUND OR INTERNAL_IFI)
  message(NOTICE "Enabling USE_IFI in the post.")
  add_definitions(-DUSE_IFI=1)
endif()

add_subdirectory(sorc)

# If desired, build the doxygen docs.
if(ENABLE_DOCS)
  find_package(Doxygen REQUIRED)
  add_subdirectory(doc)
endif()

if(BUILD_POSTEXEC)
  ### Package config
  include(CMakePackageConfigHelpers)
  set(CONFIG_INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

  export(EXPORT ${PROJECT_NAME}Exports
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-targets.cmake)

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PackageConfig.cmake.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DESTINATION})
  install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    DESTINATION ${CONFIG_INSTALL_DESTINATION})

  write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CONFIG_INSTALL_DESTINATION})

  install(EXPORT ${PROJECT_NAME}Exports
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-targets.cmake
    DESTINATION ${CONFIG_INSTALL_DESTINATION})
endif()

enable_testing()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/amio_manifest.yaml ${CMAKE_CURRENT_BINARY_DIR}/amio_manifest.yaml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/amio_output_t.yaml ${CMAKE_CURRENT_BINARY_DIR}/amio_output_t.yaml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/amio_output_gh.yaml ${CMAKE_CURRENT_BINARY_DIR}/amio_output_gh.yaml COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/amio_output_rh.yaml ${CMAKE_CURRENT_BINARY_DIR}/amio_output_rh.yaml COPYONLY)

add_subdirectory(tests)
