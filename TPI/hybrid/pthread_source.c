// Compilar:
//		gcc -o n_body_simple_NOGL n_body_simple.c -lm
// Ejecutar:
//		./n_body_simple_NOGL <nro de cuerpos> <DT> <Pasos>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#include <mpi.h>
#include "../utils/utils.h"
#include "pthread_source.h"

double tIniLoc, tFinLoc, tTotalLoc;

enum tag
{
  CUERPOS,
  FUERZAS_X,
  FUERZAS_Y,
  FUERZAS_Z,
};

enum external_flag
{
  LOCAL,
  EXTERNO
};

//
// Constantes para Algoritmo de gravitacion
//
#define G 6.673e-11

// ===============
// ===== CPU =====
// ===============

double *fuerza_totalX, *fuerza_totalY, *fuerza_totalZ;
double *fuerza_localX, *fuerza_localY, *fuerza_localZ;
double *fuerza_externaX, *fuerza_externaY, *fuerza_externaZ;

cuerpo_t *cuerpos;
float delta_tiempo = 1.0f; // Intervalo de tiempo, longitud de un paso
int pasos;
int N;
int T;
int rank;
int inicio_proceso;
int cantidad_proceso;
int block_size;
int comm_size;

pthread_barrier_t barrier;

//
// Funciones para Algoritmo de gravitacion
//

void calcularFuerzas(cuerpo_t *cuerpos, int N, int dt, int t_id)
{
  int cuerpo1, cuerpo2;
  float dif_X, dif_Y, dif_Z;
  float distancia;
  double F;
  int hasta_cuerpo = (rank == 0) ? (block_size) : N - 1;

  for (cuerpo1 = rank * (N - block_size) + t_id; cuerpo1 < hasta_cuerpo; cuerpo1 += T)
  {
    int id_c1 = t_id * N + cuerpo1;
    for (cuerpo2 = cuerpo1 + 1; cuerpo2 < N; cuerpo2++)
    {
      if ((cuerpos[cuerpo1].px == cuerpos[cuerpo2].px) && (cuerpos[cuerpo1].py == cuerpos[cuerpo2].py) && (cuerpos[cuerpo1].pz == cuerpos[cuerpo2].pz))
        continue;

      int id_c2 = t_id * N + cuerpo2;

      dif_X = cuerpos[cuerpo2].px - cuerpos[cuerpo1].px;
      dif_Y = cuerpos[cuerpo2].py - cuerpos[cuerpo1].py;
      dif_Z = cuerpos[cuerpo2].pz - cuerpos[cuerpo1].pz;

      distancia = sqrt(dif_X * dif_X + dif_Y * dif_Y + dif_Z * dif_Z);

      F = (G * cuerpos[cuerpo1].masa * cuerpos[cuerpo2].masa) / (distancia * distancia);

      dif_X *= F;
      dif_Y *= F;
      dif_Z *= F;

      fuerza_localX[id_c1] += dif_X;
      fuerza_localY[id_c1] += dif_Y;
      fuerza_localZ[id_c1] += dif_Z;

      fuerza_localX[id_c2] -= dif_X;
      fuerza_localY[id_c2] -= dif_Y;
      fuerza_localZ[id_c2] -= dif_Z;
    }
  }
}

void moverCuerpos(cuerpo_t *cuerpos, int N, int dt, int t_id)
{
  int k, i;
  int hasta_cuerpo = (rank == 0) ? (block_size) : N;
  for (i = rank * (N - block_size) + t_id; i < hasta_cuerpo; i += T)
  {
    // Se reutiliza fuerza como aceleracion
    fuerza_totalX[i] *= 1 / cuerpos[i].masa;
    fuerza_totalY[i] *= 1 / cuerpos[i].masa;
    fuerza_totalZ[i] *= 1 / cuerpos[i].masa;

    // Calculo de velocidad
    cuerpos[i].vx += fuerza_totalX[i] * dt;
    cuerpos[i].vy += fuerza_totalY[i] * dt;
    cuerpos[i].vz += fuerza_totalZ[i] * dt;

    // Calculo de la posicion
    cuerpos[i].px += cuerpos[i].vx * dt;
    cuerpos[i].py += cuerpos[i].vy * dt;
    cuerpos[i].pz += cuerpos[i].vz * dt;
  }
}

void sumarFuerzasTotales(int t_id)
{
  for (int i = rank * (N - block_size) + t_id; i < N; i += T)
  {
    fuerza_totalX[i] = 0.0;
    fuerza_totalY[i] = 0.0;
    fuerza_totalZ[i] = 0.0;

    int idx = i * N;
    for (int j = 0; j < T; j++)
    {
      int inner_idx = i + j * N;
      fuerza_totalX[i] += fuerza_localX[inner_idx];
      fuerza_totalY[i] += fuerza_localY[inner_idx];
      fuerza_totalZ[i] += fuerza_localZ[inner_idx];
      fuerza_localX[inner_idx] = 0;
      fuerza_localY[inner_idx] = 0;
      fuerza_localZ[inner_idx] = 0;
    }
  }
}

void *thread(void *args)
{
  int id = *(int *)args;

  int paso;
  for (paso = 0; paso < pasos; paso++)
  {
    if (id == 0)
    {
      if (rank == 0)
      {
        MPI_Recv(&cuerpos[block_size], (N - block_size) * sizeof(cuerpo_t), MPI_BYTE, 1, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      // RECIBO CUERPOS;
      else
      {
        MPI_Request request;
        MPI_Send(&cuerpos[N - block_size], block_size * sizeof(cuerpo_t), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
        // ENVIO CUERPO AL 0;;
      }

      // Ambos tienen BIEN LOS CUERPOS, SE PUEDE CALCULAR FUERZAS;
    }

    pthread_barrier_wait(&barrier);
    calcularFuerzas(cuerpos, N, delta_tiempo, id);
    pthread_barrier_wait(&barrier);

    sumarFuerzasTotales(id);
    pthread_barrier_wait(&barrier);
    if (rank == 0)
    {
      if (id == 0)
      {
        // ENVIO FUERZAS;
        MPI_Send(&fuerza_totalX[block_size], (N - block_size), MPI_DOUBLE, 1, 1, MPI_COMM_WORLD);
        MPI_Send(&fuerza_totalY[block_size], (N - block_size), MPI_DOUBLE, 1, 2, MPI_COMM_WORLD);
        MPI_Send(&fuerza_totalZ[block_size], (N - block_size), MPI_DOUBLE, 1, 3, MPI_COMM_WORLD);
      }
    }
    else
    {
      if (id == 0)
      {
        // RECIBO FUERZAS;
        MPI_Recv(fuerza_externaX, block_size, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(fuerza_externaY, block_size, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(fuerza_externaZ, block_size, MPI_DOUBLE, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      pthread_barrier_wait(&barrier);

      for (int i = (N - block_size) + id; i < N; i += T)
      {
        fuerza_totalX[i] += fuerza_externaX[i - (N - block_size)];
        fuerza_totalY[i] += fuerza_externaY[i - (N - block_size)];
        fuerza_totalZ[i] += fuerza_externaZ[i - (N - block_size)];
      }
    }

    pthread_barrier_wait(&barrier);
    moverCuerpos(cuerpos, N, delta_tiempo, id);
    pthread_barrier_wait(&barrier);
  }
}
double pthread_function(int rank_p, int N_p, cuerpo_t *cuerpos_p, int T_p, float delta_tiempo_p, int pasos_p, int comm_size_p,
                        double *fuerza_totalX_p, double *fuerza_totalY_p, double *fuerza_totalZ_p,
                        double *fuerza_localX_p, double *fuerza_localY_p, double *fuerza_localZ_p,
                        double *fuerza_externaX_p, double *fuerza_externaY_p, double *fuerza_externaZ_p)
{
  tIniLoc = dwalltime();
  rank = rank_p;
  N = N_p;
  T = T_p;
  delta_tiempo = delta_tiempo_p;
  pasos = pasos_p;
  comm_size = comm_size_p;

  // Asignar punteros de memoria pasados como parámetros
  cuerpos = cuerpos_p;
  fuerza_totalX = fuerza_totalX_p;
  fuerza_totalY = fuerza_totalY_p;
  fuerza_totalZ = fuerza_totalZ_p;
  fuerza_localX = fuerza_localX_p;
  fuerza_localY = fuerza_localY_p;
  fuerza_localZ = fuerza_localZ_p;
  fuerza_externaX = fuerza_externaX_p;
  fuerza_externaY = fuerza_externaY_p;
  fuerza_externaZ = fuerza_externaZ_p;

  if (rank == 0)
  {
    block_size = 0.25 * N;
  }
  else
  {
    block_size = 0.75 * N;
  }

  pthread_barrier_init(&barrier, NULL, T);

  pthread_t threads[T];
  int threads_ids[T];

  tFinLoc = dwalltime();
  tTotalLoc = tFinLoc - tIniLoc;
  int i;
  for (i = 0; i < T; i++)
  {
    threads_ids[i] = i;
    pthread_create(&threads[i], NULL, &thread, (void *)&threads_ids[i]);
  }

  for (i = 0; i < T; i++)
  {
    pthread_join(threads[i], NULL);
  }

  if (rank == 1)
  {
    MPI_Recv(cuerpos, (N - block_size) * sizeof(cuerpo_t), MPI_BYTE, 0, CUERPOS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
  else if (rank == 0)
  {
    MPI_Send(cuerpos, block_size * sizeof(cuerpo_t), MPI_BYTE, 1, CUERPOS, MPI_COMM_WORLD);
  }
  tIniLoc = dwalltime();
  pthread_barrier_destroy(&barrier);
  tFinLoc = dwalltime();
  tTotalLoc += tFinLoc - tIniLoc;
  return tTotalLoc;
}