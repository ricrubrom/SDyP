// Compilar:
//	gcc -o n_body_simple_NOGL n_body_simple.c -lm
// Ejecutar:
//	./n_body_simple_NOGL <nro de cuerpos> <DT> <Pasos>

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

enum tag
{
  CUERPOS,
  FUERZAS_X,
  FUERZAS_Y,
  FUERZAS_Z
};
enum external_flag
{
  LOCAL,
  EXTERNO
};

#define G 6.673e-11

double *fuerza_totalX, *fuerza_totalY, *fuerza_totalZ;
double *fuerza_localX, *fuerza_localY, *fuerza_localZ;
double *fuerza_externaX, *fuerza_externaY, *fuerza_externaZ;

cuerpo_t *cuerpos;
float delta_tiempo = 1.0f;
int pasos, N, T, rank, block_size;

pthread_barrier_t barrier;

void calcularFuerzas(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  int cuerpo1, cuerpo2;
  float dif_X, dif_Y, dif_Z, distancia;
  double F;
  int start = (rank == 0) ? 0 : N - block_size;
  int end = (rank == 0) ? block_size : N - 1;

  for (cuerpo1 = start + idt; cuerpo1 < end; cuerpo1 += T)
  {
    int idc1 = idt * N + cuerpo1;

    for (cuerpo2 = cuerpo1 + 1; cuerpo2 < N; cuerpo2++)
    {
      int idc2 = idt * N + cuerpo2;
      if (cuerpos[cuerpo1].px == cuerpos[cuerpo2].px &&
          cuerpos[cuerpo1].py == cuerpos[cuerpo2].py &&
          cuerpos[cuerpo1].pz == cuerpos[cuerpo2].pz)
        continue;

      dif_X = cuerpos[cuerpo2].px - cuerpos[cuerpo1].px;
      dif_Y = cuerpos[cuerpo2].py - cuerpos[cuerpo1].py;
      dif_Z = cuerpos[cuerpo2].pz - cuerpos[cuerpo1].pz;
      distancia = sqrt(dif_X * dif_X + dif_Y * dif_Y + dif_Z * dif_Z);

      F = (G * cuerpos[cuerpo1].masa * cuerpos[cuerpo2].masa) / (distancia * distancia);
      dif_X *= F;
      dif_Y *= F;
      dif_Z *= F;

      fuerza_localX[idc1] += dif_X;
      fuerza_localY[idc1] += dif_Y;
      fuerza_localZ[idc1] += dif_Z;
      fuerza_localX[idc2] -= dif_X;
      fuerza_localY[idc2] -= dif_Y;
      fuerza_localZ[idc2] -= dif_Z;
    }
  }
}

void sumFuerzas(int idt)
{
  int start = (rank == 0) ? 0 : N - block_size;

  for (int i = start + idt; i < N; i += T)
  {
    fuerza_totalX[i] = fuerza_totalY[i] = fuerza_totalZ[i] = 0.0;
    for (int j = 0; j < T; j++)
    {
      int idx = N * j + i;
      fuerza_totalX[i] += fuerza_localX[idx];
      fuerza_totalY[i] += fuerza_localY[idx];
      fuerza_totalZ[i] += fuerza_localZ[idx];
      fuerza_localX[idx] = fuerza_localY[idx] = fuerza_localZ[idx] = 0;
    }
  }
}

void moverCuerpos(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  int start = (rank == 0) ? 0 : N - block_size;
  int end = (rank == 0) ? block_size : N;

  for (int cuerpo = start + idt; cuerpo < end; cuerpo += T)
  {
    double inv_masa = (cuerpos[cuerpo].masa > 0.0f) ? 1.0f / cuerpos[cuerpo].masa : 0.0f;
    double ax = fuerza_totalX[cuerpo] * inv_masa;
    double ay = fuerza_totalY[cuerpo] * inv_masa;
    double az = fuerza_totalZ[cuerpo] * inv_masa;

    cuerpos[cuerpo].vx += ax * dt;
    cuerpos[cuerpo].vy += ay * dt;
    cuerpos[cuerpo].vz += az * dt;

    cuerpos[cuerpo].px += cuerpos[cuerpo].vx * dt;
    cuerpos[cuerpo].py += cuerpos[cuerpo].vy * dt;
    cuerpos[cuerpo].pz += cuerpos[cuerpo].vz * dt;
  }
}

void gravitacionCPU(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  if (idt == 0)
  {
    if (rank == 0)
    {
      MPI_Recv(&cuerpos[block_size], (N - block_size) * sizeof(cuerpo_t), MPI_BYTE, 1, CUERPOS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    else if (rank == 1)
    {
      MPI_Send(&cuerpos[N - block_size], block_size * sizeof(cuerpo_t), MPI_BYTE, 0, CUERPOS, MPI_COMM_WORLD);
    }
  }

  pthread_barrier_wait(&barrier);
  calcularFuerzas(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
  sumFuerzas(idt);
  pthread_barrier_wait(&barrier);

  if (rank == 0 && idt == 0)
  {
    MPI_Send(&fuerza_totalX[block_size], N - block_size, MPI_DOUBLE, 1, FUERZAS_X, MPI_COMM_WORLD);
    MPI_Send(&fuerza_totalY[block_size], N - block_size, MPI_DOUBLE, 1, FUERZAS_Y, MPI_COMM_WORLD);
    MPI_Send(&fuerza_totalZ[block_size], N - block_size, MPI_DOUBLE, 1, FUERZAS_Z, MPI_COMM_WORLD);
  }

  if (rank == 1)
  {
    if (idt == 0)
    {
      MPI_Recv(fuerza_externaX, block_size, MPI_DOUBLE, 0, FUERZAS_X, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(fuerza_externaY, block_size, MPI_DOUBLE, 0, FUERZAS_Y, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(fuerza_externaZ, block_size, MPI_DOUBLE, 0, FUERZAS_Z, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    pthread_barrier_wait(&barrier);
    for (int j = idt; j < block_size; j += T)
    {
      int idx = N - block_size + j;
      fuerza_totalX[idx] += fuerza_externaX[j];
      fuerza_totalY[idx] += fuerza_externaY[j];
      fuerza_totalZ[idx] += fuerza_externaZ[j];
    }
  }

  pthread_barrier_wait(&barrier);
  moverCuerpos(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
}

void *thread(void *arg)
{
  int idt = *(int *)arg;
  for (int paso = 0; paso < pasos; paso++)
  {
    gravitacionCPU(cuerpos, N, delta_tiempo, idt);
  }
  pthread_exit(NULL);
}

double pthread_function(int rank_p, int N_p, cuerpo_t *cuerpos_p, int T_p, float delta_tiempo_p, int pasos_p,
                        double *fuerza_totalX_p, double *fuerza_totalY_p, double *fuerza_totalZ_p,
                        double *fuerza_localX_p, double *fuerza_localY_p, double *fuerza_localZ_p,
                        double *fuerza_externaX_p, double *fuerza_externaY_p, double *fuerza_externaZ_p)
{
  // Asignación de variables globales (no se cuenta como tiempo de cómputo)
  rank = rank_p;
  N = N_p;
  T = T_p;
  delta_tiempo = delta_tiempo_p;
  pasos = pasos_p;

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

  int block_size_rank0 = (int)(0.3 * N);
  int block_size_rank1 = N - block_size_rank0;
  block_size = (rank == 0) ? block_size_rank0 : block_size_rank1;

  // Inicialización de pthread (no se cuenta como tiempo de cómputo)
  pthread_barrier_init(&barrier, NULL, T);
  pthread_t threads[T];
  int threads_ids[T];

  // AQUÍ EMPIEZA LA MEDICIÓN DEL TIEMPO DE CÓMPUTO PURO
  double tIniLoc = dwalltime();

  // Creación y ejecución de threads (ESTO SÍ se cuenta como tiempo de cómputo)
  for (int i = 0; i < T; i++)
  {
    threads_ids[i] = i;
    pthread_create(&threads[i], NULL, &thread, (void *)&threads_ids[i]);
  }
  for (int i = 0; i < T; i++)
  {
    pthread_join(threads[i], NULL);
  }

  // Comunicación final necesaria para el algoritmo (SÍ se cuenta)
  if (rank == 1)
  {
    MPI_Recv(cuerpos, (N - block_size) * sizeof(cuerpo_t), MPI_BYTE, 0, CUERPOS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
  else if (rank == 0)
  {
    MPI_Send(cuerpos, block_size * sizeof(cuerpo_t), MPI_BYTE, 1, CUERPOS, MPI_COMM_WORLD);
  }

  // AQUÍ TERMINA LA MEDICIÓN DEL TIEMPO DE CÓMPUTO PURO
  double tFinLoc = dwalltime();

  // Limpieza de pthread (no se cuenta como tiempo de cómputo)
  pthread_barrier_destroy(&barrier);

  return tFinLoc - tIniLoc;
}