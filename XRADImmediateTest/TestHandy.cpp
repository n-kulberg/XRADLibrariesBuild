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


//--------------------------------------------------------------

using namespace xrad;

void TestHandy()
{
	RealVectorF64	cc({1,2,-3});
	RealVectorF64	cc1(4, 0);
	size_t	s(512);
	RealFunctionF32	f(s);

	auto	poly = [](double x, const RealVectorF64 &cc)
	{
		double result = 0;
		for(size_t i = 0; i < cc.size(); ++i)
		{
			result += cc[i]*pow(x, i);
		}
		return result;
	};

	for(size_t i = 0; i < s; ++i)
	{
		double	x = i;//3*double(i)/s;
		f[i] = poly(x, cc) + RandomUniformF64(-1,1);
	}

	DetectLSPolynomUniformGrid(f, cc1);


}

//--------------------------------------------------------------
