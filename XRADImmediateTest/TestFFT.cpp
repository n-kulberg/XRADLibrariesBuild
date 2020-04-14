// file TestFFT.cpp
//--------------------------------------------------------------
/*!
	\file
	\brief Ассемблерная версия вычислений
*/
#include "pre.h"

#include "TestFFT.h"
#include <XRADBasic/MathFunctionTypes.h>
#include <XRADBasic/Sources/Fourier/CooleyTukeyFFT.h>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <cstring>

XRAD_USING

//--------------------------------------------------------------

#if defined(XRAD_COMPILER_MSC) && !defined(XRAD_COMPILER_CMAKE)

#define XRADImmediateTest_USE_ASM

#ifndef _M_X64

//--------------------------------------------------------------

inline void FFTv0asm_float(complexF32 *data, size_t data_size, ftDirection direction,
		const complexF32 *phasors,
		complexF32 *buffer)
{
	throw runtime_error("FFTv0_float/x86: Not implemented.");
	static_assert(sizeof(int) == sizeof(size_t), "Invalid architecture.");
}

//--------------------------------------------------------------

#else // _M_X64

//--------------------------------------------------------------

extern "C" void FFTv0_float_ASM(
		void/*complex<float>*/ *data,
		uint64_t data_size,
		uint64_t direction,
		const void/*complex<float>*/ *phasors,
		void/*complex<float>*/ *buffer);

namespace
{
inline void FFTv0asm_float(complexF32 *data, size_t data_size, ftDirection direction,
		const complexF32 *phasors,
		complexF32 *buffer)
{
	FFTv0_float_ASM(data, data_size, direction == ftReverse? 1: 0, phasors, buffer);
}
} // namespace

//--------------------------------------------------------------

#endif // _M_X64
#endif // defined(XRAD_COMPILER_MSC) && !defined(XRAD_COMPILER_CMAKE)

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

size_t FFTv0_ceil_fft_length(size_t length, size_t *power_of_2 = nullptr)
{
	if (length <= 1)
	{
		if (power_of_2)
			*power_of_2 = 0;
		return 1;
	}
	length -= 1;
	if (length > (numeric_limits<size_t>::max() >> 1))
		throw length_error(ssprintf("FFT length is too large: %zu.", EnsureType<size_t>(length)));
	size_t res_length = 1;
	size_t res_power = 0;
	for (; length; length >>= 1)
	{
		res_length <<= 1;
		++res_power;
	}
	if (power_of_2)
		*power_of_2 = res_power;
	return res_length;
}

//--------------------------------------------------------------

size_t rev_0(size_t i, size_t data_size)
{
	size_t res = 0;
	for (size_t j = data_size; j > 1; j >>= 1, i >>= 1)
		res = (res<<1) + (i & 0x01);
	return res;
}

DataArray<size_t> MakeRevIndex(size_t data_size)
{
	DataArray<size_t> result(data_size);
	for (size_t i = 0; i < data_size; ++i)
	{
		result[i] = rev_0(i, data_size);
	}
	return result;
}

//--------------------------------------------------------------

void FFTv0c_float(complexF32 *data, size_t data_size, ftDirection direction,
		const complexF32 *phasors,
		complexF32 *buffer,
		const size_t *rev_index_table)
{
	// reordering
	float mul = 1./sqrt(data_size);
	if (direction == ftForward)
	{
	 	// forward
	 	for(size_t j=0; j < data_size; ++j)
		{
			//size_t k = rev_0(j, data_size);
			size_t k = rev_index_table[j];
			if (k >= j) // ==: умножение на множитель нужно всегда
			{
				auto tmp = data[k];
				data[k] = mul*data[j];
				data[j] = mul*tmp;
			}
		}
	}
	else
	{
		// reverse
		buffer[0] = mul*data[0];
		for (size_t j=1; j < data_size; ++j)
		{
			//buffer[rev_0(data_size-j, data_size)] = mul*data[j];
			buffer[rev_index_table[data_size-j]] = mul*data[j];
		}
		memcpy (data, buffer, data_size*sizeof(*data));
	}

	// transform
	for (size_t l=2; l <= data_size; l*=2)
	{
		auto dsl = data_size/l;
		for (size_t k=0; k < data_size; k+=l)
			for (size_t j=0; j < l/2; ++j)
			{
				auto t = data[k+j];
				auto t1 = data[k+j+l/2]*phasors[j*dsl];
				data[k+j] = t + t1;
				data[k+j+l/2] = t - t1;
			}
	}
}

//--------------------------------------------------------------

template <class complex_t = complexF32>
class TransformProcessor
{
	public:
		using ComplexFunction_t = ComplexFunction<complex_t, double>;
	public:
		virtual void Init(size_t data_size) = 0;
		virtual size_t CeilFFTLength(size_t length) = 0;
		virtual void DoOne(ComplexFunction_t &data, ftDirection direction) = 0;
};

//--------------------------------------------------------------

class NullTransformProcessor: public TransformProcessor<>
{
	public:
		virtual void Init(size_t data_size) override
		{
		}
		virtual size_t CeilFFTLength(size_t length) override
		{
			return length;
		}
		virtual void DoOne(ComplexFunctionF32 &data, ftDirection direction) override
		{
		}
};

//--------------------------------------------------------------

template <class complex_t = complexF32>
class FFTTransformProcessor: public TransformProcessor<complex_t>
{
	public:
		PARENT(TransformProcessor<complex_t>);
		using typename parent::ComplexFunction_t;

		virtual void Init(size_t data_size) override
		{
			ComplexFunction_t array(data_size, complex_t(0));
			FFT(array, ftForward);
		}
		virtual size_t CeilFFTLength(size_t length) override
		{
			return ceil_fft_length(length);
		}
		virtual void DoOne(ComplexFunction_t &data, ftDirection direction) override
		{
			FFT(data, direction);
		}
};

//--------------------------------------------------------------

#ifdef XRADImmediateTest_USE_ASM
class FFTv0asmTransformProcessor: public TransformProcessor<>
{
	public:
		virtual void Init(size_t data_size) override
		{
			data_size = CeilFFTLength(data_size);
			phasors.realloc(data_size/2);
			for (size_t i = 0; i < data_size/2; ++i)
			{
				phasors[i] = polar(1., -2.*pi()*(double)i/(double)data_size);
			}
			buffer.realloc(data_size);
		}
		virtual size_t CeilFFTLength(size_t length) override
		{
			return FFTv0_ceil_fft_length(length);
		}
		virtual void DoOne(ComplexFunctionF32 &data, ftDirection direction) override
		{
			FFTv0asm_float(data.data(), data.size(), direction, phasors.data(), buffer.data());
		}
	private:
		ComplexFunctionF32 phasors;
		ComplexFunctionF32 buffer;
};
#endif // XRADImmediateTest_USE_ASM

//--------------------------------------------------------------

class FFTv0cTransformProcessor: public TransformProcessor<>
{
	public:
		virtual void Init(size_t data_size) override
		{
			data_size = CeilFFTLength(data_size);
			phasors.realloc(data_size/2);
			for (size_t i = 0; i < data_size/2; ++i)
			{
				phasors[i] = polar(1., -2.*pi()*(double)i/(double)data_size);
			}
			buffer.realloc(data_size);
			rev_index_table = MakeRevIndex(data_size);
		}
		virtual size_t CeilFFTLength(size_t length) override
		{
			return FFTv0_ceil_fft_length(length);
		}
		virtual void DoOne(ComplexFunctionF32 &data, ftDirection direction) override
		{
			FFTv0c_float(data.data(), data.size(), direction, phasors.data(), buffer.data(),
					rev_index_table.data());
		}
	private:
		ComplexFunctionF32 phasors;
		ComplexFunctionF32 buffer;
		DataArray<size_t> rev_index_table;
};

//--------------------------------------------------------------

class FFTCooleyTukeyTransformProcessor: public TransformProcessor<>
{
	public:
		static constexpr size_t base = 2;

		FFTCooleyTukeyTransformProcessor(size_t data_size):
			phasors(base, GetPower(data_size)),
			transformer(&phasors)
		{}
		virtual void Init(size_t data_size) override
		{
			phasors.Initialize(base, GetPower(data_size));
		}
		virtual size_t CeilFFTLength(size_t length) override
		{
			return phasors.ceil_fft_length(length);
		}
		virtual void DoOne(ComplexFunctionF32 &data, ftDirection direction) override
		{
			transformer.FFT(data.data(), data.size(), direction);
		}
	private:
		size_t GetPower(size_t data_size)
		{
			size_t	order = logn(data_size, base);
			if(pow(base, order) < data_size)
				++order;
			return order;
		}
	private:
		CooleyTukeyFFT::Phasors<CooleyTukeyFFT::phasor_value_type> phasors;
		CooleyTukeyFFT::Transformer<complexF32, CooleyTukeyFFT::phasor_value_type> transformer;
};

//--------------------------------------------------------------

template <class complex_t = complexF32>
class FTTransformProcessor: public TransformProcessor<complex_t>
{
	public:
		PARENT(TransformProcessor<complex_t>);
		using typename parent::ComplexFunction_t;

		virtual void Init(size_t data_size) override
		{
		}
		virtual size_t CeilFFTLength(size_t length) override
		{
			return length;
		}
		virtual void DoOne(ComplexFunction_t &data, ftDirection direction) override
		{
			FT(data, direction);
		}
};

//--------------------------------------------------------------

class ThreadData
{
	public:
		ThreadData(
				function<shared_ptr<TransformProcessor<>> ()> *tr_creator,
				const ComplexFunctionF32 *source,
				ftDirection tr_direction,
				size_t n_rep,
				condition_variable *start_cv,
				mutex *start_cv_mutex,
				atomic<bool> *break_flag):
			tr_creator(tr_creator),
			source(source),
			tr_direction(tr_direction),
			n_rep(n_rep),
			start_cv(start_cv),
			start_cv_mutex(start_cv_mutex),
			break_flag(break_flag),
			thr([this]() { run(); })
		{
		}

	private:
		function<shared_ptr<TransformProcessor<>> ()> *tr_creator;
		const ComplexFunctionF32 *source;
		ftDirection tr_direction;
		size_t n_rep;
	public:
		bool ready = false;
		mutex ready_mutex;
		condition_variable ready_cv;
	private:
		condition_variable *start_cv;
		mutex *start_cv_mutex;
		atomic<bool> *break_flag;
	public:
		bool do_start = false;
		atomic<size_t> counter = 0;
		thread thr;
		double time = 0;
		bool error = false;

	private:
		void run();
		void do_run();
};

void ThreadData::run()
{
	ThreadSetup ts; (void)ts;
	try
	{
		do_run();
	}
	catch (...)
	{
		*break_flag = true;
		error = true;
		ready_cv.notify_all();
	}
}

void ThreadData::do_run()
{
	auto tr_processor = (*tr_creator)();
	tr_processor->Init(source->size());
	ComplexFunctionF32 result(source->size());
	{
		lock_guard<mutex> lock(ready_mutex);
		ready = true;
	}
	ready_cv.notify_all();
	{
		unique_lock<mutex> start_cv_lock(*start_cv_mutex);
		start_cv->wait(start_cv_lock, [this]() { return do_start || *break_flag; });
	}
	double t0 = GetPerformanceCounterMSec();
	for (size_t i = 0; i < n_rep; ++i)
	{
		result.CopyData(*source);
		tr_processor->DoOne(result, tr_direction);
		++counter;
		if (*break_flag)
			break;
	}
	double t1 = GetPerformanceCounterMSec();
	time = t1 - t0;
}

//--------------------------------------------------------------
void TestFFTSpeedOne(const shared_ptr<TransformProcessor<>> &tr_processor,
		const ComplexFunctionF32 &source,
		size_t n_rep,
		ftDirection tr_direction,
		const wchar_t *test_name)
{
	tr_processor->Init(source.size());
	ComplexFunctionF32 result(source.size());
	GUIProgressBar pb;
	pb.start(L"Тестирование", n_rep);
	double t0 = GetPerformanceCounterMSec();
	for (size_t i = 0; i < n_rep; ++i)
	{
		result.CopyData(source);
		tr_processor->DoOne(result, tr_direction);
		++pb;
	}
	double t1 = GetPerformanceCounterMSec();
	pb.end();

	ShowString(L"FFT", ssprintf(
			L"Название теста: %ls\n"
			L"Направление преобразования: %ls\n"
			L"Количество отсчетов: %zu\n"
			L"Количество повторов: %zu\n"
			L"Полное время: %lf s\n"
			L"Время одного преобразования: %lf us",
			EnsureType<const wchar_t*>(test_name),
			EnsureType<const wchar_t*>(tr_direction == ftForward? L"Forward": L"Reverse"),
			EnsureType<size_t>(source.size()),
			EnsureType<size_t>(n_rep),
			EnsureType<double>(0.001*(t1-t0)),
			EnsureType<double>(1000.*(t1-t0)/n_rep)));
}

//--------------------------------------------------------------

void TestFFTSpeedN(function<shared_ptr<TransformProcessor<>> ()> tr_creator,
		const ComplexFunctionF32 &source,
		size_t n_rep,
		size_t n_threads,
		ftDirection tr_direction,
		const wchar_t *test_name)
{
	if (n_threads < 1)
		return;
	condition_variable start_cv;
	mutex start_cv_mutex;
	atomic<bool> break_flag = false;
	list<ThreadData> threads;
	bool global_error = false;
	try
	{
		for (size_t i = 0; i < n_threads; ++i)
		{
			threads.emplace_back(
					&tr_creator,
					&source,
					tr_direction,
					n_rep,
					&start_cv,
					&start_cv_mutex,
					&break_flag);
		}
		for (auto &td: threads)
		{
			unique_lock<mutex> lock(td.ready_mutex);
			td.ready_cv.wait(lock, [ready = &td.ready, &break_flag]()
					{
						return ready || break_flag;
					});
		}
		{
			lock_guard<mutex> lock(start_cv_mutex);
			for (auto &td: threads)
			{
				td.do_start = true;
			}
		}
		start_cv.notify_all();

		size_t total_count = n_rep * n_threads;
		GUIRandomProgressBar pb;
		pb.start(L"Тестирование");
		for (;;)
		{
			if (break_flag)
				break;
			size_t counter = 0;
			for (auto &td: threads)
			{
				counter += td.counter;
			}
			pb.set_position(double(counter) / total_count);
			if (counter == total_count)
				break;
			this_thread::sleep_for(100ms);
		}
		pb.end();
	}
	catch (...)
	{
		global_error = true;
		break_flag = true;
		start_cv.notify_all();
	}
	for (auto &td: threads)
	{
		td.thr.join();
	}
	bool init = false;
	double t_total = 0;
	double t_min = 0, t_max = 0;
	size_t count_total = 0;
	size_t count_min = 0, count_max = 0;
	size_t error_count = global_error? 1: 0;
	for (auto &td: threads)
	{
		double t = td.time;
		t_total += t;
		if (!init || t < t_min)
			t_min = t;
		if (!init || t > t_max)
			t_max = t;
		size_t count = td.counter;
		count_total += count;
		if (!init || count < count_min)
			count_min = count;
		if (!init || count > count_max)
			count_max = count;
		if (td.error)
			++error_count;
		init = true;
	}
	ShowString(L"FFT Multithreaded",
			ssprintf(L"Название теста: %ls\n",
					EnsureType<const wchar_t*>(test_name)) +
			ssprintf(L"Направление преобразования: %ls\n",
					EnsureType<const wchar_t*>(tr_direction == ftForward? L"Forward": L"Reverse")) +
			ssprintf(L"Количество отсчетов: %zu\n",
					EnsureType<size_t>(source.size())) +
			ssprintf(L"Количество повторов: %zu\n",
					EnsureType<size_t>(n_rep)) +
			ssprintf(L"Количество потоков: %zu\n",
					EnsureType<size_t>(n_threads)) +
			ssprintf(
					L"Общее количество итераций фактическое / планируемое: %zu / %zu (%.3lf%%)\n",
					EnsureType<size_t>(count_total),
					EnsureType<size_t>(n_rep*n_threads),
					EnsureType<double>(100.*double(count_total)/(n_rep*n_threads))) +
			ssprintf(L"Полное время: %lf s\n",
					EnsureType<double>(0.001*t_total)) +
			ssprintf(L"Среднее время на поток: %lf s\n",
					EnsureType<double>(0.001*t_total/n_threads)) +
			ssprintf(L"Min/max время на поток: %lf s / %lf s\n",
					EnsureType<double>(0.001*t_min),
					EnsureType<double>(0.001*t_max)) +
			ssprintf(L"Относительная разница времени на поток: %lf s / %lf%% / ~%lf итераций\n",
					EnsureType<double>(0.001*(t_max - t_min)),
					EnsureType<double>(200.*(t_max - t_min)/(t_max + t_min)),
					EnsureType<double>(n_rep * 2.*(t_max - t_min)/(t_max + t_min))) +
			ssprintf(L"Среднее время одного преобразования: %lf us\n",
					EnsureType<double>(1000.*t_total/count_total)) +
			ssprintf(L"Количество ошибок: %zu\n",
					EnsureType<size_t>(error_count)) +
			ssprintf(L"Фактическое количество повторов min/max: %zu / %zu\n",
					EnsureType<size_t>(count_min),
					EnsureType<size_t>(count_max)));
}
//--------------------------------------------------------------

void DoTestFFT()
{
	using namespace DynamicDialog;
	enum class Choices { Close, TestSpeed, TestAccurary };
	auto dialog = EnumDialog::Create(L"Тест производительности FFT",
			{
				MakeButton(L"Тест скорости", Choices::TestSpeed),
				MakeButton(L"Тест точности", Choices::TestAccurary),
				MakeButton(L"Закрыть", Choices::Close).SetCancel()
			},
			SavedGUIValue(Choices::TestSpeed),
			Choices::Close);
	size_t data_size = 512;
	dialog->CreateControl<ValueNumberEdit<size_t>>(L"Количество отсчетов", SavedGUIValue(&data_size),
			1, numeric_limits<size_t>::max(), Layout::Horizontal);
	size_t n_rep = 1000;
	dialog->CreateControl<ValueNumberEdit<size_t>>(L"Количество повторов", SavedGUIValue(&n_rep),
			1, numeric_limits<size_t>::max(), Layout::Horizontal);
	size_t n_threads = 1;
	dialog->CreateControl<ValueNumberEdit<size_t>>(L"Количество потоков", SavedGUIValue(&n_threads),
			1, numeric_limits<size_t>::max(), Layout::Horizontal);
	enum class TransformVersion { FFT,
#ifdef XRADImmediateTest_USE_ASM
		FFTv0asm,
#endif // XRADImmediateTest_USE_ASM
		FFTv0c, FFTCooleyTukey, FT, Null };
	TransformVersion tr_version = TransformVersion::FFT;
	dialog->AddControl(EnumRadioButtonChoice::Create(L"Алгоритм",
			{
				MakeButton(L"FFT (текущий)", TransformVersion::FFT),
#ifdef XRADImmediateTest_USE_ASM
				MakeButton(L"FFTv0 (ASM)", TransformVersion::FFTv0asm),
#endif // XRADImmediateTest_USE_ASM
				MakeButton(L"FFTv0 (C++)", TransformVersion::FFTv0c),
				MakeButton(L"Cooley-Tukey FFT", TransformVersion::FFTCooleyTukey),
				MakeButton(L"\"Slow\" FT", TransformVersion::FT),
				MakeButton(L"Null", TransformVersion::Null),
			},
			SavedGUIValue(&tr_version)));
	ftDirection tr_direction = ftForward;
	dialog->AddControl(EnumRadioButtonChoice::Create(L"Направление FFT",
			{
				MakeButton(L"Forward", ftForward),
				MakeButton(L"Reverse", ftReverse),
			},
			SavedGUIValue(&tr_direction)));

	for (;;)
	{
		dialog->Show();
		auto dialog_choice = dialog->Choice();
		if (dialog_choice == Choices::Close)
			break;

		try
		{
			function<shared_ptr<TransformProcessor<>> ()> tr_creator;
			const wchar_t *test_name = L"???";
			switch (tr_version)
			{
				case TransformVersion::FFT:
					tr_creator = []() { return make_shared<FFTTransformProcessor<>>(); };
					test_name = L"FFT";
					break;
#ifdef XRADImmediateTest_USE_ASM
				case TransformVersion::FFTv0asm:
					tr_creator = []() { return make_shared<FFTv0asmTransformProcessor>(); };
					test_name = L"FFTv0 (ASM)";
					break;
#endif // XRADImmediateTest_USE_ASM
				case TransformVersion::FFTv0c:
					tr_creator = []() { return make_shared<FFTv0cTransformProcessor>(); };
					test_name = L"FFTv0 (C++)";
					break;
				case TransformVersion::FFTCooleyTukey:
					tr_creator = [data_size]()
						{
							return make_shared<FFTCooleyTukeyTransformProcessor>(data_size);
						};
					test_name = L"FFT Cooley-Tukey";
					break;
				case TransformVersion::FT:
					tr_creator = []() { return make_shared<FTTransformProcessor<>>(); };
					test_name = L"FT (\"slow\")";
					break;
				case TransformVersion::Null:
					tr_creator = []() { return make_shared<NullTransformProcessor>(); };
					test_name = L"Null";
					break;
			}
			if (!tr_creator)
			{
				Error(L"Не инициализирован обработчик теста.");
				continue;
			}

			auto tr_processor = tr_creator();
			data_size = tr_processor->CeilFFTLength(data_size);

			ComplexFunctionF32 source(data_size, complexF32(0));
			for (size_t i = 0; i < source.size(); ++i)
			{
				source[i] = polar(1., 1 + double(i)/double(source.size()));
			}

			switch (dialog_choice)
			{
				case Choices::TestSpeed:
				{
					if (n_threads <= 1)
					{
						TestFFTSpeedOne(tr_processor, source, n_rep, tr_direction, test_name);
					}
					else
					{
						TestFFTSpeedN(tr_creator, source, n_rep, n_threads, tr_direction, test_name);
					}
					break;
				}
				case Choices::TestAccurary:
				{
					DisplayMathFunction(source, 0, 1, L"Source");

					shared_ptr<TransformProcessor<complexF64>> sample_tr_processor =
							make_shared<FFTTransformProcessor<complexF64>>();
					if (sample_tr_processor->CeilFFTLength(data_size) != data_size)
					{
						sample_tr_processor = make_shared<FTTransformProcessor<complexF64>>();
					}
					sample_tr_processor->Init(data_size);
					ComplexFunctionF64 sample_result(source);
					sample_tr_processor->DoOne(sample_result, tr_direction);
					DisplayMathFunction(sample_result, 0, 1, L"Sample result");

					tr_processor->Init(data_size);
					ComplexFunctionF32 result(source);
					tr_processor->DoOne(result, tr_direction);
					DisplayMathFunction(result, 0, 1, L"Result");

					ComplexFunctionF64 diff(result);
					diff -= sample_result;
					auto max_diff = MaxValueTransformed(diff, Functors::absolute_value());
					auto max_source = MaxValueTransformed(source, Functors::absolute_value());
					auto max_sample_result = MaxValueTransformed(sample_result, Functors::absolute_value());
					auto max_result = MaxValueTransformed(result, Functors::absolute_value());
					ShowText(L"Diff", ssprintf(
							L"Diff: %lg\n"
							L"Max source value: %lg\n"
							L"Max spectrum value (sample): %lg\n"
							L"Max spectrum value (tested): %lg\n",
							EnsureType<double>(max_diff),
							EnsureType<double>(max_source),
							EnsureType<double>(max_sample_result),
							EnsureType<double>(max_result)),
							false);

					DisplayMathFunction(diff, 0, 1, L"Diff");
					break;
				}
			}
		}
		catch (canceled_operation&)
		{
		}
		catch (...)
		{
			Error(GetExceptionStringOrRethrow());
		}
	}
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestFFT()
{
	DoTestFFT();
}

//--------------------------------------------------------------
