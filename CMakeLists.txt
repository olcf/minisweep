#==============================================================================

cmake_minimum_required(VERSION 3.9)

# Minisweep is pure C, but for HIP backend, requires .cpp file extensions for hipcc
project("minisweep" LANGUAGES C CXX)

# Build options:
SET(NM_VALUE "16" CACHE STRING "The number of moments. Typical values are 1, 4, 16, and 36.")
SET(NU_VALUE "4" CACHE STRING "The number of unknowns per gridcell, moment, energy group.")
SET(RELAXED_TESTING OFF CACHE BOOL "If set, do not strictly test the answers.")
SET(USE_KBA ON CACHE BOOL "Use the Koch–Baker–Alcouffe parallel wavefront sweep algorithm.")
SET(USE_MPI OFF CACHE BOOL "Enable MPI.")

# Performance-related options:
SET(USE_ACCELDIR ON CACHE BOOL "I'm really not sure...")
SET(USE_HIP OFF CACHE BOOL "Enable HIP device offloading.")
SET(USE_CUDA OFF CACHE BOOL "Enable CUDA device offloading.")
SET(USE_ACC OFF CACHE BOOL "Enable OpenACC acceleration.")
SET(USE_OMP OFF CACHE BOOL "Enable host-based OpenMP.")
SET(USE_OMP_VO_ATOMIC OFF CACHE BOOL "Enables use of the atomic OpenMP region to update of the VO vector.")
SET(USE_OMP_TASKS OFF CACHE BOOL "Enable host OpenMP tasks. Otherwise, use OpenMP threads.")
SET(USE_OMP_OFFLOAD OFF CACHE BOOL "Enable OpenMP target offloading.")

# Set values of needed flags
#SET(OMP_FLAGS "-fopenmp" CACHE STRING "C/C++ compiler flags to enable the requested host OpenMP functionality.")
SET(OMP_OFFLOAD_FLAGS "-fopenmp" CACHE STRING "C/C++ compiler flags to enable OpenMP Offloading.")

SET(INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/src/1_base
  ${CMAKE_SOURCE_DIR}/src/2_sweeper_base
  ${CMAKE_SOURCE_DIR}/src/3_sweeper
  ${CMAKE_SOURCE_DIR}/src/4_driver
  )

SET(SOURCES
  src/1_base/arguments.c
  src/1_base/env.c
  src/1_base/env_assert.c
  src/1_base/env_cuda.c
  src/1_base/env_mpi.c
  src/1_base/pointer.c
  src/2_sweeper_base/array_operations.c
  src/2_sweeper_base/dimensions.c
  src/3_sweeper/faces_kba.c
  src/3_sweeper/quantities.c
  src/3_sweeper/stepscheduler_kba.c
  src/3_sweeper/sweeper.c
  src/3_sweeper/sweeper_kernels.c
  src/4_driver/runner.c
  )

IF(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -Wmissing-prototypes -Dinline=")
ENDIF()

IF(CMAKE_C_COMPILER_ID STREQUAL "Intel")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Dinline=__forceinline -D__restrict__=restrict -restrict")
ENDIF()

# Standard options required to set -- NM and NU values
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNM_VALUE=${NM_VALUE} -DNU_VALUE=${NU_VALUE}")

# Optional general configurations that can be set with any acceleration package
IF(USE_MPI)
  find_package(MPI REQUIRED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_MPI")
ENDIF()

IF(USE_KBA)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_KBA")
ENDIF()

IF(USE_ACCELDIR)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_ACCELDIR")
ENDIF()

IF(RELAXED_TESTING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DRELAXED_TESTING")
ENDIF()

IF(NOT USE_CUDA)
  # CUDA uses CUDA_INCLUDE_DIRECTORIES. Everything else uses INLCUDE_DIRECTORIES
  INCLUDE_DIRECTORIES(${INCLUDE_DIRS})
ENDIF()

# Only one acceleration method can be enabled at a time
IF(USE_OMP)
  find_package(OpenMP REQUIRED)

  IF(USE_OMP_TASKS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -DUSE_OPENMP -DUSE_OPENMP_TASKS")
  ELSE()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS} -DUSE_OPENMP -DUSE_OPENMP_THREADS")
  ENDIF()

  ADD_LIBRARY(sweeper STATIC ${SOURCES})
  ADD_EXECUTABLE(sweep src/4_driver/sweep.c)
  TARGET_LINK_LIBRARIES(sweep sweeper)
  ADD_EXECUTABLE(tester src/4_driver/tester.c)
  TARGET_LINK_LIBRARIES(tester sweeper)
ELSEIF(USE_OMP_OFFLOAD)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OMP_OFFLOAD_FLAGS} -DUSE_OPENMP_TARGET")
  ADD_LIBRARY(sweeper STATIC ${SOURCES})
  ADD_EXECUTABLE(sweep src/4_driver/sweep.c)
  TARGET_LINK_LIBRARIES(sweep sweeper)
  ADD_EXECUTABLE(tester src/4_driver/tester.c)
  TARGET_LINK_LIBRARIES(tester sweeper)
ELSEIF(USE_CUDA)
  find_package(CUDA REQUIRED)

  SET(CUDA_SOURCES)
  FOREACH(FILE IN LISTS SOURCES)
    STRING(REPLACE ".c" ".cu" FILE2 ${FILE})
    SET(CUDA_SOURCES ${CUDA_SOURCES} ${FILE2})
  ENDFOREACH()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_CUDA")
  STRING(REPLACE " " ";" CMAKE_C_FLAGS_ADJUSTED ${CMAKE_C_FLAGS})
  SET(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};${CMAKE_C_FLAGS_ADJUSTED}")
  CUDA_INCLUDE_DIRECTORIES(${INCLUDE_DIRS})
  CUDA_ADD_LIBRARY(sweeper STATIC ${CUDA_SOURCES})
  CUDA_ADD_EXECUTABLE(sweep src/4_driver/sweep.cu)
  TARGET_LINK_LIBRARIES(sweep sweeper)
  CUDA_ADD_EXECUTABLE(tester src/4_driver/tester.cu)
  TARGET_LINK_LIBRARIES(tester sweeper)
ELSE()
  ADD_LIBRARY(sweeper STATIC ${SOURCES})
  ADD_EXECUTABLE(sweep src/4_driver/sweep.c)
  TARGET_LINK_LIBRARIES(sweep sweeper)
  ADD_EXECUTABLE(tester src/4_driver/tester.c)
  TARGET_LINK_LIBRARIES(tester sweeper)
ENDIF()

install(TARGETS sweep DESTINATION bin)
install(TARGETS tester DESTINATION bin)

#==============================================================================
