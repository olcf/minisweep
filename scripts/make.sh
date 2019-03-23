#!/bin/bash

module load cmake

if [ 1 = 1 ] ; then
  cd build_openmp4
  make VERBOSE=1
  cd ..
fi

if [ 0 = 1 ] ; then
  module swap xl pgi
  cd build_openacc
  make VERBOSE=1
  cd ..
fi

