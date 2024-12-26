#!/bin/bash

#SBATCH -N 1
#SBATCH -t 45
#SBATCH -A stf243
#SBATCH -J miniswp_amd_nompi_cpe_sweep
#SBATCH -o test_output/%x-%j.out

# expect about 90 seconds per run, so 4.5-5 minutes per ROCm version
for cpe_ver in "23.12" "24.03" "24.07" "24.11" "24.11.rocm6.3.0"; do
    export LMOD_MODULERCFILE=$HOME/lmod_modulercfiles/${cpe_ver}.lua
    module reset &> /dev/null
    source ./run_nonmpi_amd.sh
done
