#include "pre.h"

XRAD_BEGIN


double pows(double x, double p)
{
	return x > 0 ? pow(x, p) : -pow(-x, p);
}


void	SharpeningTest()
{

	GraphSet gs("graph", "x", "y");
	GraphSet gs2("derivs", "x", "y");

	double	p1 = 2;
	double	p2 = 0.5;


	double	normalizer_radius = 1;

	while (true)
	{
		size_t	N(128), upsample_factor(8), M(N*upsample_factor);

		RealFunctionF32 f(N, 0);
		RealFunctionF32	g(M);
		RealFunctionF32	h(M);

		f[N / 2] = 1;

		std::fill(f.begin() + N / 16, f.begin() + 2 * N / 16, 1.f);
		std::fill(f.begin() + 3 * N / 16, f.begin() + 4 * N / 16, 0.5f);

		for (size_t i = 5 * N / 8; i < 7 * N / 8; ++i) f[i] = double(i - 5 * N / 8) / double(N / 4);

		for (size_t i = 0; i < M; ++i)
		{
			double	x = double(i) / upsample_factor;
			g[i] = f.in(x, &interpolators::cubic);
			h[i] = f.in(x, &interpolators::icubic);
		}

		gs.ChangeGraphUniform(0, f, 0, 1, "original");
		gs.ChangeGraphUniform(1, g, 0, 1. / upsample_factor, "interp");






		RealFunctionF32 grad(M, 0), lap(M, 0);

		for (size_t i = 1; i < M - 1; ++i)
		{
			grad[i] = fabs(g[i + 1] - g[i - 1]) / 2;
			lap[i] = (g[i + 1] + g[i - 1]) - 2 * g[i];
		}
		grad *= double(upsample_factor);
		lap *= square(double(upsample_factor));
		normalizer_radius = GetFloating("Enter Normalizer radius coefficient", normalizer_radius, 0, 10);

		double	factor = -0.12 / normalizer_radius;

		RealFunctionF32	grad2(grad);
		grad2.FilterGauss(normalizer_radius*upsample_factor);
		grad2 += 1e-6f;


		factor = GetFloating("Enter Factor coefficient", factor, -10, 10);

		ComplexFunctionF32	addition(M, complexF32(0));

		for (size_t i = 0; i < M; ++i)
		{
			addition[i] += factor*pows(grad[i], p1) * pows(lap[i], p2) / pows(grad2[i], p1 + p2 - 1);
		}

		g += real(addition);

		gs2.ChangeGraphUniform(0, grad, 0, 1, "grad");
		gs2.ChangeGraphUniform(1, lap, 0, 1, "lap");
		gs.ChangeGraphUniform(2, g, 0, 1. / upsample_factor, "sharpened");

		gs.Display(false);
		gs2.Display(true);
	}
}


XRAD_END