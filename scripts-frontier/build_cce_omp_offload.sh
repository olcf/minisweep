#!/bin/bash
#------------------------------------------------------------------------------

module reset
module load cmake

if [ -f ./build_cce_omp_offload.sh ]; then
    # Then we're in the current directory, go back one
    cd ..
fi

export NM_VALUE=16
export SOURCE=$PWD
if [ -d ./minisweep ] ; then
  export SOURCE=$PWD/minisweep
fi

#--------------------------
#--- Cray/OpenMPOffload+MPI
#--------------------------

rm -rf build
mkdir -p build
pushd build

module load PrgEnv-cray
#module load cpe/24.07
module load rocm
module load craype-accel-amd-gfx90a

module unload cray-libsci
module unload darshan-runtime

cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_INSTALL_PREFIX:PATH=$(realpath ../install_cce${CRAY_CC_VERSION}_rocm${CRAY_ROCM_VERSION}_openmp_target_mpi_Release) \
    -DCMAKE_C_COMPILER:STRING=cc \
    -DUSE_MPI:BOOL=ON \
    -DUSE_KBA:BOOL=ON \
    -DUSE_OMP_OFFLOAD:BOOL=ON \
    -DOMP_OFFLOAD_FLAGS:STRING="-fopenmp" \
    -DNM_VALUE=$NM_VALUE \
    $SOURCE

make VERBOSE=1 install

popd # build_*

#--------------------------
#--- Cray/OpenMPOffload~MPI
#--------------------------

rm -rf build
mkdir -p build
pushd build

module unload cray-mpich
module unload cray-xpmem

cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_INSTALL_PREFIX:PATH=$(realpath ../install_cce${CRAY_CC_VERSION}_rocm${CRAY_ROCM_VERSION}_openmp_target_nompi_Release) \
    -DCMAKE_C_COMPILER:STRING=cc \
    -DUSE_MPI:BOOL=OFF \
    -DUSE_KBA:BOOL=ON \
    -DUSE_OMP_OFFLOAD:BOOL=ON \
    -DOMP_OFFLOAD_FLAGS:STRING="-fopenmp" \
    -DNM_VALUE=$NM_VALUE \
    $SOURCE

make VERBOSE=1 install

popd # build_*
