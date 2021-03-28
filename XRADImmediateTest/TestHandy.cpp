﻿/*!
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

//--------------------------------------------------------------

using namespace xrad;
void TestHandy()
{
	auto img = GetPainting(L"Нарисуйте картинку", 512, 512);
	DisplayMathFunction2D(img, L"Картинка");
}

//--------------------------------------------------------------
