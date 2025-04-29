#include "../../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

void proceso(int id, int P, int N)
{
  int *A = NULL, *B = NULL;
  int *parteA = NULL, *parteB = NULL;
  int max = -1, min = 99999999, sum = 0;
  double avg = 0;
  int parte_max = -1, parte_min = 99999999, parte_sum = 0;
  int i, j, check = 1;
  int tot = N * N;
  int filas = N / P;
  int parte = filas * N;

  // Inicializar A con valores aleatorios
  srand(time(NULL));
  if (id == 0)
  {
    A = malloc(sizeof(int) * tot);
    B = malloc(sizeof(int) * tot); // Asegurarse de que B tenga el tamaño correcto
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        A[i * N + j] = rand() % 1000;
      }
    }
  }

  parteA = malloc(sizeof(int) * parte);
  parteB = malloc(sizeof(int) * parte);

  // Registrar tiempo de inicio para la versión paralela
  double start_time = MPI_Wtime();

  MPI_Scatter(A, parte, MPI_INT, parteA, parte, MPI_INT, 0, MPI_COMM_WORLD);

  // Cálculos del máximo, mínimo y suma para la versión paralela
  parte_sum = 0; // Reiniciar la suma local para cada proceso
  for (i = 0; i < filas; i++)
  {
    for (j = 0; j < N; j++)
    {
      int valor = parteA[i * N + j];
      parte_sum += valor;
      if (valor > parte_max)
        parte_max = valor;
      if (valor < parte_min)
        parte_min = valor;
    }
  }

  // Reducción global de max, min y sum
  MPI_Allreduce(&parte_max, &max, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&parte_min, &min, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
  MPI_Allreduce(&parte_sum, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

  // Calculando el promedio correcto
  avg = (double)sum / tot; // El total de elementos es N * N

  // Modificar la parteB con los valores correspondientes para la versión paralela
  for (i = 0; i < filas; i++)
  {
    for (j = 0; j < N; j++)
    {
      int valor = parteA[i * N + j];
      if (valor < avg)
      {
        parteB[i * N + j] = min;
      }
      else if (valor > avg)
      {
        parteB[i * N + j] = max;
      }
      else
      {
        parteB[i * N + j] = avg;
      }
    }
  }

  // Asegúrate de que el tamaño en el gather sea correcto
  MPI_Gather(parteB, parte, MPI_INT, B, parte, MPI_INT, 0, MPI_COMM_WORLD);

  // Registrar tiempo de fin para la versión paralela
  double end_time = MPI_Wtime();

  // Versión secuencial: procesar sin paralelismo
  double start_time_sec = 0, end_time_sec = 0;
  if (id == 0)
  {
    start_time_sec = MPI_Wtime();
    // Cálculos del máximo, mínimo y suma para la versión secuencial
    max = -1;
    min = 99999999;
    sum = 0;
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        int valor = A[i * N + j];
        sum += valor;
        if (valor > max)
          max = valor;
        if (valor < min)
          min = valor;
      }
    }

    // Calculando el promedio correcto para la versión secuencial
    avg = (double)sum / (N * N); // El total de elementos es N * N

    // Modificar B según el promedio para la versión secuencial
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        int valor = A[i * N + j];
        if (valor < avg)
        {
          B[i * N + j] = min;
        }
        else if (valor > avg)
        {
          B[i * N + j] = max;
        }
        else
        {
          B[i * N + j] = avg;
        }
      }
    }
    end_time_sec = MPI_Wtime();
  }

  // Verificación del resultado
  if (id == 0)
  {
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        if (B[i * N + j] != min && B[i * N + j] != max && B[i * N + j] != avg)
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

    // Imprimir los tiempos de ejecución
    printf("Tiempo de ejecución secuencial: %f segundos\n", end_time_sec - start_time_sec);
    printf("Tiempo de ejecución paralelo (%d procesos): %f segundos\n", P, end_time - start_time);

    // Cálculo del Speedup y la eficiencia
    if (end_time - start_time > 0)
    {
      double speedup = (end_time_sec - start_time_sec) / (end_time - start_time);
      printf("Speedup: %.2f\n", speedup);
      double eficiencia = (speedup / P) * 100.0;
      printf("Eficiencia: %.2f%%\n", eficiencia);
    }

    free(A);
    free(B);
  }

  free(parteA);
  free(parteB);
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
