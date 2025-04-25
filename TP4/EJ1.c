#include "../utils/utils.h"
#include <stdio.h>
#include <mpi.h>

void mult(int *parteA, int *B, int *parteC, int N, int filas)
{
  int i, j, k;
  int iN;
  int jN;
  int sum;
  for (i = 0; i < N; i++)
  {
    iN = i * N;
    for (j = 0; j < N; j++)
    {
      jN = j * N;
      sum = 0;
      for (k = 0; k < N; k++)
      {
        sum += parteA[iN + k] + B[k + jN];
      }
      parteC[iN + j] = sum;
    }
  }
}

void proceso(int id, int P, int N)
{
  int *A, *B, *C;
  int i, j;
  int *parteA, *parteC;
  int tot = N * N, parte = tot / P, filas = N / P;

  parteA = (int *)malloc(sizeof(int) * parte);
  parteC = (int *)malloc(sizeof(int) * parte);
  B = (int *)malloc(sizeof(int) * tot);
  if (id == 0)
  {
    A = (int *)malloc(sizeof(int) * tot);
    B = (int *)malloc(sizeof(int) * tot);
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        A[i * N + j] = 1;
        B[i + j * N] = 1;
      }
    }
  }
  MPI_Scatter(A, parte, MPI_INT, parteA, parte, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Broadcast(B, tot, MPI_INT, 0, MPI_COMM_WORLD);

  mult(parteA, B, parteC, N, filas);

  MPI_Gather(parteC, parte, MPI_INT, C, parte, MPI_INT, 0, MPI_COMM_WORLD);
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);
  int id;
  int P;
  int N;
  MPI_Comm_rank(MPI_COMM_WORLD, &id);
  MPI_Comm_size(MPI_COMM_WORLD, &P);
  if (argc >= 2)
  {
    N = atoi(argv[1]);
  }
  proceso(id, P, N);
  MPI_Finalize();
  return (0);
}