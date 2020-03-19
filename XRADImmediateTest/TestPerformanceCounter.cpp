#include "pre.h"

#include "TestPerformanceCounter.h"
#include <algorithm>
#include <numeric>
#include <ctime>

//--------------------------------------------------------------

XRAD_BEGIN

namespace
{

//--------------------------------------------------------------

void TestPerformanceCounterMSec()
{
	size_t intervals_size = 1000000;
	vector<double> intervals(intervals_size, 0);
	double *interval_data = intervals.data();
	auto ct_0 = time(nullptr);
	auto ct_1 = ct_0;
	while (ct_0 == (ct_1 = time(nullptr)))
		;
	auto ct_2 = ct_1;
	double pc_t_1 = GetPerformanceCounterMSec();
	double pc_p = pc_t_1;
	size_t interval_index = 0;
	for (;;)
	{
		ct_2 = time(nullptr);
		if (ct_2 != ct_1)
			break;
		if (interval_index + 4 <= intervals_size)
		{
			double pc_0 = GetPerformanceCounterMSec();
			double pc_1 = GetPerformanceCounterMSec();
			double pc_2 = GetPerformanceCounterMSec();
			double pc_3 = GetPerformanceCounterMSec();
			interval_data[interval_index] = pc_0 - pc_p;
			interval_data[interval_index + 1] = pc_1 - pc_0;
			interval_data[interval_index + 2] = pc_2 - pc_1;
			interval_data[interval_index + 3] = pc_3 - pc_2;
			interval_index += 4;
			pc_p = pc_3;
		}
	}
	double pc_t_2 = GetPerformanceCounterMSec();

	sort(interval_data, interval_data + interval_index);
	double min_min_duration = interval_index? *interval_data: 0;
	auto nonnegative_data = find_if(interval_data, interval_data + interval_index,
			[](double v) { return v >= 0; });
	size_t negative_count = nonnegative_data - interval_data;
	auto positive_data = find_if(nonnegative_data, interval_data + interval_index,
			[](double v) { return v; });
	size_t zero_count = positive_data - nonnegative_data;
	size_t positive_count = interval_index - zero_count - negative_count;
	double min_pos_duration = positive_count? *positive_data: 0;
	double max_pos_duration = positive_count? positive_data[positive_count - 1]: 0;
	double median_pos_duration = positive_count? positive_data[positive_count / 2]: 0;
	double total_duration = accumulate(interval_data, interval_data + interval_index, double(0));
	double average_duration = interval_index? total_duration / interval_index: 0;

	vector<pair<double, size_t>> exp_histogram;
	if (min_min_duration < 0)
		exp_histogram.push_back(make_pair(min_min_duration, size_t(0)));
	exp_histogram.push_back(make_pair(double(0), size_t(0)));
	{
		double v = min_pos_duration;
		for (;;)
		{
			exp_histogram.push_back(make_pair(v, size_t(0)));
			v *= 2;
			if (v > max_pos_duration)
			{
				exp_histogram.push_back(make_pair(v, size_t(0)));
				break;
			}
		}
	}
	for (auto v: intervals)
	{
		auto it = upper_bound(exp_histogram.begin(), exp_histogram.end(), v,
				[](double v, auto &p) { return v < p.first; });
		--it;
		++it->second;
	}

	printf("C-time total duration: %.6lf sec\n", (double)(ct_2 - ct_1));
	printf("Performance counter total duration: %.12lf sec\n", (double)(0.001*(pc_t_2 - pc_t_1)));
	printf("Negative duration count: %zu (zero expected)\n", EnsureType<size_t>(negative_count));
	printf("Zero duration count: %zu\n", EnsureType<size_t>(zero_count));
	printf("Positive duration count: %zu\n", EnsureType<size_t>(positive_count));
	printf("Total collected count: %zu\n", EnsureType<size_t>(interval_index));
	printf("Total collected duration: %.6lf sec\n", (double)(0.001*total_duration));
	printf("Average collected duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(0.001*average_duration),
			EnsureType<double>(0.001*average_duration),
			EnsureType<double>(1e6*average_duration));
	printf("Min duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(0.001*min_min_duration),
			EnsureType<double>(0.001*min_min_duration),
			EnsureType<double>(1e6*min_min_duration));
	printf("Min positive duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(0.001*min_pos_duration),
			EnsureType<double>(0.001*min_pos_duration),
			EnsureType<double>(1e6*min_pos_duration));
	printf("Max positive duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(0.001*max_pos_duration),
			EnsureType<double>(0.001*max_pos_duration),
			EnsureType<double>(1e6*max_pos_duration));
	printf("Median positive duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(0.001*median_pos_duration),
			EnsureType<double>(0.001*median_pos_duration),
			EnsureType<double>(1e6*median_pos_duration));
	printf("Histogram:\n");
	for (auto &v: exp_histogram)
	{
		double t = v.first;
		printf("  [%.3le sec = %.3lf ns]: %zu\n",
				EnsureType<double>(0.001*t),
				EnsureType<double>(1e6*t),
				EnsureType<size_t>(v.second));
	}
}

//--------------------------------------------------------------

void TestPerformanceCounterStd()
{
	size_t intervals_size = 1000000;
	vector<double> intervals(intervals_size, 0);
	double *interval_data = intervals.data();
	auto ct_0 = time(nullptr);
	auto ct_1 = ct_0;
	while (ct_0 == (ct_1 = time(nullptr)))
		;
	auto ct_2 = ct_1;
	constexpr double m = 1;
	auto pc_t_1 = GetPerformanceCounterStd();
	auto pc_p = pc_t_1;
	size_t interval_index = 0;
	for (;;)
	{
		ct_2 = time(nullptr);
		if (ct_2 != ct_1)
			break;
		if (interval_index + 4 <= intervals_size)
		{
			auto pc_0 = GetPerformanceCounterStd();
			auto pc_1 = GetPerformanceCounterStd();
			auto pc_2 = GetPerformanceCounterStd();
			auto pc_3 = GetPerformanceCounterStd();
			interval_data[interval_index] = (pc_0 - pc_p).count();
			interval_data[interval_index + 1] = (pc_1 - pc_0).count();
			interval_data[interval_index + 2] = (pc_2 - pc_1).count();
			interval_data[interval_index + 3] = (pc_3 - pc_2).count();
			interval_index += 4;
			pc_p = pc_3;
		}
	}
	auto pc_t_2 = GetPerformanceCounterStd();

	sort(interval_data, interval_data + interval_index);
	double min_min_duration = interval_index? *interval_data: 0;
	auto nonnegative_data = find_if(interval_data, interval_data + interval_index,
			[](double v) { return v >= 0; });
	size_t negative_count = nonnegative_data - interval_data;
	auto positive_data = find_if(nonnegative_data, interval_data + interval_index,
			[](double v) { return v; });
	size_t zero_count = positive_data - nonnegative_data;
	size_t positive_count = interval_index - zero_count - negative_count;
	double min_pos_duration = positive_count? *positive_data: 0;
	double max_pos_duration = positive_count? positive_data[positive_count - 1]: 0;
	double median_pos_duration = positive_count? positive_data[positive_count / 2]: 0;
	double total_duration = accumulate(interval_data, interval_data + interval_index, double(0));
	double average_duration = interval_index? total_duration / interval_index: 0;

	vector<pair<double, size_t>> exp_histogram;
	if (min_min_duration < 0)
		exp_histogram.push_back(make_pair(min_min_duration, size_t(0)));
	exp_histogram.push_back(make_pair(double(0), size_t(0)));
	{
		double v = min_pos_duration;
		for (;;)
		{
			exp_histogram.push_back(make_pair(v, size_t(0)));
			v *= 2;
			if (v > max_pos_duration)
			{
				exp_histogram.push_back(make_pair(v, size_t(0)));
				break;
			}
		}
	}
	for (auto v: intervals)
	{
		auto it = upper_bound(exp_histogram.begin(), exp_histogram.end(), v,
				[](double v, auto &p) { return v < p.first; });
		--it;
		++it->second;
	}

	printf("C-time total duration: %.6lf sec\n", (double)(ct_2 - ct_1));
	printf("Performance counter total duration: %.12lf sec\n", (double)(m*(pc_t_2 - pc_t_1).count()));
	printf("Negative duration count: %zu (zero expected)\n", EnsureType<size_t>(negative_count));
	printf("Zero duration count: %zu\n", EnsureType<size_t>(zero_count));
	printf("Positive duration count: %zu\n", EnsureType<size_t>(positive_count));
	printf("Total collected count: %zu\n", EnsureType<size_t>(interval_index));
	printf("Total collected duration: %.6lf sec\n", (double)(m*total_duration));
	printf("Average collected duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(m*average_duration),
			EnsureType<double>(m*average_duration),
			EnsureType<double>(1e9*m*average_duration));
	printf("Min duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(m*min_min_duration),
			EnsureType<double>(m*min_min_duration),
			EnsureType<double>(1e9*m*min_min_duration));
	printf("Min positive duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(m*min_pos_duration),
			EnsureType<double>(m*min_pos_duration),
			EnsureType<double>(1e9*m*min_pos_duration));
	printf("Max positive duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(m*max_pos_duration),
			EnsureType<double>(m*max_pos_duration),
			EnsureType<double>(1e9*m*max_pos_duration));
	printf("Median positive duration: %.12lf (%.6le) sec = %.12lf ns\n",
			EnsureType<double>(m*median_pos_duration),
			EnsureType<double>(m*median_pos_duration),
			EnsureType<double>(1e9*m*median_pos_duration));
	printf("Histogram:\n");
	for (auto &v: exp_histogram)
	{
		double t = v.first;
		printf("  [%.3le sec = %.3lf ns]: %zu\n",
				EnsureType<double>(m*t),
				EnsureType<double>(1e9*m*t),
				EnsureType<size_t>(v.second));
	}
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestPerformanceCounter()
{
	for (;;)
	{
		using func = function<void()>;
		auto response = GetButtonDecision(L"Performance counter test",
				{
					MakeButton(L"Performance counter msec", func(TestPerformanceCounterMSec)),
					MakeButton(L"Performance counter std", func(TestPerformanceCounterStd)),
					MakeButton(L"Exit", func())
				});
		if (!response)
			break;
		SafeExecute(response);
	}
}

//--------------------------------------------------------------

XRAD_END
