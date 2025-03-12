// Upper triangular * matriz normal

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

// Dimension por defecto de las matrices
int N = 100;

// Para calcular tiempo
double dwalltime()
{
  double sec;
  struct timeval tv;

  gettimeofday(&tv, NULL);
  sec = tv.tv_sec + tv.tv_usec / 1000000.0;
  return sec;
}

int main(int argc, char *argv[])
{
  double *M, *U, *C;
  int i, j, k;
  int check = 1;
  double timetick;

  // Controla los argumentos al programa
  if ((argc != 2) || ((N = atoi(argv[1])) <= 0))
  {
    printf("\nUsar: %s n\n  n: Dimension de la matriz (nxn X nxn)\n", argv[0]);
    exit(1);
  }

  // Aloca memoria para las matrices
  M = (double *)malloc(sizeof(double) * N * N);
  U = (double *)malloc(sizeof(double) * N * N);
  C = (double *)malloc(sizeof(double) * N * N);

  // Inicializa las matrices A y B en 1, el resultado sera una matriz con todos sus valores en N
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      M[i + j * N] = 1;
      U[i * N + j - (i * (i + 1) / 2)] = 1; // No almacena los ceros
      // if (j < i)
      // {
      //   U[i * N + j] = 0;
      // }
      // else // Almacena los ceros
      // {
      //   U[i * N + j] = 1;
      // }
    }
  }

  // Realiza la multiplicacion

  timetick = dwalltime();

  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      C[i * N + j] = 0;
      for (k = i; k <= N; k++) // Sin los ceros
      // for (k = 0; k < N; k++) // Con los ceros incluidos
      {
        C[i * N + j] = C[i * N + j] + U[i * N + k - (i * (i + 1) / 2)] * M[k * N + j]; // Sin los ceros
        // C[i * N + j] = C[i * N + j] + U[i * N + k] * M[k * N + j]; // Con los ceros
      }
    }
  }

  printf("Tiempo en segundos %f\n", dwalltime() - timetick);

  free(M);
  free(U);
  free(C);
  return (0);
}
