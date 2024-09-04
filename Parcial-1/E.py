import numpy as np

values = np.array([10, 1, 5, 4, 2, 2, 3, 8, 2, 4])
offset_valuess = np.array([5.9, -3.1, 0.9, -0.1, -2.1, -2.1, -1.1, 3.9, -2.1, -0.1])
probabilidades = np.array([0.18, 0.09, 0.17, 0.1, 0.02, 0.06, 0.03, 0.04, 0.04, 0.27])

# Generar 10,000 iteraciones usando bootstrapping y la transformada inversa
n_iteraciones = 10000
medias = []

for _ in range(n_iteraciones):
	muestra = np.random.choice(offset_valuess, size=10, replace=True, p=probabilidades)
	medias.append(np.mean(muestra))

# Calcular la desviación estándar de las medias
std_dev_medias = np.std(medias)

# Visualizar la distribución de medias en un histograma
import matplotlib.pyplot as plt

plt.hist(medias, bins=30, density=True)
plt.title('Distribución de Medias (Bootstrapping)')
plt.xlabel('Media')
plt.ylabel('Frecuencia')
plt.show()