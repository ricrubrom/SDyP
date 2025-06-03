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

void calcularFuerzas(cuerpo_t *cuerpos, int N, int dt, int idt, int externo)
{
  int cuerpo1, cuerpo2;
  float dif_X, dif_Y, dif_Z;
  float distancia;
  double F;
  int start = rank * (N - block_size);
  int end = start + block_size;
  for (cuerpo1 = start + idt; cuerpo1 < end; cuerpo1 += T)
  {
    int idc1 = idt * N + cuerpo1;

    int start2 = externo ? end : cuerpo1 + 1;
    int end2 = externo ? N : end;
    for (cuerpo2 = start2; cuerpo2 < end2; cuerpo2++)
    {
      int idc2 = idt * N + cuerpo2;
      if ((cuerpos[cuerpo1].px == cuerpos[cuerpo2].px) &&
          (cuerpos[cuerpo1].py == cuerpos[cuerpo2].py) &&
          (cuerpos[cuerpo1].pz == cuerpos[cuerpo2].pz))
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
  int start = rank * (N - block_size);
  int end = start + block_size;
  for (int cuerpo = start + idt; cuerpo < end; cuerpo += T)
  {
    // Calcular aceleración (F = ma)
    double inv_masa = (cuerpos[cuerpo].masa > 0.0f) ? 1.0f / cuerpos[cuerpo].masa : 0.0f;
    double ax = fuerza_totalX[cuerpo] * inv_masa;
    double ay = fuerza_totalY[cuerpo] * inv_masa;
    double az = fuerza_totalZ[cuerpo] * inv_masa;

    // Actualizar velocidad y posición
    cuerpos[cuerpo].vx += ax * dt;
    cuerpos[cuerpo].vy += ay * dt;
    cuerpos[cuerpo].vz += az * dt;
    cuerpos[cuerpo].px += cuerpos[cuerpo].vx * dt;
    cuerpos[cuerpo].py += cuerpos[cuerpo].vy * dt;
    cuerpos[cuerpo].pz += cuerpos[cuerpo].vz * dt;

    fuerza_totalX[cuerpo] = 0.0;
    fuerza_totalY[cuerpo] = 0.0;
    fuerza_totalZ[cuerpo] = 0.0;
  }
}

void sumFuerzas(int idt)
{
  int i;
  int j;
  int index;
  int start = rank * (N - block_size);

  for (i = start + idt; i < N; i += T)
  {
    for (j = 0; j < T; j++)
    {
      index = N * j + i;
      fuerza_totalX[i] += fuerza_localX[index];
      fuerza_totalY[i] += fuerza_localY[index];
      fuerza_totalZ[i] += fuerza_localZ[index];

      fuerza_localX[index] = 0;
      fuerza_localY[index] = 0;
      fuerza_localZ[index] = 0;
    }
  }
}

void gravitacionCPU(cuerpo_t *cuerpos, int N, int dt, int idt, int paso)
{
  int i;
  MPI_Request requests_recv[comm_size];
  MPI_Request requests_send[comm_size];
  int recv_count = 0, send_count = 0;

  if (idt == 0)
  {
    // Recibir cuerpos de procesos con mayor rank
    for (i = rank + 1; i < comm_size; i++)
    {
      MPI_Irecv(&cuerpos[block_size * i], (N - block_size) * sizeof(cuerpo_t), MPI_BYTE, i, CUERPOS, MPI_COMM_WORLD, &requests_recv[recv_count++]);
    }

    // Enviar cuerpos a procesos con menor rank
    for (i = 0; i < rank; i++)
    {
      MPI_Isend(&cuerpos[rank * (N - block_size)], block_size * sizeof(cuerpo_t), MPI_BYTE, i, CUERPOS, MPI_COMM_WORLD, &requests_send[send_count++]);
    }
  }

  pthread_barrier_wait(&barrier);
  calcularFuerzas(cuerpos, N, dt, idt, LOCAL);

  // Esperar finalización de las comunicaciones
  if (idt == 0)
  {
    if (recv_count > 0)
      MPI_Waitall(recv_count, requests_recv, MPI_STATUSES_IGNORE);
    if (send_count > 0)
      MPI_Waitall(send_count, requests_send, MPI_STATUSES_IGNORE);
  }

  pthread_barrier_wait(&barrier);
  calcularFuerzas(cuerpos, N, dt, idt, EXTERNO);
  pthread_barrier_wait(&barrier);
  sumFuerzas(idt);

  // if (rank > 0)
  // {
  //   if (idt == 0)
  //   {
  //     MPI_Recv(fuerza_externaX, N, MPI_DOUBLE, rank - 1, FUERZAS_X, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  //     MPI_Recv(fuerza_externaY, N, MPI_DOUBLE, rank - 1, FUERZAS_Y, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  //     MPI_Recv(fuerza_externaZ, N, MPI_DOUBLE, rank - 1, FUERZAS_Z, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  //   }
  //   pthread_barrier_wait(&barrier);
  //   for (i = idt; i < N; i += T)
  //   {
  //     fuerza_totalX[i] += fuerza_externaX[i];
  //     fuerza_totalY[i] += fuerza_externaY[i];
  //     fuerza_totalZ[i] += fuerza_externaZ[i];
  //     fuerza_externaX[i] = 0;
  //     fuerza_externaY[i] = 0;
  //     fuerza_externaZ[i] = 0;
  //   }
  // }

  // if (rank < comm_size - 1)
  // {
  //   if (idt == 0)
  //   {
  //     MPI_Send(fuerza_totalX, N, MPI_DOUBLE, rank + 1, FUERZAS_X, MPI_COMM_WORLD);
  //     MPI_Send(fuerza_totalY, N, MPI_DOUBLE, rank + 1, FUERZAS_Y, MPI_COMM_WORLD);
  //     MPI_Send(fuerza_totalZ, N, MPI_DOUBLE, rank + 1, FUERZAS_Z, MPI_COMM_WORLD);
  //   }
  // }

  send_count = 0;
  if (rank < comm_size - 1)
  {
    if (idt == 0)
    {
      for (i = rank + 1; i < comm_size; i++)
      {
        MPI_Isend(fuerza_totalX, N, MPI_DOUBLE, i, FUERZAS_X, MPI_COMM_WORLD, &requests_send[send_count++]);
        MPI_Isend(fuerza_totalY, N, MPI_DOUBLE, i, FUERZAS_Y, MPI_COMM_WORLD, &requests_send[send_count++]);
        MPI_Isend(fuerza_totalZ, N, MPI_DOUBLE, i, FUERZAS_Z, MPI_COMM_WORLD, &requests_send[send_count++]);
      }
    }
    pthread_barrier_wait(&barrier);
  }

  if (rank > 0)
  {
    for (i = 0; i < rank; i++)
    {
      if (idt == 0)
      {
        MPI_Status status;
        MPI_Recv(fuerza_externaX, N, MPI_DOUBLE, MPI_ANY_SOURCE, FUERZAS_X, MPI_COMM_WORLD, &status);
        MPI_Recv(fuerza_externaY, N, MPI_DOUBLE, MPI_ANY_SOURCE, FUERZAS_Y, MPI_COMM_WORLD, &status);
        MPI_Recv(fuerza_externaZ, N, MPI_DOUBLE, MPI_ANY_SOURCE, FUERZAS_Z, MPI_COMM_WORLD, &status);
      }

      pthread_barrier_wait(&barrier);

      for (int j = idt; j < N; j += T)
      {
        fuerza_totalX[i] += fuerza_externaX[i];
        fuerza_totalY[i] += fuerza_externaY[i];
        fuerza_totalZ[i] += fuerza_externaZ[i];
        fuerza_externaX[i] = 0;
        fuerza_externaY[i] = 0;
        fuerza_externaZ[i] = 0;
      }
    }
  }

  pthread_barrier_wait(&barrier);
  moverCuerpos(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
}

int inicializarThread()
{
  pthread_barrier_init(&barrier, NULL, T);

  cuerpos = (cuerpo_t *)malloc(sizeof(cuerpo_t) * N);
  fuerza_totalX = (double *)calloc(N, sizeof(double));
  fuerza_totalY = (double *)calloc(N, sizeof(double));
  fuerza_totalZ = (double *)calloc(N, sizeof(double));

  fuerza_localX = (double *)calloc(N * T, sizeof(double));
  fuerza_localY = (double *)calloc(N * T, sizeof(double));
  fuerza_localZ = (double *)calloc(N * T, sizeof(double));

  if (rank > 0)
  {
    fuerza_externaX = (double *)calloc(N, sizeof(double));
    fuerza_externaY = (double *)calloc(N, sizeof(double));
    fuerza_externaZ = (double *)calloc(N, sizeof(double));
  }

  if (cuerpos == NULL || fuerza_totalX == NULL || fuerza_totalY == NULL || fuerza_totalZ == NULL ||
      fuerza_localX == NULL || fuerza_localY == NULL || fuerza_localZ == NULL)
  {
    fprintf(stderr, "Error al reservar memoria.\n");
    return -1;
  }
  if ((rank > 0) && (fuerza_externaX == NULL || fuerza_externaY == NULL || fuerza_externaZ == NULL))
  {
    fprintf(stderr, "Error al reservar memoria.\n");
    return -1;
  }

  return 0;
}

void finalizarThread(void)
{
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
    gravitacionCPU(cuerpos, N, delta_tiempo, idt, paso);
  }

  pthread_exit(NULL);
}

double pthread_function(int rank_p, int N_p, cuerpo_t *cuerpos_p, int T_p, float delta_tiempo_p, int pasos_p, int comm_size_p)
{
  tIniLoc = dwalltime();
  rank = rank_p;
  N = N_p;
  T = T_p;
  delta_tiempo = delta_tiempo_p;
  pasos = pasos_p;
  comm_size = comm_size_p;
  if (rank == 0)
  {
    block_size = 0.25 * N;
  }
  else
  {
    block_size = 0.75 * N;
  }

  if (inicializarThread() == -1)
  {
    return -1;
  }

  cuerpos = cuerpos_p;
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

  cuerpos_p = cuerpos;

  tIniLoc = dwalltime();
  finalizarThread();
  tFinLoc = dwalltime();
  tTotalLoc += tFinLoc - tIniLoc;
  return tTotalLoc;
}