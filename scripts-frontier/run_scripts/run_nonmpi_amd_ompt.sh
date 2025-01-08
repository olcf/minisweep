#!/bin/bash

#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 20
#SBATCH -A stf243
#SBATCH -J miniswp_nompi_amd_ompt
#SBATCH -o test_output/%x-%j.out

module reset
module load PrgEnv-amd
module load amd
module load rocm

export OMP_NUM_THREADS=7
#export LIBOMPTARGET_INFO=-1

INSTALLDIR=$(realpath ../install_amd${CRAY_ROCM_VERSION}_openmp_target_nompi_Release)

for i in $(seq 1 3); do
    srun -n 1 -c 7 --unbuffered --gpus-per-task=1 --gpu-bind=closest \
        ${INSTALLDIR}/bin/sweep --niterations 1 --ncell_x 64 --ncell_y 64 --ncell_z 64 --ne 32 --na 32
done
