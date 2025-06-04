#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH --tasks-per-node=8
#SBATCH --open-mode=append
#SBATCH -o salida_hibrido/output.txt
#SBATCH -e salida_hibrido/errors.txt

# Verificar argumentos
if [ $# -ne 2 ]; then
    echo "Uso: sbatch $0 N T"
    exit 1
fi

N=$1
HILOS=$2

mpirun --bind-to none -np 2 ./mpi_pthreads $N 200 1000 $HILOS
