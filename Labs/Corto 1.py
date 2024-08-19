import random
from typing import *

# 1. Configuración Inicial
TARGET_PHRASE = "to be or not to be"
POPULATION_SIZE = 200
MUTATION_RATE = 0.01
ALPHABET = "abcdefghijklmnopqrstuvwxyz "

def random_string(length: int) -> str:
	return ''.join(random.choice(ALPHABET) for i in range(length))

def fitness(individual: str):
	return sum(1 for expected, actual in zip(TARGET_PHRASE, individual) if expected == actual)

def selection(population: List[str], fitnesses: List[int]):
	total_fitness = sum(fitnesses)
	pick = random.uniform(0, total_fitness)
	current = 0
	for individual, fit in zip(population, fitnesses):
		current += fit
		if current > pick:
			return individual

def crossover(parent1: str, parent2: str):
	crossover_point = random.randint(0, len(parent1) - 1)
	child = parent1[:crossover_point] + parent2[crossover_point:]
	return child

def mutate(individual: str):
	mutated = ''.join(
		(char if random.random() > MUTATION_RATE else random.choice(ALPHABET))
		for char in individual
	)
	return mutated

def main():
	# 2. Inicialización de la Población
	population: List[str] = [random_string(len(TARGET_PHRASE)) for i in range(POPULATION_SIZE)]

	generation = 0
	best_individual = ''
	best_fitness = 0

	# 3. Ejecución del Algoritmo
	while best_fitness < len(TARGET_PHRASE):
		generation += 1
		fitnesses = [fitness(individual) for individual in population]

		new_population = []
		for i in range(POPULATION_SIZE):
			parent1 = selection(population, fitnesses)
			parent2 = selection(population, fitnesses)
			child = crossover(parent1, parent2)
			child = mutate(child)
			new_population.append(child)
		# 4. Convergencia
		population = new_population
		best_individual = max(population, key=fitness)
		best_fitness = fitness(best_individual)

		# 5. Visualización
		print(f"Generación {generation}: {best_individual} (Aptitud: {best_fitness})")

	print(f"¡Frase objetivo alcanzada en la generación {generation}!")

if __name__ == "__main__": main()