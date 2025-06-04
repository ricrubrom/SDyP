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
#include <mpi.h>
#include "../utils/utils.h"
#include "./pthreads_source.h"

//
// Constantes para Algoritmo de gravitacion
//
#define PI (3.141592653589793)
#define ESTRELLA 0
#define POLVO 1
#define H2 2 // Hidrogeno molecular

// ===============
// ===== CPU =====
// ===============

float toroide_alfa;
float toroide_theta;
float toroide_incremento;
float toroide_lado;
float toroide_r;
float toroide_R;

void inicializarEstrella(cuerpo_t *cuerpo, int i, float n)
{

  cuerpo->masa = 0.001 * 8;

  if ((toroide_alfa + toroide_incremento) >= 2 * M_PI)
  {
    toroide_alfa = 0;
    toroide_theta += toroide_incremento;
  }
  else
  {
    toroide_alfa += toroide_incremento;
  }

  cuerpo->px = (toroide_R + toroide_r * cos(toroide_alfa)) * cos(toroide_theta);
  cuerpo->py = (toroide_R + toroide_r * cos(toroide_alfa)) * sin(toroide_theta);
  cuerpo->pz = toroide_r * sin(toroide_alfa);

  cuerpo->vx = 0.0;
  cuerpo->vy = 0.0;
  cuerpo->vz = 0.0;

  cuerpo->r = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->g = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->b = 1.0; //(float )rand()/(RAND_MAX+1.0);
}

void inicializarPolvo(cuerpo_t *cuerpo, int i, float n)
{

  cuerpo->masa = 0.001 * 4;

  if ((toroide_alfa + toroide_incremento) >= 2 * M_PI)
  {
    toroide_alfa = 0;
    toroide_theta += toroide_incremento;
  }
  else
  {
    toroide_alfa += toroide_incremento;
  }

  cuerpo->px = (toroide_R + toroide_r * cos(toroide_alfa)) * cos(toroide_theta);
  cuerpo->py = (toroide_R + toroide_r * cos(toroide_alfa)) * sin(toroide_theta);
  cuerpo->pz = toroide_r * sin(toroide_alfa);

  cuerpo->vx = 0.0;
  cuerpo->vy = 0.0;
  cuerpo->vz = 0.0;

  cuerpo->r = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->g = 0.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->b = 0.0; //(float )rand()/(RAND_MAX+1.0);
}

void inicializarH2(cuerpo_t *cuerpo, int i, float n)
{

  cuerpo->masa = 0.001;

  if ((toroide_alfa + toroide_incremento) >= 2 * M_PI)
  {
    toroide_alfa = 0;
    toroide_theta += toroide_incremento;
  }
  else
  {
    toroide_alfa += toroide_incremento;
  }

  cuerpo->px = (toroide_R + toroide_r * cos(toroide_alfa)) * cos(toroide_theta);
  cuerpo->py = (toroide_R + toroide_r * cos(toroide_alfa)) * sin(toroide_theta);
  cuerpo->pz = toroide_r * sin(toroide_alfa);

  cuerpo->vx = 0.0;
  cuerpo->vy = 0.0;
  cuerpo->vz = 0.0;

  cuerpo->r = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->g = 1.0; //(float )rand()/(RAND_MAX+1.0);
  cuerpo->b = 1.0; //(float )rand()/(RAND_MAX+1.0);
}

void inicializarCuerpos(cuerpo_t *cuerpos, int N)
{
  int cuerpo;
  float n = N;

  toroide_alfa = 0.0;
  toroide_theta = 0.0;
  toroide_lado = sqrt(N);
  toroide_incremento = 2 * M_PI / toroide_lado;
  toroide_r = 1.0;
  toroide_R = 2 * toroide_r;

  srand(1);

  for (cuerpo = 0; cuerpo < N; cuerpo++)

  {
    cuerpos[cuerpo].cuerpo = (rand() % 3);

    if (cuerpos[cuerpo].cuerpo == ESTRELLA)
    {
      inicializarEstrella(&cuerpos[cuerpo], cuerpo, n);
    }
    else if (cuerpos[cuerpo].cuerpo == POLVO)
    {
      inicializarPolvo(&cuerpos[cuerpo], cuerpo, n);
    }
    else if (cuerpos[cuerpo].cuerpo == H2)
    {
      inicializarH2(&cuerpos[cuerpo], cuerpo, n);
    }
  }

  cuerpos[0].masa = 2.0e2;
  cuerpos[0].px = 0.0;
  cuerpos[0].py = 0.0;
  cuerpos[0].pz = 0.0;
  cuerpos[0].vx = -0.000001;
  cuerpos[0].vy = -0.000001;
  cuerpos[0].vz = 0.0;

  cuerpos[1].masa = 1.0e1;
  cuerpos[1].px = -1.0;
  cuerpos[1].py = 0.0;
  cuerpos[1].pz = 0.0;
  cuerpos[1].vx = 0.0;
  cuerpos[1].vy = 0.0001;
  cuerpos[1].vz = 0.0;
}

int inicializar(int argc, char *argv[], int *N, float *delta_tiempo, int *pasos, int *T, int *debug_mode, cuerpo_t **cuerpos)
{
  if (argc < 5)
  {
    printf("Ejecutar: %s <nro. de cuerpos> <DT> <pasos> <Threads> (Opcional)< -d/--debug >\n", argv[0]);
    return -1;
  }
  *N = atoi(argv[1]);
  *delta_tiempo = atof(argv[2]);
  *pasos = atoi(argv[3]);
  *T = atoi(argv[4]);
  *debug_mode = 0;

  if (argc == 6 && (strcmp(argv[5], "-d") == 0 || strcmp(argv[5], "--debug") == 0))
  {
    *debug_mode = 1;
  }

  *cuerpos = (cuerpo_t *)malloc(sizeof(cuerpo_t) * (*N)); // Reserva dinámica para cuerpos
  if (*cuerpos == NULL)
  {
    printf("Error al asignar memoria para cuerpos\n");
    return -1;
  }

  return 0;
}

void finalizar(cuerpo_t *cuerpos)
{
  if (cuerpos != NULL)
  {
    free(cuerpos);
  }
}

void printResults(int N, cuerpo_t *cuerpos)
{
  printf("VALORES FINALES:\n");
  for (int i = 0; i < N; i++)
  {
    printf("Cuerpo %d: px=%.15f, py=%.15f, pz=%.15f\n",
           i,
           cuerpos[i].px,
           cuerpos[i].py,
           cuerpos[i].pz);
  }
  printf("\n\n\n");
}

double mpi_function(int rank, cuerpo_t *cuerpos, int N, float delta_tiempo, int pasos, int T, int comm_size)
{
  // Inicializacion de vectores de fuerzas
  double *fuerza_totalX = (double *)malloc(N * sizeof(double));
  double *fuerza_totalY = (double *)malloc(N * sizeof(double));
  double *fuerza_totalZ = (double *)malloc(N * sizeof(double));

  double *fuerza_localX = (double *)malloc(N * T * sizeof(double));
  double *fuerza_localY = (double *)malloc(N * T * sizeof(double));
  double *fuerza_localZ = (double *)malloc(N * T * sizeof(double));

  double *fuerza_externaX = NULL;
  double *fuerza_externaY = NULL;
  double *fuerza_externaZ = NULL;

  if (rank > 0)
  {
    // Solo los procesos mayores a 0 las necesitan
    fuerza_externaX = (double *)malloc(N * sizeof(double));
    fuerza_externaY = (double *)malloc(N * sizeof(double));
    fuerza_externaZ = (double *)malloc(N * sizeof(double));
  }

  if (fuerza_totalX == NULL || fuerza_totalY == NULL || fuerza_totalZ == NULL ||
      fuerza_localX == NULL || fuerza_localY == NULL || fuerza_localZ == NULL)
  {
    fprintf(stderr, "Error al reservar memoria.\n");
    // Liberación individual de recursos ya reservados
    if (fuerza_totalX)
      free(fuerza_totalX);
    if (fuerza_totalY)
      free(fuerza_totalY);
    if (fuerza_totalZ)
      free(fuerza_totalZ);
    if (fuerza_localX)
      free(fuerza_localX);
    if (fuerza_localY)
      free(fuerza_localY);
    if (fuerza_localZ)
      free(fuerza_localZ);
    return -1;
  }

  if ((rank > 0) && (fuerza_externaX == NULL || fuerza_externaY == NULL || fuerza_externaZ == NULL))
  {
    fprintf(stderr, "Error al reservar memoria.\n");
    free(fuerza_totalX);
    free(fuerza_totalY);
    free(fuerza_totalZ);
    free(fuerza_localX);
    free(fuerza_localY);
    free(fuerza_localZ);
    if (fuerza_externaX)
      free(fuerza_externaX);
    if (fuerza_externaY)
      free(fuerza_externaY);
    if (fuerza_externaZ)
      free(fuerza_externaZ);
    return -1;
  }

  // Broadcast inicial para distribuir los cuerpos a todos los procesos
  MPI_Bcast(cuerpos, N * sizeof(cuerpo_t), MPI_BYTE, 0, MPI_COMM_WORLD);

  double tiempo_inicio_computo = dwalltime();

  // Cálculo multithread usando Pthreads
  double tiempo_inicializacion_pthread = pthreads_function(
      rank, N, cuerpos, T, delta_tiempo, pasos,
      fuerza_totalX, fuerza_totalY, fuerza_totalZ,
      fuerza_localX, fuerza_localY, fuerza_localZ,
      fuerza_externaX, fuerza_externaY, fuerza_externaZ);

  double tiempo_fin_computo = dwalltime();
  double tiempo_computo_total = tiempo_fin_computo - tiempo_inicio_computo - tiempo_inicializacion_pthread;

  // Liberar memoria usada
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

  return tiempo_computo_total;
}

int main(int argc, char *argv[])
{
  cuerpo_t *cuerpos = NULL;
  float delta_tiempo = 1.0f;
  int pasos;
  int N;
  int T;
  int rank;
  int comm_size;
  int provided_rank;
  int debug_mode = 0;
  double tiempo_computo = 0.0;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided_rank);
  if (provided_rank < MPI_THREAD_MULTIPLE)
  {
    fprintf(stderr, "MPI no soporta el nivel de concurrencia requerido.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
    return -1;
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  if (inicializar(argc, argv, &N, &delta_tiempo, &pasos, &T, &debug_mode, &cuerpos) == -1)
  {
    if (cuerpos)
      free(cuerpos);
    MPI_Finalize();
    return -1;
  }

  if (rank == 0)
  {
    inicializarCuerpos(cuerpos, N);
  }

  tiempo_computo = mpi_function(rank, cuerpos, N, delta_tiempo, pasos, T, comm_size);

  if (tiempo_computo == -1)
  {
    if (rank == 0)
      printf("Error en pthreads_function\n");
    if (cuerpos)
      free(cuerpos);
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  if (rank == 0)
  {
    if (debug_mode)
    {
      printResults(N, cuerpos);
    }
    printf("N= %d\nT=%d\n", N, T);
    printf("Tiempo en segundos: %.15f segundos\n", tiempo_computo);
  }

  free(cuerpos);

  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();
  return 0;
}
