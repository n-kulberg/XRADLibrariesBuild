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
#include <XRADGUI/Sources/RasterImageFile/RasterImageFile.h>
#include "TestHandy.h"

//--------------------------------------------------------------

using namespace xrad;

void	TestRasterImageFile()
{
	wstring	fn = GetFileNameRead(L"Open raster image", saved_default_value);
	file::raster_image	f(fn);

	DisplayMathFunction2D(f.lightness(), "lightness");
	DisplayMathFunction2D(f.channel(color_channel::hue), "hue");
	DisplayMathFunction2D(f.rgb(), "image");
}

void	TestHandy()
{
	TestRasterImageFile();
}


//--------------------------------------------------------------
