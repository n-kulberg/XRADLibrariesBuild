#include "pre.h"

#include "TestMoveSemantics.h"
#include "TestArrays.h"
#include "TestMath.h"
#include "TestJson.h"
#include "TestTextHandling.h"
#include "TestFileOperations.h"
#include "TestGUIFunctions.h"
#include "TestPerformanceCounter.h"
#include "TestThreads.h"
#include "TestFunctions.h"
#include "TestFFT.h"
#include "TestHandy.h"
#ifdef XRAD_COMPILER_MSC
#include "XRADNatvisTest.h"
#endif // XRAD_COMPILER_MSC

#include <iostream>

#ifdef XRAD_COMPILER_MSC
#include <vld.h>
#endif // XRAD_COMPILER_MSC

//--------------------------------------------------------------

int xrad::xrad_main(int in_argc, char *in_argv[])
{
	XRAD_USING

	try
	{
		SetVersionInfo(
			"XRADImmediateTest version 0.0.99\n"
			"XRAD library test application.");

		printf("command string arguments:\n");
		for(int i = 0; i < in_argc; ++i)
		{
			printf("%d:\t%s\n", i, in_argv[i]);
		}
		fflush(stderr);
		fflush(stdout);

		for (;;)
		{
			using func = function<void()>;
			auto response = GetButtonDecision(L"Choose test",
					{
						MakeButton(L"Move semantics", func(TestMoveSemantics)),
						MakeButton(L"Test arrays", func(TestArrays)),
						MakeButton(L"Test math", func(TestMathFunctions)),
						MakeButton(L"JSON", func(TestJson)),
						MakeButton(L"Text handling", func(TestTextHandling)),
						MakeButton(L"I/O and file operations test", func(TestIO)),
						MakeButton(L"GUI", func(TestGUIFunctions)),
						MakeButton(L"Performance counter", func(TestPerformanceCounter)),
						MakeButton(L"Threads", func(TestThreads)),
						MakeButton(L"Round", func(TestRound)),
#ifdef XRAD_COMPILER_MSC
						MakeButton(L"Natvis (debugger visualization)", func(XRADNatvisTest)),
#endif // XRAD_COMPILER_MSC
						MakeButton(L"Test functions", func(TestFunctions)),
						MakeButton(L"Test FFT", func(TestFFT)),
						MakeButton(L"Handy", func(TestHandy)),
						MakeButton(L"Exit", func())
					});
			if (!response)
				break;

			printf("\nTest begin\n");
			fflush(stdout);
			try
			{
				response();
			}
			catch (canceled_operation &)
			{
				fflush(stderr);
				cerr << "\ntest canceled\n";
				cerr.flush();
			}
			catch (quit_application &)
			{
				throw;
			}
			catch (...)
			{
				fflush(stderr);
				string message = GetExceptionString();
				cerr << "\ntest failed:\n" << message << "\n";
				cerr.flush();
				Error(message);
			}
			fflush(stdout);
			cout << "\rtest ended\n";
			cout.flush();
		}
	}
	catch(quit_application &ex)
	{
		fflush(stdout);
		cout << "\n" << ex.what() << ", exit code = " << ex.exit_code << "\n";
		cout.flush();
		return ex.exit_code;
	}
	catch (...)
	{
		fflush(stderr);
		cerr << "\nUnhandled exception:\n" << GetExceptionString() << "\n";
		cerr.flush();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

//--------------------------------------------------------------
