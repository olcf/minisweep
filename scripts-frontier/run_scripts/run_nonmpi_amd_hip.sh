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
INSTALLDIR=$(realpath ../install_amd${CRAY_ROCM_VERSION}_hip_nompi_Release)

for i in $(seq 1 3); do
    ${INSTALLDIR}/bin/sweep --niterations 1 --ncell_x 64 --ncell_y 64 --ncell_z 64 \
        --ne 32 --na 32 --nblock_z 64 \
        --is_using_device 1 --nthread_octant 8 --nthread_e 32
done
