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

extern double pthread_function(int rank, int N, cuerpo_t *cuerpos, int T, float dt, int pasos, int comm_size_p);

#endif