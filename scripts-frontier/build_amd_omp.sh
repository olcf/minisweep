#!/bin/bash
#------------------------------------------------------------------------------

module reset
module load cmake

export NM_VALUE=16
export SOURCE=$PWD
if [ -d ./minisweep ] ; then
  export SOURCE=$PWD/minisweep
fi

#--------------------
#--- AMD/OpenMPOffload
#--------------------

rm -rf build
mkdir -p build
pushd build

module load cray-mpich
module load amd

cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_INSTALL_PREFIX:PATH=$(realpath ../install_amd${CRAY_AMD_COMPILER_VERSION}_mpi_Release) \
    -DCMAKE_C_COMPILER:STRING=amdclang \
    -DCMAKE_C_FLAGS="-I${MPICH_DIR}/include" \
    -DCMAKE_EXE_LINKER_FLAGS="-L${MPICH_DIR}/lib -lmpi_amd" \
    -DUSE_MPI:BOOL=ON \
    -DUSE_OPENMP:BOOL=ON \
    -DUSE_KBA:BOOL=ON \
    -DNM_VALUE=$NM_VALUE \
    $SOURCE

make VERBOSE=1 install

popd # build_*
