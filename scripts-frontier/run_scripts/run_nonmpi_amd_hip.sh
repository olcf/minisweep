#!/bin/bash

#SBATCH -N 1
#SBATCH -n 1
#SBATCH -t 20
#SBATCH -A stf243
#SBATCH -J miniswp_nompi_amd_hip
#SBATCH -o test_output/%x-%j.out

module reset
module load PrgEnv-amd
module load rocm

echo "ROCm Version: $CRAY_ROCM_VERSION"
#INSTALLDIR=$(realpath ../install_amd${CRAY_ROCM_VERSION}_openmp_target_nompi_Release)
INSTALLDIR=$(realpath ../install_amd${CRAY_ROCM_VERSION}_hip_nompi_Release)
#INSTALLDIR=$(realpath /lustre/orion/stf243/world-shared/sauetest/harness/apps/borg/minisweep/hip_n0001/Run_Archive/1734123973.7980793/build_directory/install)

for i in $(seq 1 3); do
    ${INSTALLDIR}/bin/sweep --niterations 1 --ncell_x 128 --ncell_y 128 --ncell_z 64 \
        --ne 32 --na 32 --nblock_z 64 \
        --is_using_device 1 --nthread_octant 8 --nthread_e 32
done
