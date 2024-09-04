import numpy as np
import matplotlib.pyplot as plt

def generador_congruencial(n, x0):
    """
    Generador Congruencial Lineal Mixto para generar números pseudoaleatorios.

    Args:
        n (int): Número de valores a generar.
        x0 (int): Semilla inicial para el generador.

    Returns:
        tuple: Una tupla que contiene:
            - np.array: Array de números aleatorios generados.
            - int: La última semilla utilizada (para su uso en futuras llamadas).
    """
    a = 1664525  # Constante multiplicativa
    c = 1013904223  # Constante aditiva
    m = 2**32  # Módulo
    x = x0
    resultados = []
    for _ in range(n):
        x = (a * x + c) % m
        resultados.append(x / m)
    return np.array(resultados), x

# Valores y media
valores = np.array([2, 6, 2, 6, 8, 4, 3, 4, 1, 6])
media = np.mean(valores)
valores_offset = valores - media

# Set de probabilidades
probabilidades = [0.34, 0.13, 0.06, 0.08, 0.01, 0.04, 0.03, 0.04, 0.01, 0.26]

# Cálculo de la función de distribución acumulada (CDF)
cdf = np.cumsum(probabilidades)

# Generar 10,000 iteraciones con la CDF
n_iteraciones = 10000
iteraciones = []

for _ in range(n_iteraciones):
    """
    Genera iteraciones utilizando un número aleatorio uniforme y
    selecciona un valor de `valores` basado en la CDF.
    """
    u = np.random.uniform()
    indice = np.searchsorted(cdf, u)
    iteraciones.append(valores[indice])

# Convertir a array de numpy
iteraciones = np.array(iteraciones)

# Generar medias de subconjuntos de iteraciones
medias_iteraciones = [np.mean(np.random.choice(iteraciones, size=10, replace=True)) for _ in range(n_iteraciones)]

# Histograma de las medias
plt.hist(medias_iteraciones, bins=30, edgecolor='black')
plt.title('Histograma de Distribución de Medias')
plt.xlabel('Media')
plt.ylabel('Frecuencia')
plt.show()

# Calcular la desviación estándar de las medias
desviacion_estandar = np.std(medias_iteraciones)
print(f'Desviación estándar de la distribución de medias: {desviacion_estandar}')

# Dividir las medias en 5 rangos uniformes
rango_min = np.min(medias_iteraciones)
rango_max = np.max(medias_iteraciones)
rangos = np.linspace(rango_min, rango_max, 6)

# Contar la cantidad de medias en cada rango y calcular sus probabilidades
contadores = np.histogram(medias_iteraciones, bins=rangos)[0]
probabilidades_rangos = contadores / n_iteraciones

print(f'Probabilidades por rango: {probabilidades_rangos}')

# Parámetro de rechazo y aceptación
c_param = max(probabilidades_rangos)  # Constante c
aceptados = []

# Recorremos todas las iteraciones
for _ in range(n_iteraciones):
    """
    Genera dos números aleatorios utilizando el generador congruencial
    y aplica el método de aceptación y rechazo para determinar si
    el valor generado es aceptado o rechazado.
    """
    # Actualizamos x0 en cada llamada para evitar valores repetidos
    u1, x0 = generador_congruencial(1, x0)
    u2, x0 = generador_congruencial(1, x0)
    if u2 < (u1 * c_param):
        aceptados.append(u1[0])
        print(f"Aceptado: u1={u1[0]}, u2={u2[0]}")
    else:
        print(f"Rechazado: u1={u1[0]}, u2={u2[0]}")

# Graficar la función de masa de probabilidad para valores aceptados
if aceptados:
    """
    Si hay valores aceptados, se grafica su histograma,
    que representa la función de masa de probabilidad.
    """
    plt.hist(aceptados, bins=30, edgecolor='black', density=True)
    plt.title('Función de Masa de Probabilidad')
    plt.xlabel('Valor')
    plt.ylabel('Probabilidad')
    plt.show()
else:
    print("No se aceptaron valores.")
