#!/bin/bash
#-----------------------------------------------------------------------------------------------------

module load cmake

#-----
printf -- '-%.0s' {1..79}; echo ""
#-----

for BUILD in Debug Release ; do

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

done
