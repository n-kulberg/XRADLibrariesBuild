﻿/*!
	\file
	\date 8:11:2016 17:41
	\author kns
*/
#ifndef GenerateFigures_h__
#define GenerateFigures_h__

#include <XRADBasic/MathFunctionTypesMD.h>

XRAD_BEGIN

void	SimulateSponge(RealFunctionMD_UI8	&f, size_t n_points, double agility, double average_speed, double string_radius, size_t foundation_thickness);
void	TestSpongeSimulation();

XRAD_END

#endif // GenerateFigures_h__
