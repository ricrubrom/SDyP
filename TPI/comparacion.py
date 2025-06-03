import os
import re

# Detectar archivos .log que empiezan con n_body
todos_los_logs = [f for f in os.listdir(
    '.') if f.startswith('n_body') and f.endswith('.log')]

# Diccionario para mantener orden de prioridad
orden_claves = []
archivos = {}
tiempos = {}

for archivo in sorted(todos_los_logs):
    if archivo == 'n_body.log':
        clave = 'secuencial'
    elif 'pthread' in archivo:
        match = re.search(r'n_body_pthread_(\d+)\.log', archivo)
        if not match:
            continue
        hilos = int(match.group(1))
        clave = f'pthreads-{hilos:03d}'
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

    # Extraer tiempo total del archivo
    with open(archivo, 'r') as f:
        for linea in f:
            if 'Tiempo en segundos' in linea:
                match = re.search(r"([\d.]+)", linea)
                if match:
                    tiempo = float(match.group(1))
                    tiempos[clave] = tiempo
                break

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

# Comparaciones ordenadas
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
        suma_error_pct_x = 0.0
        suma_error_pct_y = 0.0
        suma_error_pct_z = 0.0
        eps = 1e-12

        log.write(
            f"{'ID':<3} {'ΔX':>10} {'ΔY':>10} {'ΔZ':>12} {'eX':>10} {'eY':>10} {'eZ':>10}\n")
        for i in sorted(ids_comunes):
            x1, y1, z1 = posiciones_a[i]
            x2, y2, z2 = posiciones_b[i]

            dx = abs(x1 - x2)
            dy = abs(y1 - y2)
            dz = abs(z1 - z2)

            suma_dif_x += dx
            suma_dif_y += dy
            suma_dif_z += dz

            ex = (dx / (abs(x1) + eps)) * 100
            ey = (dy / (abs(y1) + eps)) * 100
            ez = (dz / (abs(z1) + eps)) * 100

            suma_error_pct_x += ex
            suma_error_pct_y += ey
            suma_error_pct_z += ez

            if dx > max_dif_x[1]:
                max_dif_x = (i, dx)
            if dy > max_dif_y[1]:
                max_dif_y = (i, dy)
            if dz > max_dif_z[1]:
                max_dif_z = (i, dz)

            if ex > 0.0000009 or ey > 0.0000009 or ez > 0.0000009:
                log.write(
                    f"{i:<5} {dx:12.6e} {dy:12.6e} {dz:12.6e} {ex:.6f}% {ey:.6f}% {ez:.6f}%\n")

        num_cuerpos = len(ids_comunes)
        prom_x = suma_dif_x / num_cuerpos
        prom_y = suma_dif_y / num_cuerpos
        prom_z = suma_dif_z / num_cuerpos

        prom_pct_x = suma_error_pct_x / num_cuerpos
        prom_pct_y = suma_error_pct_y / num_cuerpos
        prom_pct_z = suma_error_pct_z / num_cuerpos

        # Cálculo de speedup y eficiencia (solo si uno es secuencial)
        if 'secuencial' in (a, b):
            base = 'secuencial'
            otro = b if a == 'secuencial' else a
            tiempo_seq = tiempos.get('secuencial', 0.0)
            tiempo_otro = tiempos.get(otro, 0.0)
            speedup = tiempo_seq / tiempo_otro if tiempo_otro > 0 else 0

            # Determinar cantidad de hilos/procesos
            match = re.search(r'(\d+)', otro)
            procesos = 1
            hilos = 1
            if 'pthread' in otro:
                hilos = int(match.group(1))
            elif 'hybrid' in otro:
                match = re.search(r'hybrid-(\d+)-(\d+)', otro)
                if match:
                    procesos = int(match.group(1))
                    hilos = int(match.group(2))

            eficiencia = speedup / (hilos * procesos) * 100

            log.write("\nResumen:\n\n")
            log.write(f"Cantidad de cuerpos procesados: {num_cuerpos}\n")
            log.write(f"Tiempo secuencial: {tiempo_seq:.6f} s\n")
            log.write(f"Tiempo {otro}: {tiempo_otro:.6f} s\n\n\n")
            log.write(f"Speedup: {speedup:.4f}\n")
            log.write(f"Eficiencia: {eficiencia:.4f}%\n\n\n")
        else:
            log.write("\nResumen:\n")
            log.write(f"Cantidad de cuerpos procesados: {num_cuerpos}\n\n\n")

        log.write(
            f"Máxima diferencia en X: cuerpo {max_dif_x[0]} con ΔX = {max_dif_x[1]:.6e}\n")
        log.write(
            f"Máxima diferencia en Y: cuerpo {max_dif_y[0]} con ΔY = {max_dif_y[1]:.6e}\n")
        log.write(
            f"Máxima diferencia en Z: cuerpo {max_dif_z[0]} con ΔZ = {max_dif_z[1]:.6e}\n\n")
        log.write(f"Promedio ΔX = {prom_x:.6e}\n")
        log.write(f"Promedio ΔY = {prom_y:.6e}\n")
        log.write(f"Promedio ΔZ = {prom_z:.6e}\n\n")
        log.write(f"Error porcentual promedio ΔX% = {prom_pct_x:.6f}%\n")
        log.write(f"Error porcentual promedio ΔY% = {prom_pct_y:.6f}%\n")
        log.write(f"Error porcentual promedio ΔZ% = {prom_pct_z:.6f}%\n\n")
        log.write("-" * 60 + "\n\n")
