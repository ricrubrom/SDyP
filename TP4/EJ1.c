#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void mult(int *parteA, int *B, int *parteC, int N, int filas)
{
  int i, j, k;
  int iN, jN, sum;
  for (i = 0; i < filas; i++)
  {
    iN = i * N;
    for (j = 0; j < N; j++)
    {
      jN = j * N;
      sum = 0;
      for (k = 0; k < N; k++)
      {
        sum += parteA[iN + k] * B[k + jN];
      }
      parteC[iN + j] = sum;
    }
  }
}

void proceso(int id, int P, int N)
{
  int tot = N * N;
  int filas = N / P;
  int parte = filas * N;

  int *A = NULL, *C = NULL;
  int *parteA = malloc(sizeof(int) * parte);
  int *parteC = calloc(parte, sizeof(int)); // importante usar calloc
  int *B = malloc(sizeof(int) * tot);
  int i, j, check = 1;

  if (id == 0)
  {
    A = malloc(sizeof(int) * tot);
    C = malloc(sizeof(int) * tot);
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        A[i * N + j] = 1;
        B[i + j * N] = 1; // almacenada por columnas
      }
    }
  }

  MPI_Scatter(A, parte, MPI_INT, parteA, parte, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(B, tot, MPI_INT, 0, MPI_COMM_WORLD);

  mult(parteA, B, parteC, N, filas);

  MPI_Gather(parteC, parte, MPI_INT, C, parte, MPI_INT, 0, MPI_COMM_WORLD);

  if (id == 0)
  {
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        if (C[i * N + j] != N)
        {
          check = 0;
        }
      }
    }
    if (check)
    {
      printf("Resultado correcto\n");
    }
    else
    {
      printf("Resultado incorrecto\n");
    }
    free(A);
    free(C);
  }

  free(B);
  free(parteA);
  free(parteC);
  printf("Proceso %d terminado\n", id);
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int id, P, N;
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Comm_size(MPI_COMM_WORLD, &P);

  if (argc < 2)
  {
    if (id == 0)
      printf("Uso: %s <N>\n", argv[0]);
    MPI_Finalize();
    return 1;
  }

  N = atoi(argv[1]);

  if (N % P != 0)
  {
    if (id == 0)
      printf("Error: N debe ser divisible por la cantidad de procesos\n");
    MPI_Finalize();
    return 1;
  }

  proceso(id, P, N);

  MPI_Finalize();
  return 0;
}
