#!/bin/bash

#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 45
#SBATCH -A stf243
#SBATCH -J miniswp_nompi_cray_ompt
#SBATCH -o test_output/%x-%j.out

module reset
module load PrgEnv-cray
module load cce
module load rocm

export OMP_NUM_THREADS=7
#export CRAY_ACC_DEBUG=3

INSTALLDIR=$(realpath ../install_cce${CRAY_CC_VERSION}_rocm${CRAY_ROCM_VERSION}_openmp_target_nompi_Release)

for i in $(seq 1 3); do
    srun -N 1 -n 1 --gpus-per-task=1 --gpu-bind=closest --unbuffered \
        ${INSTALLDIR}/bin/sweep --niterations 1 --ncell_x 64 --ncell_y 64 --ncell_z 64 --ne 32 --na 32
done
