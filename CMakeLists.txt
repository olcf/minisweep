#==============================================================================

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

# Minisweep is pure C, but for HIP backend, requires .cpp file extensions for hipcc
PROJECT("minisweep" LANGUAGES C)

# Build options:
SET(NM_VALUE "16" CACHE STRING "The number of moments. Typical values are 1, 4, 16, and 36.")
SET(NU_VALUE "4" CACHE STRING "The number of unknowns per gridcell, moment, energy group.")
SET(RELAXED_TESTING OFF CACHE BOOL "If set, do not strictly test the answers.")
SET(USE_KBA ON CACHE BOOL "Use the Koch–Baker–Alcouffe parallel wavefront sweep algorithm.")
SET(USE_MPI OFF CACHE BOOL "Enable MPI.")
SET(USE_ACCELDIR ON CACHE BOOL "I'm really not sure, but doesn't work without it")

# Performance-related options:
SET(USE_OMP OFF CACHE BOOL "Enable host-based OpenMP.")

# GPU kernel-based acceleration
SET(USE_HIP OFF CACHE BOOL "Enable HIP device offloading.")
SET(HIP_ARCH "gfx90a" CACHE STRING "The GPU architecture to pass through --offload-arch on the HIP backend.")
SET(USE_CUDA OFF CACHE BOOL "Enable CUDA device offloading.")

# OpenACC acceleration (not available on Frontier)
SET(USE_ACC OFF CACHE BOOL "Enable OpenACC acceleration.")

# OpenMP Target offload
SET(USE_OMP_VO_ATOMIC OFF CACHE BOOL "Enables use of the atomic OpenMP region to update of the VO vector.")
SET(USE_OMP_TASKS OFF CACHE BOOL "Enable host OpenMP tasks. Otherwise, use OpenMP threads.")
SET(USE_OMP_OFFLOAD OFF CACHE BOOL "Enable OpenMP target offloading.")
SET(OMP_OFFLOAD_FLAGS "-fopenmp" CACHE STRING "C compiler flags to enable OpenMP Offloading.")

# An alternative OpenMP offload implementation
SET(USE_OPENMP4 OFF CACHE BOOL "Enable the device-based OpenMP 4 implementation.")

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

# A subset of sources, this is the sources that contain GPU kernels
# Only used by HIP currently
SET(GPU_SOURCES
  src/1_base/env_cuda.c
  src/3_sweeper/sweeper.c
  src/3_sweeper/sweeper_kernels.c
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
ELSEIF(USE_OPENMP4)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_OPENMP4")
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

ELSEIF(USE_HIP)
  ENABLE_LANGUAGE(CXX)
  # HIP backend requires C++ compiler
  IF(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "HIP backend requires a Clang-based compiler.")
  ENDIF()

  LIST(APPEND CMAKE_MODULE_PATH
     $ENV{ROCM_PATH}/lib/cmake/hip)
  LIST(APPEND CMAKE_PREFIX_PATH $ENV{ROCM_PATH})

  FIND_PACKAGE(hip REQUIRED)

  # Change C to C++ extensions for sources
  FOREACH(FILE IN LISTS SOURCES)
    STRING(REPLACE ".c" ".cpp" FILE2 ${FILE})
    LIST(REMOVE_ITEM SOURCES ${FILE})
    LIST(APPEND SOURCES ${FILE2})
  ENDFOREACH()

  # remove HIP sources from SOURCES
  FOREACH(FILE IN LISTS GPU_SOURCES)
    STRING(REPLACE ".c" ".cpp" FILE2 ${FILE})
    LIST(REMOVE_ITEM SOURCES ${FILE2})
    # Rename to C++ extension in GPU_SOURCES
    LIST(REMOVE_ITEM GPU_SOURCES ${FILE})
    LIST(APPEND GPU_SOURCES ${FILE2})
    # Declare as HIP source
    SET_SOURCE_FILES_PROPERTIES(${FILE} PROPERTIES HIP_SOURCE_PROPERTY_FORMAT 1)
  ENDFOREACH()

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_HIP")

  # the hip_sweeper library collects all the HIP code from Minisweep
  ADD_LIBRARY(hip_sweeper STATIC ${GPU_SOURCES})
  TARGET_COMPILE_OPTIONS(hip_sweeper PRIVATE -x hip --offload-arch=${HIP_ARCH})
  ADD_LIBRARY(sweeper STATIC ${SOURCES})

  ADD_EXECUTABLE(sweep src/4_driver/sweep.cpp)
  TARGET_LINK_LIBRARIES(sweep sweeper hip_sweeper hip::host)
  ADD_EXECUTABLE(tester src/4_driver/tester.cpp)
  TARGET_LINK_LIBRARIES(tester sweeper hip_sweeper hip::host)
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
