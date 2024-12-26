#!/bin/bash
#------------------------------------------------------------------------------

module reset
module load cmake

if [ -f ./build_amd_omp_offload.sh ]; then
    # Then we're in the current directory, go back one
    cd ..
fi

export NM_VALUE=16
export SOURCE=$PWD
if [ -d ./minisweep ] ; then
  export SOURCE=$PWD/minisweep
fi

#-------------------------
#--- AMD/OpenMPOffload+MPI
#-------------------------

rm -rf build
mkdir -p build
pushd build

module load PrgEnv-amd
module load cray-mpich
module load amd
module load rocm

module unload cray-libsci
module unload darshan-runtime

cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_INSTALL_PREFIX:PATH=$(realpath ../install_amd${CRAY_ROCM_VERSION}_openmp_target_mpi_Release) \
    -DCMAKE_C_COMPILER:STRING=amdclang \
    -DCMAKE_C_FLAGS="-U__CUDA_ARCH__ -I${MPICH_DIR}/include" \
    -DCMAKE_EXE_LINKER_FLAGS="-L${MPICH_DIR}/lib -lmpi_amd" \
    -DUSE_MPI:BOOL=ON \
    -DUSE_KBA:BOOL=ON \
    -DUSE_OMP_OFFLOAD:BOOL=ON \
    -DOMP_OFFLOAD_FLAGS:STRING="-fopenmp -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -march=gfx90a" \
    -DNM_VALUE=$NM_VALUE \
    $SOURCE

make VERBOSE=1 install

popd # build_*

#-------------------------
#--- AMD/OpenMPOffload~MPI
#-------------------------

rm -rf build
mkdir -p build
pushd build

module unload cray-mpich
module unload cray-libsci

cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_INSTALL_PREFIX:PATH=$(realpath ../install_amd${CRAY_ROCM_VERSION}_openmp_target_nompi_Release) \
    -DCMAKE_C_COMPILER:STRING=amdclang \
    -DCMAKE_C_FLAGS="-U__CUDA_ARCH__" \
    -DUSE_MPI:BOOL=OFF \
    -DUSE_KBA:BOOL=ON \
    -DUSE_OMP_OFFLOAD:BOOL=ON \
    -DOMP_OFFLOAD_FLAGS:STRING="-fopenmp -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -march=gfx90a" \
    -DNM_VALUE=$NM_VALUE \
    $SOURCE

make VERBOSE=1 install

popd # build_*
