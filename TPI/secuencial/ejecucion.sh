#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH --open-mode=append
#SBATCH -o salida_secuencial/output.txt
#SBATCH -e salida_secuencial/errores.txt

# Verificar argumentos
if [ $# -ne 1 ]; then
    echo "Uso: sbatch $0 N "
    exit 1
fi

N=$1
./n_body_seq $N 200 1000
