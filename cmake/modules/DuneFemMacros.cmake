#find endian headers
set(ENDIAN_HEADER_ROOT "" CACHE STRING "path of endian header")
check_include_file_cxx(endian.h HAVE_ENDIAN_HEADER_HH)
if( HAVE_ENDIAN_HEADER_HH )
  find_path(SYSTEM_ENDIAN_HEADER_PATH
    NAMES endian.h
    PATHS ${ENDIAN_HEADER_ROOT}
    DOC "Path where endian.h was found"
  )
  if(EXISTS SYSTEM_ENDIAN_HEADER_PATH)
    set(SYSTEM_ENDIAN_HEADER "<endian.h>")
  endif(EXISTS SYSTEM_ENDIAN_HEADER_PATH)
else(HAVE_ENDIAN_HEADER_HH)
  check_include_file_cxx(machine/endian.h HAVE_ENDIAN_MACHINE_HEADER_HH)
  if( HAVE_ENDIAN_MACHINE_HEADER_HH )
    find_path(SYSTEM_ENDIAN_HEADER_PATH
      NAMES endian.h
      PATHS "${ENDIAN_HEADER_ROOT}/machine"
      PATH_SUFFIXES "machine"
      DOC "Path where machine/endian.h was found"
    )
    if(EXISTS SYSTEM_ENDIAN_HEADER_PATH)
      set(SYSTEM_ENDIAN_HEADER "<machine/endian.h>")
    endif(EXISTS SYSTEM_ENDIAN_HEADER_PATH)
  endif(HAVE_ENDIAN_MACHINE_HEADER_HH)
endif(HAVE_ENDIAN_HEADER_HH)
mark_as_advanced(ENDIAN_HEADER_ROOT SYSTEM_ENDIAN_HEADER_PATH)

include(CheckCXXSourceCompiles)

# check for OpenMP
set(USE_OPENMP OFF CACHE BOOL "whether we are using OpenMP.")
# if open mp should be used perform cmake check
if(USE_OPENMP)
  include(FindOpenMP)
  if(OPENMP_FOUND)
    # add flags to compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif(OPENMP_FOUND)
endif(USE_OPENMP)

# check for phtreads
include(FindPThreads)

###############################
# end pthreads
################################

find_package(SIONlib)
include(AddSIONlibFlags)
find_package(PAPI)
include(AddPAPIFlags)

# PETSC_ROOT overrules PETSC_DIR which overrules ENV PETSC_DIR
if( PETSC_ROOT )
  set(PETSC_DIR ${PETSC_ROOT})
endif()

if( NOT PETSC_DIR )
  set(PETSC_DIR $ENV{PETSC_DIR})
else()
  # set PETSC_DIR as environment variable to avoid trouble
  set(ENV{PETSC_DIR} ${PETSC_DIR})
endif()
if( NOT PETSC_ARCH )
  set(PETSC_ARCH $ENV{PETSC_ARCH})
else()
  # set PETSC_ARCH as environment variable to avoid trouble
  set(ENV{PETSC_ARCH} ${PETSC_ARCH})
endif()

find_package(PETSc)
if(PETSC_FOUND)
  # set HAVE_PETSC for config.h
  set(HAVE_PETSC ${PETSC_FOUND})
  # log result
  file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
    "Determining location of PETSC succeded:\n"
    "Include directory: ${PETSC_INCLUDES}\n"
    "Library directory: grep${PETSC_LIBRARIES}\n\n")
  set(PETSC_DUNE_COMPILE_FLAGS "-DENABLE_PETSC=1")
  foreach(dir ${PETSC_INCLUDES})
    set(PETSC_DUNE_COMPILE_FLAGS "${PETSC_DUNE_COMPILE_FLAGS} -I${dir}")
  endforeach()
  set(PETSC_DUNE_COMPILE_FLAGS "${PETSC_DUNE_COMPILE_FLAGS}"
    CACHE STRING "Compile Flags used by DUNE when compiling with PETSc programs")
  set(PETSC_DUNE_LIBRARIES ${PETSC_LIBRARIES}
    CACHE STRING "Libraries used by DUNE when linking PETSc programs")
  foreach(dir ${PETSC_INCLUDES})
    set_property(GLOBAL APPEND PROPERTY ALL_PKG_FLAGS "-I${dir}")
  endforeach()
  set_property(GLOBAL APPEND PROPERTY ALL_PKG_LIBS "${PETSC_LIBRARIES}")
  # register includes and libs to global flags
  dune_register_package_flags(COMPILE_DEFINITIONS "ENABLE_PETSC=1"
                              INCLUDE_DIRS ${PETSC_INCLUDES}
                              LIBRARIES ${PETSC_LIBRARIES})
else()
  # log errornous result
  file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
    "Determing location of PETSc failed:\n"
    "Include directory: ${PETSC_INCLUDES}\n"
    "Library directory: ${PETSC_LIBRARIES}\n\n")
endif()

# AMGX solver (AmgXWrapper) needs PETSC matrix structures, therefore PETSC is needed
if(PETSC_FOUND)
  find_package(AmgXSolver)
endif()

# check for Eigen3
find_package(Eigen3 CONFIG)
if(EIGEN3_FOUND)
  set(HAVE_EIGEN 1)
  add_definitions(${EIGEN3_DEFINITIONS})
  include_directories(${EIGEN3_INCLUDE_DIRS})
endif()

find_package(ViennaCL)

####### abbreviations
include(FemShort)

####### hacks
include(CommandLineHacks)

# check SuiteSparse support
find_package(SuiteSparse OPTIONAL_COMPONENTS UMFPACK SPQR LDL)
include(AddSuiteSparseFlags)

# torture tests AKA nighly builds
include(FemTortureTests)
