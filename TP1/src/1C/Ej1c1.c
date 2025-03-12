// ORDENADO POR COLUMNAS
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
  double *A, *C;
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
  A = (double *)malloc(sizeof(double) * N * N);
  C = (double *)malloc(sizeof(double) * N * N);

  // Inicializa las matrices A y B en 1, el resultado sera una matriz con todos sus valores en N
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      A[i + j * N] = 1;
    }
  }

  // Realiza la multiplicacion

  timetick = dwalltime();

  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      C[i + j * N] = 0;
      for (k = 0; k < N; k++)
      {
        C[i + j * N] = C[i + j * N] + A[i + k * N] * A[k + j * N];
      }
    }
  }

  printf("Tiempo en segundos %f\n", dwalltime() - timetick);

  // Verifica el resultado
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
    {
      check = check && (C[i + j * N] == N);
    }
  }

  if (check)
  {
    printf("Multiplicacion de matrices resultado correcto\n");
  }
  else
  {
    printf("Multiplicacion de matrices resultado erroneo\n");
  }

  free(A);
  free(C);
  return (0);
}
