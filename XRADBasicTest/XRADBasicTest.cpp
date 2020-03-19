//--------------------------------------------------------------
#include "pre.h"

#include <XRADBasic/Core.h>
#include <XRADBasic/Sources/Utils/ConsoleProgress.h>
#include <XRADBasic/Sources/Utils/ParallelProcessor.h>
#include <XRADBasic/Sources/Utils/TimeProfiler.h>
#include <XRADBasic/MathFunctionTypes.h>
#include <XRADBasic/FFT1D.h>
#include <thread>
#include <cstring>

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

void SetLang(const string &lang_id)
{
	printf("Set language: \"%s\".\n", EnsureType<const char*>(lang_id.c_str()));
	SetLanguageId(lang_id);
}

//--------------------------------------------------------------

void TestLang()
{
	printf(convert_to_string(tr_ru_en(
					L"Текущий язык: \"%s\"\n",
					L"Current language: \"%s\".\n")).c_str(),
			EnsureType<const char*>(GetLanguageId().c_str()));
}

//--------------------------------------------------------------

template <class C>
unsigned int c_to_uint(C c)
{
	return c;
}

template<>
unsigned int c_to_uint(char c)
{
	return (unsigned char)c;
}

template <class S>
void print_seq(const char *title, const char *type_name, const S &seq)
{
	printf("%s<%s>:", title, type_name);
	for (auto v: seq)
	{
		printf(" 0x%X", c_to_uint<typename S::value_type>(v));
	}
	printf("\n");
	fflush(stdout);
}

template <class C1, class C2>
void TestCodeCvtHelper(const vector<int> &test_str)
{
	try
	{
		printf("Testing codecvt<%s, %s>...\n", typeid(C1).name(), typeid(C2).name());
		fflush(stdout);
		using codecvt_t = codecvt<C1, C2, mbstate_t>;
		auto cvt_loc = locale("");
		const auto& cvt = use_facet<codecvt_t>(cvt_loc);
		printf("Codecvt created.\n");
		fflush(stdout);
		{
			mbstate_t state = {};
			basic_string<C2> s2(test_str.begin(), test_str.end());
			print_seq("In", typeid(C2).name(), s2);
			vector<C1> os1(test_str.size() * 2, 0);
			const C2 *s2_it = nullptr;
			C1 *os1_it = nullptr;
			cvt.in(state, s2.c_str(), s2.c_str() + s2.length(), s2_it,
					os1.data(), os1.data() + os1.size(), os1_it);
			print_seq("Out", typeid(C1).name(), os1);
		}
		{
			mbstate_t state = {};
			basic_string<C1> s1(test_str.begin(), test_str.end());
			print_seq("In", typeid(C1).name(), s1);
			vector<C2> os2(test_str.size() * 2, 0);
			const C1 *s1_it = nullptr;
			C2 *os2_it = nullptr;
			cvt.out(state, s1.c_str(), s1.c_str() + s1.length(), s1_it,
					os2.data(), os2.data() + os2.size(), os2_it);
			print_seq("Out", typeid(C2).name(), os2);
		}
	}
	catch (exception &e)
	{
		printf("Exception: %s\n", e.what());
		fflush(stdout);
	}
	printf("Testing codecvt<%s, %s> end.\n", typeid(C1).name(), typeid(C2).name());
	fflush(stdout);
}

void TestCodeCvt()
{
	vector<int> test_str = {
		0x21, // '!'
		0x30, // '0'
		0x31, // '1'
		0x41, // 'A'
		0x42, // 'B'
		0x411, // u"Б"
		0x1F34C, // U"\U0001F34C"
		0xD0, 0x91 // u8"Б"
		};
	TestCodeCvtHelper<wchar_t, char>(test_str);
	TestCodeCvtHelper<wchar_t, char16_t>(test_str);
#if !(defined(_MSC_VER) && (_MSC_VER < 1924))
	// Этот код в MSVC2015..2017 не линкуется из-за ошибки в runtime-библиотеках.
	TestCodeCvtHelper<char16_t, char>(test_str);
	TestCodeCvtHelper<char32_t, char>(test_str);
#endif
}

//--------------------------------------------------------------

const uchar_t *to_uchar(const char *s)
{
	return reinterpret_cast<const uchar_t*>(s);
}

void TestSSPrintf()
{
	printf("Testing ssprintf()\n");
	fflush(stdout);
	string s;
	s = ssprintf_core("Simple string");
	printf("[core.1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf_core("Int (123456): %i", 123456789);
	printf("[core.i.1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Simple string");
	printf("[1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Int (123456): %i", 123456789);
	printf("[i.1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Int x 2 (123456789, 987654321): %i, %i", 123456789, 987654321);
	printf("[i.2] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Int x 9 (1, 2, 3, 4, 5, 6, 7, 8, 9): %i, %i, %i, %i, %i, %i, %i, %i, %i",
			1, 2, 3, 4, 5,  6, 7, 8, 9);
	printf("[i.3] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Double (123456.789123): %lf", 123456.789123);
	printf("[d.1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Double x 2 (123456.789123, 3.141592654): %lf, %lf", 123456.789123, 3.141592654);
	printf("[d.2] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Double x 9 (1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9): "
			"%lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf",
			1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9);
	printf("[d.3] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Int, Double x 9 (1, 1.1, 2, 2.2, 3, 3.3, 4, 4.4, 5, 5.5, 6, 6.6, 7, 7.7, 8, 8.8, 9, 9.9): "
			"%i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf",
			1, 1.1, 2, 2.2, 3, 3.3, 4, 4.4, 5, 5.5, 6, 6.6, 7, 7.7, 8, 8.8, 9, 9.9);
	printf("[id.1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Const char* (test): \"%s\"", "test");
	printf("[s.1] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);
	s = ssprintf("Const char* x 2 (first, second): \"%s\", \"%s\"", "first", "second");
	printf("[s.2] \"%s\"\n", EnsureType<const char*>(s.c_str()));
	fflush(stdout);

	wstring ws;
	ws = ssprintf(L"Simple string");
	printf("[w:1] \"%s\"\n", EnsureType<const char*>(convert_to_string(ws).c_str()));
	fflush(stdout);
	ws = ssprintf(L"Int (123456): %i", 123456789);
	printf("[w:i.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(ws).c_str()));
	fflush(stdout);
	ws = ssprintf(L"Double (123456.789123): %lf", 123456.789123);
	printf("[w:d.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(ws).c_str()));
	fflush(stdout);
	ws = ssprintf(L"Int, Double x 9 (1, 1.1, 2, 2.2, 3, 3.3, 4, 4.4, 5, 5.5, 6, 6.6, 7, 7.7, 8, 8.8, 9, 9.9): "
			"%i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf",
			1, 1.1, 2, 2.2, 3, 3.3, 4, 4.4, 5, 5.5, 6, 6.6, 7, 7.7, 8, 8.8, 9, 9.9);
	printf("[w:id.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(ws).c_str()));
	fflush(stdout);
	ws = ssprintf(L"Const wchar_t* (test): \"%ls\"", L"test");
	printf("[w:s.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(ws).c_str()));
	fflush(stdout);
	ws = ssprintf(L"Const wchar_t* x 2 (first, second): \"%ls\", \"%ls\"", L"first", L"second");
	printf("[w:s.2] \"%s\"\n", EnsureType<const char*>(convert_to_string(ws).c_str()));
	fflush(stdout);

	ustring us;
	us = to_uchar("UString");
	printf("[u:0] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
	us = ssprintf(to_uchar("Simple string"));
	printf("[u:1] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
	us = ssprintf(to_uchar("Int (123456): %i"), 123456789);
	printf("[u:i.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
	us = ssprintf(to_uchar("Double (123456.789123): %lf"), 123456.789123);
	printf("[u:d.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
	us = ssprintf(to_uchar("Int, Double x 9 (1, 1.1, 2, 2.2, 3, 3.3, 4, 4.4, 5, 5.5, 6, 6.6, 7, 7.7, 8, 8.8, 9, 9.9): "
			"%i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf, %i, %lf"),
			1, 1.1, 2, 2.2, 3, 3.3, 4, 4.4, 5, 5.5, 6, 6.6, 7, 7.7, 8, 8.8, 9, 9.9);
	printf("[u:id.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
	us = ssprintf(to_uchar("Const wchar_t* (test): \"%s\""), to_uchar("test"));
	printf("[u:s.1] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
	us = ssprintf(to_uchar("Const wchar_t* x 2 (first, second): \"%s\", \"%s\""), to_uchar("first"), to_uchar("second"));
	printf("[u:s.2] \"%s\"\n", EnsureType<const char*>(convert_to_string(us).c_str()));
	fflush(stdout);
}

//--------------------------------------------------------------

void WasteTime(physical_time dt)
{
	auto t = GetPerformanceCounter();
	while (GetPerformanceCounter() - t < dt)
		;
}

//--------------------------------------------------------------

void TestProgress()
{
	// В качестве верхнего предела прогрессов использованы разные числа, чтобы проверить
	// корректность хода подпрогрессов.
	int count1 = 3;
	int count2 = 4;
	int count3 = 5;
	physical_time desired_time = sec(10);
	physical_time dt = desired_time / (count1*count2*count3);

	ProgressBar progress(ConsoleProgressProxy());
	progress.start(tr_ru_en(
			L"Тестирование индикатора прогресса",
			L"Testing progress indicator"),
			count1);

	for (int i = 0; i < count1; i++)
	{
		{
			auto sub_pp = progress.substep(0, 0.3);
			ProgressBar sub_progress(sub_pp);
			sub_progress.start(tr_ru_en(
					L"Подпрогресс 1",
					L"Subprogres 1"),
					count2*count3);
			for (int j = 0; j < count2*count3; ++j)
			{
				WasteTime(dt*0.3);
				++sub_progress;
			}
			sub_progress.end();
		}
		{
			auto sub_pp = progress.substep(0.3, 1);
			RandomProgressBar sub_progress(sub_pp);
			sub_progress.start(tr_ru_en(
					L"Подпрогресс 2",
					L"Subprogress 2"),
					count2);
			for (int j = 0; j < count2; ++j)
			{
				auto sub_pp_2 = sub_progress.subprogress(j, j+1);
				RandomProgressBar sub_progress_2(sub_pp_2);
				sub_progress_2.start(tr_ru_en(
						L"Подпрогресс 2.1",
						L"Subprogress 2.1"),
						1.5);
				for (int k = 0; k < count3; ++k)
				{
					WasteTime(dt*0.7);
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

enum class ParProcTestKind { Ordinary, RuntimeError, CancelOperation, QuitApplication };
enum class ParProcMode { ParallelProcessor, OMP };

void TestParProc(ParProcTestKind test_kind, ParProcMode mode)
{
	printf("TestParProc\n");
	try
	{
		const size_t step_count = 2;
		auto action = [test_kind](size_t i)
				{
					switch (test_kind)
					{
						default:
						case ParProcTestKind::Ordinary:
							break;
						case ParProcTestKind::RuntimeError:
							throw runtime_error(ssprintf("ParProc exception @%zu", (size_t)i));
						case ParProcTestKind::CancelOperation:
							if (i == 1)
							{
								throw canceled_operation("ParProc cancel");
							}
							throw runtime_error("ParProc exception");
						case ParProcTestKind::QuitApplication:
							if (i == 1)
							{
								throw quit_application("ParProc quit", 1);
							}
							throw runtime_error("ParProc exception");
					}
				};
		switch (mode)
		{
			case ParProcMode::ParallelProcessor:
			{
				ParallelProcessor pp;
				pp.init(step_count, ParallelProcessor::e_force_parallel);
				pp.set_error_process_mode(ParallelProcessor::skip_rest);
				pp.perform(action, L"TestParProc", ConsoleProgressProxy());
				break;
			}
			case ParProcMode::OMP:
			{
				ThreadErrorCollector ec("TestParProc");
				#pragma omp parallel for schedule (guided)
				for (ptrdiff_t i = 0; i < (ptrdiff_t)step_count; ++i)
				{
					try
					{
						action(i);
					}
					catch (...)
					{
						ec.CatchException();
					}
				}
				ec.ThrowIfErrors();
				break;
			}
		}
	}
	catch (...)
	{
		printf("TestParProc exception:\n%s\n",
				EnsureType<const char*>(GetExceptionString().c_str()));
	}
	printf("TestParProc end\n");
}

//--------------------------------------------------------------

#ifdef _MSC_VER

#pragma optimize ("", off)
void RaiseNullPointerReference()
{
	int i = *(int*)nullptr;
	(void)i;
}
#pragma optimize ("", on)

#elif defined(__GNUC__)

void __attribute__((optimize("O0"))) RaiseNullPointerReference()
{
	int i = *(int*)nullptr;
	(void)i;
}

#else
#error This code is nonportable in case of optimization.
#endif

enum class ExceptionTestKind { runtime, quit, hardware, thread_sw, thread_hw };

void TestExceptions(ExceptionTestKind test_kind)
{
	ThreadSetup ts; (void)ts;
	try
	{
		switch (test_kind)
		{
			case ExceptionTestKind::runtime:
				printf("TestExceptions (runtime)\n");
				throw runtime_error("Test runtime exception.");
			case ExceptionTestKind::quit:
				printf("TestExceptions (quit)\n");
				throw quit_application("Test quit.", 0);
			case ExceptionTestKind::hardware:
				printf("TestExceptions (hardware)\n");
				RaiseNullPointerReference();
				break;
			case ExceptionTestKind::thread_sw:
			{
				printf("TestExceptions (thread_sw)\n");
				std::exception_ptr ep;
				std::thread t([&ep]()
						{
							ThreadSetup ts; (void)ts;
							try
							{
								throw runtime_error("Test runtime exception.");
							}
							catch (...)
							{
								ep = std::current_exception();
							}
						});
				t.join();
				if (ep)
				{
					std::rethrow_exception(ep);
				}
				break;
			}
			case ExceptionTestKind::thread_hw:
			{
				printf("TestExceptions (thread_hw)\n");
				std::exception_ptr ep;
				std::thread t([&ep]()
						{
							ThreadSetup ts; (void)ts;
							try
							{
								RaiseNullPointerReference();
							}
							catch (...)
							{
								ep = std::current_exception();
							}
						});
				t.join();
				if (ep)
				{
					std::rethrow_exception(ep);
				}
				break;
			}
		}
		printf("Exception has not been raised.\n");
	}
	catch (...)
	{
		printf("TestExceptions: in catch(...)\n");
		fflush(stdout);
		string message = GetExceptionStringOrRethrow();
		printf("Exception (GetExceptionStringOrRethrow):\n%s\n",
				EnsureType<const char*>(message.c_str()));
	}
}

//--------------------------------------------------------------

void PrintfFunction(const string &name, const RealFunctionF32 &f)
{
	printf("%s: {", EnsureType<const char*>(name.c_str()));
	for (size_t i = 0; i < f.size(); ++i)
	{
		if (i)
			printf(", ");
		printf("%.3f", EnsureType<float>(f[i]));
	}
	printf("}\n");
}

void PrintfFunction(const string &name, const ComplexFunctionF32 &f)
{
	printf("%s: {", EnsureType<const char*>(name.c_str()));
	for (size_t i = 0; i < f.size(); ++i)
	{
		if (i)
			printf(", ");
		printf("(%.3f, %.3f)", EnsureType<float>(f[i].re), EnsureType<float>(f[i].im));
	}
	printf("}\n");
}

void TestFunctions()
{
	const size_t f_size = ceil_fft_length(8);
	RealFunctionF32 f_1(f_size), f_2(f_size);
	for (size_t i = 0; i < f_size; ++i)
	{
		f_1[i] = 1+i;
		f_2[i] = (2+i)*(2+i);
	}
	PrintfFunction("f1", f_1);
	PrintfFunction("f2", f_2);

	RealFunctionF32 rf;
	auto g_1 = f_2 * f_1;
	RealFunctionF32 gg_1(g_1);
	gg_1 = g_1;
	PrintfFunction("f2*f1", gg_1);
	RealFunctionF32 g_2 = f_2 / f_1;
	PrintfFunction("f2/f1", g_2);

	ComplexFunctionF32 cf;
	ComplexFunctionF32 cf_1(f_1), cf_2(f_2);
	auto cg_1 = cf_2 * cf_1;
	ComplexFunctionF32 cgg_1(cg_1);
	cgg_1 = cg_1;
	PrintfFunction("cf2*cf1", cgg_1);
	ComplexFunctionF32 cg_2 = cf_2 / cf_1;
	PrintfFunction("cf2/cf1", cg_2);

	ComplexFunctionF32 h_1(f_1);
	FFT(h_1, ftForward);
	auto h_2(h_1);
	FFT(h_2, ftReverse);
	PrintfFunction("FFT(f1)", h_1);
	PrintfFunction("FFT-1(FFT(f1))", h_2);
}

//--------------------------------------------------------------

void TestAll()
{
	TestLang();
	SetLang("ru");
	TestLang();
	SetLang("en");
	TestLang();
	TestProgress();
	for (auto kind: { ParProcTestKind::Ordinary, ParProcTestKind::RuntimeError, ParProcTestKind::CancelOperation, ParProcTestKind::QuitApplication })
	{
		for (auto mode: { ParProcMode::ParallelProcessor, ParProcMode::OMP })
		{
			TestParProc(kind, mode);
		}
	}
	TestExceptions(ExceptionTestKind::runtime);
	//TestExceptions(ExceptionTestKind::hardware);
	TestExceptions(ExceptionTestKind::thread_sw);
	//TestExceptions(ExceptionTestKind::thread_hw);
	//TestExceptions(ExceptionTestKind::quit);

	TestFunctions();
}

//--------------------------------------------------------------

void Help(const char *args0)
{
	string filename = args0;
	// Извлечение имени файла в рамках XRADBasic недоступно. Используем arg0 как есть.
	printf("%s <parameters>\n",
			EnsureType<const char*>(filename.c_str()));
	printf(
R"raw(Parameters (the order is important):
	--help - show help
	--set_lang en|ru - set interface language
	--lang - language test
	--codecvt - codecvt test
	--ssprintf - ssprintf test
	--progress - progress test
	--set_par_proc_mode pp | omp - set parallel processor mode (parallel processor (default) or OMP)
	--par_proc - parallel processor test
	--par_proc_e - parallel processor test (with runtime error exception)
	--par_proc_c - parallel processor test (with cancel operation exception)
	--par_proc_q - parallel processor test (with quit application exception)
	--exc_runtime - runtime exception test
	--exc_quit - quit application exception test
	--exc_hardware - hardware exception test (1)
	--exc_thread_sw - runtime exception in a spawned thread test
	--exc_thread_hw - hardware exception in a spawned thread test (1)
	--functions - function object test
	--all - all tests (except for --exc_hardware, --exc_thread_hw, --exc_quit)

	(1) These tests can lead to program termination on some plaftorms.
)raw");
}

//--------------------------------------------------------------

int main_unsafe(int argn, char **args)
{
	if (argn == 1)
	{
		Help(args[0]);
		return 0;
	}
	ParProcMode par_proc_mode = ParProcMode::ParallelProcessor;
	for (int i = 1; i < argn; ++i)
	{
		const auto *param_name = args[i];
		if (!strcmp(param_name, "--help"))
		{
			Help(args[0]);
		}
		else if (!strcmp(param_name, "--set_lang"))
		{
			if (i+1 < argn)
			{
				SetLang(args[i+1]);
				printf("Set language \"%s\".\n", EnsureType<char*>(args[i+1]));
				SetLanguageId(args[i+1]);
				++i;
			}
		}
		else if (!strcmp(param_name, "--lang"))
		{
			TestLang();
		}
		else if (!strcmp(param_name, "--codecvt"))
		{
			TestCodeCvt();
		}
		else if (!strcmp(param_name, "--ssprintf"))
		{
			TestSSPrintf();
		}
		else if (!strcmp(param_name, "--progress"))
		{
			TestProgress();
		}
		else if (!strcmp(param_name, "--set_par_proc_mode"))
		{
			if (i+1 < argn)
			{
				const char *mode_str = args[i+1];
				printf("Set par_proc mode \"%s\".\n", EnsureType<const char*>(mode_str));
				if (!strcmp(mode_str, "pp"))
				{
					par_proc_mode = ParProcMode::ParallelProcessor;
				}
				else if (!strcmp(mode_str, "omp"))
				{
					par_proc_mode = ParProcMode::OMP;
				}
				else
				{
					printf("Invalid mode name.\n");
				}
				++i;
			}
		}
		else if (!strcmp(param_name, "--par_proc"))
		{
			TestParProc(ParProcTestKind::Ordinary, par_proc_mode);
		}
		else if (!strcmp(param_name, "--par_proc_e"))
		{
			TestParProc(ParProcTestKind::RuntimeError, par_proc_mode);
		}
		else if (!strcmp(param_name, "--par_proc_c"))
		{
			TestParProc(ParProcTestKind::CancelOperation, par_proc_mode);
		}
		else if (!strcmp(param_name, "--par_proc_q"))
		{
			TestParProc(ParProcTestKind::QuitApplication, par_proc_mode);
		}
		else if (!strcmp(param_name, "--exc_runtime"))
		{
			TestExceptions(ExceptionTestKind::runtime);
		}
		else if (!strcmp(param_name, "--exc_quit"))
		{
			TestExceptions(ExceptionTestKind::quit);
		}
		else if (!strcmp(param_name, "--exc_hardware"))
		{
			TestExceptions(ExceptionTestKind::hardware);
		}
		else if (!strcmp(param_name, "--exc_thread_sw"))
		{
			TestExceptions(ExceptionTestKind::thread_sw);
		}
		else if (!strcmp(param_name, "--exc_thread_hw"))
		{
			TestExceptions(ExceptionTestKind::thread_hw);
		}
		else if (!strcmp(param_name, "--functions"))
		{
			TestFunctions();
		}
		else if (!strcmp(param_name, "--all"))
		{
			TestAll();
		}
		else
		{
			printf("Error: Unknown parameter: \"%s\".\n",
					EnsureType<const char*>(param_name));
		}
	}
	return 0;
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

int main(int argn, char **args)
{
	try
	{
		return main_unsafe(argn, args);
	}
	catch (exception &)
	{
		printf("Exception in main: %s\n", EnsureType<const char*>(GetExceptionString().c_str()));
		return 3;
	}
	catch (...)
	{
		printf("Internal error: Unhandled unknown exception in main.\n%s\n",
				EnsureType<const char*>(GetExceptionString().c_str()));
		return 3;
	}
}

//--------------------------------------------------------------
