#include "Flip.hpp"

FlipFluid::FlipFluid() {

	scene = Scene();
	scene.numPressureIters = 50;
	scene.numParticleIters = 2;

	uint res = 100;
	dvec1 spacing = 0.5;

	dvec1 tankHeight = 1.0 * 400.0;
	dvec1 tankWidth = 1.0 * 300.0;
	h = tankHeight / res;
	density = 1000.0;

	dvec1 relWaterHeight = 0.8;
	dvec1 relWaterWidth = 0.6;

	// dam break

	// compute number of particles

	dvec1 r = 0.3 * h;	// particle radius w.r.t. cell size
	dvec1 dx = 2.0 * r;
	dvec1 dy = sqrt(3.0) / 2.0 * dx;

	dvec1 numX = floor((relWaterWidth * tankWidth - 2.0 * h - 2.0 * r) / dx);
	dvec1 numY = floor((relWaterHeight * tankHeight - 2.0 * h - 2.0 * r) / dy);
	maxParticles = numX * numY;


	this->density = density;
	fNumX = floor(tankWidth / spacing) + 1;
	fNumY = floor(tankHeight / spacing) + 1;
	h = max(tankWidth / fNumX, tankHeight / fNumY);
	fInvSpacing = 1.0 / h;
	fNumCells = fNumX * fNumY;

	u = vector(fNumCells, 0.0);
	v = vector(fNumCells, 0.0);
	du = vector(fNumCells, 0.0);
	dv = vector(fNumCells, 0.0);
	prevU = vector(fNumCells, 0.0);
	prevV = vector(fNumCells, 0.0);
	p = vector(fNumCells, 0.0);
	s = vector(fNumCells, 0.0);
	cellType = vector(fNumCells, Cell_Type::AIR_CELL);
	cellColor = vector(fNumCells, dvec3(0.0));

	// particles

	particlePos = vector(maxParticles, dvec2(0.0));
	particleColor = vector(maxParticles, dvec3(1.0));

	particleVel = vector(maxParticles, dvec2(0.0));
	particleDensity = vector(fNumCells, 0.0);
	particleRestDensity = 0.0;

	particleRadius = 0.25;
	pInvSpacing = 1.0 / (2.2 * particleRadius);
	pNumX = floor(tankWidth * pInvSpacing) + 1;
	pNumY = floor(tankHeight * pInvSpacing) + 1;
	pNumCells = pNumX * pNumY;

	numCellParticles = vector(pNumCells, 0u);
	firstCellParticle = vector(pNumCells + 1, 0u);
	cellParticleIds = vector(maxParticles, 0u);

	numParticles = 0;

	// create particles
	numParticles = numX * numY;
	uint p = 0;
	for (uint i = 0; i < numX; i++) {
		for (uint j = 0; j < numY; j++) {
			particlePos[p].x = h + r + dx * i + (j % 2 == 0 ? 0.0 : r);
			particlePos[p].y = h + r + dy * j;
		}
	}

	// setup grid cells for tank

	uint n = fNumY;

	for (uint i = 0; i < fNumX; i++) {
		for (uint j = 0; j < fNumY; j++) {
			uint s = 1.0;	// fluid
			if (i == 0 || i == fNumX-1 || j == 0)
				s = 0.0;	// solid
			this->s[i * n + j] = s;
		}
	}
}

void FlipFluid::integrateParticles(dvec1 dt, dvec1 gravity) {
	for (uint i = 0; i < numParticles; i++) {
		particleVel[i] += dt * gravity;
		particlePos[i] += particleVel[i] * dt;
	}
}

void FlipFluid::pushParticlesApart(uint numIters) {
	dvec1 colorDiffusionCoeff = 0.001;

	// count particles per cell
	fill(numCellParticles.begin(), numCellParticles.end(), 0u);

	for (uint i = 0; i < numParticles; i++) {
		dvec1 x = particlePos[i].x;
		dvec1 y = particlePos[i].y;

		dvec1 xi = clamp(floor(x * pInvSpacing), 0.0, pNumX - 1);
		dvec1 yi = clamp(floor(y * pInvSpacing), 0.0, pNumY - 1);
		uint cellNr = xi * pNumY + yi;
		numCellParticles[cellNr]++;
	}

	// partial sums

	uint first = 0;

	for (uint i = 0; i < pNumCells; i++) {
		first += numCellParticles[i];
		firstCellParticle[i] = first;
	}
	firstCellParticle[pNumCells] = first;		// guard

	// fill particles into cells

	for (uint i = 0; i < numParticles; i++) {
		dvec1 x = particlePos[i].x;
		dvec1 y = particlePos[i].y;

		dvec1 xi = clamp(floor(x * pInvSpacing), 0.0, pNumX - 1);
		dvec1 yi = clamp(floor(y * pInvSpacing), 0.0, pNumY - 1);
		uint cellNr = xi * pNumY + yi;
		firstCellParticle[cellNr]--;
		cellParticleIds[firstCellParticle[cellNr]] = i;
	}

	// push particles apart

	dvec1 minDist = 2.0 * particleRadius;
	dvec1 minDist2 = minDist * minDist;

	for (uint iter = 0; iter < numIters; iter++) {

		for (uint i = 0; i < numParticles; i++) {
			dvec1 px = particlePos[i].x;
			dvec1 py = particlePos[i].y;

			dvec1 pxi = floor(px * pInvSpacing);
			dvec1 pyi = floor(py * pInvSpacing);
			uint x0 = max(pxi - 1, 0.0);
			uint y0 = max(pyi - 1, 0.0);
			uint x1 = min(pxi + 1, pNumX - 1);
			uint y1 = min(pyi + 1, pNumY - 1);

			for (uint xi = x0; xi <= x1; xi++) {
				for (uint yi = y0; yi <= y1; yi++) {
					uint cellNr = xi * pNumY + yi;
					uint first = firstCellParticle[cellNr];
					uint last = firstCellParticle[cellNr + 1];
					for (uint j = first; j < last; j++) {
						uint id = cellParticleIds[j];
						if (id == i)
							continue;
						dvec1 qx = particlePos[id].x;
						dvec1 qy = particlePos[id].y;

						dvec1 dx = qx - px;
						dvec1 dy = qy - py;
						dvec1 d2 = dx * dx + dy * dy;
						if (d2 > minDist2 || d2 == 0.0)
							continue;
						dvec1 d = sqrt(d2);
						dvec1 s = 0.5 * (minDist - d) / d;
						dx *= s;
						dy *= s;
						particlePos[i].x -= dx;
						particlePos[i].y -= dy;
						particlePos[i].x += dx;
						particlePos[i].y += dy;

						// diffuse colors

						for (uint k = 0; k < 3; k++) {
							dvec1 color0 = particleColor[i][k];
							dvec1 color1 = particleColor[id][k];
							dvec1 color = (color0 + color1) * 0.5;
							particleColor[i][k] = color0 + (color - color0) * colorDiffusionCoeff;
							particleColor[id][k] = color1 + (color - color1) * colorDiffusionCoeff;
						}
					}
				}
			}
		}
	}
}

void FlipFluid::handleParticleCollisions() {
	dvec1 r = particleRadius;

	dvec1 minX = h + r;
	dvec1 maxX = (fNumX - 1) * h - r;
	dvec1 minY = h + r;
	dvec1 maxY = (fNumY - 1) * h - r;


	for (uint i = 0; i < numParticles; i++) {
		dvec1 x = particlePos[i].x;
		dvec1 y = particlePos[i].y;

		// wall collisions
		if (x < minX) {
			x = minX;
			particleVel[i].x = 0.0;

		}
		if (x > maxX) {
			x = maxX;
			particleVel[i].x = 0.0;
		}
		if (y < minY) {
			y = minY;
			particleVel[i].y = 0.0;
		}
		if (y > maxY) {
			y = maxY;
			particleVel[i].y = 0.0;
		}
		particlePos[i].x = x;
		particlePos[i].y = y;
	}
}

void FlipFluid::updateParticleDensity() {
	dvec1 n = fNumY;
	dvec1 h1 = fInvSpacing;
	dvec1 h2 = 0.5 * h;

	vector<dvec1> d = particleDensity;
	fill(d.begin(), d.end(), 0.0);

	for (uint i = 0; i < numParticles; i++) {
		dvec1 x = particlePos[i].x;
		dvec1 y = particlePos[i].y;

		x = clamp(x, h, (fNumX - 1) * h);
		y = clamp(y, h, (fNumY - 1) * h);

		dvec1 x0 = floor((x - h2) * h1);
		dvec1 tx = ((x - h2) - x0 * h) * h1;
		dvec1 x1 = min(x0 + 1, fNumX - 2.0);

		dvec1 y0 = floor((y - h2) * h1);
		dvec1 ty = ((y - h2) - y0 * h) * h1;
		dvec1 y1 = min(y0 + 1, fNumY - 2.0);

		dvec1 sx = 1.0 - tx;
		dvec1 sy = 1.0 - ty;

		if (x0 < fNumX && y0 < fNumY) d[x0 * n + y0] += sx * sy;
		if (x1 < fNumX && y0 < fNumY) d[x1 * n + y0] += tx * sy;
		if (x1 < fNumX && y1 < fNumY) d[x1 * n + y1] += tx * ty;
		if (x0 < fNumX && y1 < fNumY) d[x0 * n + y1] += sx * ty;
	}

	if (particleRestDensity == 0.0) {
		dvec1 sum = 0.0;
		dvec1 numFluidCells = 0;

		for (uint i = 0; i < fNumCells; i++) {
			if (cellType[i] == Cell_Type::FLUID_CELL) {
				sum += d[i];
				numFluidCells++;
			}
		}

		if (numFluidCells > 0)
			particleRestDensity = sum / numFluidCells;
	}
}

void FlipFluid::transferVelocities(bool toGrid, dvec1 flipRatio) {
	dvec1 n = fNumY;
	dvec1 h1 = fInvSpacing;
	dvec1 h2 = 0.5 * h;

	if (toGrid) {
		prevU = u;
		prevV = v;

		fill(du.begin(), du.end(), 0.0);
		fill(dv.begin(), dv.end(), 0.0);
		fill(u.begin(), u.end(), 0.0);
		fill(v.begin(), v.end(), 0.0);

		for (uint i = 0; i < fNumCells; i++)
			cellType[i] = s[i] == 0.0 ? Cell_Type::SOLID_CELL : Cell_Type::AIR_CELL;

		for (uint i = 0; i < numParticles; i++) {
			dvec1 x = particlePos[i].x;
			dvec1 y = particlePos[i].y;
			uint xi = clamp(floor(x * h1), 0.0, fNumX - 1.0);
			uint yi = clamp(floor(y * h1), 0.0, fNumY - 1.0);
			uint cellNr = xi * n + yi;
			if (cellType[cellNr] == Cell_Type::AIR_CELL)
				cellType[cellNr] = Cell_Type::FLUID_CELL;
		}
	}

	for (uint component = 0; component < 2; component++) {

		uint dx = component == 0 ? 0.0 : h2;
		uint dy = component == 0 ? h2 : 0.0;

		vector<dvec1> f = component == 0 ? u : v;
		vector<dvec1> prevF = component == 0 ? prevU : prevV;
		vector<dvec1> d = component == 0 ? du : dv;

		for (uint i = 0; i < numParticles; i++) {
			dvec1 x = particlePos[i].x;
			dvec1 y = particlePos[i].y;

			x = clamp(x, h, (fNumX - 1) * h);
			y = clamp(y, h, (fNumY - 1) * h);

			uint x0 = min(floor((x - dx) * h1), fNumX - 2.0);
			uint tx = ((x - dx) - x0 * h) * h1;
			uint x1 = min(x0 + 1.0, fNumX - 2.0);

			uint y0 = min(floor((y - dy) * h1), fNumY - 2.0);
			uint ty = ((y - dy) - y0 * h) * h1;
			uint y1 = min(y0 + 1.0, fNumY - 2.0);

			dvec1 sx = 1.0 - tx;
			dvec1 sy = 1.0 - ty;

			uint d0 = sx * sy;
			uint d1 = tx * sy;
			uint d2 = tx * ty;
			uint d3 = sx * ty;

			uint nr0 = x0 * n + y0;
			uint nr1 = x1 * n + y0;
			uint nr2 = x1 * n + y1;
			uint nr3 = x0 * n + y1;

			if (toGrid) {
				dvec1 pv = particleVel[i][component];
				f[nr0] += pv * d0;  d[nr0] += d0;
				f[nr1] += pv * d1;  d[nr1] += d1;
				f[nr2] += pv * d2;  d[nr2] += d2;
				f[nr3] += pv * d3;  d[nr3] += d3;
			}
			else {
				dvec1 offset = component == 0 ? n : 1;
				dvec1 valid0 = cellType[nr0] != Cell_Type::AIR_CELL || cellType[nr0 - offset] != Cell_Type::AIR_CELL ? 1.0 : 0.0;
				dvec1 valid1 = cellType[nr1] != Cell_Type::AIR_CELL || cellType[nr1 - offset] != Cell_Type::AIR_CELL ? 1.0 : 0.0;
				dvec1 valid2 = cellType[nr2] != Cell_Type::AIR_CELL || cellType[nr2 - offset] != Cell_Type::AIR_CELL ? 1.0 : 0.0;
				dvec1 valid3 = cellType[nr3] != Cell_Type::AIR_CELL || cellType[nr3 - offset] != Cell_Type::AIR_CELL ? 1.0 : 0.0;

				dvec1 v = particleVel[i][component];
				dvec1 d = valid0 * d0 + valid1 * d1 + valid2 * d2 + valid3 * d3;

				if (d > 0.0) {

					dvec1 picV = (valid0 * d0 * f[nr0] + valid1 * d1 * f[nr1] + valid2 * d2 * f[nr2] + valid3 * d3 * f[nr3]) / d;
					dvec1 corr = (valid0 * d0 * (f[nr0] - prevF[nr0]) + valid1 * d1 * (f[nr1] - prevF[nr1])
						+ valid2 * d2 * (f[nr2] - prevF[nr2]) + valid3 * d3 * (f[nr3] - prevF[nr3])) / d;
					dvec1 flipV = v + corr;

					particleVel[i][component] = (1.0 - flipRatio) * picV + flipRatio * flipV;
				}
			}
		}

		if (toGrid) {
			for (uint i = 0; i < f.size(); i++) {
				if (d[i] > 0.0)
					f[i] /= d[i];
			}

			// restore solid cells

			for (uint i = 0; i < fNumX; i++) {
				for (uint j = 0; j < fNumY; j++) {
					bool solid = cellType[i * n + j] == Cell_Type::SOLID_CELL;
					if (solid || (i > 0 && cellType[(i - 1) * n + j] == Cell_Type::SOLID_CELL))
						u[i * n + j] = prevU[i * n + j];
					if (solid || (j > 0 && cellType[i * n + j - 1] == Cell_Type::SOLID_CELL))
						v[i * n + j] = prevV[i * n + j];
				}
			}
		}
	}
}

void FlipFluid::solveIncompressibility(uint numIters, dvec1 dt, dvec1 overRelaxation, bool compensateDrift) {

	std::fill(p.begin(), p.end(), 0.0);
	prevU = u;
	prevV = v;

	uint n = fNumY;
	dvec1 cp = density * h / dt;

	dvec1 u;
	dvec1 v;

	for (uint i = 0; i < fNumCells; i++) {
		u = this->u[i];
		v = this->v[i];
	}

	for (uint iter = 0; iter < numIters; iter++) {
		for (uint i = 1; i < fNumX - 1; i++) {
			for (uint j = 1; j < fNumY - 1; j++) {

				if (cellType[i * n + j] != Cell_Type::FLUID_CELL)
					continue;

				uint center = i * n + j;
				uint left = (i - 1) * n + j;
				uint right = (i + 1) * n + j;
				uint bottom = i * n + j - 1;
				uint top = i * n + j + 1;

				dvec1 s = this->s[center];
				dvec1 sx0 = this->s[left];
				dvec1 sx1 = this->s[right];
				dvec1 sy0 = this->s[bottom];
				dvec1 sy1 = this->s[top];
				s = sx0 + sx1 + sy0 + sy1;
				if (s == 0.0)
					continue;

				dvec1 div = this->u[right] - this->u[center] +
					this->v[top] - this->v[center];

				if (particleRestDensity > 0.0 && compensateDrift) {
					dvec1 k = 1.0;
					dvec1 compression = particleDensity[i * n + j] - particleRestDensity;
					if (compression > 0.0)
						div = div - k * compression;
				}

				dvec1 p = -div / s;
				p *= overRelaxation;
				this->p[center] += cp * p;

				this->u[center] -= sx0 * p;
				this->u[right] += sx1 * p;
				this->v[center] -= sy0 * p;
				this->v[top] += sy1 * p;
			}
		}
	}
}

void FlipFluid::updateParticleColors() {
	dvec1 h1 = fInvSpacing;

	for (uint i = 0; i < numParticles; i++) {

		dvec1 s = 0.01;

		particleColor[3 * i] = clamp(particleColor[3 * i] - s, 0.0, 1.0);
		particleColor[3 * i + 1] = clamp(particleColor[3 * i + 1] - s, 0.0, 1.0);
		particleColor[3 * i + 2] = clamp(particleColor[3 * i + 2] + s, 0.0, 1.0);

		dvec1 x = particlePos[i].x;
		dvec1 y = particlePos[i].y;
		dvec1 xi = clamp(floor(x * h1), 1.0, fNumX - 1.0);
		dvec1 yi = clamp(floor(y * h1), 1.0, fNumY - 1.0);
		dvec1 cellNr = xi * fNumY + yi;

		dvec1 d0 = particleRestDensity;

		if (d0 > 0.0) {
			dvec1 relDensity = particleDensity[cellNr] / d0;
			if (relDensity < 0.7) {
				dvec1 s = 0.8;
				particleColor[i].x = s;
				particleColor[i].y = s;
				particleColor[i].z = 1.0;
			}
		}
	}
}

void FlipFluid::setSciColor(uint cellNr, dvec1 val, dvec1 minVal, dvec1 maxVal) {
	val = min(max(val, minVal), maxVal - 0.0001);
	dvec1 d = maxVal - minVal;
	val = d == 0.0 ? 0.5 : (val - minVal) / d;
	dvec1 m = 0.25;
	int num = floor(val / m);
	dvec1 s = (val - num * m) / m;
	dvec1 r, g, b;

	switch (num) {
	case 0: r = 0.0; g = s; b = 1.0; break;
	case 1: r = 0.0; g = 1.0; b = 1.0 - s; break;
	case 2: r = s; g = 1.0; b = 0.0; break;
	case 3: r = 1.0; g = 1.0 - s; b = 0.0; break;
	}

	cellColor[cellNr].x = r;
	cellColor[cellNr].y = g;
	cellColor[cellNr].z = b;
}

void FlipFluid::updateCellColors() {
	std::fill(cellColor.begin(), cellColor.end(), dvec3(0.0));

	for (uint i = 0; i < fNumCells; i++) {

		if (cellType[i] == Cell_Type::SOLID_CELL) {
			cellColor[i].x = 0.5;
			cellColor[i].y = 0.5;
			cellColor[i].z = 0.5;
		}
		else if (cellType[i] == Cell_Type::FLUID_CELL) {
			dvec1 d = particleDensity[i];
			if (particleRestDensity > 0.0)
				d /= particleRestDensity;
			setSciColor(i, d, 0.0, 2.0);
		}
	}
}

void FlipFluid::simulate(dvec1 dt) {
	uint numSubSteps = 1;
	dvec1 sdt = dt / numSubSteps;

	for (uint step = 0; step < numSubSteps; step++) {
		integrateParticles(sdt, scene.gravity);
		if (scene.separateParticles)
			pushParticlesApart(scene.numParticleIters);
		handleParticleCollisions();
		transferVelocities(true, scene.flipRatio);
		updateParticleDensity();
		solveIncompressibility(scene.numPressureIters, sdt, scene.overRelaxation, scene.compensateDrift);
		transferVelocities(false, scene.flipRatio);
	}

	updateParticleColors();
	updateCellColors();
}