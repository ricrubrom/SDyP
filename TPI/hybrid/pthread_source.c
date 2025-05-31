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
#include "../utils/utils.h"
#include "pthread_source.h"

double tIniLoc, tFinLoc, tTotalLoc;

//
// Constantes para Algoritmo de gravitacion
//
#define PI (3.141592653589793)
#define G 6.673e-11
#define ESTRELLA 0
#define POLVO 1
#define H2 2 // Hidrogeno molecular

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

void calcularFuerzas(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  int cuerpo1, cuerpo2;
  float dif_X, dif_Y, dif_Z;
  float distancia;
  double F;

  for (cuerpo1 = idt; cuerpo1 < N - 1; cuerpo1 += T)
  {
    int idc1 = idt * N + cuerpo1;
    for (cuerpo2 = cuerpo1 + 1; cuerpo2 < N; cuerpo2++)
    {
      int idc2 = idt * N + cuerpo2;
      if ((cuerpos[cuerpo1].px == cuerpos[cuerpo2].px) && (cuerpos[cuerpo1].py == cuerpos[cuerpo2].py) && (cuerpos[cuerpo1].pz == cuerpos[cuerpo2].pz))
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

void moverCuerpos(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  // Acumular fuerzas locales en un temporal
  double fx = 0.0, fy = 0.0, fz = 0.0;
  for (int cuerpo = idt; cuerpo < N; cuerpo += T)
  {
    for (int k = 0; k < T; k++)
    {
      fx += fuerza_localX[k * N + cuerpo];
      fy += fuerza_localY[k * N + cuerpo];
      fz += fuerza_localZ[k * N + cuerpo];
      fuerza_localX[k * N + cuerpo] = 0.0; // Limpiar fuerzas locales
      fuerza_localY[k * N + cuerpo] = 0.0;
      fuerza_localZ[k * N + cuerpo] = 0.0;
    }

    // Calcular aceleración (F = ma)
    double inv_masa = (cuerpos[cuerpo].masa > 0.0f) ? 1.0f / cuerpos[cuerpo].masa : 0.0f;
    double ax = fx * inv_masa;
    double ay = fy * inv_masa;
    double az = fz * inv_masa;

    // Actualizar velocidad y posición
    cuerpos[cuerpo].vx += ax * dt;
    cuerpos[cuerpo].vy += ay * dt;
    cuerpos[cuerpo].vz += az * dt;
    cuerpos[cuerpo].px += cuerpos[cuerpo].vx * dt;
    cuerpos[cuerpo].py += cuerpos[cuerpo].vy * dt;
    cuerpos[cuerpo].pz += cuerpos[cuerpo].vz * dt;

    fx = 0.0;
    fy = 0.0;
    fz = 0.0;
  }
}

void gravitacionCPU(cuerpo_t *cuerpos, int N, int dt, int idt)
{
  int i;
  if (idt == 0)
  {
    if (rank == 0)
    {
      MPI_Recv();
    }
    else
    {
      for (int i = rank - 1; i >= 0; i--)
      {
        MPI_Send();
      }
    }
  }
  pthread_barrier_wait(&barrier);
  calcularFuerzas(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);

  if (rank == 0)
  {
    sumFuerzas();
    if (idt == 0)
    {
      for (i = rank + 1; i <= comm_size; i++)
      {
        MPI_Send();
      }
    }
  }
  else
  {
    sumFuerzas();
    if (idt == 0)
    {
      for (int i = rank - 1; i >= 0; i--)
      {
        MPI_Recv();
      }
    }
    pthread_barrier_wait(&barrier);
    sum_fuerza_externa();
  }

  pthread_barrier_wait(&barrier);
  moverCuerpos(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
}

int inicializarThread()
{
  pthread_barrier_init(&barrier, NULL, T);

  cuerpos = (cuerpo_t *)malloc(sizeof(cuerpo_t) * N);
  fuerza_totalX = (double *)malloc(sizeof(double) * N);
  fuerza_totalY = (double *)malloc(sizeof(double) * N);
  fuerza_totalZ = (double *)malloc(sizeof(double) * N);

  fuerza_localX = (double *)calloc(N * T, sizeof(double));
  fuerza_localY = (double *)calloc(N * T, sizeof(double));
  fuerza_localZ = (double *)calloc(N * T, sizeof(double));

  if (rank > 0)
  {
    fuerza_externaX = (double *)malloc(sizeof(double) * N * T);
    fuerza_externaY = (double *)malloc(sizeof(double) * N * T);
    fuerza_externaZ = (double *)malloc(sizeof(double) * N * T);
  }

  if (cuerpos == NULL || fuerza_totalX == NULL || fuerza_totalY == NULL || fuerza_totalZ == NULL ||
      fuerza_localX == NULL || fuerza_localY == NULL || fuerza_localZ == NULL ||
      fuerza_externaX == NULL || fuerza_externaY == NULL || fuerza_externaZ == NULL)
  {
    fprintf(stderr, "Error al reservar memoria.\n");
    return -1;
  }

  return 0;
}

void finalizarThread(void)
{
  free(cuerpos);
  free(fuerza_totalX);
  free(fuerza_totalY);
  free(fuerza_totalZ);
  free(fuerza_localX);
  free(fuerza_localY);
  free(fuerza_localZ);
  if (rank > 0)
  {
    free(fuerza_externaX);
    free(fuerza_externaY);
    free(fuerza_externaZ);
  }
  pthread_barrier_destroy(&barrier);
}

void *thread(void *arg)
{
  int idt = *(int *)arg;
  int paso;

  for (paso = 0; paso < pasos; paso++)
  {
    gravitacionCPU(cuerpos, N, delta_tiempo, idt);
  }

  pthread_exit(NULL);
}

double pthread_function(int rank_p, int N_p, cuerpo_t *cuerpos_p, int T_p, float delta_tiempo_p, int pasos_p, int comm_size_p)
{
  tIniLoc = dwalltime();
  rank = rank_p;
  N = N_p;
  cuerpos = cuerpos_p;
  T = T_p;
  delta_tiempo = delta_tiempo_p;
  pasos = pasos_p;
  comm_size = comm_size_p;
  if (rank == 0)
  {
    block_size = 0.25 * N;
  }
  {
    block_size = 0.75 * N;
  }

  if (inicializarThread() == -1)
  {
    return -1;
  }

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
  tIniLoc = dwalltime();
  finalizarThread();
  tFinLoc = dwalltime();
  tTotalLoc += tFinLoc - tIniLoc;
  return tTotalLoc;
}