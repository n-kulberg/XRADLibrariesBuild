// file XRADPerformanceTest.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "PerformanceTests.h"
#include <XRADBasic/Core.h>
#include <XRADSystem/System.h>
#include <XRADSystem/CFile.h>
#include <cstring>
#include <cinttypes>

#ifdef XRAD_COMPILER_MSC
	#include <XRADConsoleUI/Sources/PlatformSpecific/MSVC/MSVC_XRADConsoleUILink.h>
#endif // XRAD_COMPILER_MSC

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

void Help(const char *args0)
{
	string filename;
	SplitFilename(convert_to_string(GetPathGenericFromAutodetect(convert_to_wstring(args0))),
			nullptr, &filename);
	printf("%s <parameters>\n",
			EnsureType<const char*>(filename.c_str()));
	printf(
R"raw(Parameters (the order is important):
	--help - show help
	--set_lang en|ru - set interface language
	--perf_omp_math <outer_count> <inner_count> <thread_count>
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
		else if (!strcmp(param_name, "--perf_omp_math"))
		{
			if (i+3 < argn)
			{
				const char *outer_str = args[i+1];
				const char *inner_str = args[i+2];
				const char *thread_str = args[i+3];
				auto outer_count = strtoll(outer_str, nullptr, 0);
				auto inner_count = strtoll(inner_str, nullptr, 0);
				auto thread_count = strtol(thread_str, nullptr, 0);
				PerformanceOMPMath(outer_count, inner_count, thread_count);
				i += 3;
			}
			else
			{
				printf("perf_omp_math: required parameters missing.\n");
			}
		}
		else
		{
			printf("Error: Unknown parameter: \"%s\".",
					EnsureType<const char*>(param_name));
		}
	}
	return 0;
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

int xrad::xrad_main(int argn, char **args)
{
	try
	{
		return main_unsafe(argn, args);
	}
	catch (exception &)
	{
		printf("%s\n", EnsureType<const char*>(GetExceptionString().c_str()));
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
