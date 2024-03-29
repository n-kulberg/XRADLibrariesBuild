﻿// file TestProgress.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestProgress.h"

XRAD_BEGIN

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

// void	RunSubprogress_2Sub(int count, physical_time dt, ProgressProxy pp)
// {
// 	StepProgressBar progress(pp);
// 	progress.start("Тестирование индикатора прогресса", count);
// 	for (int i = 0; i < count; i++)
// 	{
// 		Delay(dt*0.3);
// 		++progress;
// 		if (ControlPressed()) Pause();
// 	}
// 	progress.end();
//
//
// 	const int count3 = 3;
// 	progress.start("Тестирование индикатора прогресса", count / count3);
// 	for (int i = 0; i < count / count3; i++)
// 	{
// 		StepProgressBar sub_progress(progress.subprogress(i, i+1));
// 		sub_progress.start(L"Подпрогресс", count3);
// 		for (int j = 0; j < count3; ++j)
// 		{
// 			Delay(dt*0.7);
// 			++sub_progress;
// 		}
// 		sub_progress.end();
// 		++progress;
// 		if (ControlPressed()) Pause();
// 	}
// }

//--------------------------------------------------------------

void TestProgressSimple()
{
	int count = (int)GetUnsigned(L"Введите число шагов индикатора", MakeGUIValue<int>(50, saved_default_value), 0, max_int());
	physical_time desired_time = sec(10);
	physical_time dt = desired_time / (count ? count : 1);

	GUIProgressBar progress;
	progress.start("Тестирование индикатора прогресса", count);

	for (int i = 0; i < count; i++)
	{
		Delay(dt);
		++progress;
	}
	ShowFloating("Process time", physical_time(progress.end()).sec());
}

//--------------------------------------------------------------

void TestRandomProgress()
{
	double max_value = GetFloating(L"Введите максимум индикатора",
			MakeGUIValue<double>(1.5, saved_default_value), 0, max_double());
	size_t count = 50;
	physical_time desired_time = sec(10);
	physical_time dt = desired_time / (count ? count : 1);

	GUIRandomProgressBar progress;
	progress.start("Тестирование индикатора прогресса", max_value);

	for (size_t i = 0; i < count; i++)
	{
		Delay(dt);
		progress.set_position(double(i)/count * max_value);
	}
}

//--------------------------------------------------------------
//
// void TestProgressWithSubOldStyle()
// {
// 	// Тест устаревшего способа создания подпрогрессов
// 	int count1 = 5;
// 	int count2 = 10;
// 	physical_time desired_time = sec(10);
// 	physical_time dt = desired_time / (count1*count2);
//
// 	GUIStepProgressBar progress;
// 	progress.start("Тестирование индикатора прогресса", count1);
//
// 	for (int i = 0; i < count1; i++)
// 	{
// 		RunSubprogress_2Sub(count2, dt, progress.subprogress(i, i+1));
// 		//		printf("\rTesting progress, step %d", i);
// 		//		Pause();
// 		++progress;
// 		//		Pause();
// 	}
// 	ShowFloating("Process time", progress.end().sec());
// }

//--------------------------------------------------------------

// void TestProgressWithSubOldStyleBuggy()
// {
// 	// Тест устаревшего способа создания подпрогрессов, с преднамеренной ошибкой
// 	int count1 = 5;
// 	int count2 = 10;
// 	physical_time desired_time = sec(10);
// 	physical_time dt = desired_time / (count1*count2);
//
// 	StepProgressBar progress(GUIProgressProxy());
// 	progress.start("Тестирование индикатора прогресса", count1); // не указано 2 подпрогресса
//
// 	for (int i = 0; i < count1; i++)
// 	{
// 		RunSubprogress_2Sub(count2, dt, progress.subprogress(i,i+1));
// 		++progress;
// 	}
// }

//--------------------------------------------------------------

void TestProgressSubMethod()
{
	// В качестве верхнего предела прогрессов использованы разные числа, чтобы проверить
	// корректность хода подпрогрессов.
	int count1 = 3;
	int count2 = 4;
	int count3 = 5;
	physical_time desired_time = sec(10);
	physical_time dt = desired_time / (count1*count2*count3);

	ProgressBar progress(GUIProgressProxy());
	progress.start("Тестирование индикатора прогресса", count1);

	for (int i = 0; i < count1; i++)
	{
		{
			auto sub_pp = progress.substep(0, 0.3);
			ProgressBar sub_progress(sub_pp);
			sub_progress.start("Подпрогресс 1", count2*count3);
			for (int j = 0; j < count2*count3; ++j)
			{
				Delay(dt*0.3);
				++sub_progress;
			}
			sub_progress.end();
		}
		{
			auto sub_pp = progress.substep(0.3, 1);
			RandomProgressBar sub_progress(sub_pp);
			sub_progress.start("Подпрогресс 2", count2);
			for (int j = 0; j < count2; ++j)
			{
				auto sub_pp_2 = sub_progress.subprogress(j, j+1);
				RandomProgressBar sub_progress_2(sub_pp_2);
				sub_progress_2.start(L"Подпрогресс 2.1", 1.5);
				for (int k = 0; k < count3; ++k)
				{
					Delay(dt*0.7);
					sub_progress_2.set_position(1.5*double(k+1)/count3);
				}
				sub_progress_2.end();
				sub_progress.set_position(j+1);
			}
			sub_progress.end();
		}
		++progress;
	}
}

//--------------------------------------------------------------

void TestAutoProgressScheduler()
{
//	Операция, занимающая N*d миллисекунд
	auto	lambda = [](int N, int d, ProgressProxy pp)
	{
		ProgressBar	p(pp);
		p.start("Processing", N);
		for (int i = 0; i < N; ++i)
		{
			Delay(msec(d));
			++p;
		}
		//		Pause();
	};
	RandomProgressBar	p(GUIProgressProxy());
	int	slow_delay(500), fast_delay(100);

	bool	proportional_init = false;
	//	Начальные значения имеют смысл только при удаленной ветке
	//	HKEY_CURRENT_USER\Software\XRAD\XRADImmediateTest.exe\AutoProgressScheduler\TestSubprogress
	//	При proportional_init = true прогресс двигается равномерно сразу (время выполнения 18 с, прогноз сразу верный)
	//	При proportional_init = false прогресс вначале движется быстро, прогноз 6 с, потом замедляется и вырастает до 18
	//	После нескольких запусков ситуация выравнивается.
	//	Быстрота автоподстройки определяется параметром AutoProgressScheduler::auto_tune_speed.
	//	Если эта величина слишком близка к 1, прогресс станет подвержен случайным изменениям от вызова к вызову и вновь станет нестабильным.
	AutoProgressIndicatorScheduler scheduler(L"TestSubprogress",
	{
		ProgressOperation(L"fast", proportional_init ? fast_delay : 50, true),
		ProgressOperation(L"slow", proportional_init ? slow_delay : 50, true),
	});
	scheduler.SetAutoTuneSpeed(0.75);
	p.start("Fast, slow", scheduler.n_steps());
	scheduler.run_and_register_cost(0, p, [&lambda, &fast_delay](auto subpp) {lambda(30, fast_delay, subpp); });
	scheduler.run_and_register_cost(1, p, [&lambda, &slow_delay](auto subpp) {lambda(30, slow_delay, subpp); });
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestProgress()
{
	for (;;)
	{
		auto experiment = GetButtonDecision(L"Choose progress test",
		{
			MakeButton(L"Progress bar simple", make_fn(TestProgressSimple)),
			MakeButton(L"Progress bar random", make_fn(TestRandomProgress)),
		//	MakeButton(L"Progress bar with sub (old style)", make_fn(TestProgressWithSubOldStyle)),
		//	MakeButton(L"Progress bar with sub (old style, buggy)", make_fn(TestProgressWithSubOldStyleBuggy)),
			MakeButton(L"Progress bar with sub method", make_fn(TestProgressSubMethod)),
			MakeButton(L"Auto progress scheduler", make_fn(TestAutoProgressScheduler)),
			MakeButton(L"Cancel", function<void()>())
		});
		if (!experiment)
			break;
		SafeExecute(experiment);
	}
}

//--------------------------------------------------------------

XRAD_END

//--------------------------------------------------------------
