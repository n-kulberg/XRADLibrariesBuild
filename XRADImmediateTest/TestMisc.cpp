/*!
	\file
	\date 2018/07/05 17:21
	\author kulberg
*/
#include "pre.h"
#include "TestMisc.h"
#include <XRADBasic/Sources/Utils/ValuePredicates.h>
#include <XRADBasic/Sources/Utils/numbers_in_string.h>

//--------------------------------------------------------------

XRAD_BEGIN

//--------------------------------------------------------------

void	TestPredicates()
{
	auto f1 = predicate::greater(2.);
	auto f2 = predicate::less_or_equal(3.);
	auto	f12 = f1 & f2;

	size_t	N(12);//, factor(2);
	RealFunctionF32	func1(N), func2(N), func12(N);
	RealFunctionF32	_func1(N), _func2(N), _func12(N);
	double	x0(0), x1(N/2);
	double	dx = (x1-x0)/N;

//	для проверки строгости неравенств можно дополнительно посмотреть результат под отладчиком
// 	f12(1);
// 	f12(2.5);
// 	f12(3.001);
// 	f12(3);

	for(size_t i = 0; i < N; ++i)
	{
		double	x = x0 + i*dx;
		func1[i] = f1(x);
		func2[i] = f2(x);
		func12[i] = (f1&f2)(x);

		_func1[i] = !f1(x);
		_func2[i] = !f2(x);
		_func12[i] = !(f1&f2)(x);
	}

	DisplayMathFunction(func1, x0, dx, "F1", false);
	DisplayMathFunction(func2, x0, dx, "F2", false);
	DisplayMathFunction(func12, x0, dx, "F12", false);

	DisplayMathFunction(_func1, x0, dx, "!F1", false);
	DisplayMathFunction(_func2, x0, dx, "!F2", false);
	DisplayMathFunction(_func12, x0, dx, "!F12");

}

//--------------------------------------------------------------

void	TestDigits()
{
	XRAD_USING;
//	double	v0 = 10.10000001;
//	double	v0 = 1.1000001;
	double	v0 = 10000000.999999;
	double	v1 = 1000000000.0001020304050607080901020304050;
	wstring	format = L"%.50g";
//	wstring	vs0 = L"1.00001020304050607080901020304050";//ssprintf(format, v0);
	wstring	vs0 = L"000001000.000000100000009e2";//ssprintf(format, v0);
	wstring	vs1 = L"001.99999E-2";//ssprintf(format, v1);
	wstring	s0 = ssprintf(L"%.16g", v0);
	wstring	s1 = ssprintf(L"%.16g", v1);

	smart_round(vs0);
	smart_round(vs1);

	ShowString(s0, vs0);
	ShowString(s1, vs1);

	throw quit_application("ok", 0);
}

//--------------------------------------------------------------

namespace
{

void	TestAnisotropicFiltering1D__unused()
{

//	TestDigits();
//	TestPredicates();
//	return;

	int graph_size = 1000;
	int graph_size_gauss = 250;
	RealFunctionF32 graph_func_noisy(graph_size, 0);
	for(int i = 0; i < graph_size; ++i)
	{
		if(i >(graph_size / 2.))
		{
			graph_func_noisy[i] = 10;
		}
		graph_func_noisy[i] += RandomGaussian(0, 0.5);
	}
	RealFunctionF32 gauss_func(graph_size_gauss, 0);

	for(int i = 0; i < graph_size_gauss; ++i)
	{
		double x = (i - (graph_size_gauss / 2));
		gauss_func[i] = gauss(fabs(x), 40) * 5;
	}
	RealFunctionF32 gauss_func_half(gauss_func);
	for(int i = graph_size_gauss/2; i < graph_size_gauss; ++i)
	{
		gauss_func_half[i] = 0;
	}


	GraphSet	graph_set_noisy(L"Набор графиков", L"Y", L"X");
	graph_set_noisy.AddGraphUniform(graph_func_noisy, -(graph_size / 2.), 1, "Original Function");
	double x0(0);
	double dx(1);
	for(int i = 0; i < 4; ++i)
	{
		graph_set_noisy.AddGraphUniform(gauss_func, -(graph_size / 2.)+ graph_size_gauss*x0, dx, "Wavelet Approximation Function");
		x0 += double(1. / (i + 1.));
		dx /= 2.;
	}
	graph_set_noisy.Display(true);

	GraphSet	graph_set_denoised(L"Набор графиков", L"Y", L"X");


	RealFunctionF32 graph_func_denoised_wavelet(graph_size, 0);
	RealFunctionF32 graph_func_denoised_aniso(graph_size, 0);
	RealFunctionF32 graph_func_denoised_1(graph_func_noisy);
	RealFunctionF32 graph_func_denoised_2(graph_func_noisy);
	RealFunctionF32 graph_func_denoised_3(graph_func_noisy);
	RealFunctionF32 graph_func_denoised_4(graph_func_noisy);
	RealFunctionF32 graph_func_denoised_left(graph_func_noisy);
	RealFunctionF32 graph_func_denoised_right(graph_func_noisy);
	double main_filter_factor_1 = ExponentialFlterCoefficient(20);
	double main_filter_factor_2 = ExponentialFlterCoefficient(10);
	double main_filter_factor_3 = ExponentialFlterCoefficient(5);
	double main_filter_factor_4 = ExponentialFlterCoefficient(2.5);
	double main_filter_factor_exp = ExponentialFlterCoefficient(40);
	ExponentialBlur1D(graph_func_denoised_left, main_filter_factor_exp, exponential_blur_reverse);
	ExponentialBlur1D(graph_func_denoised_right, main_filter_factor_exp, exponential_blur_forward);
	BiexpBlur1D(graph_func_denoised_1, main_filter_factor_1);
	BiexpBlur1D(graph_func_denoised_2, main_filter_factor_2);
	BiexpBlur1D(graph_func_denoised_3, main_filter_factor_3);
	BiexpBlur1D(graph_func_denoised_4, main_filter_factor_4);


	for(int i = 0; i <= 250; ++i)
	{
		graph_func_denoised_wavelet[i] = graph_func_denoised_1[i];
		graph_func_denoised_wavelet[graph_size - 1 - i] = graph_func_denoised_1[graph_size - 1 - i];
	}
	for(int i = 250; i <= 375; ++i)
	{
		graph_func_denoised_wavelet[i] = graph_func_denoised_2[i];
		graph_func_denoised_wavelet[graph_size - 1 - i] = graph_func_denoised_2[graph_size - 1 - i];
	}
	for(int i = 375; i <= 445; ++i)
	{
		graph_func_denoised_wavelet[i] = graph_func_denoised_3[i];
		graph_func_denoised_wavelet[graph_size - 1 - i] = graph_func_denoised_3[graph_size - 1 - i];
	}
	for(int i = 445; i <= 500; ++i)
	{
		graph_func_denoised_wavelet[i] = graph_func_denoised_4[i];
		graph_func_denoised_wavelet[graph_size - 1 - i] = graph_func_denoised_4[graph_size - 1 - i];
	}

	for(int i = 0; i <= graph_size/2; ++i)
	{
		graph_func_denoised_aniso[i] = graph_func_denoised_right[i];
		graph_func_denoised_aniso[graph_size-i-1] = graph_func_denoised_left[graph_size - i - 1];
	}
	graph_func_denoised_aniso[graph_size / 2] = graph_func_denoised_left[graph_size / 2];

	GraphSet	graph_set_aniso(L"Набор графиков", L"Y", L"X");
	graph_set_aniso.AddGraphUniform(graph_func_denoised_aniso, -(graph_size / 2.), 1, "Denoised Function");
	graph_set_aniso.AddGraphUniform(gauss_func, -(graph_size / 2.), 1.25, "Approximation Function");
	graph_set_aniso.AddGraphUniform(gauss_func_half, -((graph_size / 6)), 1.25, "Approximation Function");

	DisplayMathFunction(graph_func_denoised_aniso, -(graph_size / 2.), 1, "Denoised Aniso");
	DisplayMathFunction(graph_func_denoised_wavelet, -(graph_size / 2.), 1, "Denoised Wavelet");
	graph_set_aniso.Display(true);


	graph_set_denoised.AddGraphUniform(graph_func_denoised_wavelet, -(graph_size / 2.), 1, "1");

	double x0_1(0);
	double dx_1(1);
	for(int i = 0; i < 4; ++i)
	{
		graph_set_denoised.AddGraphUniform(gauss_func, -(graph_size / 2.) + graph_size_gauss*x0_1, dx_1, "Wavelet Approximation Function");
		x0_1 += double(1. / (i + 1.));
		dx_1 /= 2.;
	}
	graph_set_denoised.Display(true);
}

} // namespace

//--------------------------------------------------------------

XRAD_END
