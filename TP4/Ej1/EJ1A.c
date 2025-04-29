#include "../../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

// Función para realizar la multiplicación de matrices en paralelo
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

// Función para realizar la multiplicación de matrices de forma secuencial
void mult_secuencial(int *A, int *B, int *C, int N)
{
  int i, j, k;
  int iN, jN, sum;
  for (i = 0; i < N; i++)
  {
    iN = i * N;
    for (j = 0; j < N; j++)
    {
      jN = j * N;
      sum = 0;
      for (k = 0; k < N; k++)
      {
        sum += A[iN + k] * B[k + jN];
      }
      C[iN + j] = sum;
    }
  }
}

// Función principal para manejar el proceso de cada nodo
void proceso(int id, int P, int N)
{
  int tot = N * N; // Total de elementos en la matriz
  int filas = N / P; // Número de filas asignadas a cada proceso
  int parte = filas * N; // Tamaño de la submatriz para cada proceso
  int *A = NULL, *C = NULL, *C_sec = NULL; // Matrices completas
  int *parteA = malloc(sizeof(int) * parte); // Submatriz A para cada proceso
  int *parteC = calloc(parte, sizeof(int)); // Submatriz C para cada proceso
  int *B = malloc(sizeof(int) * tot); // Matriz B compartida
  int i, j, check = 1;

  double start_parallel, end_parallel, start_sec, end_sec;

  // Inicialización de matrices en el proceso 0
  if (id == 0)
  {
    A = malloc(sizeof(int) * tot);
    C = malloc(sizeof(int) * tot);
    C_sec = malloc(sizeof(int) * tot);
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        A[i * N + j] = 1; // Inicialización de A con 1
        B[i + j * N] = 1; // Inicialización de B con 1
      }
    }
  }

  // Sincronización antes de iniciar la ejecución paralela
  MPI_Barrier(MPI_COMM_WORLD);
  start_parallel = dwalltime();

  // Distribución de datos entre procesos
  if (id == 0)
  {
    for (i = 1; i < P; i++)
    {
      MPI_Send(A + i * parte, parte, MPI_INT, i, 99, MPI_COMM_WORLD); // Enviar submatriz A
      MPI_Send(B, tot, MPI_INT, i, 99, MPI_COMM_WORLD); // Enviar matriz B
    }
    for (i = 0; i < parte; i++)
    {
      parteA[i] = A[i]; // Copiar la parte correspondiente de A
    }
  }
  else
  {
    MPI_Recv(parteA, parte, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Recibir submatriz A
    MPI_Recv(B, tot, MPI_INT, 0, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Recibir matriz B
  }

  // Multiplicación de matrices en paralelo
  mult(parteA, B, parteC, N, filas);

  // Recolección de resultados en el proceso 0
  if (id == 0)
  {
    for (i = 0; i < filas; i++)
    {
      for (j = 0; j < N; j++)
      {
        C[i * N + j] = parteC[i * N + j]; // Copiar resultados locales
      }
    }
    for (i = 1; i < P; i++)
    {
      MPI_Recv(C + i * parte, parte, MPI_INT, i, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Recibir resultados de otros procesos
    }
  }
  else
  {
    MPI_Send(parteC, parte, MPI_INT, 0, 99, MPI_COMM_WORLD); // Enviar resultados al proceso 0
  }

  // Sincronización después de la ejecución paralela
  MPI_Barrier(MPI_COMM_WORLD);
  end_parallel = dwalltime();

  // Ejecución secuencial en el proceso 0 para comparación
  if (id == 0)
  {
    start_sec = dwalltime();
    mult_secuencial(A, B, C_sec, N); // Multiplicación secuencial
    end_sec = dwalltime();

    double parallel_time = end_parallel - start_parallel;
    double secuencial_time = end_sec - start_sec;

    // Verificación de resultados
    for (i = 0; i < N; i++)
    {
      for (j = 0; j < N; j++)
      {
        if (C[i * N + j] != N || C_sec[i * N + j] != N)
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

    // Imprimir tiempos y métricas de rendimiento
    printf("Tiempo secuencial: %.5f segundos\n", secuencial_time);
    printf("Tiempo paralelo (%d procesos): %.5f segundos\n", P, parallel_time);
    printf("Speedup: %.2f\n", secuencial_time / parallel_time);
    printf("Eficiencia: %.2f%%\n", (secuencial_time / (parallel_time * P)) * 100.0);

    free(A);
    free(C);
    free(C_sec);
  }

  // Liberar memoria
  free(B);
  free(parteA);
  free(parteC);
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  int id, P, N;
  MPI_Comm_rank(MPI_COMM_WORLD, &id); // Obtener el ID del proceso
  MPI_Comm_size(MPI_COMM_WORLD, &P); // Obtener el número total de procesos

  // Validación de argumentos
  if (argc < 2)
  {
    if (id == 0)
      printf("Uso: %s <N>\n", argv[0]);
    MPI_Finalize();
    return 1;
  }

  N = atoi(argv[1]); // Tamaño de la matriz

  // Validación de divisibilidad de N entre P
  if (N % P != 0)
  {
    if (id == 0)
      printf("Error: N debe ser divisible por la cantidad de procesos\n");
    MPI_Finalize();
    return 1;
  }

  // Ejecutar el proceso principal
  proceso(id, P, N);

  MPI_Finalize();
  return 0;
}
