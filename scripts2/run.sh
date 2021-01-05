#!/bin/bash
#-----------------------------------------------------------------------------------------------------
# bsub -P stf006 -nnodes 1 -W 120 -alloc_flags gpumps -Is $SHELL

#BUILD=Debug
BUILD=Release

NX=64 NY=64 M="#REF   result: 8.67023349e+14  diff: 0.000e+00  PASS  time: 3.548  GF/s: 207.007"
#NX=64 NY=36 M="#REF   result: 8.67023349e+14  diff: 0.000e+00  PASS  time: 3.548  GF/s: 207.007"
#NX=64 NY=32 M="#REF   result: 7.70687422e+14  diff: 0.000e+00  PASS  time: 3.465  GF/s: 188.435"
#NX=32 NY=32 M="#REF   result: 3.85343711e+14  diff: 0.000e+00  PASS  time: 1.965  GF/s: 166.078"
#NX=8 NY=32 M="#REF   result: 9.63359277e+13  diff: 0.000e+00  PASS  time: 0.246  GF/s: 331.285"
#NX=1 NY=32 M="#REF   result: 1.20419910e+13  diff: 0.000e+00  PASS  time: 0.703  GF/s: 14.513"

#-----
printf -- '-%.0s' {1..79}; echo ""
#-----

if [ 1 = 1 ] ; then

  pushd build_openacc_$BUILD

  module -q load pgi

  echo "Warmup run:"
  jsrun --smpiargs=-gpu --nrs 6 --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host 6 --tasks_per_rs 1 -X 1 \
    ./sweep --nproc_x 2 --nproc_y 3 --niterations 1 --ncell_x 2 --ncell_y 3 --ncell_z 1 --ne 64 --na 32 --nblock_z 1 2>&1 >/dev/null

# Increment the proc count by factors of 2; try to keep process grid as square as possible.

# nblock_z = 4  for openacc limits scalability but is necessary so that the quantum of work
# per gpu per sweep step is not too small, so the efficiency is reasonable.
# nblock_z = 64 for cuda case works great, as intended, though limited by strong scaling.

  echo "$M"
  for i in {1..1} ; do
    #jsrun -n 1 -g 1 --smpiargs=none ./sweep --niterations 1 --ncell_x $NX --ncell_y 32 --ncell_z 64 --ne 64 --na 32 # --nblock_z 64
    np=1 nr=1
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 1 --nproc_y 1 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 1
    np=6 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 2 --nproc_y 3 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
    np=12 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 4 --nproc_y 3 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
    np=24 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 4 --nproc_y 6 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
    np=48 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 8 --nproc_y 6 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
    np=96 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 8 --nproc_y 12 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
    np=960 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 30 --nproc_y 32 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
    np=1920 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 40 --nproc_y 48 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 4
  done

  module -q unload pgi

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

fi

if [ 1 = 1 ] ; then

  pushd build_cuda_$BUILD

  module -q load gcc
  module -q load cuda

  echo "Warmup run:"
  jsrun --smpiargs=-gpu --nrs 6 --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host 6 --tasks_per_rs 1 -X 1 \
    ./sweep --nproc_x 2 --nproc_y 3 --niterations 1 --ncell_x 2 --ncell_y 3 --ncell_z 1 --ne 64 --na 32 --nblock_z 1 2>&1 >/dev/null

  echo "$M"
  for i in {1..1} ; do
    #jsrun -n 1 -g 1 --smpiargs=none ./sweep --niterations 1 --ncell_x $NX --ncell_y 32 --ncell_z 64 --ne 64 --na 32 # --nblock_z 64
    np=1 nr=1
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 1 --nproc_y 1 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=6 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 2 --nproc_y 3 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=12 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 4 --nproc_y 3 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=24 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 4 --nproc_y 6 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=48 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 8 --nproc_y 6 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=96 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 8 --nproc_y 12 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
  done

  module -q unload gcc
  module -q unload cuda

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

fi

if [ 1 = 1 ] ; then

  pushd build_openmp_target_$BUILD

  module -q load xl/16.1.1-8
  module -q load cuda

  echo "Warmup run:"
  jsrun --smpiargs=-gpu --nrs 6 --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host 6 --tasks_per_rs 1 -X 1 \
    ./sweep --nproc_x 2 --nproc_y 3 --niterations 1 --ncell_x 2 --ncell_y 3 --ncell_z 1 --ne 64 --na 32 --nblock_z 1 2>&1 >/dev/null

  echo "$M"
  for i in {1..1} ; do
    #jsrun -n 1 -g 1 --smpiargs=none ./sweep --niterations 1 --ncell_x $NX --ncell_y 32 --ncell_z 64 --ne 64 --na 32 # --nblock_z 64
    np=1 nr=1
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 1 --nproc_y 1 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=6 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 2 --nproc_y 3 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=12 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 4 --nproc_y 3 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=24 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 4 --nproc_y 6 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=48 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 8 --nproc_y 6 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
    np=96 nr=6
    echo "np = $np  nr = $nr"
    jsrun --smpiargs=-gpu --nrs $np --bind packed:7 --cpu_per_rs 7 --gpu_per_rs 1 --rs_per_host $nr --tasks_per_rs 1 -X 1 \
      ./sweep --nproc_x 8 --nproc_y 12 --niterations 1 --ncell_x $NX --ncell_y $NY --ncell_z 64 --ne 64 --na 32 --nblock_z 64 \
              --is_using_device 1 --nthread_e 64 --nthread_octant 8 --ncell_y_per_subblock 1 --nthread_y 2 --ncell_x_per_subblock 1
  done

  module -q unload xl
  module -q unload cuda

  popd

  #-----
  printf -- '-%.0s' {1..79}; echo ""
  #-----

fi
