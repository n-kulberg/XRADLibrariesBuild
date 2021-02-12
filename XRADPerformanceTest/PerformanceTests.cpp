//--------------------------------------------------------------
#include "pre.h"

#include "PerformanceTests.h"
#include <XRADBasic/Sources/Utils/ConsoleProgress.h>
#include <XRADBasic/Sources/Utils/ProgressProxyApi.h>
#include <omp.h>

//--------------------------------------------------------------

void PerformanceOMPMath(int64_t outer_count, int64_t inner_count, int thread_count)
{
	omp_set_num_threads(thread_count);
	vector<int64_t> results(outer_count);
	ProgressProxyApi::EnableLog(true);
	ProgressBar pb(ConsoleProgressProxy());
	pb.start("PerformanceOMPMath", 1);
	ThreadErrorCollector ec("PerformanceOMPMath");
	#pragma omp parallel for schedule (guided)
	for(int64_t i = 0; i < outer_count; ++i)
	{
		if (ec.HasErrors())
#ifdef XRAD_COMPILER_MSC
			break;
#else
			continue;
#endif
		ThreadSetup ts; (void)ts;
		try
		{
			int64_t sum = 0;
			for (int64_t j = 0; j < inner_count; ++j)
			{
				sum += j;
			}
			results[i] = sum;
		}
		catch (...)
		{
			ec.CatchException();
		}
	}
	ec.ThrowIfErrors();
	++pb;
	pb.end();
}

//--------------------------------------------------------------
