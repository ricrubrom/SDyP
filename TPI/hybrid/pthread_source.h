#ifndef PTHREAD_SOURCE_H
#define PTHREAD_SOURCE_H
#define G 6.673e-11
#include <pthread.h>

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

extern double pthread_function(int rank_p, int N_p, cuerpo_t *cuerpos_p, int T_p, float delta_tiempo_p, int pasos_p, int comm_size_p,
                               double *fuerza_totalX_p, double *fuerza_totalY_p, double *fuerza_totalZ_p,
                               double *fuerza_localX_p, double *fuerza_localY_p, double *fuerza_localZ_p,
                               double *fuerza_externaX_p, double *fuerza_externaY_p, double *fuerza_externaZ_p);
#endif