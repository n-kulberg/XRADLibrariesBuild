// file TestMath.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestMath.h"
#include <XRADBasic/Tests/TestHelpers.h>

//--------------------------------------------------------------

XRAD_BEGIN

//--------------------------------------------------------------

namespace
{

template <class TI>
void TestRoundItem(double v,
		bool sample_flag, TI sample_value)
{
	auto Error = [](const string &s)
	{
		printf("%s\n", s.c_str());
	};

	TI value;
	auto conv = try_iround_n(v, &value);
	if (conv != sample_flag || value != sample_value)
	{
		string result_str;
		if (std::is_unsigned<TI>::value)
			result_str = ssprintf("%llu", (unsigned long long)value);
		else
			result_str = ssprintf("%lli", (long long)value);
		string sample_value_str;
		if (std::is_unsigned<TI>::value)
			sample_value_str = ssprintf("%llu", (unsigned long long)sample_value);
		else
			sample_value_str = ssprintf("%lli", (long long)sample_value);
		Error(ssprintf("Error in try_iround_n(double v, %s* result): v=%lf, return=%s, *result=%s (expected: %s, %s)",
				EnsureType<const char*>(typeid(TI).name()),
				EnsureType<double>(v),
				EnsureType<const char*>(conv? "true": "false"),
				EnsureType<const char*>(result_str.c_str()),
				EnsureType<const char*>(sample_flag? "true": "false"),
				EnsureType<const char*>(sample_value_str.c_str())));
	}
	TI nc_value = iround_n<TI>(v);
	if (nc_value != value)
	{
		string result_str;
		if (std::is_unsigned<TI>::value)
			result_str = ssprintf("%llu", (unsigned long long)nc_value);
		else
			result_str = ssprintf("%lli", (long long)nc_value);
		string try_result_str;
		if (std::is_unsigned<TI>::value)
			try_result_str = ssprintf("%llu", (unsigned long long)value);
		else
			try_result_str = ssprintf("%lli", (long long)value);
		Error(ssprintf("Error in iround_n<%s>(double v): v=%lf, return=%s (expected: %s)",
				EnsureType<const char*>(typeid(TI).name()),
				EnsureType<double>(v),
				EnsureType<const char*>(result_str.c_str()),
				EnsureType<const char*>(try_result_str.c_str())));
	}
}

} // namespace

//--------------------------------------------------------------

void TestRound()
{
	TestRoundItem<int>(0, true, 0);
	TestRoundItem<int>(1, true, 1);
	TestRoundItem<int>(-1, true, -1);
	TestRoundItem<int>(0.4, true, 0);
	TestRoundItem<int>(0.6, true, 1);
	TestRoundItem<int>(-0.4, true, 0);
	TestRoundItem<int>(-0.6, true, -1);
	TestRoundItem<int>(0.5, true, 0);
	TestRoundItem<int>(1.5, true, 2);
	TestRoundItem<int>(-0.5, true, 0);
	TestRoundItem<int>(-1.5, true, -2);
	TestRoundItem<int>(numeric_limits<int>::max(), true, numeric_limits<int>::max());
	TestRoundItem<int>(numeric_limits<int>::min(), true, numeric_limits<int>::min());
	TestRoundItem<int>(double(numeric_limits<int>::max())+1, false, numeric_limits<int>::max());
	TestRoundItem<int>(double(numeric_limits<int>::min())-1, false, numeric_limits<int>::min());
	TestRoundItem<unsigned int>(0, true, 0);
	TestRoundItem<unsigned int>(1, true, 1);
	TestRoundItem<unsigned int>(-1, false, 0);
	TestRoundItem<unsigned int>(numeric_limits<unsigned int>::max(), true, numeric_limits<unsigned int>::max());
	TestRoundItem<unsigned int>(double(numeric_limits<unsigned int>::max())+1, false, numeric_limits<unsigned int>::max());

	// Максимальные по модулю 64-битные целые значения (long long) не могут быть представлены точно
	// в 64-битном типе с плавающей точкой (double).
	// Поэтому здесь задаются и вычисляются нужные для теста константы под конкретные типы double
	// и long long.
	static_assert(numeric_limits<long long>::digits == 63, "This test expects 64 bit long long.");
	static_assert(numeric_limits<double>::digits == 53, "This test expects 64 bit double (53 bit mantissa).");
	double long_long_max_d = 9223372036854774784.0; // = std::nextafter(double(numeric_limits<long long>::max()), 0)
	double long_long_max_d_ll = 9223372036854774784ll;
	double long_long_max_d_next = 9223372036854775808.0; // 2^63, точно представимо в double.
	double long_long_min_d = -9223372036854775808.0; // -2^63, точно представимо в double.
	double long_long_min_d_ll = -9223372036854775808ll;
	double long_long_min_d_next = std::nextafter(long_long_min_d, numeric_limits<double>::lowest());
	TestRoundItem<long long>(0, true, 0);
	TestRoundItem<long long>(1, true, 1);
	TestRoundItem<long long>(-1, true, -1);
	TestRoundItem<long long>(long_long_max_d, true, long_long_max_d_ll);
	TestRoundItem<long long>(long_long_min_d, true, long_long_min_d_ll);
	TestRoundItem<long long>(long_long_max_d_next, false, numeric_limits<long long>::max());
	TestRoundItem<long long>(long_long_min_d_next, false, numeric_limits<long long>::min());

	TestRoundItem<unsigned long long>(0, true, 0);
	TestRoundItem<unsigned long long>(1, true, 1);
	TestRoundItem<unsigned long long>(-1, false, 0);
	TestRoundItem<unsigned long long>(long_long_max_d, true, long_long_max_d_ll);
	// Здесь возвращаемое значение (result) больше исходного значения double (существенно больше),
	// см. комментарии к функции:
	TestRoundItem<unsigned long long>(long_long_max_d_next, false, numeric_limits<unsigned long long>::max());
	//TestRoundItem<unsigned long long>(numeric_limits<unsigned long long>::max(), true, numeric_limits<unsigned long long>::max());
	TestRoundItem<unsigned long long>(long_long_min_d, false, 0);

	TestRoundItem<int>(numeric_limits<double>::infinity(), false, numeric_limits<int>::max());
	TestRoundItem<int>(-numeric_limits<double>::infinity(), false, numeric_limits<int>::min());
	TestRoundItem<int>(numeric_limits<double>::quiet_NaN(), false, 0);
	TestRoundItem<int>(numeric_limits<double>::quiet_NaN(), false, 0);
	TestRoundItem<int>(numeric_limits<double>::signaling_NaN(), false, 0);
}

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

void Test1DFIRFilterAuto(TestHelpers::ErrorReporter *error_reporter, size_t N)
{
	auto report_error = [error_reporter](const string &error_string)
	{
		// Debugger breakpoint can be set here.
		error_reporter->ReportError(error_string);
	};

	RealFunctionF32 func(N);
	for (size_t i = 0; i < func.size(); ++i)
	{
		func[i] = i;
	}
	RealFunctionF32 func_src(func);
	FilterKernelReal filter(3, extrapolation::by_last_value);
	func.Filter(filter);
	for (size_t i = 0; i < func.size(); ++i)
	{
		double src_v = func_src[i];
		if (func.size() > 1)
		{
			if (i == 0)
				src_v = (func_src[0]*2 + func_src[1])/3.;
			else if (i == func.size() - 1)
				src_v = (func_src[i-1] + func_src[i]*2)/3.;
		}
		if (fabs(func[i] - src_v) > 1e-3)
		{
			string message = ssprintf(
					"Error filtering with FIRFilter 1D: invalid value at (%zu).\n",
					EnsureType<size_t>(i));
			message += "Filter: (3, by_last_value) {1/3, 1/3, 1/3}\n";
			message += "Source matrix:\n";
			message += "\t";
			for (size_t rj = 0; rj < func_src.size(); ++rj)
			{
				message += ssprintf(" %4.0f", EnsureType<float>(func_src[rj]));
			}
			message += "\n";
			message += "Filtered matrix:\n";
			message += "\t";
			for (size_t rj = 0; rj < func.size(); ++rj)
			{
				message += ssprintf(" %4.3f", EnsureType<float>(func[rj]));
			}
			message += "\n";
			report_error(message);
			break;
		}
	}
}

//--------------------------------------------------------------

void Test1DFIRFilter()
{
	class ErrorReporter : public TestHelpers::ErrorReporter
	{
		public:
			virtual void ReportError(const string &error_message) override
			{
				printf("Test1DFIRFilter error: %s\n", error_message.c_str());
				Error(ssprintf("Test1DFIRFilter error:\n%s", error_message.c_str()));
			}
	};
	ErrorReporter error_reporter;
	Test1DFIRFilterAuto(&error_reporter, 4);
	Test1DFIRFilterAuto(&error_reporter, 3);
	Test1DFIRFilterAuto(&error_reporter, 2);
	Test1DFIRFilterAuto(&error_reporter, 1);
}

//--------------------------------------------------------------

void Test2DFIRFilterAuto(TestHelpers::ErrorReporter *error_reporter, size_t M, size_t N)
{
	auto report_error = [error_reporter](const string &error_string)
	{
		// Debugger breakpoint can be set here.
		error_reporter->ReportError(error_string);
	};

	RealFunction2D_F32 func(M, N);
	for (size_t i = 0; i < func.sizes(0); ++i)
	{
		auto &row = func.row(i);
		for (size_t j = 0; j < row.size(); ++j)
		{
			row[j] = i * row.size() + j;
		}
	}
	RealFunction2D_F32 func_src(func);
	FIRFilterKernel2DMask<bool> filter(square3, 0.5);
	func.Filter(filter);
	for (size_t i = 0; i < func.sizes(0); ++i)
	{
		for (size_t j = 0; j < func.sizes(1); ++j)
		{
			size_t src_i = i;
			size_t src_j = j;
			if (func.sizes(0) > 1)
			{
				if (i == 0 && j < func.sizes(1) - 1)
					++src_j;
				if (i == func.sizes(0) - 1 && j > 0)
					--src_j;
			}
			if (func.at(i, j) != func_src.at(src_i, src_j))
			{
				string message = ssprintf(
						"Error filtering with FIRFilter 2D: invalid value at (%zu, %zu).\n",
						EnsureType<size_t>(i), EnsureType<size_t>(j));
				message += "Filter: (square3, 0.5)\n";
				message += "Source matrix:\n";
				for (size_t ri = 0; ri < func_src.sizes(0); ++ri)
				{
					message += "\t";
					for (size_t rj = 0; rj < func_src.sizes(1); ++rj)
					{
						message += ssprintf(" %4.0f", EnsureType<float>(func_src.at(ri, rj)));
					}
					message += "\n";
				}
				message += "Filtered matrix:\n";
				for (size_t ri = 0; ri < func.sizes(0); ++ri)
				{
					message += "\t";
					for (size_t rj = 0; rj < func.sizes(1); ++rj)
					{
						message += ssprintf(" %4.0f", EnsureType<float>(func.at(ri, rj)));
					}
					message += "\n";
				}
				report_error(message);
				break;
			}
		}
	}
}

//--------------------------------------------------------------

void Test2DFIRFilter()
{
	class ErrorReporter : public TestHelpers::ErrorReporter
	{
		public:
			virtual void ReportError(const string &error_message) override
			{
				printf("Test2DFIRFilter error: %s\n", error_message.c_str());
				Error(ssprintf("Test2DFIRFilter error:\n%s", error_message.c_str()));
			}
	};
	ErrorReporter error_reporter;
	Test2DFIRFilterAuto(&error_reporter, 4, 3);
	Test2DFIRFilterAuto(&error_reporter, 4, 2);
	Test2DFIRFilterAuto(&error_reporter, 2, 4);
	Test2DFIRFilterAuto(&error_reporter, 2, 2);
	Test2DFIRFilterAuto(&error_reporter, 3, 1);
	Test2DFIRFilterAuto(&error_reporter, 1, 3);
	Test2DFIRFilterAuto(&error_reporter, 1, 1);
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestMathFunctions()
{
	for (;;)
	{
		auto answer = GetButtonDecision(L"Test math",
				{
					MakeButton(L"1D FIR filter", Test1DFIRFilter),
					MakeButton(L"2D FIR filter", Test2DFIRFilter),
					MakeButton(L"Cancel", (void (*)())nullptr)
				});
		if (!answer)
			break;
		SafeExecute(answer);
	}
}

XRAD_END

//--------------------------------------------------------------
