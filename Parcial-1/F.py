import numpy as np

values = np.array([10, 1, 5, 4, 2, 2, 3, 8, 2, 4])
offset_valuess = np.array([5.9, -3.1, 0.9, -0.1, -2.1, -2.1, -1.1, 3.9, -2.1, -0.1])
probabilidades = np.array([0.18, 0.09, 0.17, 0.1, 0.02, 0.06, 0.03, 0.04, 0.04, 0.27])

# Parámetros del generador congruencial linear mixto
a = 1103515245
c = 12345
m = 2**31
x = np.random.randint(0, m)

def f(val: float): # f(u) es la función de probabilidad target
	return 1.0

def lcg():
	global x
	x = (a * x + c) % m
	return x / m

# Generar valores aleatorios para aceptación y rechazo
c_param = 1.0  # valor de "c" ajustado
for _ in range(1000):
	u = lcg()
	v = lcg()
	if v <= c_param * f(u):
		# Valor aceptado
		pass