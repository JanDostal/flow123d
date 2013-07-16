#InstallBoost.cmake
#
# Created on: Jul 20, 2012
#     Author: jb
#


if (NOT EXTERNAL_BDDCML_DIR)
    set(EXTERNAL_BDDCML_DIR "${EXTERNAL_PROJECT_DIR}/bddcml_build")
endif()    

if (NOT EXTERNAL_BLOPEX_DIR)
    message(STATUS "Missing BLOPEX.")
endif()    

message(STATUS "=== Installing BDDCML ===")

############################################################################    
# generate fortran magle file    
# run separate CMake
execute_process(COMMAND ${CMAKE_COMMAND} ${PROJECT_SOURCE_DIR}/CMake/Modules/FCMagleHeader -DCMAKE_Fortran_COMPILER=${PETSC_VAR_FC} 
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/CMake/Modules/FCMagleHeader )
# path to the magling header is used in bddcml_make_inc.template    
set(FC_MAGLE_INCLUDE "${PROJECT_SOURCE_DIR}/CMake/Modules/FCMagleHeader")     

    
################################################################################    
# Create make.inc configuration file
set(MPI_Fortran_COMPILER  ${PETSC_VAR_FC} )
set(MPI_Fortran_COMPILE_FLAGS ${PETSC_VAR_FC_FLAGS} )
set(BDDCML_Fortran_COMPILER ${PETSC_VAR_FC})
set(BDDCML_Fortran_FLAGS ${PETSC_VAR_FC_FLAGS})

set(MPI_C_COMPILER ${PETSC_VAR_CC})
set(MPI_C_COMPILE_FLAGS ${PETSC_VAR_CC_FLAGS})

set(Parmetis_INCLUDES ${PETSC_VAR_PARMETIS_INCLUDE})
set(Metis_INCLUDES ${PETSC_VAR_METIS_INCLUDE})
set(Mumps_INCLUDES ${PETSC_VAR_MUMPS_INCLUDE})

set(BDDCML_ROOT  ${EXTERNAL_BDDCML_DIR}/src)
set(Mumps_LIBRARIES ${PETSC_VAR_MUMPS_LIB})
set(ScaLAPACK_LIBRARIES ${PETSC_VAR_SCALAPACK_LIB})
set(BLAS_LAPACK_LIBRARIES ${PETSC_VAR_BLASLAPACK_LIB})
set(Parmetis_LIBRARIES ${PETSC_VAR_PARMETIS_LIB})
set(Metis_LIBRARIES ${PETSC_VAR_METIS_LIB})
set(BLOPEX_DIR ${EXTERNAL_BLOPEX_DIR}/src/blopex-read-only)
set(MPI_LIBRARIES)

set(MAKE_INC ${EXTERNAL_BDDCML_DIR}/bddcml_make_inc)
message(STATUS "BDDCML_ROOT:"  ${BDDCML_ROOT})
configure_file(${PROJECT_SOURCE_DIR}/CMake/Modules/bddcml_make_inc.template ${MAKE_INC} @ONLY)




# A temporary CMakeLists.txt
#
# We use patch in order to use Fortran name magling detected by CMake
# to generate the patch use:
# 
#  diff -Naur f_symbol.h  new_f_symbol.h 
# 
set (cmakelists_fname "${EXTERNAL_BDDCML_DIR}/CMakeLists.txt")
file (WRITE "${cmakelists_fname}"
"
  ## This file was autogenerated by InstallBDDCML.cmake
  cmake_minimum_required(VERSION 2.8)
  include(ExternalProject)
  ExternalProject_Add(BDDCML
    DOWNLOAD_DIR ${EXTERNAL_BDDCML_DIR} 
    URL \"http://bacula.nti.tul.cz/~jan.brezina/flow123d_libraries/bddcml-2.2.tar.gz\"
    SOURCE_DIR ${EXTERNAL_BDDCML_DIR}/src
    BINARY_DIR ${EXTERNAL_BDDCML_DIR}/src
    PATCH_COMMAND patch ${EXTERNAL_BDDCML_DIR}/src/src/f_symbol.h ${PROJECT_SOURCE_DIR}/CMake/Modules/BDDCML_patch/f_symbol.h.patch
    CONFIGURE_COMMAND cp ${MAKE_INC} ${EXTERNAL_BDDCML_DIR}/src/make.inc
    BUILD_COMMAND make all
    INSTALL_COMMAND \"\"
  )
")

# run cmake
execute_process(COMMAND ${CMAKE_COMMAND} ${EXTERNAL_BDDCML_DIR} 
    WORKING_DIRECTORY ${EXTERNAL_BDDCML_DIR})

find_program (MAKE_EXECUTABLE NAMES make gmake)
# run make
execute_process(COMMAND ${MAKE_EXECUTABLE} VERBOSE=1 BDDCML
   WORKING_DIRECTORY ${EXTERNAL_BDDCML_DIR})    

# copy FC_Magle.h to BDDCML sources
file (COPY ${FC_MAGLE_INCLUDE}/FC_Magle.h DESTINATION ${EXTERNAL_BDDCML_DIR}/src/src )

message(STATUS "BDDCML build done")