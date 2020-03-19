// file TestThreads.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestThreads.h"
#include <XRAD/GUI/DynamicDialog.h>
#include <XRADBasic/Sources/Utils/ProcessorPoolDispatcher.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <list>

//--------------------------------------------------------------

XRAD_BEGIN

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

class Log
{
	public:
		Log()
		{
			stream = fopen("c:\\temp\\test_threads.txt", "wb");
			if (!stream)
				stream = stdout;
		}
		~Log()
		{
			if (stream != stdout)
			{
				fclose(stream);
			}
		}
		void Write(const string &str)
		{
			unique_lock<mutex> lock(mx);
			fprintf(stream, "%s", str.c_str());
		}
	private:
		FILE *stream;
		mutex mx;
};

//--------------------------------------------------------------

class Worker
{
	public:
		Worker(chrono::milliseconds timeout, size_t worker_id, Log *log): timeout(timeout), worker_id(worker_id), log(log) {}
		void Work(size_t debug_thread_id);
		size_t WorkerId() const { return worker_id; }
	private:
		chrono::milliseconds timeout;
		size_t worker_id;
		Log *log;
};

void Worker::Work(size_t debug_thread_id)
{
	if (log)
		log->Write(ssprintf("Worker[%zu]::Work(): id = %zu begin\n", worker_id, debug_thread_id));
	if (timeout.count())
		this_thread::sleep_for(timeout);
	if (log)
		log->Write(ssprintf("Worker[%zu]::Work(): id = %zu end\n", worker_id, debug_thread_id));
}

//--------------------------------------------------------------

/*!
	\brief Тестовый класс многопоточного менеджера обработчиков. Реализация №1

	Недостатки:

	- Принципиальный недостаток: плохая балансировка запросов на обработку из разных потоков.
	Если число обработчиков меньше числа потоков, которые к ним обращаются,
	и в каждом потоке между последовательными обращениями к обработчикам проходит
	очень мало времени,
	то потоки, получившие обработчики, после их освобождения при следующем запросе
	с большой вероятностью получат обработчики без ожидания, в то время как потоки,
	находящиеся в ожидании из-за нехватки обработчиков, продолжат ожидание.

	- При поиске свободного обработчика происходит перебор массива
	обработчиков от первого элемента до первого свободного. Операция
	проверки быстрая, но её асимптотическая сложность O(N),
	что плохо с точки зрения масштабируемости по количеству потоков.

	- Выбор свободного обработчика производится в однопоточном режиме.
	Это плохо с точки зрения масштабируемости по количеству потоков.
*/
class MultiWorkerV1Test
{
	public:
		template <class WorkerCreator>
		MultiWorkerV1Test(size_t n_workers, WorkerCreator worker_creator, Log *log);
		void Work(int id);
	private:
		struct WorkItem
		{
			bool busy;
			Worker worker;

			WorkItem(Worker &&worker): busy(false), worker(worker) {}
		};
		WorkItem *GetWorkItem(int id);

		vector<WorkItem> items;
		mutex mx;
		condition_variable cv;
		Log *log;
};

template <class WorkerCreator>
MultiWorkerV1Test::MultiWorkerV1Test(size_t n_workers, WorkerCreator worker_creator, Log *log):
	log(log)
{
	for (size_t i = 0; i < n_workers; ++i)
	{
		items.push_back(WorkItem(worker_creator(i)));
	}
}

void MultiWorkerV1Test::Work(int id)
{
	auto *wi = GetWorkItem(id);

	class busy_unlocker
	{
		public:
			busy_unlocker(bool *busy, mutex *mx, condition_variable *cv): busy(busy), mx(mx), cv(cv) {}
			busy_unlocker(const busy_unlocker &) = delete;
			busy_unlocker &operator= (const busy_unlocker &) = delete;
			~busy_unlocker()
			{
				{
					unique_lock<mutex> lock(*mx);
					*busy = false;
				}
				cv->notify_one();
			}
		private:
			bool *busy;
			mutex *mx;
			condition_variable *cv;
	};

	busy_unlocker bl(&wi->busy, &mx, &cv);
	wi->worker.Work(id);
}

MultiWorkerV1Test::WorkItem *MultiWorkerV1Test::GetWorkItem(int id)
{
	unique_lock<mutex> lock(mx);
	for (;;)
	{
		if (items.empty())
			throw runtime_error("MultiWorkerTest::GetWorkItem(): Work item array is empty.");
		for (auto &item: items)
		{
			if (!item.busy)
			{
				if (log)
					log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %i. Found.\n", id));
				item.busy = true;
				return &item;
				// при возврате выполняется lock.unlock()
			}
		}
		if (log)
			log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %i. Waiting...\n", id));
		cv.wait(lock);
	}
}

//--------------------------------------------------------------

/*!
	\brief Тестовый класс многопоточного менеджера обработчиков. Реализация №2

	По сравнению с \ref MultiWorkerV1Test уменьшена асимптотическая сложность
	поиска свободного обработчика до O(1), но принципиальный недостаток с плохой
	балансировкой запросов сохранился. Также сохранился недостаток с однопоточным
	режимом выбора свободного обработчика.
*/
class MultiWorkerV2Test
{
	public:
		template <class WorkerCreator>
		MultiWorkerV2Test(size_t n_workers, WorkerCreator worker_creator, Log *log);
		void Work(int id);
	private:
		list<Worker>::iterator GetWorkItem(int id);

		list<Worker> free_items;
		list<Worker> busy_items;
		mutex mx;
		condition_variable cv;
		Log *log;
};

template <class WorkerCreator>
MultiWorkerV2Test::MultiWorkerV2Test(size_t n_workers, WorkerCreator worker_creator, Log *log):
	log(log)
{
	for (size_t i = 0; i < n_workers; ++i)
	{
		free_items.push_back(worker_creator(i));
	}
}

void MultiWorkerV2Test::Work(int id)
{
	auto wit = GetWorkItem(id);

	class free_busy_unlocker
	{
		public:
			free_busy_unlocker(list<Worker>::iterator item_it, list<Worker> *free_items, list<Worker> *busy_items,
					mutex *mx, condition_variable *cv):
				item_it(item_it), free_items(free_items), busy_items(busy_items), mx(mx), cv(cv) {}
			free_busy_unlocker(const free_busy_unlocker &) = delete;
			free_busy_unlocker &operator= (const free_busy_unlocker &) = delete;
			~free_busy_unlocker()
			{
				{
					unique_lock<mutex> lock(*mx);
					free_items->splice(free_items->begin(), *busy_items, item_it);
				}
				cv->notify_one();
			}
		private:
			list<Worker>::iterator item_it;
			list<Worker> *free_items, *busy_items;
			mutex *mx;
			condition_variable *cv;
	};

	free_busy_unlocker bl(wit, &free_items, &busy_items, &mx, &cv);
	wit->Work(id);
}

list<Worker>::iterator MultiWorkerV2Test::GetWorkItem(int id)
{
	unique_lock<mutex> lock(mx);
	for (;;)
	{
		if (!free_items.empty())
		{
			auto item_it = free_items.begin();
			busy_items.splice(busy_items.begin(), free_items, item_it);
			if (log)
				log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %i. Found.\n", id));
			return item_it;
			// при возврате выполняется lock.unlock()
		}
		if (busy_items.empty())
			throw runtime_error("MultiWorkerTest::GetWorkItem(): Work item array is empty.");
		if (log)
			log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %i. Waiting...\n", id));
		cv.wait(lock);
	}
}

//--------------------------------------------------------------

/*!
	\brief Тестовый класс многопоточного менеджера обработчиков. Реализация №3

	Входящие запросы обрабатываются в порядке поступления.

	Недостаток: Выбор свободного обработчика производится в однопоточном режиме.
	Это плохо с точки зрения масштабируемости по количеству потоков.
*/
class MultiWorkerV3Test
{
	public:
		template <class WorkerCreator>
		MultiWorkerV3Test(size_t n_workers, WorkerCreator worker_creator, Log *log);
		void Work(size_t debug_thread_id);
	private:
		list<Worker>::iterator GetWorkItem(size_t debug_thread_id);

		list<Worker> free_items;
		list<Worker> busy_items;
		mutex mx;
		struct CVItem
		{
			condition_variable* pcv;
			list<Worker>::iterator *pit;
			size_t debug_thread_id;

			CVItem(condition_variable* pcv, list<Worker>::iterator *pit, size_t debug_thread_id):
				pcv(pcv), pit(pit), debug_thread_id(debug_thread_id) {}
		};
		list<CVItem> cvs;
		Log *log;
};

template <class WorkerCreator>
MultiWorkerV3Test::MultiWorkerV3Test(size_t n_workers, WorkerCreator worker_creator, Log *log):
	log(log)
{
	for (size_t i = 0; i < n_workers; ++i)
	{
		free_items.push_back(worker_creator(i));
	}
}

void MultiWorkerV3Test::Work(size_t debug_thread_id)
{
	auto wit = GetWorkItem(debug_thread_id);

	class free_busy_unlocker
	{
		public:
			free_busy_unlocker(list<Worker>::iterator item_it, list<Worker> *free_items, list<Worker> *busy_items,
					mutex *mx, list<CVItem> *cvs,
					Log *log, size_t debug_thread_id):
				item_it(item_it), free_items(free_items), busy_items(busy_items), mx(mx), cvs(cvs),
				log(log), debug_thread_id(debug_thread_id) {}
			free_busy_unlocker(const free_busy_unlocker &) = delete;
			free_busy_unlocker &operator= (const free_busy_unlocker &) = delete;
			~free_busy_unlocker()
			{
				unique_lock<mutex> lock(*mx);
				if (cvs->empty())
				{
					if (log)
						log->Write(ssprintf("unlocker: [%zu] id = %zu -> free\n", item_it->WorkerId(), debug_thread_id));
					free_items->splice(free_items->begin(), *busy_items, item_it);
				}
				else
				{
					auto &cv_item = cvs->front();
					if (log)
					{
						log->Write(ssprintf("unlocker: [%zu] id = %zu -> id = %zu\n",
								item_it->WorkerId(), debug_thread_id, cv_item.debug_thread_id));
					}
					*cv_item.pit = item_it;
					auto *cv = cv_item.pcv;
					cvs->pop_front(); // cv_item становится недействительным после этого вызова.
					lock.unlock();
					// Последовательность unlock(); notify_one(); предпочтительнее с точки зрения производительности
					// чем обратная последовательность.
					cv->notify_one();
				}
			}
		private:
			list<Worker>::iterator item_it;
			list<Worker> *free_items, *busy_items;
			mutex *mx;
			list<CVItem> *cvs;
			Log *log;
			size_t debug_thread_id;
	};

	free_busy_unlocker bl(wit, &free_items, &busy_items, &mx, &cvs, log, debug_thread_id);
	wit->Work(debug_thread_id);
}

list<Worker>::iterator MultiWorkerV3Test::GetWorkItem(size_t debug_thread_id)
{
	unique_lock<mutex> lock(mx);
	if (!free_items.empty())
	{
		auto item_it = free_items.begin();
		busy_items.splice(busy_items.begin(), free_items, item_it);
		if (log)
			log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %zu. Found free.\n", debug_thread_id));
		return item_it;
		// При возврате выполняется lock.unlock().
	}
	if (busy_items.empty())
		throw runtime_error("MultiWorkerTest::GetWorkItem(): Work item array is empty.");
	if (log)
		log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %zu. Waiting...\n", debug_thread_id));

	condition_variable local_cv;
	auto item_it = busy_items.end();
	cvs.push_back(CVItem(&local_cv, &item_it, debug_thread_id));
	for (;;)
	{
		local_cv.wait(lock);
		if (item_it != busy_items.end()) // Проверка на spurious wakeup.
		{
			// item_it уже находится в списке busy_items.
			if (log)
				log->Write(ssprintf("MultiWorkerTest::GetWorkItem(): id = %zu. Found from queue.\n", debug_thread_id));
			return item_it;
			// При возврате выполняется lock.unlock().
		}
	}
}

//--------------------------------------------------------------

void TestThreadsV3()
{
	size_t num_workers = 3;
	size_t num_threads = 4;
	Log log;
	MultiWorkerV3Test mw(num_workers,
			[&log](size_t worker_id) { return Worker(100ms, worker_id, &log); },
			&log);
	mutex work_mutex;
	bool end_work = false;
	auto work_func = [&mw, &work_mutex, &end_work](size_t debug_thread_id, size_t *counter)
		{
			for (;;)
			{
				mw.Work(debug_thread_id);
				++*counter;
				unique_lock<mutex> lock(work_mutex);
				if (end_work)
					break;
			}
		};
	vector<size_t> counters(num_threads, 0);
	vector<thread> threads;
	threads.reserve(num_threads);
	for (size_t i = 0; i < num_threads; ++i)
	{
		size_t *counter = &counters[i];
		threads.push_back(thread([&work_func, i, counter]() { work_func(i, counter); }));
	}
	ProgressBar progress(GUIProgressProxy());
	progress.start("Testing", 50);
	for (int p = 0; p < 50; ++p)
	{
		this_thread::sleep_for(100ms);
		++progress;
	}
	progress.end();
	{
		unique_lock<mutex> lock(work_mutex);
		end_work = true;
	}
	for (size_t i = 0; i < num_threads; ++i)
		threads[i].join();
	for (size_t i = 0; i < num_threads; ++i)
		log.Write(ssprintf("Counter (id = %zu) = %zu\n", i, counters[i]));
}

//--------------------------------------------------------------

double DelayUsec(int d)
{
	double dt = 0.001 * d;
	auto t0 = GetPerformanceCounterMSec();
	for (;;)
	{
		auto t = GetPerformanceCounterMSec() - t0;
		if (t >= dt || t < 0)
			return t;
	}
}

//--------------------------------------------------------------

void TestPPD()
{
	using namespace DynamicDialog;
	auto dialog = OKCancelDialog::Create(L"Тест производительности многопоточного диспетчера");
	double test_time_sec = 1;
	dialog->CreateControl<ValueNumberEdit<double>>(L"Продолжительность теста, сек.",
			SavedGUIValue(&test_time_sec), 0.001, 1e9, Layout::Horizontal);
	size_t n_threads = 1;
	dialog->CreateControl<ValueNumberEdit<size_t>>(L"Количество потоков", SavedGUIValue(&n_threads),
			1, numeric_limits<size_t>::max(), Layout::Horizontal);
	size_t n_workers = 4;
	dialog->CreateControl<ValueNumberEdit<size_t>>(L"Количество обработчиков",
			SavedGUIValue(&n_workers), 1, numeric_limits<size_t>::max(), Layout::Horizontal);
	int delay_usec = 10;
	dialog->CreateControl<ValueNumberEdit<int>>(L"Длительность операции, мкс", SavedGUIValue(&delay_usec),
			0, 1000000, Layout::Horizontal);
	for (;;)
	{
		dialog->Show();
		auto dialog_choice = dialog->Choice();
		if (dialog_choice != OKCancelDialog::Result::OK)
			break;

		ProcessorPoolDispatcher<function<void ()>> dispatcher(n_workers,
				[delay_usec](size_t)
				{
					return [delay_usec]()
							{
								if (delay_usec)
									DelayUsec(delay_usec);
							};
				});
		atomic<bool> end_work = false;
		vector<size_t> counters(n_threads, 0);
		vector<double> times(n_threads, 0);
		vector<thread> threads;
		threads.reserve(n_threads);
		try
		{
			for (size_t i = 0; i < n_threads; ++i)
			{
				size_t *counter = &counters[i];
				double *time = &times[i];
				threads.push_back(thread([&dispatcher, &end_work, counter, time]()
						{
							double t0 = GetPerformanceCounterMSec();
							for (;;)
							{
								dispatcher.Perform();
								++*counter;
								if (end_work)
									break;
							}
							double t1 = GetPerformanceCounterMSec();
							*time = t1 - t0;
						}));
			}
			GUIRandomProgressBar pb;
			pb.start(L"Testing");
			double t0 = GetPerformanceCounterMSec();
			for (;;)
			{
				this_thread::sleep_for(100ms);
				double t = GetPerformanceCounterMSec();
				double pos = range(0.001*(t - t0), 0, test_time_sec) / test_time_sec;
				pb.set_position(pos);
				if (pos >= 1)
					break;
			}
			pb.end();
		}
		catch (...)
		{
		}
		end_work = true;
		for (auto &thread: threads)
			thread.join();
		size_t total_count = 0;
		double total_time = 0;
		for (size_t i = 0; i < threads.size(); ++i)
		{
			total_count += counters[i];
			total_time += times[i];
		}
		ShowString(L"Производительность",
				ssprintf(L"Среднее количество итераций на поток: %.1lf\n",
						EnsureType<double>(double(total_count)/n_threads)) +
				ssprintf(L"Среднее время на одну итерацию в потоке, мкс: %.3lf\n",
						EnsureType<double>(1000. * total_time / total_count)) +
				ssprintf(L"Среднее время на одну итерацию в очереди, мкс: %.3lf\n",
						EnsureType<double>(1000. * total_time / total_count *
								(n_threads > n_workers ? double(n_workers)/n_threads : 1.))));
		// Результаты на ASUS UX32VD (Core i7: 4 ядра = 2*2, 2.2 ГГц):
		// - Время синхронизации, когда обработчики свободны: ~1 мкс.
		// - Время синхронизации, когда обработчики заняты (8 потоков, 4 обработчика): ~10 мкс.
		// - Для справки: время FFT float длины 512 составляет около 13 мкс.
	}
}

//--------------------------------------------------------------

void TestSyncTimes()
{
	using namespace DynamicDialog;
	auto dialog = OKCancelDialog::Create(L"Тест производительности синхронизации");
	double test_time_sec = 1;
	dialog->CreateControl<ValueNumberEdit<double>>(L"Продолжительность теста, сек.",
			SavedGUIValue(&test_time_sec), 0.001, 1e9, Layout::Horizontal);
	size_t n_thread_groups = 1;
	dialog->CreateControl<ValueNumberEdit<size_t>>(L"Количество групп потоков",
			SavedGUIValue(&n_thread_groups), 1, numeric_limits<size_t>::max(), Layout::Horizontal);
	int delay_usec = 10;
	dialog->CreateControl<ValueNumberEdit<int>>(L"Длительность операции, мкс", SavedGUIValue(&delay_usec),
			0, 1000000, Layout::Horizontal);
	bool do_sync = true;
	dialog->CreateControl<ValueCheckBox>(L"Синхронизировать потоки", SavedGUIValue(&do_sync));
	for (;;)
	{
		dialog->Show();
		auto dialog_choice = dialog->Choice();
		if (dialog_choice != OKCancelDialog::Result::OK)
			break;

		constexpr size_t n_threads = 2;
		atomic<bool> end_work = false;
		vector<size_t> counters(n_threads * n_thread_groups, 0);
		vector<double> times(n_threads * n_thread_groups, 0);
		vector<double> net_times(n_threads * n_thread_groups, 0);
		vector<condition_variable> cvs(n_threads * n_thread_groups);
		vector<int> readies(n_threads * n_thread_groups, 0);
		vector<mutex> mutexes(n_threads * n_thread_groups);
		vector<thread> threads;
		threads.reserve(n_threads * n_thread_groups);
		try
		{
			for (size_t g = 0; g < n_thread_groups; ++g)
			{
				size_t group_i = g * n_threads;
				readies[group_i] = 1;
				cvs[group_i].notify_one();
				for (size_t i = 0; i < n_threads; ++i)
				{
					size_t *counter = &counters[group_i + i];
					double *time = &times[group_i + i];
					double *net_time = &net_times[group_i + i];
					auto *cv = &cvs[group_i + i];
					auto *ready = &readies[group_i + i];
					auto *m = &mutexes[group_i + i];
					auto next_i = group_i + (i+1)%n_threads;
					auto *next_cv = &cvs[next_i];
					auto *next_ready = &readies[next_i];
					auto *next_m = &mutexes[next_i];
					threads.push_back(thread([&end_work, counter, time, net_time, cv, ready, m,
								next_cv, next_ready, next_m,
								delay_usec, do_sync]()
							{
								double t0 = GetPerformanceCounterMSec();
								double tt = 0;
								for (;;)
								{
									if (do_sync)
									{
										unique_lock<mutex> lock(*m);
										cv->wait(lock, [ready, &end_work]() { return *ready || end_work; });
										*ready = 0;
									}
									if (end_work)
										break;

									tt += DelayUsec(delay_usec);
									++*counter;

									if (do_sync)
									{
										{
											lock_guard<mutex> lock(*next_m);
											*next_ready = 1;
										}
										next_cv->notify_one();
									}
								}
								double t1 = GetPerformanceCounterMSec();
								*time = t1 - t0;
								*net_time = tt;
							}));
				}
			}
			GUIRandomProgressBar pb;
			pb.start(L"Testing");
			double t0 = GetPerformanceCounterMSec();
			for (;;)
			{
				this_thread::sleep_for(100ms);
				double t = GetPerformanceCounterMSec();
				double pos = range(0.001*(t - t0), 0, test_time_sec) / test_time_sec;
				pb.set_position(pos);
				if (pos >= 1)
					break;
			}
			pb.end();
		}
		catch (...)
		{
		}
		end_work = true;
		for (size_t i = 0; i < threads.size(); ++i)
		{
			cvs[i].notify_one();
			threads[i].join();
		}
		size_t total_count = 0;
		double total_time = 0;
		double total_net_time = 0;
		for (size_t i = 0; i < threads.size(); ++i)
		{
			total_count += counters[i];
			total_time += times[i];
			total_net_time += net_times[i];
		}
		ShowString(L"Производительность",
				ssprintf(L"Среднее количество итераций на поток: %.1lf\n",
						EnsureType<double>(double(total_count)/threads.size())) +
				ssprintf(L"Среднее время на одну итерацию в потоке, мкс: %.3lf (net: %.3lf)\n",
						EnsureType<double>(1000. * total_time / total_count),
						EnsureType<double>(1000. * total_net_time / total_count)) +
				ssprintf(L"Разница за счет синхронизации, мкс: %.3lf (%.3lf)\n",
						EnsureType<double>(1000. * (total_time - total_net_time) / total_count),
						EnsureType<double>(1000. * (total_time - n_threads*total_net_time) / total_count)) +
				ssprintf(L"Среднее время на одну итерацию в очереди, мкс: %.3lf (net: %.3lf)\n",
						EnsureType<double>(1000. * total_time / total_count / n_threads),
						EnsureType<double>(1000. * total_net_time / total_count / n_threads)) +
				ssprintf(L"Разница за счет синхронизации, мкс: %.3lf (%.3lf)\n",
						EnsureType<double>(1000. * (total_time - total_net_time) / total_count / n_threads),
						EnsureType<double>(1000. * (total_time - n_threads*total_net_time) / total_count / n_threads)));
	}
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestThreads()
{
	for (;;)
	{
		auto action = GetButtonDecision(L"Тип теста",
				{
					MakeButton(L"MultiWorkerV3Test", TestThreadsV3),
					MakeButton(L"ProcessorPoolDispatcher test", TestPPD),
					MakeButton(L"Тест производительности объектов синхронизации", TestSyncTimes),
					MakeButton(L"Отмена", (void (*)())nullptr),
				});
		if (!action)
			break;
		SafeExecute(action);
	}
}

XRAD_END

//--------------------------------------------------------------
