      program verify_yaml
      use xml_perl_data
      use CTLBLK_mod, only: filenameflat
      implicit none

      character(len=256) :: file_txt, file_yaml
      type(paramset_t), pointer :: pset_txt(:), pset_yaml(:)
      integer :: i, j, k, n, status
      logical :: match

      call get_command_argument(1, file_txt)
      call get_command_argument(2, file_yaml)

      print *, "Comparing ", trim(file_txt), " and ", trim(file_yaml)

      ! Load TXT
      filenameflat = file_txt
      call read_postxconfig()
      allocate(pset_txt(size(paramset)))
      do i = 1, size(paramset)
        pset_txt(i)%datset = paramset(i)%datset
        ! Copying everything would be long, let's just do a deep enough check
        allocate(pset_txt(i)%param(size(paramset(i)%param)))
        do j = 1, size(paramset(i)%param)
          pset_txt(i)%param(j) = paramset(i)%param(j)
        end do
      end do

      ! Load YAML
      filenameflat = file_yaml
      call read_postxconfig()
      allocate(pset_yaml(size(paramset)))
      do i = 1, size(paramset)
        pset_yaml(i)%datset = paramset(i)%datset
        allocate(pset_yaml(i)%param(size(paramset(i)%param)))
        do j = 1, size(paramset(i)%param)
          pset_yaml(i)%param(j) = paramset(i)%param(j)
        end do
      end do

      if (size(pset_txt) /= size(pset_yaml)) then
        print *, "Paramset count mismatch: ", size(pset_txt), size(pset_yaml)
        stop 1
      end if

      do i = 1, size(pset_txt)
        if (pset_txt(i)%datset /= pset_yaml(i)%datset) then
           print *, "Datset mismatch at ", i, ": ", pset_txt(i)%datset, " vs ", pset_yaml(i)%datset
           stop 1
        end if
        if (size(pset_txt(i)%param) /= size(pset_yaml(i)%param)) then
           print *, "Param count mismatch in pset ", i, ": ", size(pset_txt(i)%param), size(pset_yaml(i)%param)
           stop 1
        end if
        do j = 1, size(pset_txt(i)%param)
           if (pset_txt(i)%param(j)%shortname /= pset_yaml(i)%param(j)%shortname) then
              print *, "Shortname mismatch at ", i, j, ": ", pset_txt(i)%param(j)%shortname, " vs ", pset_yaml(i)%param(j)%shortname
              stop 1
           end if
           if (size(pset_txt(i)%param(j)%level) /= size(pset_yaml(i)%param(j)%level)) then
              print *, "Level size mismatch at ", i, j, ": ", size(pset_txt(i)%param(j)%level), size(pset_yaml(i)%param(j)%level)
              stop 1
           end if
        end do
      end do

      print *, "Verification SUCCESSFUL"

      end program verify_yaml
