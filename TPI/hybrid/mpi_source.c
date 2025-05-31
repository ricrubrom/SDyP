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
#include "./pthread_source.h"

double tIni, tFin, tTotal;

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

  srand(3);

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

  *cuerpos = (cuerpo_t *)malloc(sizeof(cuerpo_t) * (*N));
  if (*cuerpos == NULL)
  {
    printf("Error al asignar memoria para cuerpos\n");
    return -1;
  }

  return 0;
}

void finalizar(cuerpo_t *cuerpos)
{
  free(cuerpos);
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
  MPI_Bcast(cuerpos, N * sizeof(cuerpo_t), MPI_BYTE, 0, MPI_COMM_WORLD);

  int tiempo_declaracion = pthread_function(rank, N, cuerpos, T, delta_tiempo, pasos, comm_size);
  if (tiempo_declaracion == -1)
  {
    printf("Error en pthread_function\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }
  else
    return tiempo_declaracion;
}

int main(int argc, char *argv[])
{

  cuerpo_t *cuerpos;
  float delta_tiempo = 1.0f; // Intervalo de tiempo, longitud de un paso
  int pasos;
  int N;
  int T;
  int rank;
  int comm_size;
  int provided_rank;
  int debug_mode = 0; // Modo de depuración, 0: sin depuración, 1: con depuración

  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided_rank);
  if (provided_rank != MPI_THREAD_MULTIPLE)
  {
    printf("El entorno MPI no soporta el nivel de concurrencia requerido.\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

  if (inicializar(argc, argv, &N, &delta_tiempo, &pasos, &T, &debug_mode, &cuerpos) == -1)
  {
    return -1;
  }

  if (rank == 0)
  {
    inicializarCuerpos(cuerpos, N);
    tIni = dwalltime();
  }
  tIni += mpi_function(rank, cuerpos, N, delta_tiempo, pasos, T, comm_size);
  tFin = dwalltime();
  tTotal = tFin - tIni;

  if (rank == 0)
  {
    if (debug_mode)
    {
      printResults(N, cuerpos);
    }
    printf("Tiempo en segundos: %.15f\n", tTotal);
  }

  finalizar(cuerpos);
  MPI_Finalize();
  return (0);
}