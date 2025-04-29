#include "../../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

void proceso(int id, int P, int N)
{
  int *A = NULL, *B = NULL;
  int *parteB = NULL;
  int parte = N / P;
  int i, j, check = 1;

  A = (int *)malloc(sizeof(int) * N);
  parteB = (int *)malloc(sizeof(int) * parte);
  if (id == 0)
  {
    B = (int *)malloc(sizeof(int) * N);
    for (i = 0; i < N; i++)
    {
      A[i] = i;
    }
  }

  // Versión paralela: Iniciar el tiempo de arranque
  double start_parallel_time = MPI_Wtime();

  MPI_Bcast(A, N, MPI_INT, 0, MPI_COMM_WORLD);

  // Realizar el trabajo paralelo
  for (i = id * parte; i < (id + 1) * parte; i++)
  {
    int changed = 0;
    for (j = i + 1; j < N; j++)
    {
      if (A[j] == 2 * A[i])
      {
        changed = 1;
        parteB[i - id * parte] = j - i;
        break;
      }
    }
    if (!changed)
    {
      parteB[i - id * parte] = -1;
    }
  }

  // Recoger los resultados de los procesos
  MPI_Gather(parteB, parte, MPI_INT, B, parte, MPI_INT, 0, MPI_COMM_WORLD);

  // Medir el tiempo después de la ejecución paralela
  double end_parallel_time = MPI_Wtime();
  double parallel_time = end_parallel_time - start_parallel_time;

  if (id == 0)
  {
    // Medir tiempo de ejecución secuencial con MPI_Wtime
    double start_sequential_time = MPI_Wtime();

    for (i = 0; i < N; i++)
    {
      int changed = 0;
      for (j = i + 1; j < N; j++)
      {
        if (A[j] == 2 * A[i])
        {
          changed = 1;
          B[i] = j - i;
          break;
        }
      }
      if (!changed)
      {
        B[i] = -1;
      }
    }

    double end_sequential_time = MPI_Wtime();
    double secuencial_time = end_sequential_time - start_sequential_time;

    // Calcular Speedup y Eficiencia
    double speedup = secuencial_time / parallel_time;
    double eficiencia = speedup / P;

    // Mostrar los resultados
    printf("Tiempo de ejecución secuencial: %f segundos\n", secuencial_time);
    printf("Tiempo de ejecución paralelo: %f segundos\n", parallel_time);
    printf("Speedup: %f\n", speedup);
    printf("Eficiencia: %f\n", eficiencia);

    // Mostrar los resultados secuenciales
    // for (i = 0; i < N; i++)
    // {
    //   if (B[i] != -1)
    //   {
    //     printf("El elemento %d tiene un doble en la posición %d\n", A[i], B[i]);
    //   }
    //   else
    //   {
    //     printf("El elemento %d no tiene un doble\n", A[i]);
    //   }
    // }
  }

  free(A);
  free(parteB);
  if (id == 0)
  {
    free(B);
  }
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
