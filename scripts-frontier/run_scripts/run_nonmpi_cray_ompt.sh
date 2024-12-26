#!/bin/bash

#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 20
#SBATCH -A stf243
#SBATCH -J miniswp_nompi_cray_ompt
#SBATCH -o test_output/%x-%j.out

module reset
module load PrgEnv-cray
module load cce
module load rocm

export OMP_NUM_THREADS=7

INSTALLDIR=$(realpath ../install_cce${CRAY_CC_VERSION}_rocm${CRAY_ROCM_VERSION}_openmp_target_nompi_Release)

for i in $(seq 1 1); do
    #rocprof --sys-trace --hsa-trace -o minisweep_nompi_cray_j${SLURM_JOBID}.csv ${INSTALLDIR}/bin/sweep --niterations 1 --ncell_x 32 --ncell_y 32 --ncell_z 64 --ne 64 --na 32 --nblock_z 64
    ${INSTALLDIR}/bin/sweep --niterations 1 --ncell_x 32 --ncell_y 32 --ncell_z 64 --ne 64 --na 32 --nblock_z 64
done
