// file TestFileOperations.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestFileOperations.h"

#include <XRADSystem/CFile.h>
#include <iostream>

//--------------------------------------------------------------

XRAD_BEGIN

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

void TestReadWriteDeleteFiles()
{
	wstring warning = L"--ПРЕДУПРЕЖДЕНИЕ-- --ОСТОРОЖНО--\n"
		"Не использовать этот тест на важных данных!!!\n\n"
		"Этот тест открывает файлы на чтение, запись, удаляет файлы и может привести к необратимым изменениям!!!\n\n"
		"После тестов проверить папки на наличие не удалённых файлов! В случае краха каких-либо тестов удалить оставшиеся файлы руками!\n\n" //todo (Kovbas) нужно сделать возможность автоматического удаления файлов и папок, созданных внутри наших программ

		"Каждый цикл теста выполняет следующие действия:\n"																																																																															  //тест работает следующим образом:
		"1. Создаёт файл через запись и записывает туда что-нибудь небольшое. Если ок, то в отчёт записывается, что запись работает.\n"
		"2. Открывает ранее созданный файл. Если ок, то в отчёт записывается, что чтение работает.\n"
		"3. Удаляет ранее созданный файл. Если ок, то в отчёт записывается, что удаление работает.\n"
		"Если шаг 1 не отработал, то следующие шаги не выполняются.\n\n"

		"Тест должен быть сделан для следующих случаев:\n"
		"а) все папки в пути по 8 символов, чтобы исключить \"усечение\" имён до 8ми символов\n"
		"б) НЕ все папки в пути по 8 символов, чтобы посмотреть как будет работать с \"усечёнными\" путями\n"
		"Эти пункты применить для следующих источников путей:\n"
		"1. короткие пути (до 259 символов)\n"
		"2. для длинных путей (больше 259 символов)\n"
		"3. сетевые пути\n"
		"4. путь на диск, сделанный как subst (нужно ли? Опыты показали, что с этими дисками работает как с обычными. Что подтверждается админской практикой, когда необходимо добраться до длинного пути, то делают несколько subst)\n"
		"5. хард линки (нужно ли?)\n"
		"6. сим линки (нужно ли?)\n\n"

		"Для продолжения нажмите Ctrl+G\n"
		"Для выхода из теста нажмите Ctrl+Q";

	//ShowText(L"Тестирование работы с путями к файлам.", warning);
	TextDisplayer txtDisplayer(L"Тестирование работы с путями к файлам.");
	txtDisplayer.SetText(warning);
	txtDisplayer.Display();

	while (true)
	{
		wstring report;
		auto filePath_info = [&report](const wstring &path, const wstring &what_was_done) -> wstring
		{
			wstring tmpTxt = L"File:\n" + path + L"\nwas " + what_was_done + L".\nFile path length is " + to_wstring(path.size()) + L".";
			report += tmpTxt;
			return tmpTxt;
		};

		wstring	folderPath = GetFolderNameWrite(L"Выберите папку, в которой будет вестись работа с тестовым файлом.");
		wstring fileName = L"tst_file.txt";
		auto path = folderPath + L"/" + fileName;
		try
		{
			shared_cfile fw;
			fw.open(path, L"wt");
			fw.write(fileName.c_str(), fileName.size(), sizeof(*fileName.c_str()));
			fw.close();
			ShowString(L"", filePath_info(path, L"written"));
			report += L"\n";

			fw.open(path, L"rt");
			fw.close();
			ShowString(L"", filePath_info(path, L"openned"));
			report += L"\n";

			if (DeleteFile(path))
				filePath_info(path, L"deleted");
			else
				filePath_info(path, L"not deleted");
		}
		catch (exception &ex)
		{
			report += L"\n" + convert_to_wstring(ex.what());
			DeleteFile(path);
			txtDisplayer.Close();
		}
		ShowString(L"Отчёт о проведённом тесте.", report);
	}

	txtDisplayer.Close();
}

//--------------------------------------------------------------

/*!
	\brief проверка обработки путей для передачи их для отображения пользователю и в системные функции для работы с файлами
*/
void Test_PathPreparing()
{
	auto lambda = [](const list<wstring> &orig_paths)
	{
		size_t i(0);
		wstring msg;
		for (auto el : orig_paths)
		{
			++i;
			msg += L"Original path " + to_wstring(i) + L":\n" + el +
				L"\nHuman readable:\n" + GetPathHumanReadable(el) +
				L"\nMachine readable:\n" + GetPathMachineReadable(el) + L"\n\n";
		}

		ShowText(L"Path processing results:", msg);
	};

	list<wstring> lst;

	lst.clear();
	lst.push_back(L"c:/temp/temp.txt");
	lst.push_back(L"c:/temp//temp.txt");
	lst.push_back(L"//?//c:/temp/temp.txt");
	lst.push_back(L"//?//c:/temp//temp.txt");
	lambda(lst);

	lst.clear();
	lst.push_back(L"c:\\temp\\temp.txt");
	lst.push_back(L"c:\\temp\\\\temp.txt");
	lst.push_back(L"\\\\?\\c:\\temp\\temp.txt");
	lst.push_back(L"\\\\?\\\\c:\\temp\\\\temp.txt");
	lambda(lst);

	lst.clear();
	lst.push_back(L"//srv_name/temp/temp.txt");
	lst.push_back(L"//srv_name/temp//temp.txt");
	lst.push_back(L"/unc/srv_name/temp/temp.txt");
	lst.push_back(L"//unc/srv_name/temp//temp.txt");
	lst.push_back(L"//?/unc/srv_name/temp/temp.txt");
	lst.push_back(L"//?//unc/srv_name/temp//temp.txt");
	lambda(lst);
}

//--------------------------------------------------------------

void TestStandardLocations()
{
	wstring text;
	text += ssprintf(L"WGetApplicationPath: \"%ls\"\n", WGetApplicationPath().c_str());
	text += ssprintf(L"WGetApplicationName: \"%ls\"\n", WGetApplicationName().c_str());
	text += ssprintf(L"WGetApplicationDirectory: \"%ls\"\n", WGetApplicationDirectory().c_str());
	text += ssprintf(L"WGetTempDirectory: \"%ls\"\n", WGetTempDirectory().c_str());
	text += ssprintf(L"WGetCurrentDirectory: \"%ls\"\n", WGetCurrentDirectory().c_str());
	ShowText(L"Standard locations", text);
}

//--------------------------------------------------------------

void TestStdio()
{
	static bool test_stdout = true;
	static bool test_stderr = true;
	static bool test_iostreams = true;
	static bool test_slow_update = true;
	static bool test_cr = true;
	static bool test_speed = false;
	static bool test_unicode_by_parts = true;
	static bool test_no_flush = true;
	vector<wstring> names;
	vector<bool*> values;
	#define check(t, v) names.push_back(t); values.push_back(v);
	check(L"Stdout", &test_stdout)
	check(L"Stderr", &test_stderr)
	check(L"cout, cerr", &test_iostreams)
	check(L"Slow update", &test_slow_update)
	check(L"Обработка '\\r'", &test_cr)
	check(L"Скорость", &test_speed)
	check(L"Unicode по частям", &test_unicode_by_parts)
	check(L"Вывод без flush", &test_no_flush)
	#undef check
	GetCheckboxDecision(L"Выберите тесты", names, values);
	string unicode = convert_to_string(L"Unicode: ABZ_АБЯ_维基百科_\u00C0\u03A3\uFB01\U0001F30D.");
	if (test_stdout)
	{
		fprintf(stdout, "Test for stdout\n%s\n", unicode.c_str());
		fflush(stdout);
	}
	if (test_stderr)
	{
		fprintf(stderr, "Test for stderr\n%s\n", unicode.c_str());
		fflush(stderr);
	}
	if (test_iostreams)
	{
		cout << "Test for cout\n" << unicode << "\n";
		cout.flush();
		cerr << "Test for cerr\n" << unicode << "\n";
		cerr.flush();
	}
	if (test_slow_update)
	{
		fprintf(stdout, "Tesing slow update\n");
		fprintf(stdout, "Paused for 1 second...");
		fflush(stdout);
		Delay(sec(1));
		fprintf(stdout, "OK\n");
		fflush(stdout);
	}
	if (test_cr)
	{
		fprintf(stdout, "Test for \\r:\n");
		for (int i = 0; i < 100; ++i)
		{
			fprintf(stdout, "\ri = %i", i);
			fflush(stdout);
			Delay(sec(0.1));
		}
		fprintf(stdout, "\nOK\n");
	}
	if (test_speed)
	{
		fprintf(stdout, "Speed test:\n");
		for (int i = 0; i < 10000; ++i)
		{
			fprintf(stdout, "\rspeed i = %i", i);
			fflush(stdout);
		}
		fprintf(stdout, "\nOK\n");
	}
	if (test_unicode_by_parts)
	{
		// Этот символ должен кодироваться более чем 1 байтом.
		string unicode_c = convert_to_string(L"\uFB01");
		string unicode_c1 = unicode_c.substr(0, unicode_c.length()/2);
		string unicode_c2 = unicode_c.substr(unicode_c.length()/2);
		fprintf(stdout, "Unicode by parts: \"%s", unicode_c1.c_str());
		fflush(stdout);
		Delay(sec(1));
		fprintf(stdout, "%s\" (\"%s\")\n", unicode_c2.c_str(), unicode_c.c_str());
		fflush(stdout);
	}
	if (test_no_flush)
	{
		if (!test_iostreams)
		{
			fprintf(stdout, "No flush test (stdout).\n");
			fprintf(stderr, "No flush test (stderr).\n");
		}
		else
		{
			cout << "No flush test (cout).\n";
			cerr << "No flush test (cerr).\n";
		}
		Delay(sec(5));
	}
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestIO()
{
	for (;;)
	{
		auto experiment = GetButtonDecision(L"Choose experiment",
		{
			MakeButton(L"Read, Write and Delete Files", make_fn(TestReadWriteDeleteFiles)),
			MakeButton(L"Path preparing test", make_fn(Test_PathPreparing)),
			MakeButton(L"Standard locations test", make_fn(TestStandardLocations)),
			MakeButton(L"Stdio", make_fn(TestStdio)),
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
