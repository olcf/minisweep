#!/bin/bash
#------------------------------------------------------------------------------

module load cmake

#-----
printf -- '-%.0s' {1..79}; echo ""
#-----

for BUILD in Debug Release ; do

  #--------------------
  #--- PGI/OpenACC
  #--------------------

  pushd build_openacc_$BUILD

  module load pgi

  make VERBOSE=1
  code=$?
  if [ $code != 0 ] ; then
    exit $code
  fi

  module unload pgi

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

  #--------------------
  #--- GCC/CUDA
  #--------------------

  pushd build_cuda_$BUILD

  module load gcc
  module load cuda

  make VERBOSE=1
  code=$?
  if [ $code != 0 ] ; then
    exit $code
  fi

  module unload gcc
  module unload cuda

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

  #--------------------
  #--- XL/OpenMP4
  #--------------------

  pushd build_openmp_target_$BUILD

  module load xl/16.1.1-8
  module load cuda

  make VERBOSE=1
  code=$?
  if [ $code != 0 ] ; then
    exit $code
  fi

  module unload xl
  module unload cuda

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----
done

#------------------------------------------------------------------------------
