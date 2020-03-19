#include "pre.h"
#include "TestFunctionsArchive.h"
#include <XRADSystem/Sources/IniFile/XRADIniFile.h>
#include <XRADBasic/FFT1D.h>
#include <XRADBasic/Sources/Utils/RandomNoiseGenerator.h>
#include <XRADBasic/Sources/Math/SpecialFunctions.h>

#include <iostream>


/*!
 * \file TestFunctionsArchive.cpp
 * \date 2017/06/20 12:12
 *
 * \author kulberg
 *
 * \brief
 *
 * TODO: long description
 *
 * \note
*/

XRAD_BEGIN

void	FFTBenchmark()
{
	/*
	Время БПФ на большом массиве NxN
	N	t (ms)
	16		0.00092
	32		0.0095
	64		0.011
	128		0.079
	256		0.086
	512		0.099
	1024	0.66
	2048	0.74
	4096	0.88
	8192	5.4

	Время БПФ на меньшем массиве Nxn_repeats
	N	t (ms)
	16		0.0093
	32		0.0095
	64		0.01
	128		0.076
	256		0.085
	512		0.1
	1024	0.66
	2048	0.78
	4096	1.1
	8192	6.1

	Время БПФ, многопоточная версия
	N	t (ms)
	16		0.0027
	32		0.0024
	64		0.004
	128		0.0094
	256		0.0093
	512		0.011
	1024	0.057
	2048	0.066
	4096	0.082
	8192	0.45


	*/
	static size_t	N = 256;
	N = ceil_fft_length(GetUnsigned("FFT length", N, 16, 65536));
	size_t	repeats(16*1024/N);
	size_t	repeats1 = N;//range(repeats, 1, N);
	ComplexFunction2D_F32	f(N, N, complexF32(0));
	ComplexFunctionF32	g(N);

	for(size_t i = 0; i < N; ++i) g[i] = gauss(double(i)-N/2, N/8);// +RandomUniformF64(0, 1./N);


	FFTf(g, fftFwd|fftRollBefore|fftRollAfter);

	TimeProfiler	tp;
	tp.Start();
	// тест быстродействия
#if 1
//	Вот мой код:
	ComplexFunctionF32 f1(N);
	FFT(f1, ftForward); // инициализация таблиц
	ComplexFunctionF32 f2(f1);
	double t0 = GetPerformanceCounterMSec();
	for(size_t i=0; i<repeats; ++i)
	{
		memcpy(&f1[0], &f2[0], N * sizeof(f1[0]));
		FFT(f1, ftForward);
	}
	double t1 = GetPerformanceCounterMSec();
	ShowString("Result", ssprintf("Time: %lf; 1 iter = %lf mks\n", (t1 - t0)*1000, ((t1-t0)/repeats)*1000));
#else
	StartProgress("Benchmark", repeats);
	for(int j = 0; j < int(repeats); ++j)
	{
//		#pragma omp parallel for schedule (guided)
		for(int i = 0; i < int(N); ++i)
		{
		#define fn row
			FFTf(f.fn(i), fftFwd);
			FFTf(f.fn(i), fftRev);
		#undef fn
		}
		NextProgress();
	}
#endif
	tp.Stop();
//	EndProgress();

	// тест корректности
	f.fill(complexF32(0));
//	#pragma omp parallel for schedule (guided)
	for(int i = 0; i < int(N); ++i)
	{
		f.at(i,i) = 1;

		FFTf(f.col(i), fftFwd | fftRollAfter);
		f.col(i) *= g;
		//ApplyWindowFunction(f.col(i), e_blackman_nuttall_win);
		FFTf(f.col(i), fftRev| fftRollBefore);
	}

	DisplayMathFunction2D(f, ssprintf("N= %d, t = %.2g mks", N, tp.LastElapsed().mksec()/(repeats*repeats1)));

}


int	tablesize(2000);

class	SomeNoiseGenerator : public TableRandomNoiseGenerator
{
	TableDistributionContainer	dc;
public:
	SomeNoiseGenerator() : TableRandomNoiseGenerator(tablesize)
	{
		double	minval(1), dx(0.1);
		RealFunctionF64	pdf(100);
		double	divisor(0);
		for(int i = 0; i < 100; ++i)
		{
			pdf[i] = i;
			divisor += i;
		}
		pdf /= divisor;
		dc.SetPDF(pdf, minval, dx);
		CreateTransformFunctionPDF(pdf, minval, dx);
		DisplayMathFunction(pdf_table(), minval, dx, "pdf");
		DisplayMathFunction(cdf_table(), minval, dx, "cdf");
		DisplayMathFunction(transform_table(), 0, 1./tablesize, "transform f");
	}
	const DistributionContainer &GetDistributionContainer(){
		return dc;
	}
};


void	TestRandoms()
{
// 	LookForPair();
// 	return;
	size_t	n_tests = 16536*65536;
//	unsigned int	n_tests = 32768*65536;
//	unsigned int	n_tests = 8192*65536;
	int	value_range = 65536*8;
	double	v1 = 0;
	double	v2 = 1;//4./32768;
	double	treshold = v2-v1;
	double	dt = treshold/value_range;
	RealFunctionI32	histogram_large(value_range+1, 0);

	for(unsigned int n = 0; n < n_tests; ++n)
	{
//		#error
		// остановка: проверка свойств функций. нижеидущая что-то показала нехорошее. посмотреть, когда у нее начинаются повторы
//		double	random_value = RandomUniformNoncorrelated(0,1);
		double	random_value = RandomUniformF64(0, 1);
		if(random_value >= v1 && random_value < v2)
		{
			++histogram_large[(random_value-v1)/dt];
		}
	}

	DisplayMathFunction(histogram_large, v1, dt, "histogram_large");
}



void	TestIni()
{
	std::vector<wstring> filename_list;
	std::vector<wstring> foldername_list;
	wstring filter = L"*.ini*";
	wstring folder_name = L"c:\\temp\\CT";

	IniFileWriter	w;
	w.open(L"c:\\temp\\test.ini");
	w.write_section("section_1");
	w.write_int(string("integral"), 32);
	w.write_double(string("double"), 3.14);
	w.write_wstring(string("wstring"), L"e\"nglish, русская, Répétez, quote [للُبنانيين], [角色脾气], linefeed\n literal ={\\x813e}, esc = \\xfeda");
//		w.write_wstring(string("wstring"), L"\n");
	w.write_string("string", "english, русская, linefeed\n literal ={\\x813e}, esc = \\xfeda");
	w.write_int("int_value", 1536);
	w.write_section("BasicParams");
	w.close();

	IniFileReader	r;

	r.open(L"c:\\temp\\test.ini");
	r.set_section("section_1");
	int	n1 = r.read_int("integral", 32); n1;//to avoid warning
	double d1 = r.read_double("double", 3.14);d1;//to avoid warning
	wstring	ws1 = r.read_wstring("wstring");
	string	s1 = r.read_string("wstring");

	ShowString(L"ws1", ws1);


	wstring	ws2 = r.read_wstring("string");
	string	s2 = r.read_string("string");
	ShowString(L"ws2", ws2);

	r.close();
	int	n = r.read_int("int_value", 0);
	cout << "\n\n" << s1 << "\n\n" << s2 << "\n------------------\n";
	wcout << "\n\n" << ws1 << "\n\n" << ws2;
	wcout << L"\nint = " << n << L"\n---";
	fflush(stdout);

//		ForceDebugBreak();

//		return;
//	wstring wstr = WGetAplicationDirectory();
	GetDirectoryContent(filename_list, foldername_list, folder_name, filter);

	std::vector<wstring> preset_description_list;
	for(size_t i = 0; i <= filename_list.size()-1; ++i)
	{
		IniFileReader ini_file_read;
		wstring filename = folder_name + wpath_separator() + filename_list[i];
		ini_file_read.open(filename);
		ini_file_read.set_section("BasicParams");
		wstring preset_description = ini_file_read.read_wstring("preset_description");
// 			double	something = ini_file_read.read_double("something", i);
// 			ShowFloating("something", something);
		ini_file_read.close();
		preset_description_list.push_back(preset_description);
	}
	preset_description_list.push_back(L"Exit");
	size_t preset_decision = GetButtonDecision(L"Choose preset", preset_description_list);
	if(preset_decision == filename_list.size())
	{
		throw canceled_operation("size_t GetSeriesNum(dicom_study_map data_map_params),canceled operation");
	}
	wstring full_path = folder_name + wpath_separator() + filename_list[preset_decision];

// 		RealFunctionF64 test_distribution(4096);
// 		std::vector<wstring> files, folders;
// 		wstring test_dir = GetExistingDirectoryName(L"Укажите папку");
// 		string filter = " ";
// 		GetDirectoryContent(files, folders, test_dir, filter);
// 		int i = GetButtonDecision(L"choose", files);

}


namespace SpecialFunctions
{
double	In_taylor_series_exp(double x, double nu);
};
void	TestMarcumQ1()
{
	int	N = 512;
	double	a_max = 10;
	double x0 = 0;
	double x1 = a_max;
	double dx = (x1-x0)/N;
	RealFunction2D_F64	marcum(N, N);
	int order = 1;


	try
	{
		while(true)
		{
			order = GetSigned("Order", order, -100, 100);

			GUIProgressBar progress;
			progress.start("Creating XRAD", N);
			for(int i = 0; i < N; ++i)
			{
				double a = x0 + i*dx;
				for(int j = 0; j < N; ++j)
				{
					double b = x0 + j*dx;
					marcum.at(i,j) = Qm(a, b, order);
				}
// #error исправить Qm при M>10
				++progress;
			}
			progress.end();
			DisplayMathFunction2D(marcum, ssprintf("marcum %d", order));
		}
	}
	catch(canceled_operation &)
	{

	}
}


XRAD_END
