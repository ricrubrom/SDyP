#!/bin/bash

# Verificar argumentos mínimos
if [ $# -lt 2 ]; then
  echo "Uso: $0 <tipo> <nro de cuerpos> [--graph|-g]"
  echo "tipo: 0 para secuencial, 1 para pthreads, 2 para híbrido"
  echo "--graph o -g: incluir esta opción para generar gráfico"
  exit 1
fi

tipo=$1
nro_cuerpos=$2
shift 2

# Cambio estado de graficar si se requiere generar gráfico
graficar=false
for arg in "$@"; do
  if [ "$arg" = "--graph" ] || [ "$arg" = "-g" ]; then
    graficar=true
    break
  fi
done

case $tipo in
  0)
    echo "Ejecutando secuencial con $nro_cuerpos cuerpos..."
    if $graficar; then
      echo "Generando gráfico..."
      gcc -o n_body sequential/n_body_opengl.c utils/utils.c -lm -lGL -lGLU -lglut
      ./n_body $nro_cuerpos 200 100000
    else 
      gcc -o n_body sequential/n_body.c utils/utils.c -lm
      ./n_body $nro_cuerpos 200 1000 > ./n_body.log
    fi
    [ -e "n_body" ] && rm n_body
    ;;
  1)
    echo "Ejecutando pthreads con $nro_cuerpos cuerpos..."
    if $graficar; then
      echo "Generando gráfico..."
      gcc -pthread -o n_body_pthread pthread/n_body_pthread_opengl.c utils/utils.c -lm -lGL -lGLU -lglut
      echo "Con 4 hilos..."
      ./n_body_pthread $nro_cuerpos 200 100000 4 
      echo "Con 8 hilos..."
      ./n_body_pthread $nro_cuerpos 200 100000 8 
    else
      gcc -pthread -o n_body_pthread pthread/n_body_pthread.c utils/utils.c -lm
      echo "Con 1 hilos..."
      ./n_body_pthread $nro_cuerpos 200 1000 1 > ./n_body_pthread_1.log
      echo "Con 4 hilos..."
      ./n_body_pthread $nro_cuerpos 200 1000 4 > ./n_body_pthread_4.log
      echo "Con 8 hilos..."
      ./n_body_pthread $nro_cuerpos 200 1000 8 > ./n_body_pthread_8.log
      echo "Con 16 hilos..."
      ./n_body_pthread $nro_cuerpos 200 1000 16 > ./n_body_pthread_16.log
    fi
    [ -e "n_body_pthread" ] && rm n_body_pthread
    ;;
  2)
    echo "Ejecutando híbrido con $nro_cuerpos cuerpos..."
    mpicc -lpthread -lm -o mpi_pthreads hybrid/mpi_source.c hybrid/pthreads_source.c utils/utils.c
    mpirun --bind-to none -np 2 mpi_pthreads $nro_cuerpos 200 1000 4
    [ -e "mpi_pthreads" ] && rm mpi_pthreads
    ;;
  *)
    echo "Tipo no reconocido: $tipo. Usa 0, 1 o 2."
    exit 1
    ;;
esac