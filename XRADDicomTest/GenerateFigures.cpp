#include <pre.h>
#include "GenerateFigures.h"
#include <XRAD/GUI/SaveRasterImage.h>


/********************************************************************
	created:	2016/11/08
	created:	8:11:2016   17:41
	author:		kns
*********************************************************************/

#include "MathFunctionGUIMD.h"

#include <XRADDicom/XRADDicom.h>
#include <XRADDicomGUI/XRADDicomGUI.h>

XRAD_BEGIN

void	PutCircle(RealFunction2D_UI8 &slice, point2_F32 &p, double r)
{
	double	smoothness = 0.5;//0...1
	ptrdiff_t	v0 = range(p.y()-r-1, 0, slice.vsize()-1);
	ptrdiff_t	v1 = range(p.y()+r+1, 0, slice.vsize()-1);
	ptrdiff_t	h0=range(p.x()-r-1, 0, slice.hsize()-1);
	ptrdiff_t	h1=range(p.x()+r+1, 0, slice.hsize()-1);
	float	power = 0.5;
	for(ptrdiff_t i = v0; i < v1; ++i)
	{
		for(ptrdiff_t j = h0; j < h1; ++j)
		{
			double	distance = std::hypot(double(i)-p.y(), double(j)-p.x());
			float	b = 255.*pow(range((r-distance)/smoothness, 0, 1), power);
			slice.at(i,j) = max(uint8_t(b), slice.at(i,j));
		}
	}
}

//recommended start params
// n_points = 4*s
// agility = 0.05
// average_speed = 0.02
// string_radius = 2.5
// 	foundation_thickness = 20;


inline double cycle(const double &x, const double &min_val, const double &max_val)
{
	if(x <= max_val && x >= min_val)
	{
		return x;
	}
	else
	{
		double div = (max_val-min_val);
		double	n = (x-min_val)/div;
		return min_val + (n-floor(n))*div;
	}
}


void	SimulateSponge(RealFunctionMD_UI8	&volume, size_t n_points, double agility, double average_speed, double string_radius, size_t foundation_thickness)
{

	MathFunction<point2_F32, double, number_complexity::scalar>	positions(n_points);//использование неуместного number_complexity
	MathFunction<point2_F32, double, number_complexity::scalar>	speeds(n_points);

	for(size_t i = 0; i < n_points; ++i)
	{
		positions[i].x() = RandomUniformF64(0, volume.sizes(1));
		positions[i].y() = RandomUniformF64(0, volume.sizes(2));

		speeds[i].x() = RandomUniformF64(-average_speed, average_speed);
		speeds[i].y() = RandomUniformF64(-average_speed, average_speed);
	}

	GUIProgressBar progress;
	progress.start("Sponge simulation", volume.sizes(0));
	for(size_t i = 0; i < volume.sizes(0) - 4*foundation_thickness; ++i)
	{
		auto slice = volume.GetSlice({i, slice_mask(1), slice_mask(0)});
		for(size_t j = 0; j < n_points; ++j)
		{
			point2_F32	speed_increment(RandomUniformF64(-agility, agility), RandomUniformF64(-agility, agility));
			speeds[j] += speed_increment;

			speeds[j].x() = cycle(speeds[j].x(),-average_speed, average_speed);
			speeds[j].y() = cycle(speeds[j].y(), -average_speed, average_speed);


			if(!in_range(positions[j].y() + speed_increment.y(), 0, slice.vsize()-1)) speeds[j].y() *= -1;

			if(!in_range(positions[j].x() + speed_increment.x(), 0, slice.hsize()-1)) speeds[j].x() *= -1;

			positions[j] += speeds[j];

			PutCircle(slice, positions[j], string_radius);
		}
		progress++;
	}

	//дно
	for(size_t i = 0; i < 0.5 * foundation_thickness; ++i) volume.GetSlice({i, slice_mask(1), slice_mask(0)}).fill(255);

	for(size_t i = 0; i < 4*foundation_thickness; ++i)
	{		
		volume.GetSlice({slice_mask(1), i, slice_mask(0)}).fill(255);
		volume.GetSlice({slice_mask(1), volume.sizes(1) - i, slice_mask(0)}).fill(255);

		volume.GetSlice({slice_mask(1), slice_mask(0), i}).fill(255);
		//volume.GetSlice({slice_mask(1), slice_mask(0), volume.sizes(2) - i}).fill(255);// одну стенку оставляем открытой
	}

	for(size_t i = 0; i < 3*foundation_thickness; ++i)
	{
		volume.GetSlice({slice_mask(1), i, slice_mask(0)}).fill(0);
		volume.GetSlice({slice_mask(1), volume.sizes(1) - i, slice_mask(0)}).fill(0);

		volume.GetSlice({slice_mask(1), slice_mask(0), i}).fill(0);
		volume.GetSlice({slice_mask(1), slice_mask(0), volume.sizes(2) - i}).fill(0);
	}


	progress.end();

	DisplayMathFunction3D(volume, "sponge");


	progress.start(L"writing plates", volume.sizes(0));
	size_t j = 0;
	for(size_t i = 0; i < volume.sizes(0); ++i)
	{
		wstring	filename;

		filename = ssprintf(L"c://temp//sponge//%d.png", ++j);
		SaveRasterImage(filename, volume.GetSlice({i, slice_mask(1), slice_mask(0)}).ref(), 0, 255);

		filename = ssprintf(L"c://temp//sponge//%d.png", ++j);
		SaveRasterImage(filename, volume.GetSlice({i, slice_mask(1), slice_mask(0)}).ref(), 0, 255);
		++progress;
	}
	progress.end();

}

void	GenerateTomogram(TomogramAcquisition &tomogram, const RealFunctionMD_F32 &volume)
{
	//todo возможно, это нужно перенести в Process классы и разделить действия по ним

//	wstring	name;

//	double slice_thickness(0.1);

	size_t	n_slices = volume.sizes(0);

	//?tomogram.slices().MakeCopy(volume);

	//?tomogram.thickness.realloc(n_slices, slice_thickness);
	/*+
	tomogram.scales_xy.realloc(n_slices, point2_F32(1,1));
	tomogram.image_positions_patient.realloc(n_slices, point3_F32(0,0,0));*/

	//+- tomogram.rescale_intercept.realloc(n_slices, 0);
	//+- tomogram.rescale_slope.realloc(n_slices, 1);

	//tomogram.locations.realloc(n_slices);
	for(size_t i = 0; i < n_slices; ++i)
	{
		;//+tomogram.image_positions_patient[i].z() = i*slice_thickness;
	}

}

//TODO Смысл этой функции и всего исходника: научиться делать томограмму, не зависящую от какого-либо набора, созданного кем-то еще.
void	TestSpongeSimulation()
{
	//recommended start params
	size_t	n_slices = 500;
	size_t	w = 2160;
	size_t	h = 3840;

	double	string_radius = 2.5;

	size_t	n_strings = 0.5*w*h/(pi()*square(string_radius));//половина площади, полное затухание 7.5 МГц на 1 см
	double	max_depth = 7;//cm
	n_strings /= max_depth;

	printf("\n n_strings = %zu\n", n_strings);
	fflush(stdout);

	double	agility = 0.5;
	double	average_speed = 2*agility;
	size_t	foundation_thickness = 40;
	RealFunctionMD_UI8	volume({n_slices, h, w}, 0);
	ProcessAcquisition_ptr sim_ct;

	SimulateSponge(volume, n_strings, agility, average_speed, string_radius, foundation_thickness);

	Pause();
	return;

	volume *= 1024;
	GenerateTomogram(dynamic_cast<CTAcquisition&>(*sim_ct), volume);
	wstring root_foldername = L"c:\\temp\\data_store\\clinic\\2017_write_subset\\subset_simulated";
	DisplayProcessAcquisition(*sim_ct, L"Simulated CT");
	//todo (Kovbas) вернуть функцию ниже
	/*
	sim_ct->save_to_file(
		root_foldername,
		e_save_to_new_file,
		Dicom::e_jpeg_lossless,
		Dicom::instance_cache(),//пустой кэш, никакой перезаписи
		GUIProgressProxy());*/
}

XRAD_END
