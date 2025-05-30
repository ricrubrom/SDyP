import itertools
import re

# Archivos de entrada
archivos = {
    'secuencial': 'n_body.log',
    'pthreads-1': 'n_body_pthread_1.log',
    'pthreads-4': 'n_body_pthread_4.log',
    'pthreads-8': 'n_body_pthread_8.log',
    'pthreads-16': 'n_body_pthread_16.log'
}

# Función para parsear posiciones desde archivos con formato "Cuerpo X: px=..., py=..., pz=..."


def cargar_posiciones(path):
    cuerpos = {}
    patron = re.compile(
        r"Cuerpo (\d+): px=([-+eE0-9.]+), py=([-+eE0-9.]+), pz=([-+eE0-9.]+)")
    with open(path, 'r') as f:
        for linea in f:
            match = patron.match(linea.strip())
            if match:
                id_cuerpo = int(match.group(1))
                px = float(match.group(2))
                py = float(match.group(3))
                pz = float(match.group(4))
                cuerpos[id_cuerpo] = (px, py, pz)
    return cuerpos


# Cargar los datos
datos = {nombre: cargar_posiciones(path) for nombre, path in archivos.items()}

# Comparaciones por pares
pares = list(itertools.combinations(datos.keys(), 2))

with open("comparaciones.log", "w") as log:
    for a, b in pares:
        log.write(f"Comparando: {a} vs {b}\n")
        posiciones_a = datos[a]
        posiciones_b = datos[b]

        max_dif_x = (None, 0.0)
        max_dif_y = (None, 0.0)
        max_dif_z = (None, 0.0)
        suma_dif_x = 0.0
        suma_dif_y = 0.0
        suma_dif_z = 0.0

        num_cuerpos = len(posiciones_a)

        log.write(f"{'ID':<5} {'ΔX':>12} {'ΔY':>12} {'ΔZ':>12}\n")
        for i in range(num_cuerpos):
            x1, y1, z1 = posiciones_a[i]
            x2, y2, z2 = posiciones_b[i]

            dx = abs(x1 - x2)
            dy = abs(y1 - y2)
            dz = abs(z1 - z2)

            suma_dif_x += dx
            suma_dif_y += dy
            suma_dif_z += dz

            if dx > max_dif_x[1]:
                max_dif_x = (i, dx)
            if dy > max_dif_y[1]:
                max_dif_y = (i, dy)
            if dz > max_dif_z[1]:
                max_dif_z = (i, dz)

            log.write(f"{i:<5} {dx:12.6e} {dy:12.6e} {dz:12.6e}\n")

        prom_x = suma_dif_x / num_cuerpos
        prom_y = suma_dif_y / num_cuerpos
        prom_z = suma_dif_z / num_cuerpos

        log.write("\nResumen:\n")
        log.write(
            f"Máxima diferencia en X: cuerpo {max_dif_x[0]} con ΔX = {max_dif_x[1]:.6e}\n")
        log.write(
            f"Máxima diferencia en Y: cuerpo {max_dif_y[0]} con ΔY = {max_dif_y[1]:.6e}\n")
        log.write(
            f"Máxima diferencia en Z: cuerpo {max_dif_z[0]} con ΔZ = {max_dif_z[1]:.6e}\n")
        log.write(f"Promedio ΔX = {prom_x:.6e}\n")
        log.write(f"Promedio ΔY = {prom_y:.6e}\n")
        log.write(f"Promedio ΔZ = {prom_z:.6e}\n")
        log.write("-" * 60 + "\n\n")
