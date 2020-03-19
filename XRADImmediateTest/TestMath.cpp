// file TestMath.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestMath.h"
#include <XRAD/Tests/TestHelpers.h>

//--------------------------------------------------------------

XRAD_BEGIN

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
