// Ejercicio 1
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[])
{
   double *A;
   int i, j;
   int check = 1;

   int numThreads = atoi(argv[2]);
   int N = atoi(argv[1]);
   omp_set_num_threads(numThreads);

   // Aloca memoria para la matriz
   A = (double *)malloc(sizeof(double) * N * N);

   // Inicializa la matriz. Cada posicion debe quedar con el valor I*J
   //  I => fila J=> columna.

   // El problea era que el pragma omp parallel for estaba mal ubicado, estaba dentro del for de las filas, lo que causaba que i se inicializara siempre en 0
   // Puede estar adentro del for, pero para eso debe tener firstprivate(i), o estar afuera del for
   for (i = 0; i < N; i++)
   {
#pragma omp parallel for shared(A) private(j) firstprivate(i)
      for (j = 0; j < N; j++)
      {
         A[i * N + j] = i * j;
      }
   }

   // Verifica el resultado
   for (i = 0; i < N; i++)
   {
      for (j = 0; j < N; j++)
      {
         check = check && (A[i * N + j] == i * j);
      }
   }

   if (check)
   {
      printf("Resultado correcto\n");
   }
   else
   {
      printf("Resultado erroneo \n");
   }

   free(A);

   return (0);
}
