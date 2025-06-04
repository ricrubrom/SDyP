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

double tIni, tFin, tTotal;

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

//
// Estructuras y variables para Algoritmo de gravitacion
//
typedef struct cuerpo cuerpo_t;
struct cuerpo
{
  double masa;
  double px;
  double py;
  double pz;
  double vx;
  double vy;
  double vz;
  double r;
  double g;
  double b;
  int cuerpo;
};

double *fuerza_totalX, *fuerza_totalY, *fuerza_totalZ;
double *fuerza_localX, *fuerza_localY, *fuerza_localZ;
float toroide_alfa;
float toroide_theta;
float toroide_incremento;
float toroide_lado;
float toroide_r;
float toroide_R;

cuerpo_t *cuerpos;
float delta_tiempo = 1.0f; // Intervalo de tiempo, longitud de un paso
int pasos;
int N;
int T;
int debug_mode = 0; // Modo de depuración, 0: sin depuración, 1: con depuración

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
    int idx1 = idt * N + cuerpo1;
    for (cuerpo2 = cuerpo1 + 1; cuerpo2 < N; cuerpo2++)
    {
      int idx2 = idt * N + cuerpo2;
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

      fuerza_localX[idx1] += dif_X;
      fuerza_localY[idx1] += dif_Y;
      fuerza_localZ[idx1] += dif_Z;

      fuerza_localX[idx2] -= dif_X;
      fuerza_localY[idx2] -= dif_Y;
      fuerza_localZ[idx2] -= dif_Z;
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
      int idx = k * N + cuerpo;
      fx += fuerza_localX[idx];
      fy += fuerza_localY[idx];
      fz += fuerza_localZ[idx];
      fuerza_localX[idx] = 0.0; // Limpiar fuerzas locales
      fuerza_localY[idx] = 0.0;
      fuerza_localZ[idx] = 0.0;
    }

    // Calcular aceleración (F = ma)
    double inv_masa = (cuerpos[cuerpo].masa > 0.0f) ? 1.0f / cuerpos[cuerpo].masa : 0.0f;
    double ax = fx * inv_masa;
    double ay = fy * inv_masa;
    double az = fz * inv_masa;

    // float ax = fx / cuerpos[cuerpo].masa;
    // float ay = fy / cuerpos[cuerpo].masa;
    // float az = fz / cuerpos[cuerpo].masa;

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
  calcularFuerzas(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
  moverCuerpos(cuerpos, N, dt, idt);
  pthread_barrier_wait(&barrier);
}

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

    fuerza_totalX[cuerpo] = 0.0;
    fuerza_totalY[cuerpo] = 0.0;
    fuerza_totalZ[cuerpo] = 0.0;

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

int inicializar(int argc, char *argv[])
{

  if (argc < 5)
  {
    printf("Ejecutar: %s <nro. de cuerpos> <DT> <pasos> <Threads> (Opcional)< -d/--debug >\n", argv[0]);
    return -1;
  }

  N = atoi(argv[1]);
  delta_tiempo = atof(argv[2]);
  pasos = atoi(argv[3]);

  T = atoi(argv[4]);

  if (argc == 6 && (strcmp(argv[5], "-d") == 0 || strcmp(argv[5], "--debug") == 0))
  {
    debug_mode = 1;
  }

  pthread_barrier_init(&barrier, NULL, T);

  cuerpos = (cuerpo_t *)malloc(sizeof(cuerpo_t) * N);
  fuerza_totalX = (double *)malloc(sizeof(double) * N);
  fuerza_totalY = (double *)malloc(sizeof(double) * N);
  fuerza_totalZ = (double *)malloc(sizeof(double) * N);

  fuerza_localX = (double *)malloc(sizeof(double) * N * T);
  fuerza_localY = (double *)malloc(sizeof(double) * N * T);
  fuerza_localZ = (double *)malloc(sizeof(double) * N * T);

  inicializarCuerpos(cuerpos, N);
  return 0;
}

void finalizar(void)
{
  free(cuerpos);
  free(fuerza_totalX);
  free(fuerza_totalY);
  free(fuerza_totalZ);
  free(fuerza_localX);
  free(fuerza_localY);
  free(fuerza_localZ);
  pthread_barrier_destroy(&barrier);
}

void printResults()
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

int main(int argc, char *argv[])
{
  if (inicializar(argc, argv) == -1)
  {
    return -1;
  }

  pthread_t threads[T];
  int threads_ids[T];

  tIni = dwalltime();
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

  tFin = dwalltime();
  tTotal = tFin - tIni;

  if (debug_mode)
  {
    printResults();
  }

  printf("N= %d\nT=%d\n", N, T);
  printf("Tiempo en segundos: %.15f\n", tTotal);

  finalizar();
  return (0);
}