/*!
	\file
	\brief Песочница для тестов

	Здесь можно быстро создавать временные тесты.

	Если созданный тест представляет ненулевую ценность, его следует перенести на постоянное место
	жительства в другой файл.

	Предполагается, что в "стационарном" состоянии этот файл содержит только функцию-заготовку
	TestHandy с пустым телом.
*/
#include "pre.h"

#include "TestHandy.h"
#include <XRADBasic/Sources/Utils/LeastSquares.h>
#include <XRADBasic/Sources/Containers/WindowFunction.h>


//--------------------------------------------------------------

using namespace xrad;

void TestHandy()
{
	auto type = window_function::hamming;
	size_t s0 = 32;
	size_t s = 1024;
	ComplexFunctionF32	f = window_function::create(type, s, (s-s0)/2, (s - s0) / 2);
	f /= sqrt(double(s)/s0);
	f /= AverageValue(f);
	f /= sqrt(s);

	DisplayMathFunction(f, 0, 1, window_function::name(type));

// 	RealVectorF64	cc({1,2,-3});
// 	RealVectorF64	cc1(4, 0);
// 	size_t	s(512);
// 	RealFunctionF32	f(s);
// 
// 	auto	poly = [](double x, const RealVectorF64 &cc)
// 	{
// 		double result = 0;
// 		for(size_t i = 0; i < cc.size(); ++i)
// 		{
// 			result += cc[i]*pow(x, i);
// 		}
// 		return result;
// 	};
// 
// 	for(size_t i = 0; i < s; ++i)
// 	{
// 		double	x = i;//3*double(i)/s;
// 		f[i] = poly(x, cc) + RandomUniformF64(-1,1);
// 	}
// 
// 	DetectLSPolynomUniformGrid(f, cc1);
// 

}

//--------------------------------------------------------------
