#!/bin/bash
#------------------------------------------------------------------------------

module load cmake

export SOURCE=$PWD
if [ -d ./minisweep ] ; then
  export SOURCE=$PWD/minisweep
fi
export INSTALL=$PWD/install
export NM_VALUE=16
#export NM_VALUE=1
#export BUILD=Debug
#export BUILD=Release

#-----
printf -- '-%.0s' {1..79}; echo ""
#-----

for BUILD in Debug Release ; do

  mkdir -p build_openacc_$BUILD
  pushd build_openacc_$BUILD

  module load pgi
  export BUILD

  #. ../minisweep/scripts/cmake_openacc.sh
  #. ../minisweep/scripts/cmake_kba_openacc.sh
  . $SOURCE/scripts/cmake_kba_smpi_openacc.sh

  module unload pgi

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

  mkdir -p build_cuda_$BUILD
  pushd build_cuda_$BUILD

  module load gcc
  module load cuda

  #. ../minisweep/scripts/cmake_openacc.sh
  #. ../minisweep/scripts/cmake_kba_openacc.sh
  #. ../minisweep/scripts/cmake_kba_smpi_openacc.sh
  . $SOURCE/scripts/cmake_ibm_power_mpi_cuda.sh

  module unload gcc
  module unload cuda

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

  mkdir -p build_openmp_target_$BUILD
  pushd build_openmp_target_$BUILD

  module load xl/16.1.1-8
  module load cuda

  . $SOURCE/scripts/cmake_kba_smpi_openmp_target.sh

  module unload xl
  module unload cuda

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

done

#------------------------------------------------------------------------------
