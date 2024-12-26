#!/bin/bash

#SBATCH -N 8
#SBATCH -n 8
#SBATCH -t 30
#SBATCH -A stf243
#SBATCH -J miniswp_mpi_cray_ompt
#SBATCH -o test_output/%x-%j.out

module reset
module load PrgEnv-cray
module load cce
module load rocm

export OMP_NUM_THREADS=7

INSTALLDIR=$(realpath ../install_cce${CRAY_CC_VERSION}_rocm${CRAY_ROCM_VERSION}_openmp_target_mpi_Release)

for i in $(seq 1 1); do
    srun -N 8 -n 64 --gpus-per-task=1 --gpu-bind=closest -c 7 ${INSTALLDIR}/bin/sweep \
        --niterations 1 --nproc_x 8 --nproc_y 8 --nthread_e 1 \
        --ncell_x $((32*8)) --ncell_y $((32*8)) --ncell_z 64 --ne 32 --na 32 --nblock_z 64
done
