#!/bin/bash

#SBATCH -N 2
#SBATCH -t 60
#SBATCH -A stf243
#SBATCH -J miniswp_cpu_mpi_cpe_sweep
#SBATCH -o test_output/%x-%j.out

# expect about 90 seconds per run, so 4.5-5 minutes per ROCm version
for cpe_ver in "23.12" "24.03" "24.07" "24.11" "24.11.rocm6.3.0"; do
    export LMOD_MODULERCFILE=$HOME/lmod_modulercfiles/${cpe}.lua
    module reset
    module load PrgEnv-amd

    export OLD_LD_PATH=$LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=$CRAY_LD_LIBRARY_PATH:$LD_LIBRARY_PATH

    INSTALLDIR=$(realpath ../install_amd${CRAY_AMD_COMPILER_VERSION}_mpi_Release)

    if [ ! -d $INSTALLDIR ]; then
        echo "Couldn't find install for ROCm/${CRAY_AMD_COMPILER_VERSION}. Skipping."
    else
        for i in $(seq 1 1); do
            srun -N 2 -n 112 -c 1 --gpus-per-node=8 --gpu-bind=closest \
                ${INSTALLDIR}/bin/sweep --nproc_x 14 --nproc_y 8 \
                --niterations 10 --ncell_x 96 --ncell_y 64 \
                --ncell_z 64 --ne 64 --na 32 --nblock_z 8 --nthread_e 1
                #${INSTALLDIR}/bin/sweep --nproc_x 4 --nproc_y 4 \
                #--niterations 1 --ncell_x $((4*64)) --ncell_y $((4*32)) \
                #--ncell_z 64 --ne 64 --na 32 --nblock_z 64
        done
    fi
    export LD_LIBRARY_PATH=$OLD_LD_PATH
done

# --ncell_x 96 --ncell_y 64 --ncell_z 64 --ne 64 --na 32 --nblock_z 8 --nthread_e 7
