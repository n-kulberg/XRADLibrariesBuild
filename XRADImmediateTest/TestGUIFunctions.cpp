#include "pre.h"
#include "TestGUIFunctions.h"
/*!
	\file
	\date 2018/07/05 17:10
	\author kulberg

	\brief
*/

#include <XRADBasic/Sources/Containers/VectorFunction.h>
#include "TestDynamicDialog.h"
#include "TestProgress.h"

#include <iostream>
#include <cstring>

XRAD_BEGIN

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

void	TestXRADObjects()
{
	const size_t	s0 = 100;
	const size_t	s1 = 150;
	const size_t	s2 = 200;

	typedef ComplexFunctionMD_F32 cmdf_t;
	//	typedef ComplexFunctionMD_I32F cmdf_t;
	typedef cmdf_t::slice_type	cslice_t;

	cmdf_t	complex_mdf({ s0, s1, s2 });
	MathFunctionMD<ColorImageF32> color_mdf({ s0, s1, s2 });

	FIRFilter2DReal	filter;
	double	filter_radius = GetFloating("Filter radius", 4, 0.0, infinity());

	GUIProgressBar progress;
	progress.start("Preparing animation", s0);
	for (size_t i = 0; i < s0; ++i)
	{
		ColorImageF32	color_mdf_slice;
		cslice_t	complex_mdf_slice;
		complex_mdf.GetSlice(complex_mdf_slice, { i, slice_mask(0), slice_mask(1) });
		color_mdf.GetSlice(color_mdf_slice, { i, slice_mask(0), slice_mask(1) });
		double	x = double(i) / s0;

		for (int j = 0; j < s1; ++j)
		{
			double	y = double(j) / s1;
			for (int k = 0; k < s2; ++k)
			{
				double factor = 256;
				double	z = double(k) / s2;
				complex_mdf_slice.at(j, k).re = factor*RandomUniformF64(-1, 1);
				complex_mdf_slice.at(j, k).im = factor*RandomUniformF64(-1, 1);
				color_mdf_slice.at(j, k).red() = 64.*(1. + cos(2 * pi()*x)) * (1. + sin(4 * pi()*y)) * z;
				color_mdf_slice.at(j, k).green() = 64.*((1. + cos(5 * pi()*x)) * (1. + sin(9 * pi()*y)) * (1. - z));
				color_mdf_slice.at(j, k).blue() = 0;


				// 				if(RandomUniformF64(0,1)<0.001) complex_mdf_slice.at(j,k) += 10;
				// 				if(RandomUniformF64(0,1)<0.001) complex_mdf_slice.at(j,k) -= 10;
				if (RandomUniformF64(0, 1) < 0.5)
				{
					complex_mdf_slice.at(j, k) *= 0.01;
				}
				if ((i == s0 / 2) && (j == s1 / 2) && (k == s2 / 2))
				{
					complex_mdf_slice.at(j, k) = factor * 1000;
				}
				else
				{
					complex_mdf_slice.at(j, k) = factor*(i + j + k);
				}
			}
		}

		++progress;
	}
	progress.end();

	point3_F64 filter_radiuses;
	filter_radiuses.z() = 1 * filter_radius;
	filter_radiuses.y() = 1 * filter_radius;
	filter_radiuses.x() = 1 * filter_radius;
	if (!CapsLock())
	{
		BiexpBlur3D(complex_mdf, filter_radiuses.z(), filter_radiuses.y(), filter_radiuses.x());
		BiexpBlur3D(color_mdf, filter_radiuses.z(), filter_radiuses.y(), filter_radiuses.x());
	}

	//	color_mdf /= 100;
	//	DisplayMathFunction3D(real(complex_mdf), "complex 3D object.re");
	DisplayMathFunction3D(complex_mdf, "complex 3D object", ScanFrameSector(cm(4), cm(10), degrees(-30), degrees(30)));
	DisplayMathFunction3D(color_mdf, "color 3D object", ScanFrameRectangle(cm(12), cm(10)));
}

//--------------------------------------------------------------

void	TestRasters()
{
	int	vs = 321;
	int	hs = 171;

	RealFunction2D_F32	rf(vs, hs);
	//	RealFunction2D_UI8	pixmap(vs,hs);
	//	DataArray2D<DataArray<ColorPixel> >	pixmap(vs,hs);

	ColorImageF32	pixmap(vs, hs);
	//	ColorImage<ColorSampleF32, double>	pixmap(vs,hs);
	DataArray2D<DataArray<float> >	pixmap_gs(vs, hs);
	DataArray2D<DataArray<complexF32> > pixmap_c(vs, hs);

	for (int i = 0; i < vs; i++)
	{
		double	y = double(i) / vs * pi();
		for (int j = 0; j < hs; ++j)
		{
			//			double	x = ((4.*i)/vs) * double(j)/hs * pi();
			double	x = double(j) / hs * pi();
			double val = (sin(16 * x)*sin(6 * y) + 1) * 128;
			rf.at(i, j) = val;
			double factor = 0.05;
			pixmap_c.at(i, j).re = pixmap.at(i, j).red() = factor*((1 + sin(4 * x)) + RandomUniformF64(0, 1.2));
			pixmap_c.at(i, j).im = pixmap.at(i, j).green() = factor*((1 + cos(8 * y)) + RandomUniformF64(0, 2.5));//255-val;
			pixmap.at(i, j).blue() = val;//RandomUniformF64(0,255);
//			pixmap.at(i,j).alpha() = j;

			pixmap_gs.at(i, j) = val / 100 - 1;
		}
	}

	ColorPixel	p(23);

	ColorSampleF32 s1(100), s2(100), s3(100);
	ColorSampleF32 p2(100);
	s2 = s1 += p2;
	s3 = p2 += s1;
	s3 += 1;
	//	p2 = s1 += p2;

	ColorFunctionF32	cf(100, ColorSampleF32(0));
	cf /= 2;
	cf += ColorSampleF32(1, 1, 1);
	//	cf += 1.;
	cf += cf;

	//pixmap += pixmap;
//	pixmap += ColorPixel(100);

	DisplayMathFunction2D(pixmap_gs, "gs", ScanFrameSector(cm(6), cm(12), degrees(-30), degrees(20)));
	DisplayMathFunction2D(pixmap_c, "complex", ScanFrameSector(cm(6), cm(12), degrees(-20), degrees(20)));
	DisplayMathFunction2D(pixmap, "rgb", ScanFrameSector(cm(6), cm(12), degrees(-10), degrees(20)));
	//	return;

	red(cf) *= 2;
	green(cf) /= 2;
	//
	blue(cf[0]) *= 2;
	cf[0].red() /= 2;


	cf.red() += 3.141926f;
	cf.green() += 2.718281828f;

	//	RealFunction2D_F32::ref_invariable part;
	//	RealFunction2D_F32 part;
	RealFunctionF32 part;
	cf.GetRed(part);

	part.fill(0);

	//	DisplayMathFunction2D(cf, "Color function", ScanFrameRectangle(cm(5), cm(5)));
	DisplayMathFunction(cf, 0, 1, "Color function");
	// 	DisplayMathFunction(part, "Complex function real", ScanFrameRectangle(cm(5), cm(5)));
	// 	DisplayMathFunction(imag(cf), "Complex function imag", ScanFrameRectangle(cm(5), cm(5)));
	// 	DisplayMathFunction2D(rf, "Real function", ScanFrameRectangle(cm(5), cm(5)));
}

//--------------------------------------------------------------

void TestRealGraphs()
{
	//тестирование графиков
	int	N1 = 512;
	RealFunctionF32	xx(N1);
	RealFunctionF32	yy(N1);
	VectorFunction2_F32 xy(N1);

	for (int i = 0; i < N1; i++)
	{
		xx[i] = i;
		yy[i] = double(square(i)) / N1;
	}

	DisplayMathFunction(yy, 0, 1, "Y(i)");

	GraphSet	gs(L"Набор графиков", L"Ордината", L"Абсцисса");


	gs.AddGraphParametric(yy, xx, L"Прямая функция");
	gs.Display();

	gs.SetGraphStyle(dashed_black_lines, 1.5);

	gs.AddGraphParametric(xx, yy, "Обратная функция");

	gs.Display();
	range2_F64 current_scale;
	gs.GetScale(current_scale);
	//	range2_F64 gs2 = current_scale/2;
	//	range2_F64 gs2 = current_scale + point2_F64(100,100);
	//	range2_F64 gs2 = current_scale/2 + point2_F64(100,100);
	// 	gs.SetScale(gs2);

	range2_F64	f1, f2, f3;
	f1 = f2 + f2 - point2_F64(1, 1);
	f1 = f2 + f2 - f3;

	//	range2_F64	some = current_scale*2 - point2_F64(100,100);

	gs.SetScale(current_scale * 2 - point2_F64(100, 100));
	//	gs.SetScale(current_scale - point2_F64(100,100));


	gs.Display();

	for (int i = 0; i < N1; i++)
	{
		double	t = 8.*pi()*double(i) / N1;
		double	r = t + 1;
		xx[i] = r*sin(t);
		yy[i] = r*cos(t);
	}
	gs.ChangeGraphParametric(0, xx, yy, "spiral 1");
	gs.ChangeGraphParametric(1, yy, xx, "spiral 2");

	while(auto fn = GetButtonDecision(L"Тест отображения",
		{
			MakeButton(L"Скрыть график со спиралью", make_fn([&](){gs.Hide();})),
			MakeButton(L"Показать график со спиралью", make_fn([&](){gs.Display(false);})),
			MakeButton(L"Выход", make_fn((void(*)())nullptr))
		}
		  ))
	{
		if(fn) fn();
	}

	if(YesOrNo(L"Закрыть график со спиралью?", true))
	{
		gs.Close();
	}
}

void TestComplexGraphs()
{
	size_t data_size = 2048;
	ComplexFunctionF32 data(data_size);
	for (size_t i = 0; i < data_size; ++i)
	{
		data[i] = polar(0.5 * data_size + i, double(i) * i * i / (data_size * data_size) * 2 * pi());
	}
	DisplayMathFunction(data, 0, 1, L"Комплексная функция");
}

void	TestGraphs()
{
	for (;;)
	{
		auto choice = GetButtonDecision(L"Выберите вид графиков",
		{
			MakeButton(L"Действительные функции", TestRealGraphs),
			MakeButton(L"Комплексные функции", TestComplexGraphs),
			MakeButton(L"Отмена", (void(*)())nullptr)
		});
		if (!choice)
			break;
		try
		{
			choice();
		}
		catch (canceled_operation &)
		{
		}
		catch (quit_application &)
		{
			throw;
		}
		catch (...)
		{
			Error(GetExceptionString());
		}
	}
}

//--------------------------------------------------------------

void TestTextDisplayer()
{
	TextDisplayer txt(L"Text displayer");
	bool editable = false;
	bool fixed_width = false;
	double font_size = 10;
	bool persistent = false;
	bool stay_on_top = false;
	for (;;)
	{
		auto action = GetButtonDecision(L"Test displayer actions",
				{
					MakeButton(L"Display", make_fn([&txt]() { txt.Display(); })),
					MakeButton(L"Set text", make_fn([&txt]()
						{
							txt.SetText(
									ssprintf(L"Text sample: time = %lf.",
											EnsureType<double>(current_time().sec())));
						})),
					MakeButton(L"Get text", make_fn([&txt]()
						{
							wstring str = txt.WGetText();
							ShowString(L"Text displayer text", str);
						})),
					MakeButton(L"Toggle editable", make_fn([&txt, &editable]()
						{
							editable = !editable;
							txt.SetEditable(editable);
						})),
					MakeButton(L"Toggle fixed width", make_fn([&txt, &fixed_width]()
						{
							fixed_width = !fixed_width;
							txt.SetFixedWidth(fixed_width);
						})),
					MakeButton(L"Set font size", make_fn([&txt, &font_size]()
						{
							font_size = GetFloating(L"Font size", font_size, 1e-3, 1e3);
							txt.SetFontSize(font_size);
						})),
					MakeButton(L"Toggle persistent", make_fn([&txt, &persistent]()
						{
							persistent = !persistent;
							txt.SetPersistent(persistent);
						})),
					MakeButton(L"Toggle stay on top", make_fn([&txt, &stay_on_top]()
						{
							stay_on_top = !stay_on_top;
							txt.SetStayOnTop(stay_on_top);
						})),
					MakeButton(L"OK", function<void ()>())
				});
		if(!action)
		{
			if(YesOrNo(L"Закрыть окно с текстом?", true)) txt.Close();
			break;
		}
		SafeExecute(action);
	}
}

//--------------------------------------------------------------

void TestDisplayText()
{
	for (;;)
	{
		auto action = GetButtonDecision(L"Display text actions",
				{
					MakeButton(L"TextDisplayer", make_fn(TestTextDisplayer)),
					MakeButton(L"ShowText", make_fn([]()
						{
							ShowText(L"Test", L"Text\nMultiline");
						})),
					MakeButton(L"OK", function<void ()>())
				});
		if (!action)
			break;
		SafeExecute(action);
	}
}

//--------------------------------------------------------------

void	TestFileForms()
{
	XRAD_USING;
	// тестирование форм выбора/сохранения файла
	try
	{
		string	file_name = GetFileNameRead("Выбор файла");
		ShowString("Результат GetFileNameRead", file_name);
	}
	catch (canceled_operation &)
	{
		ShowString("Результат GetFileNameRead", "отмена");
	}

	try
	{
		bool use_default_name = YesOrNo(L"Использовать имя файла по умолчанию?");
		string file_name = use_default_name ?
			GetFileNameWrite("Сохранение файла", "name_by_default") :
			GetFileNameWrite("Сохранение файла");
		ShowString("Результат GetFileNameWrite", file_name);
	}
	catch (canceled_operation &)
	{
		ShowString("Результат GetFileNameWrite", "отмена");
	}

	try
	{
		string folder_name = GetFolderNameRead("Выбор каталога на чтение");
		ShowString("Результат GetFolderNameRead", folder_name);
	}
	catch (canceled_operation &)
	{
		ShowString("Результат GetFolderNameRead", "отмена");
	}

	try
	{
		string folder_name = GetFolderNameWrite("Выбор каталога на запись");
		ShowString("Результат GetFolderNameWrite", folder_name);
	}
	catch (canceled_operation &)
	{
		ShowString("Результат GetFolderNameWrite", "отмена");
	}
}

//--------------------------------------------------------------

void TestErrorDialog()
{
	Error(L"Сообщение char");
	Error(L"Сообщение wchar_t");
}

//--------------------------------------------------------------

void TestDecide()
{
	size_t base_no = Decide("Base implementation test",
	{
		"Num 0",
		"Num 1",
		"Num 2"
	});
	ShowUnsigned("Base implementation result", base_no);
	base_no = Decide(L"Базовая реализация с Юникодом (测试)",
	{
		L"数 0",
		L"数 1",
		L"数 2"
	});
	ShowUnsigned("Результат базовой реализации с Юникодом", base_no);
	base_no = Decide(L"Базовая реализация с кнопкой по умолчанию (Num 1)",
	{
		L"Num 0",
		L"Num 1",
		L"Num 2"
	}, 1);
	ShowUnsigned("Результат базовой реализации с кнопкой по умолчанию", base_no);
	string button_result = Decide(L"Реализация с классом Button",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")),
		MakeButton(L"Num 2", string("num_2"))
	});
	ShowString("Результат реализации с классом Button", button_result);
	button_result = Decide(L"Реализация с классом Button и значением по умолчанию #1",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")),
		MakeButton(L"Num 2", string("num_2"))
	}, string("num_1"));
	ShowString("Результат реализации с классом Button и значением по умолчанию #1", button_result);
	button_result = Decide(L"Реализация с классом Button и значением по умолчанию #2",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")),
		MakeButton(L"Num 2", string("num_2"))
	}, MakeGUIValue(string("num_1"), saved_default_value));
	ShowString("Результат реализации с классом Button и значением по умолчанию #2", button_result);
	button_result = Decide(L"Реализация с классом Button и значением по умолчанию #3",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")).SetDefault(),
		MakeButton(L"Num 2", string("num_2"))
	});
	ShowString("Результат реализации с классом Button и значением по умолчанию #3", button_result);
	auto button_f_result = Decide(L"Реализация с классом Button с функтором и значением по умолчанию",
	{
		MakeButton(L"Num 0", make_fn([]() { return "num_0"; })).SetDefault(false),
		MakeButton(L"Num 1", make_fn([]() { return "num_1"; })).SetDefault(true),
		MakeButton(L"Num 2", make_fn([]() { return "num_2"; }))
	});
	ShowString("Результат реализации с классом Button с функтором и значением по умолчанию", button_f_result());
}

//--------------------------------------------------------------

void TestGetButtonDecision()
{
	size_t base_no = GetButtonDecision("Base implementation test",
	{
		"Num 0",
		"Num 1",
		"Num 2"
	});
	ShowUnsigned("Base implementation result", base_no);
	base_no = GetButtonDecision(L"Базовая реализация с Юникодом (测试)",
	{
		L"数 0",
		L"数 1",
		L"数 2"
	});
	ShowUnsigned("Результат базовой реализации с Юникодом", base_no);
	base_no = GetButtonDecision(L"Базовая реализация с кнопкой по умолчанию (Num 1)",
	{
		L"Num 0",
		L"Num 1",
		L"Num 2"
	}, 1);
	ShowUnsigned("Результат базовой реализации с кнопкой по умолчанию", base_no);
	string button_result = GetButtonDecision(L"Реализация с классом Button",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")),
		MakeButton(L"Num 2", string("num_2"))
	});
	ShowString("Результат реализации с классом Button", button_result);
	button_result = GetButtonDecision(L"Реализация с классом Button и значением по умолчанию #1",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")),
		MakeButton(L"Num 2", string("num_2"))
	}, string("num_1"));
	ShowString("Результат реализации с классом Button и значением по умолчанию #1", button_result);
	button_result = GetButtonDecision(L"Реализация с классом Button и значением по умолчанию #2",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")),
		MakeButton(L"Num 2", string("num_2"))
	}, MakeGUIValue(string("num_1"), saved_default_value));
	ShowString("Результат реализации с классом Button и значением по умолчанию #2", button_result);
	button_result = GetButtonDecision(L"Реализация с классом Button и значением по умолчанию #3",
	{
		MakeButton(L"Num 0", string("num_0")),
		MakeButton(L"Num 1", string("num_1")).SetDefault(),
		MakeButton(L"Num 2", string("num_2"))
	});
	ShowString("Результат реализации с классом Button и значением по умолчанию #3", button_result);
	auto button_f_result = GetButtonDecision(L"Реализация с классом Button с функтором и значением по умолчанию",
	{
		MakeButton(L"Num 0", make_fn([]() { return "num_0"; })).SetDefault(false),
		MakeButton(L"Num 1", make_fn([]() { return "num_1"; })).SetDefault(true),
		MakeButton(L"Num 2", make_fn([]() { return "num_2"; }))
	});
	ShowString("Результат реализации с классом Button с функтором и значением по умолчанию", button_f_result());
}

//--------------------------------------------------------------

class NonCopyableFunctor
{
	public:
		int operator() (int x) { return x; }

		NonCopyableFunctor() {}
		NonCopyableFunctor(const NonCopyableFunctor &) = delete;
		NonCopyableFunctor &operator=(const NonCopyableFunctor &) = delete;
		NonCopyableFunctor(NonCopyableFunctor &&) = default;
		NonCopyableFunctor &operator=(NonCopyableFunctor &&) = default;
};

void TestFunctorButtonDecision()
{
	auto f00s = make_fn((double (*)(double))std::sin);
	auto v00s = f00s(0.5);
	(void)v00s;
	auto f01s = make_fn(strcmp);
	//auto f02s = make_fn(printf); // error: variadic functions are not supported
	auto f03s = make_fn([](float) { return 0; });
	auto v03s = f03s(0.4f);
	(void)v03s;
	auto f04s = make_fn(Functors::pow_value());
	//auto f05s = make_fn(Functors::absolute_value()); // error: ambiguous operator()
	NonCopyableFunctor func06s;
	auto f06s = make_fn(ref(func06s));
	//auto f07s = make_fn(func06s); // Наличия move-операций в классе недостаточно.

	auto f08s = make_fn(f00s); // копируем функтор
	auto f08s2 = make_fn(std::move(f08s)); // оптимизированная операция: перемещение f08s в f08s2

	auto answer_s = GetButtonDecision("Functions S1",
			{
				MakeButton(L"Sin", make_fn((double (*)(double))std::sin)),
				MakeButton(L"Cos", make_fn([](double x) { return std::cos(x); })),
				MakeButton(L"None", function<double(double)>())
			});
	double ra_s = answer_s? answer_s(pi()/6): -123;
	ShowString("Functor", ssprintf(
			"f(PI/6)=%lf",
			ra_s));
	int f2_lambda_capture = 1;
	auto f2 = function<double(int, const int&, int&)>(
			[&f2_lambda_capture](int x1, const int &x2, int &x3)
			{
				int result = f2_lambda_capture + x1 + x2 + x3;
				f2_lambda_capture += 10;
				x3 += 100;
				return 0.1 * result;
			});
	auto answer2_s = GetButtonDecision("Functions S2",
			{
				MakeButton(L"f2", make_fn(ref(f2))),
				MakeButton(L"f3", make_fn([&f2_lambda_capture](int x1, const int &x2, int &x3)
					{
						int result = f2_lambda_capture + x1 + x2 + x3 + 10000;
						f2_lambda_capture += 10;
						x3 += 100;
						return 0.1 * result;
					})),
				MakeButton(L"None", function<double (int, const int&, int&)>())
			});
	int f2_i = 3;
	string ra2_s = "(none)";
	if (answer2_s)
		ra2_s = ssprintf("%lf", EnsureType<double>(answer2_s(1, 2, f2_i)));
	ShowString("Functor", ssprintf(
			"f2(...)=%s",
			EnsureType<const char*>(ra2_s.c_str())));

	int action_3_v = 0;
	auto action_3_1 = [&action_3_v](int x) { action_3_v = 100 + x; };
	auto action_3_2 = [&action_3_v](int x) { action_3_v = 200 + x; return x; };
	using params_3_t = void(int);
	auto answer_3_s = GetButtonDecision("Functions S3",
			{
				MakeButton(L"Action 1", function<params_3_t>(action_3_1)),
				MakeButton(L"Action 2", function<params_3_t>(action_3_2)),
				MakeButton(L"None", function<params_3_t>())
			});
	if (answer_3_s)
		answer_3_s(1);
	ShowString("Functor S3", ssprintf(
			"action_3_v=%i",
			action_3_v));

//	Тест устаревшей функции: при компиляции должно возникнуть предупреждение.
//	20190211 Уже убедились многократно, что на VS2015 предупреждение возникает
//	как надо. Чтобы не отвлекало, отключаем для этого компилятора
#if _MSC_VER!=1900
	vector<wstring> headers = { L"First", L"Cancel" };
	vector<int> values = { 1, -1 };
	int value = GetButtonDecision(L"Obsolete", headers, values); // Компилятор должен выдать предупреждение C4996.
#endif
}

//--------------------------------------------------------------

void TestGetValue()
{
	//	wstring str1 = GetString(L"Введите строку", L"検査 Testing Проверка Unicode");
	wstring str0 = GetString(L"Введите строку 0");
	wstring str1 = GetString(L"Введите строку 1", GUIValue<wstring>(str0));
	ShowString(L"Результат Get_String", str1);
	auto	valuel = GetSigned("Введите целое число (s)", MakeGUIValue<ptrdiff_t>(3, saved_default_value), -10, -50, out_of_range_allowed);

	//TODO ниже тест на прохождение unicode через string, к вопросу о pragma utf8
	// однако пока этим не занимаемся, предупреждение отменяем
#pragma warning (disable:4566)
	ShowSigned("Вы ввели сейчас (Résultat d’une même chose)", valuel);
	auto	valueu = GetUnsigned("Введите целое число (u)", MakeGUIValue<size_t>(3, saved_default_value), 0, 100, out_of_range_allowed);
	ShowUnsigned("Вы ввели сейчас (u)", valueu);
	double	valued = GetFloating("Введите число с плавающей запятой", MakeGUIValue(3., saved_default_value), -10, 5e100);
	ShowFloating("Вы ввели сейчас", valued);
}

//--------------------------------------------------------------

void TestGetCheckbox()
{
	bool value1 = false;
	bool value2 = true;

	bool result = GetCheckboxDecision("Initially:", //2,
	{ "value1?", "value2?" },
	{ &value1,&value2 });
	string s = ssprintf("value1=%i, value2=%i, result=%i", (int)value1, (int)value2, (int)result);
	ShowString("Результат Get_Checkbox_Decision", s.c_str());
}

//--------------------------------------------------------------

void TestProgressWithDynamicDialog()
{
	ProgressBar pb(GUIProgressProxy());
	pb.start(L"Progress with DynamicDialog", 10);
	for (int i = 0; i < 5; ++i)
		++pb;
	TestDynamicDialog();
	for (int i = 5; i < 10; ++i)
		++pb;
}

//--------------------------------------------------------------

void TestProgressWithPause()
{
	ProgressBar pb(GUIProgressProxy());
	pb.start(L"Progress with Pause", 10);
	for (int i = 0; i < 5; ++i)
		++pb;
	Pause();
	for (int i = 5; i < 10; ++i)
		++pb;
}

//--------------------------------------------------------------

void	TestDelays()
{
	//--------------------------------------------------------------------------------------------------
	// тестирование Pause() и Delay(3)
	//--------------------------------------------------------------------------------------------------
	double k = 0;
	for (int i = 1; i <= 10; i++)
	{
		k = k + i * 10;

		if (i == 5)
		{
			Pause();
		}

		if (i == 6)
		{
			cerr << "\rDelay 3 sec\n";
			fflush(stderr);
			Delay(sec(3));
		}
		if (i == 7)
		{
			Pause();
		}
	}
}

//--------------------------------------------------------------

} // namespace



//--------------------------------------------------------------

void	TestGUIFunctions()
{
	for (;;)
	{
		auto experiment = GetButtonDecision(L"Choose experiment",
		{
			MakeButton(L"Error dialog", make_fn(TestErrorDialog)),
			MakeButton(L"Decide", make_fn(TestDecide)),
			MakeButton(L"GetButtonDecision", make_fn(TestGetButtonDecision)),
			MakeButton(L"Functor button decision", make_fn(TestFunctorButtonDecision)),
			MakeButton(L"Get value", make_fn(TestGetValue)),
			MakeButton(L"Get checkbox", make_fn(TestGetCheckbox)),
			MakeButton(L"Display text", make_fn(TestDisplayText)),
			MakeButton(L"File forms", make_fn(TestFileForms)),
			MakeButton(L"Dynamic dialog", make_fn(TestDynamicDialog)),
			MakeButton(L"Progress with dynamic dialog", make_fn(TestProgressWithDynamicDialog)),
			MakeButton(L"Progress with pause", make_fn(TestProgressWithPause)),
			MakeButton(L"Progress bar", make_fn(TestProgress)),
			MakeButton(L"Graphs", make_fn(TestGraphs)),
			MakeButton(L"Rasters", make_fn(TestRasters)),
			MakeButton(L"XRAD objects", make_fn(TestXRADObjects)),
			MakeButton(L"Delays", make_fn(TestDelays)),
			MakeButton(L"Others", make_fn([]()
				{
					ShowUnsigned("sizeof(size_t)", sizeof(size_t));
				})),
			MakeButton(L"Cancel", function<void()>())
		});
		if (!experiment)
			break;
		SafeExecute(experiment);
	}
}

//--------------------------------------------------------------

XRAD_END
