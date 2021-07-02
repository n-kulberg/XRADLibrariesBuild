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
#include <XRADGUI/XRAD.h>
#include "TestHandy.h"
#include <XRADGUI/Sources/RasterImageFile/RasterImageFile.h>

//--------------------------------------------------------------

using namespace xrad;
void TestHandy()
{
// 	RealFunction2D_F32 img (512, 512, 0);
// 
// 	for(size_t i = 0; i < 512; ++i) img.at(i, i) = 255;

	wstring	filename = GetFileNameRead(L"Select image file");

	RasterImageFile	file(filename, L"rb");

	auto img = file.rgb();

	DisplayMathFunction2D(img, L"Картинка");
}

//--------------------------------------------------------------
