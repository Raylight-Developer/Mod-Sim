class LCG:
	def __init__(self, seed=3, a=5, c=7, m=200):
		self.seed = seed
		self.a = a
		self.c = c
		self.m = m

	def next(self):
		self.seed = (self.a * self.seed + self.c) % self.m
		return self.seed

	def random(self):
		return self.next()# / self.m

lcg = LCG()
for _ in range(10):
	print(lcg.random())