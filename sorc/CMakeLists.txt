if(INTERNAL_IFI)
  add_subdirectory(libIFI.fd)
endif()

add_subdirectory(helm/libs/logs)
add_subdirectory(helm/libs/halo)
add_subdirectory(helm/libs/tick)
add_subdirectory(helm/libs/dagr)
add_subdirectory(upp_cpp)

if(BUILD_POSTEXEC)
  add_subdirectory(ncep_post.fd)
endif()
