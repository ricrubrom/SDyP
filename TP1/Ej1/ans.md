## A

Analizar el algoritmo matrices.c que resuelve la multiplicación de matrices
C=AB donde A, B y C son matrices cuadradas de $N*N$:

### i)

Comparar el tiempo de ejecución original con el tiempo de eliminar las funciones getValor y setValor. ¿Son necesarias estas funciones?¿Qué puede decirse del overhead introducido por los llamados a estas funciones?

- El overhead introducido por estas funciones es muy grande, pues para una matriz de $1024 * 1024$ este programa se demoro mas de 16 segundos menos

### ii)

Partiendo del código sin las funciones getValor y setValor, comparar la
solución que almacena A, B y C por filas con las solución que almacena
B por columnas. ¿Qué conclusión puede obtenerse de la diferencia en
los tiempos de ejecución? ¿Cómo se relaciona con el principio de
localidad?

- La ejecucion se demora mas cuando estan ordenadas por columnas. Esto se debe a que al hacer el producto de las matrices, A y C se recorren por filas, pero al estar ordenadas por columna, no se cumple el principio de localidad (?), lo cual genera muchos fallos de cache. Lo ideal seria que A y C

## B

Analizar los algoritmos que resuelven distintas operaciones sobre matrices de $N*N$:
- expMatrices1.c: dos versiones que resuelven la operación AB + AC + AD
- expMatrices2.c: dos versiones que resuelven la operación AB + CD
- expMatrices3.c: dos versiones que resuelven la operación BA + CAD

Ejecutar con distintos valores de N. Comparar los tiempos de ejecución de las dos
versiones en cada caso. ¿Qué versión es más rápida? ¿Por qué?

- Para los 3 casos es mas rapida la version con menos bucles, esto se debe a que se debe iterar menos, y se aprovecha mejor el principio de localidad.
