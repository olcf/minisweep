#!/bin/bash

module load cmake

if [ 1 = 1 ] ; then
  mkdir -p build_openmp4
  cd build_openmp4
  env BUILD=RELEASE NM_VALUE=16 COMPILER=xl SOURCE=../minisweep INSTALL=../install ../minisweep/scripts/cmake_ibm_power_mpi_openmp4.sh
  cd ..
fi

if [ 1 = 0 ] ; then
  module swap xl pgi
  mkdir -p build_openacc
  cd build_openacc
  env BUILD=RELEASE NM_VALUE=16 SOURCE=../minisweep INSTALL=../install ../minisweep/scripts/cmake_openacc.sh
  cd ..
fi

