        module xml_perl_data
!------------------------------------------------------------------------
!> @file 
!> @brief This module reads in Perl XML processed flat file or YAML file
!> and handles parameter marshalling for existing POST program
!> 
!> ### Program history log:
!> Date | Programmer | Comments
!> -----|------------|---------
!> March, 2015 | Lin Gan   | Initial Code
!> July,  2016 | J. Carley | Clean up prints 
!> July, 2024  | Wen Meng  | Increase datset length
!> May, 2025   | Ben Blake | Remove hardcoded value for tprec
!> May, 2026   | Jules     | Add YAML support using eckit
!>
!------------------------------------------------------------------------
!> @defgroup xml_perl_data_mod xml_perl_data
!> Sets parameters that are used to read in 
!! Perl XML processed flat file and handle parameter marshalling for 
!! existing POST program.
!
        implicit none
!
!> @ingroup xml_perl_data_mod 
!> @{ Parameters that are used to read in Perl XML processed flat file 
!!  and handle parameter marshalling for existing POST program.
   integer :: NFCST,NBC,LIST,IOUT,NTSTM,                 &
             NRADS,NRADL,NDDAMP,IDTAD,NBOCO,NSHDE,NCP,IMDLTY
!> @}

!> @ingroup xml_perl_data_mod 
!> @{ Parameters that are used to read in Perl XML processed flat file 
!! and handle parameter marshalling for existing POST program.
	  type param_t
	    integer                              :: post_avblfldidx=-9999
	    character(len=80)                    :: shortname=''
	    character(len=300)                   :: longname=''
	    integer                              :: mass_windpoint=1
	    character(len=30)                    :: pdstmpl='tmpl4_0'
	    character(len=30)                    :: pname=''
	    character(len=10)                    :: table_info=''
	    character(len=80)                    :: stats_proc=''
	    character(len=80)                    :: fixed_sfc1_type=''
       integer, dimension(:), pointer       :: scale_fact_fixed_sfc1 => null()
       real, dimension(:), pointer          :: level => null()
       character(len=80)                    :: fixed_sfc2_type=''
       integer, dimension(:), pointer       :: scale_fact_fixed_sfc2 => null() 
       real, dimension(:), pointer          :: level2 => null()
       character(len=80)                    :: aerosol_type=''
       character(len=80)                    :: prob_type=''
	    character(len=80)                    :: typ_intvl_size=''
 	    integer                              :: scale_fact_1st_size=0
	    real                                 :: scale_val_1st_size=0.0
	    integer                              :: scale_fact_2nd_size=0
	    real                                 :: scale_val_2nd_size=0.0
	    character(len=80)                    :: typ_intvl_wvlen=''
	    integer                              :: scale_fact_1st_wvlen=0
	    real                                 :: scale_val_1st_wvlen=0.0
	    integer                              :: scale_fact_2nd_wvlen=0
	    real                                 :: scale_val_2nd_wvlen=0.0
            integer                              :: scale_fact_lower_limit=0
            real                                 :: scale_val_lower_limit=0.0
            integer                              :: scale_fact_upper_limit=0
            real                                 :: scale_val_upper_limit=0.0
	    real, dimension(:), pointer          :: scale => null()  
	    integer                              :: stat_miss_val=0
	    integer                              :: leng_time_range_prev=0
	    integer                              :: time_inc_betwn_succ_fld=0
	    character(len=80)                    :: type_of_time_inc=''
	    character(len=20)                    :: stat_unit_time_key_succ=''
	    character(len=20)                    :: bit_map_flag=''
          end type param_t
!> @}

!> @ingroup xml_perl_data_mod
!> @{ Parameters that are used to read in Perl XML processed flat file
!! and handle parameter marshalling for existing POST program.
          type paramset_t
	    character(len=20)                     :: datset=''
	    integer                              :: grid_num=255
	    character(len=20)                     :: sub_center=''
	    character(len=20)                    :: version_no=''
	    character(len=20)                    :: local_table_vers_no=''
	    character(len=20)                    :: sigreftime=''
	    character(len=20)                    :: prod_status=''
	    character(len=20)                    :: data_type=''
	    character(len=20)                    :: gen_proc_type=''
	    character(len=30)                    :: time_range_unit=''
	    character(len=50)                    :: orig_center=''
	    character(len=30)                    :: gen_proc=''
	    character(len=50)                    :: packing_method=''
	    character(len=30)                    :: order_of_sptdiff='1st_ord_sptdiff'
	    character(len=20)                    :: field_datatype=''
	    character(len=30)                    :: comprs_type=''
!> @}
!> @ingroup xml_perl_data_mod 
!> @{ Parameters that are used to read in Perl XML processed flat file 
!! and handle parameter marshalling for existing POST program.
            character(len=50)                    :: type_ens_fcst=''
            character(len=50)                    :: type_derived_fcst=''
            type(param_t), dimension(:), pointer :: param => null()
          end type paramset_t
!> @}
!> @ingroup xml_perl_data_mod 
!> @{ Parameters that are used to read in Perl XML processed flat file 
!! and handle parameter marshalling for existing POST program. 
          type post_avblfld_t
            type(param_t), dimension(:), pointer :: param => null()
          end type post_avblfld_t
!> @}

!> @ingroup xml_perl_data_mod 
!> @{ Parameters that are used to read in Perl XML processed flat file 
!! and handle parameter marshalling for existing POST program. 
          type (paramset_t), dimension(:), pointer :: paramset
          type (post_avblfld_t),save               :: post_avblflds
!> @}

        interface
          integer(4) function yaml_load_file(filename) bind(C, name="yaml_load_file")
            use iso_c_binding
            character(kind=c_char), intent(in) :: filename(*)
          end function yaml_load_file

          subroutine yaml_free() bind(C, name="yaml_free")
          end subroutine yaml_free

          integer(4) function yaml_get_paramset_count() bind(C, name="yaml_get_paramset_count")
          end function yaml_get_paramset_count

          integer(4) function yaml_get_param_count(pset_idx) bind(C, name="yaml_get_param_count")
            integer(4), value :: pset_idx
          end function yaml_get_param_count

          subroutine yaml_get_paramset_string(pset_idx, key, out, out_len) bind(C, name="yaml_get_paramset_string")
            use iso_c_binding
            integer(4), value :: pset_idx
            character(kind=c_char), intent(in) :: key(*)
            character(kind=c_char), intent(out) :: out(*)
            integer(4), value :: out_len
          end subroutine yaml_get_paramset_string

          integer(4) function yaml_get_paramset_int(pset_idx, key, default_val) bind(C, name="yaml_get_paramset_int")
            use iso_c_binding
            integer(4), value :: pset_idx
            character(kind=c_char), intent(in) :: key(*)
            integer(4), value :: default_val
          end function yaml_get_paramset_int

          subroutine yaml_get_param_string(pset_idx, param_idx, key, out, out_len) bind(C, name="yaml_get_param_string")
            use iso_c_binding
            integer(4), value :: pset_idx
            integer(4), value :: param_idx
            character(kind=c_char), intent(in) :: key(*)
            character(kind=c_char), intent(out) :: out(*)
            integer(4), value :: out_len
          end subroutine yaml_get_param_string

          integer(4) function yaml_get_param_int(pset_idx, param_idx, key, default_val) bind(C, name="yaml_get_param_int")
            use iso_c_binding
            integer(4), value :: pset_idx
            integer(4), value :: param_idx
            character(kind=c_char), intent(in) :: key(*)
            integer(4), value :: default_val
          end function yaml_get_param_int

          real(8) function yaml_get_param_double(pset_idx, param_idx, key, default_val) bind(C, name="yaml_get_param_double")
            use iso_c_binding
            integer(4), value :: pset_idx
            integer(4), value :: param_idx
            character(kind=c_char), intent(in) :: key(*)
            real(8), value :: default_val
          end function yaml_get_param_double

          integer(4) function yaml_get_param_array_size(pset_idx, param_idx, key) bind(C, name="yaml_get_param_array_size")
            use iso_c_binding
            integer(4), value :: pset_idx
            integer(4), value :: param_idx
            character(kind=c_char), intent(in) :: key(*)
          end function yaml_get_param_array_size

          subroutine yaml_get_param_array_float(pset_idx, param_idx, key, out, out_len) bind(C, name="yaml_get_param_array_float")
            use iso_c_binding
            integer(4), value :: pset_idx
            integer(4), value :: param_idx
            character(kind=c_char), intent(in) :: key(*)
            real(4), intent(out) :: out(*)
            integer(4), value :: out_len
          end subroutine yaml_get_param_array_float

          subroutine yaml_get_param_array_int(pset_idx, param_idx, key, out, out_len) bind(C, name="yaml_get_param_array_int")
            use iso_c_binding
            integer(4), value :: pset_idx
            integer(4), value :: param_idx
            character(kind=c_char), intent(in) :: key(*)
            integer(4), intent(out) :: out(*)
            integer(4), value :: out_len
          end subroutine yaml_get_param_array_int
        end interface

        contains
!> @brief Reads in and processes the postxconfig file
        subroutine read_postxconfig()

         use rqstfld_mod,only: num_post_afld,MXLVL,lvlsxml
         use CTLBLK_mod, only:tprec,tclod,trdlw,trdsw,tsrfc &
                              ,tmaxmin,td3d,me,filenameflat
         use iso_c_binding, only: c_char, c_null_char
         implicit none

! Read in the flat file postxconfig-NT.txt or YAML file
! for current working parameters and param
	integer   paramset_count, param_count

! temp array count
        integer   cc
        integer   level_array_count
        integer   cv
        integer   level2_array_count
        integer   scale_array_count
        integer   i,j

! evil for empty default char "?"
        character(len=80)   testcharname
        character           dummy_char
        integer             testintname

        integer :: status, k
        character(len=256) :: c_filename

        if (index(filenameflat, ".yaml") > 0 .or. index(filenameflat, ".yml") > 0) then
          c_filename = trim(filenameflat)//c_null_char
          status = yaml_load_file(c_filename)
          if (status /= 0) then
            print *, "Error loading YAML file: ", trim(filenameflat)
            return
          endif

          paramset_count = yaml_get_paramset_count()

          if(associated(paramset)) then
            if(size(paramset)>0) then
              do i=1,size(paramset)
                if (associated(paramset(i)%param)) then
                  if (size(paramset(i)%param)>0) then
                    do j=1,size(paramset(i)%param)
                      if (associated(paramset(i)%param(j)%scale_fact_fixed_sfc1)) &
                          deallocate(paramset(i)%param(j)%scale_fact_fixed_sfc1)
                      if (associated(paramset(i)%param(j)%level)) &
                          deallocate(paramset(i)%param(j)%level)
                      if (associated(paramset(i)%param(j)%scale_fact_fixed_sfc2)) &
                          deallocate(paramset(i)%param(j)%scale_fact_fixed_sfc2)
                      if (associated(paramset(i)%param(j)%level2)) &
                          deallocate(paramset(i)%param(j)%level2)
                      if (associated(paramset(i)%param(j)%scale)) &
                          deallocate(paramset(i)%param(j)%scale)
                    enddo
                    deallocate(paramset(i)%param)
                    nullify(paramset(i)%param)
                  endif
                endif
              enddo
            endif
            deallocate(paramset)
          endif

          allocate(paramset(paramset_count))
          num_post_afld = 0
          do i = 1, paramset_count
            param_count = yaml_get_param_count(i-1)
            allocate(paramset(i)%param(param_count))
            num_post_afld = num_post_afld + param_count
          enddo

          if(allocated(lvlsxml)) deallocate(lvlsxml)
          allocate(lvlsxml(MXLVL,num_post_afld))
          lvlsxml = 0.0

          k = 0
          do i = 1, paramset_count
            call get_yaml_string(i-1, -1, "datset", paramset(i)%datset)
            paramset(i)%grid_num = yaml_get_paramset_int(i-1, "grid_num"//c_null_char, 255)
            call get_yaml_string(i-1, -1, "sub_center", paramset(i)%sub_center)
            call get_yaml_string(i-1, -1, "version_no", paramset(i)%version_no)
            call get_yaml_string(i-1, -1, "local_table_vers_no", paramset(i)%local_table_vers_no)
            call get_yaml_string(i-1, -1, "sigreftime", paramset(i)%sigreftime)
            call get_yaml_string(i-1, -1, "prod_status", paramset(i)%prod_status)
            call get_yaml_string(i-1, -1, "data_type", paramset(i)%data_type)
            call get_yaml_string(i-1, -1, "gen_proc_type", paramset(i)%gen_proc_type)
            call get_yaml_string(i-1, -1, "time_range_unit", paramset(i)%time_range_unit)
            call get_yaml_string(i-1, -1, "orig_center", paramset(i)%orig_center)
            call get_yaml_string(i-1, -1, "gen_proc", paramset(i)%gen_proc)
            call get_yaml_string(i-1, -1, "packing_method", paramset(i)%packing_method)
            call get_yaml_string(i-1, -1, "order_of_sptdiff", paramset(i)%order_of_sptdiff)
            call get_yaml_string(i-1, -1, "field_datatype", paramset(i)%field_datatype)
            call get_yaml_string(i-1, -1, "comprs_type", paramset(i)%comprs_type)

            if(paramset(i)%gen_proc_type=='ens_fcst')then
              call get_yaml_string(i-1, -1, "type_ens_fcst", paramset(i)%type_ens_fcst)
              tclod   = tprec
              trdlw   = tprec
              trdsw   = tprec
              tsrfc   = tprec
              tmaxmin = tprec
              td3d    = tprec
            end if

            param_count = size(paramset(i)%param)
            do j = 1, param_count
              k = k + 1
              paramset(i)%param(j)%post_avblfldidx = yaml_get_param_int(i-1, j-1, "post_avblfldidx"//c_null_char, -9999)
              call get_yaml_string(i-1, j-1, "shortname", paramset(i)%param(j)%shortname)
              call get_yaml_string(i-1, j-1, "longname", paramset(i)%param(j)%longname)
              paramset(i)%param(j)%mass_windpoint = yaml_get_param_int(i-1, j-1, "mass_windpoint"//c_null_char, 1)
              call get_yaml_string(i-1, j-1, "pdstmpl", paramset(i)%param(j)%pdstmpl)
              call get_yaml_string(i-1, j-1, "pname", paramset(i)%param(j)%pname)
              call get_yaml_string(i-1, j-1, "table_info", paramset(i)%param(j)%table_info)
              call get_yaml_string(i-1, j-1, "stats_proc", paramset(i)%param(j)%stats_proc)
              call get_yaml_string(i-1, j-1, "fixed_sfc1_type", paramset(i)%param(j)%fixed_sfc1_type)

              cc = yaml_get_param_array_size(i-1, j-1, "scale_fact_fixed_sfc1"//c_null_char)
              if (cc > 0) then
                allocate(paramset(i)%param(j)%scale_fact_fixed_sfc1(cc))
                call yaml_get_param_array_int(i-1, j-1, "scale_fact_fixed_sfc1"//c_null_char, &
                                              paramset(i)%param(j)%scale_fact_fixed_sfc1, cc)
              else
                allocate(paramset(i)%param(j)%scale_fact_fixed_sfc1(1))
                paramset(i)%param(j)%scale_fact_fixed_sfc1(1) = 0
              endif

              level_array_count = yaml_get_param_array_size(i-1, j-1, "level"//c_null_char)
              if (level_array_count > 0) then
                allocate(paramset(i)%param(j)%level(level_array_count))
                call get_yaml_array_real(i-1, j-1, "level", paramset(i)%param(j)%level, level_array_count)
                if (level_array_count <= MXLVL) then
                   lvlsxml(1:level_array_count, k) = paramset(i)%param(j)%level(1:level_array_count)
                endif
              else
                allocate(paramset(i)%param(j)%level(1))
                paramset(i)%param(j)%level(1) = 0.0
              endif

              call get_yaml_string(i-1, j-1, "fixed_sfc2_type", paramset(i)%param(j)%fixed_sfc2_type)
              cv = yaml_get_param_array_size(i-1, j-1, "scale_fact_fixed_sfc2"//c_null_char)
              if (cv > 0) then
                allocate(paramset(i)%param(j)%scale_fact_fixed_sfc2(cv))
                call yaml_get_param_array_int(i-1, j-1, "scale_fact_fixed_sfc2"//c_null_char, &
                                              paramset(i)%param(j)%scale_fact_fixed_sfc2, cv)
              else
                allocate(paramset(i)%param(j)%scale_fact_fixed_sfc2(1))
                paramset(i)%param(j)%scale_fact_fixed_sfc2(1) = 0
              endif

              level2_array_count = yaml_get_param_array_size(i-1, j-1, "level2"//c_null_char)
              if (level2_array_count > 0) then
                allocate(paramset(i)%param(j)%level2(level2_array_count))
                call get_yaml_array_real(i-1, j-1, "level2", paramset(i)%param(j)%level2, level2_array_count)
              else
                allocate(paramset(i)%param(j)%level2(1))
                paramset(i)%param(j)%level2(1) = 0.0
              endif

              call get_yaml_string(i-1, j-1, "aerosol_type", paramset(i)%param(j)%aerosol_type)
              call get_yaml_string(i-1, j-1, "prob_type", paramset(i)%param(j)%prob_type)
              call get_yaml_string(i-1, j-1, "typ_intvl_size", paramset(i)%param(j)%typ_intvl_size)
              paramset(i)%param(j)%scale_fact_1st_size = yaml_get_param_int(i-1, j-1, "scale_fact_1st_size"//c_null_char, 0)
              paramset(i)%param(j)%scale_val_1st_size = real(yaml_get_param_double(i-1, j-1, "scale_val_1st_size"//c_null_char, 0.0d0))
              paramset(i)%param(j)%scale_fact_2nd_size = yaml_get_param_int(i-1, j-1, "scale_fact_2nd_size"//c_null_char, 0)
              paramset(i)%param(j)%scale_val_2nd_size = real(yaml_get_param_double(i-1, j-1, "scale_val_2nd_size"//c_null_char, 0.0d0))
              call get_yaml_string(i-1, j-1, "typ_intvl_wvlen", paramset(i)%param(j)%typ_intvl_wvlen)
              paramset(i)%param(j)%scale_fact_1st_wvlen = yaml_get_param_int(i-1, j-1, "scale_fact_1st_wvlen"//c_null_char, 0)
              paramset(i)%param(j)%scale_val_1st_wvlen = real(yaml_get_param_double(i-1, j-1, "scale_val_1st_wvlen"//c_null_char, 0.0d0))
              paramset(i)%param(j)%scale_fact_2nd_wvlen = yaml_get_param_int(i-1, j-1, "scale_fact_2nd_wvlen"//c_null_char, 0)
              paramset(i)%param(j)%scale_val_2nd_wvlen = real(yaml_get_param_double(i-1, j-1, "scale_val_2nd_wvlen"//c_null_char, 0.0d0))
              paramset(i)%param(j)%scale_fact_lower_limit = yaml_get_param_int(i-1, j-1, "scale_fact_lower_limit"//c_null_char, 0)
              paramset(i)%param(j)%scale_val_lower_limit = real(yaml_get_param_double(i-1, j-1, "scale_val_lower_limit"//c_null_char, 0.0d0))
              paramset(i)%param(j)%scale_fact_upper_limit = yaml_get_param_int(i-1, j-1, "scale_fact_upper_limit"//c_null_char, 0)
              paramset(i)%param(j)%scale_val_upper_limit = real(yaml_get_param_double(i-1, j-1, "scale_val_upper_limit"//c_null_char, 0.0d0))

              scale_array_count = yaml_get_param_array_size(i-1, j-1, "scale"//c_null_char)
              if (scale_array_count > 0) then
                allocate(paramset(i)%param(j)%scale(scale_array_count))
                call get_yaml_array_real(i-1, j-1, "scale", paramset(i)%param(j)%scale, scale_array_count)
              else
                allocate(paramset(i)%param(j)%scale(1))
                paramset(i)%param(j)%scale(1) = 0.0
              endif

              paramset(i)%param(j)%stat_miss_val = yaml_get_param_int(i-1, j-1, "stat_miss_val"//c_null_char, 0)
              paramset(i)%param(j)%leng_time_range_prev = yaml_get_param_int(i-1, j-1, "leng_time_range_prev"//c_null_char, 0)
              paramset(i)%param(j)%time_inc_betwn_succ_fld = yaml_get_param_int(i-1, j-1, "time_inc_betwn_succ_fld"//c_null_char, 0)
              call get_yaml_string(i-1, j-1, "type_of_time_inc", paramset(i)%param(j)%type_of_time_inc)
              call get_yaml_string(i-1, j-1, "stat_unit_time_key_succ", paramset(i)%param(j)%stat_unit_time_key_succ)
              call get_yaml_string(i-1, j-1, "bit_map_flag", paramset(i)%param(j)%bit_map_flag)
            enddo
          enddo
          post_avblflds%param => paramset(1)%param
          call yaml_free()
          return
        endif

! open the Post flat file
!        open(UNIT=22,file="postxconfig-NT.txt",     &
        open(UNIT=22,file=trim(filenameflat),     &
             form="formatted", access="sequential", &
             status="old", position="rewind")

! Take the first line as paramset_count
	read(22,*)paramset_count

        if(associated(paramset)) then
          if(size(paramset)>0) then
            do i=1,size(paramset)
              if (associated(paramset(i)%param)) then
                if (size(paramset(i)%param)>0) then
                  do j=1,size(paramset(i)%param)
                    if (associated(paramset(i)%param(j)%scale_fact_fixed_sfc1)) &
                        deallocate(paramset(i)%param(j)%scale_fact_fixed_sfc1)
                    if (associated(paramset(i)%param(j)%level)) &
                        deallocate(paramset(i)%param(j)%level)
                    if (associated(paramset(i)%param(j)%scale_fact_fixed_sfc2)) &
                        deallocate(paramset(i)%param(j)%scale_fact_fixed_sfc2)
                    if (associated(paramset(i)%param(j)%level2)) &
                        deallocate(paramset(i)%param(j)%level2)
                    if (associated(paramset(i)%param(j)%scale)) &
                        deallocate(paramset(i)%param(j)%scale)
                  enddo
                  deallocate(paramset(i)%param)
                  nullify(paramset(i)%param)
                endif
              endif
            enddo
          endif
          deallocate(paramset)
        endif

! Allocate paramset array size
        allocate(paramset(paramset_count))

! Take the second line as param_count (on n..1 down loop)
! stored as FILO

! Initialize num_post_afld here
     num_post_afld = 0

        do i = paramset_count, 1, -1
          read(22,*)param_count

          allocate(paramset(i)%param(param_count))

! LinGan lvlsxml is now a sum of flat file read out
! Also allocate lvlsxml for rqstfld_mod
          num_post_afld = num_post_afld + param_count

        end do
        
        if(allocated(lvlsxml)) deallocate(lvlsxml)
        allocate(lvlsxml(MXLVL,num_post_afld))

! For each paramset_count to read in all 16 control contain
        do i = 1, paramset_count
! allocate array size from param for current paramset
! filter_char_inp is to check if "?" is found 
!   then replace to empty string because it means no input. 
          read(22,*)paramset(i)%datset
          call filter_char_inp(paramset(i)%datset)

          param_count = size (paramset(i)%param)

          read(22,*)paramset(i)%grid_num
          read(22,*)paramset(i)%sub_center
            call filter_char_inp(paramset(i)%sub_center)
          read(22,*)paramset(i)%version_no
            call filter_char_inp(paramset(i)%version_no)
          read(22,*)paramset(i)%local_table_vers_no
            call filter_char_inp(paramset(i)%local_table_vers_no)
          read(22,*)paramset(i)%sigreftime
            call filter_char_inp(paramset(i)%sigreftime)
          read(22,*)paramset(i)%prod_status
            call filter_char_inp(paramset(i)%prod_status)
          read(22,*)paramset(i)%data_type
            call filter_char_inp(paramset(i)%data_type)
          read(22,*)paramset(i)%gen_proc_type
            call filter_char_inp(paramset(i)%gen_proc_type)
          read(22,*)paramset(i)%time_range_unit
            call filter_char_inp(paramset(i)%time_range_unit)
          read(22,*)paramset(i)%orig_center
            call filter_char_inp(paramset(i)%orig_center)
          read(22,*)paramset(i)%gen_proc
            call filter_char_inp(paramset(i)%gen_proc)
          read(22,*)paramset(i)%packing_method
            call filter_char_inp(paramset(i)%packing_method)
          read(22,*)paramset(i)%order_of_sptdiff
          read(22,*)paramset(i)%field_datatype
            call filter_char_inp(paramset(i)%field_datatype)
          read(22,*)paramset(i)%comprs_type
            call filter_char_inp(paramset(i)%comprs_type)
          if(paramset(i)%gen_proc_type=='ens_fcst')then
            read(22,*)paramset(i)%type_ens_fcst
            call filter_char_inp(paramset(i)%type_ens_fcst)
            tclod   = tprec
            trdlw   = tprec
            trdsw   = tprec
            tsrfc   = tprec
            tmaxmin = tprec
            td3d    = tprec
          end if          
! Loop param_count (param datas 161) for gfsprs
	  do j = 1, param_count
	    read(22,*)paramset(i)%param(j)%post_avblfldidx
            read(22,*)paramset(i)%param(j)%shortname
            read(22,'(A300)')paramset(i)%param(j)%longname
              call filter_char_inp(paramset(i)%param(j)%longname)

            read(22,*)paramset(i)%param(j)%mass_windpoint
            read(22,*)paramset(i)%param(j)%pdstmpl
            read(22,*)paramset(i)%param(j)%pname
              call filter_char_inp(paramset(i)%param(j)%pname)

            read(22,*)paramset(i)%param(j)%table_info
              call filter_char_inp(paramset(i)%param(j)%table_info)
            read(22,*)paramset(i)%param(j)%stats_proc
              call filter_char_inp(paramset(i)%param(j)%stats_proc)
            read(22,*)paramset(i)%param(j)%fixed_sfc1_type
              call filter_char_inp(paramset(i)%param(j)%fixed_sfc1_type)
! Read array count for scale_fact_fixed_sfc1
            read(22,*)cc
!
            allocate( paramset(i)%param(j)%scale_fact_fixed_sfc1(1))

            if (cc > 0) then 
!  
              deallocate( paramset(i)%param(j)%scale_fact_fixed_sfc1)

              allocate( paramset(i)%param(j)%scale_fact_fixed_sfc1(cc))
              read(22,*)paramset(i)%param(j)%scale_fact_fixed_sfc1
            else
! If array count is zero dummy out the line
!
              paramset(i)%param(j)%scale_fact_fixed_sfc1(1)=0

              read(22,*)dummy_char
            endif

            read(22,*)level_array_count
            allocate( paramset(i)%param(j)%level(1))
            if (level_array_count > 0) then
              deallocate( paramset(i)%param(j)%level)
              allocate( paramset(i)%param(j)%level(level_array_count))
              read(22,*)paramset(i)%param(j)%level
            else
              paramset(i)%param(j)%level(1)=0
              read(22,*)dummy_char
            endif

            read(22,*)paramset(i)%param(j)%fixed_sfc2_type
              call filter_char_inp(paramset(i)%param(j)%fixed_sfc2_type)
            read(22,*)cv
         allocate( paramset(i)%param(j)%scale_fact_fixed_sfc2(1))
            if (cv > 0) then
              deallocate(paramset(i)%param(j)%scale_fact_fixed_sfc2)
              allocate(paramset(i)%param(j)%scale_fact_fixed_sfc2(cv))
              read(22,*)paramset(i)%param(j)%scale_fact_fixed_sfc2
            else
              paramset(i)%param(j)%scale_fact_fixed_sfc2(1)=0
              read(22,*)dummy_char
            endif

            read(22,*)level2_array_count
            if (level2_array_count > 0) then
              allocate(paramset(i)%param(j)%level2(level2_array_count))
              read(22,*)paramset(i)%param(j)%level2
            else
              read(22,*)dummy_char
            endif

            read(22,*)paramset(i)%param(j)%aerosol_type
              call filter_char_inp(paramset(i)%param(j)%aerosol_type)
            read(22,*)paramset(i)%param(j)%prob_type
              call filter_char_inp(paramset(i)%param(j)%prob_type)
            read(22,*)paramset(i)%param(j)%typ_intvl_size
              call filter_char_inp(paramset(i)%param(j)%typ_intvl_size)

            read(22,*)paramset(i)%param(j)%scale_fact_1st_size
            read(22,*)paramset(i)%param(j)%scale_val_1st_size
            read(22,*)paramset(i)%param(j)%scale_fact_2nd_size
            read(22,*)paramset(i)%param(j)%scale_val_2nd_size
            read(22,*)paramset(i)%param(j)%typ_intvl_wvlen
              call filter_char_inp(paramset(i)%param(j)%typ_intvl_wvlen)
 
            read(22,*)paramset(i)%param(j)%scale_fact_1st_wvlen
            read(22,*)paramset(i)%param(j)%scale_val_1st_wvlen
            read(22,*)paramset(i)%param(j)%scale_fact_2nd_wvlen
            read(22,*)paramset(i)%param(j)%scale_val_2nd_wvlen
            read(22,*)paramset(i)%param(j)%scale_fact_lower_limit
            read(22,*)paramset(i)%param(j)%scale_val_lower_limit
            read(22,*)paramset(i)%param(j)%scale_fact_upper_limit
            read(22,*)paramset(i)%param(j)%scale_val_upper_limit
            read(22,*)scale_array_count
            allocate(paramset(i)%param(j)%scale(1))
            if (scale_array_count > 0) then
              deallocate(paramset(i)%param(j)%scale)
              allocate(paramset(i)%param(j)%scale(scale_array_count))
              read(22,*)paramset(i)%param(j)%scale
            else
              paramset(i)%param(j)%scale(1)=0
              read(22,*)dummy_char
            endif  
            read(22,*)paramset(i)%param(j)%stat_miss_val
            read(22,*)paramset(i)%param(j)%leng_time_range_prev
            read(22,*)paramset(i)%param(j)%time_inc_betwn_succ_fld
            read(22,*)paramset(i)%param(j)%type_of_time_inc

              call filter_char_inp(paramset(i)%param(j)%type_of_time_inc)
            read(22,*)paramset(i)%param(j)%stat_unit_time_key_succ
              call filter_char_inp(paramset(i)%param(j)%stat_unit_time_key_succ)
            read(22,*)paramset(i)%param(j)%bit_map_flag
              call filter_char_inp(paramset(i)%param(j)%bit_map_flag)

! End of reading param
          end do

        post_avblflds%param => paramset(i)%param

! End of reading paramset
        end do
        close (UNIT=22)

        end subroutine read_postxconfig

        subroutine get_yaml_string(pset_idx, param_idx, key, out)
          use iso_c_binding, only: c_char, c_null_char
          integer, intent(in) :: pset_idx, param_idx
          character(*), intent(in) :: key
          character(*), intent(out) :: out
          character(len=80) :: c_key
          character(len=len(out)) :: c_out
          integer :: null_idx

          c_key = trim(key)//c_null_char
          if (param_idx < 0) then
            call yaml_get_paramset_string(pset_idx, c_key, c_out, len(c_out))
          else
            call yaml_get_param_string(pset_idx, param_idx, c_key, c_out, len(c_out))
          endif
          null_idx = index(c_out, c_null_char)
          if (null_idx > 0) then
            out = c_out(1:null_idx-1)
          else
            out = c_out
          endif
          call filter_char_inp_string(out)
        end subroutine get_yaml_string

        subroutine get_yaml_array_real(pset_idx, param_idx, key, out, n)
          use iso_c_binding, only: c_char, c_null_char
          integer, intent(in) :: pset_idx, param_idx, n
          character(*), intent(in) :: key
          real, intent(out) :: out(n)
          real(4), allocatable :: tmp(:)
          character(len=80) :: c_key

          c_key = trim(key)//c_null_char
          allocate(tmp(n))
          call yaml_get_param_array_float(pset_idx, param_idx, c_key, tmp, n)
          out = tmp
          deallocate(tmp)
        end subroutine get_yaml_array_real

!> @brief Checks parameter set to see whether "?" is found and, if so, replaces it with an empty string because it means no input.
!> @param[inout] inpchar Input character
        subroutine filter_char_inp (inpchar)
          implicit none
          character, intent(inout)    :: inpchar
          if (inpchar == "?") then
            inpchar = ""
          endif
        end subroutine filter_char_inp

        subroutine filter_char_inp_string (inpstr)
          implicit none
          character(*), intent(inout)    :: inpstr
          if (trim(inpstr) == "?") then
            inpstr = ""
          endif
        end subroutine filter_char_inp_string

        end module
