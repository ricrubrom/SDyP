#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH --open-mode=append
#SBATCH -o salida_pthread/output.txt
#SBATCH -e salida_pthread/errors.txt

# Verificar argumentos
if [ $# -ne 2 ]; then
    echo "Uso: sbatch $0 N HILOS"
    exit 1
fi

N=$1
T=$2

./n_body_pthread $N 200 1000 $T
