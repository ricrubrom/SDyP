import os
import itertools
import re

# Detectar archivos .log que empiezan con n_body
todos_los_logs = [f for f in os.listdir(
    '.') if f.startswith('n_body') and f.endswith('.log')]

# Diccionario para mantener orden de prioridad
orden_claves = []
archivos = {}

for archivo in sorted(todos_los_logs):
    if archivo == 'n_body.log':
        clave = 'secuencial'
    elif 'pthread' in archivo:
        match = re.search(r'n_body_pthread_(\d+)\.log', archivo)
        if not match:
            continue
        hilos = int(match.group(1))
        clave = f'pthreads-{hilos:03d}'  # para orden lexicográfico
    elif 'hybrid' in archivo:
        match = re.search(r'n_body_hybrid_(\d+)_(\d+)\.log', archivo)
        if not match:
            continue
        maquinas = int(match.group(1))
        hilos = int(match.group(2))
        clave = f'hybrid-{maquinas:03d}-{hilos:03d}'
    else:
        continue
    archivos[clave] = archivo
    orden_claves.append(clave)

# Eliminar duplicados respetando el orden
orden_claves = list(dict.fromkeys(orden_claves))

# Función para parsear posiciones


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


# Cargar posiciones
datos = {nombre: cargar_posiciones(
    archivos[nombre]) for nombre in orden_claves}

# Comparaciones ordenadas: secuencial con todos, luego pthreads con restantes, etc.
comparados = set()
pares = []
for i, a in enumerate(orden_claves):
    for b in orden_claves[i+1:]:
        if (a, b) not in comparados and (b, a) not in comparados:
            pares.append((a, b))
            comparados.add((a, b))

# Generar archivo de comparación
with open("comparaciones.log", "w") as log:
    for a, b in pares:
        log.write(f"Comparando: {a} vs {b}\n")
        posiciones_a = datos[a]
        posiciones_b = datos[b]

        ids_comunes = set(posiciones_a.keys()) & set(posiciones_b.keys())

        if not ids_comunes:
            log.write("No hay cuerpos en común entre estos dos archivos.\n\n")
            continue

        max_dif_x = (None, 0.0)
        max_dif_y = (None, 0.0)
        max_dif_z = (None, 0.0)
        suma_dif_x = 0.0
        suma_dif_y = 0.0
        suma_dif_z = 0.0

        log.write(f"{'ID':<5} {'ΔX':>12} {'ΔY':>12} {'ΔZ':>12}\n")
        for i in sorted(ids_comunes):
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

        num_cuerpos = len(ids_comunes)
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
