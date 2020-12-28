// file XRADConsoleTest.cpp
//--------------------------------------------------------------
#include "pre.h"

#include <XRADBasic/Core.h>
#include <XRADSystem/System.h>
#include <XRADSystem/CFile.h>
#include <cstring>
#include <cinttypes>
#if (__cplusplus >= 201703) || (defined(XRAD_COMPILER_MSC) && (XRAD_COMPILER_MSC >= 1924))
#define TEST_FILESYSTEM
#include <filesystem>
#endif

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

void TestFileNamePatternMatch1(const wstring &pat, vector<wstring> &&names_yes,
		vector<wstring> &&names_no,
		bool *global_error)
{
	printf("Filter: \"%s\".\n", EnsureType<const char*>(convert_to_string(pat).c_str()));
	bool result = true;
	FileNamePatternMatch filter(pat);
	for (auto &name: names_yes)
	{
		bool filter_result = filter(name);
		printf("  filter(\"%s\"): %i%s\n",
				EnsureType<const char*>(convert_to_string(name).c_str()),
				filter_result? 1: 0,
				EnsureType<const char*>(filter_result? "": " - ERROR"));
		if (!filter_result)
			result = false;
	}
	for (auto &name: names_no)
	{
		bool filter_result = filter(name);
		printf("  filter(\"%s\"): %i%s\n",
				EnsureType<const char*>(convert_to_string(name).c_str()),
				filter(name)? 1: 0,
				EnsureType<const char*>(!filter_result? "": " - ERROR"));
		if (filter_result)
			result = false;
	}
	if (!result)
		*global_error = true;
}

void TestFileNamePattern()
{
	bool error = false;

	// Пустой фильтр: допустимо любое имя файла.
	TestFileNamePatternMatch1(L"", { L"~" }, {}, &error);

	// Фильтры вида "*." + расширение.
	TestFileNamePatternMatch1(
			L"*.a",
			{ L"f.a", L"f.b.a" },
			{ L".a", L"f.b", L"f", L"f.ab", L"f.a.b", L"" },
			&error);
	TestFileNamePatternMatch1(
			L"*.a.b",
			{ L"f.a.b" },
			{ L".a.b", L"f.a", L"f.b.a", L"f.b", L"f", L"f.ab", L"f.a.bc", L"f.a.b.c", L"" },
			&error);
	TestFileNamePatternMatch1(L" *.a ", { L"f.a" }, { L"f.b", L"" }, &error);
	TestFileNamePatternMatch1(L" *. a ", { L"f. a" }, { L"f.a", L"f.b", L"" }, &error);
	TestFileNamePatternMatch1(L"*.a;*.b", { L"f.a", L"f.b" }, { L"f.c", L"" }, &error);
	TestFileNamePatternMatch1(L" *.a ; *.b ", { L"f.a", L"f.b" }, { L"f.c", L"" }, &error);

	// Фильтр "*.*".
	TestFileNamePatternMatch1(L"*.*", { L"~", L"", L".a" }, {}, &error);
	TestFileNamePatternMatch1(L"*.a;*.*;*.b", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L" *.a ; *.* ; *.b ", { L"~", L"" }, {}, &error);

	// Фильтр "*".
	TestFileNamePatternMatch1(L"*", { L"~", L"", L".a" }, {}, &error);
	TestFileNamePatternMatch1(L"*.a;*;*.b", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L" *.a ; * ; *.b ", { L"~", L"" }, {}, &error);

	// Фильтр "*.".
	TestFileNamePatternMatch1(
			L"*.",
			{ L"f", L"fabc" },
			{ L"f.x", L"", L".a" },
			&error);
	TestFileNamePatternMatch1(
			L"*.a;*.;*.b",
			{ L"f.a", L"f", L"fabc", L"f.b" },
			{ L"f.x", L""},
			&error);
	TestFileNamePatternMatch1(
			L" *.a ; *. ; *.b ",
			{ L"f.a", L"f", L"fabc", L"f.b" },
			{ L"f.x", L""},
			&error);

	// Недопустимые фильтры (игнорируются, получается пустой фильтр).
	TestFileNamePatternMatch1(L"*.a?", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L"*.a*", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L"*.a/", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L"*.a\\", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L" ", { L"~", L"" }, {}, &error);
	TestFileNamePatternMatch1(L"  ", { L"~", L"" }, {}, &error);

	if (error)
		printf("Test failed.\n");
	else
		printf("Test passed.\n");
}

//--------------------------------------------------------------

string TimeToString(const time_t &t)
{
	char str_buf[1024];
	auto local_tm = localtime(&t);
	if (local_tm)
	{
		str_buf[strftime(str_buf, sizeof(str_buf), "%F %T%z", local_tm)] = 0;
		return str_buf;
	}
	return string();
}

//--------------------------------------------------------------

void PrintDirectoryContent(const wstring &path, const DirectoryContentInfo &dir_info, bool detailed)
{
	printf("Directory: \"%s\"\n",
			EnsureType<const char*>(convert_to_string(path).c_str()));
	for (auto &f: dir_info.files)
	{
		if (detailed)
		{
			printf("  file:\n");
			printf("    name = \"%s\"\n", EnsureType<const char*>(convert_to_string(f.filename).c_str()));
			printf("    size = %llu\n", EnsureType<unsigned long long>(f.size));
			printf("    time = %s\n", EnsureType<const char*>(TimeToString(f.time_write).c_str()));
		}
		else
		{
			printf("  file: \"%s\"\n", EnsureType<const char*>(convert_to_string(f.filename).c_str()));
		}
	}
	for (auto &d: dir_info.directories)
	{
		if (detailed)
		{
			printf("  directory:\n");
			printf("    name = \"%s\"\n", EnsureType<const char*>(convert_to_string(d.filename).c_str()));
			printf("    time = %s\n", EnsureType<const char*>(TimeToString(d.time_write).c_str()));
		}
	}
	for (auto &d: dir_info.directories)
	{
		PrintDirectoryContent(path + L"/" + d.filename, d.content, detailed);
	}
}

//--------------------------------------------------------------

#ifdef TEST_FILESYSTEM
void TestFSPathItem(const wstring &wstr)
{
	//auto path = filesystem::path(wstr,
	//		filesystem::path::format::native_format);
	//auto path = filesystem::path(convert_to_string(wstr),
	//		filesystem::path::format::native_format);
	printf("Creating path from (via string8/u8path): \"%s\"...\n",
			EnsureType<const char*>(convert_to_string(wstr).c_str()));
	auto path = filesystem::u8path(convert_to_string8(wstr));
	string s8 = path.u8string();
	printf("Result (via u8): \"%s\".\n",
			EnsureType<const char*>(string8_to_string(s8).c_str()));
}
#endif

//--------------------------------------------------------------

void TestFSPath()
{
#ifdef TEST_FILESYSTEM
	TestFSPathItem(L"ascii");
	TestFSPathItem(L"cyr_АБЯабя");
	TestFSPathItem(L"id_维基百科");
	TestFSPathItem(L"u1F34C_\U0001F34C");
#else
	printf("No support for filesystem. The test is skipped.\n");
#endif
}

//--------------------------------------------------------------

class TestFileSystemUtils
{
public:
	static void fail()
	{
		printf("Test failed.\n");
	}

	void result(bool condition)
	{
		if (!condition)
			failed = true;
	}

	static auto result_str(bool condition) -> const char *
	{
		return condition? "": ": FAILED";
	}

	void test_create_path(const wstring &dir_name)
	{
		printf("CreatePath(\"%s\")...\n",
				EnsureType<const char*>(convert_to_string(dir_name).c_str()));
		bool res = CreatePath(dir_name);
		result(res);
		printf("result = %i%s\n", res? 1: 0, result_str(res));

		printf("DirectoryExists(\"%s\")...\n",
				EnsureType<const char*>(convert_to_string(dir_name).c_str()));
		res = DirectoryExists(dir_name);
		result(res);
		printf("result = %i%s\n", res? 1: 0, result_str(res));
	}

	void test_create_file(const wstring &filename)
	{
		printf("CreateFile(\"%s\")...\n",
				EnsureType<const char*>(convert_to_string(filename).c_str()));
		shared_cfile file(filename, L"wb");
		fprintf(file.c_file(), "Content\n");
		file.flush();
		constexpr file_size_t ExpectedFileSize = 8;
		auto f_sz = filesize(file.c_file());
		result(f_sz == ExpectedFileSize);
		printf("File size: %" PRIu64 "%s\n",
				EnsureType<uint64_t>(f_sz),
				result_str(f_sz == ExpectedFileSize));
		file.close();

		printf("FileExists(\"%s\")...\n",
				EnsureType<const char*>(convert_to_string(filename).c_str()));
		bool res = FileExists(filename);
		result(res);
		printf("result = %i%s\n", res? 1: 0, result_str(res));
	}

	void test_file_exists(const wstring &filename, bool should_exist)
	{
		printf("FileExists(\"%s\")...\n",
				EnsureType<const char*>(convert_to_string(filename).c_str()));
		bool res = FileExists(filename);
		result(res == should_exist);
		printf("result = %i%s\n", res? 1: 0, result_str(res == should_exist));
	}

	void test_delete_file(const wstring &filename)
	{
		printf("DeleteFile(\"%s\")...\n",
				EnsureType<const char*>(convert_to_string(filename).c_str()));
		bool res = DeleteFile(filename);
		result(res);
		printf("result = %i%s\n", res? 1: 0, result_str(res));
	}

	const char *total_result_str() const
	{
		return failed? "FAILED": "auto-OK, check some results manually";
	}

private:
	bool failed = false;
};

void TestFileSystem(const wstring &subdir_test,
		const wstring &subdir_not_existing,
		const wstring &subdir_1_2,
		const wstring &subdir_3,
		const wstring &subdir_4,
		const wstring &f0_1,
		const wstring &f1,
		const wstring &f1_1,
		const wstring &f2,
		const wstring &f3,
		const wstring &f4,
		const wstring &filter)
{
	TestFileSystemUtils t;

	printf("GetApplicationPath()...\n");
	string application_path = GetApplicationPath();
	printf("result = \"%s\"\n",
			EnsureType<const char*>(application_path.c_str()));

	printf("WGetTempDirectory()...\n");
	wstring temp_dir = WGetTempDirectory();
	printf("result = \"%s\"\n",
			EnsureType<const char*>(convert_to_string(temp_dir).c_str()));

	printf("DirectoryExists(TempDir)...\n");
	bool res = DirectoryExists(temp_dir);
	t.result(res);
	printf("result = %i%s\n", res? 1: 0, t.result_str(res));
	if (!res)
	{
		t.fail();
		return;
	}

	t.test_file_exists(temp_dir, false);

	printf("CreateFolder(TempDir, \"%s\")...\n",
			EnsureType<const char*>(convert_to_string(subdir_test).c_str()));
	res = CreateFolder(temp_dir, subdir_test);
	t.result(res);
	printf("result = %i%s\n", res? 1: 0, t.result_str(res));
	if (!res)
	{
		t.fail();
		return;
	}

	wstring test_dir = temp_dir + L"/" + subdir_test;

	wstring test_dir_sub_err = test_dir + L"/" + subdir_not_existing;
	printf("DirectoryExists(\"%s\")...\n",
			EnsureType<const char*>(convert_to_string(test_dir_sub_err).c_str()));
	res = DirectoryExists(test_dir_sub_err);
	t.result(!res);
	printf("result = %i%s\n", res? 1: 0, t.result_str(!res));

	wstring test_dir_sub_1_2 = test_dir + L"/" + subdir_1_2;
	t.test_create_path(test_dir_sub_1_2);

	wstring test_dir_sub_3 = test_dir + L"/" + subdir_3;
	t.test_create_path(test_dir_sub_3);

	wstring test_dir_sub_4 = test_dir + L"/" + subdir_4;
	t.test_create_path(test_dir_sub_4);

	wstring f0 = L"file0.txt";
	wstring filename0 = test_dir + L"/" + f0;
	t.test_create_file(filename0);

	printf("DirectoryExists(\"%s\")...\n",
			EnsureType<const char*>(convert_to_string(filename0).c_str()));
	res = DirectoryExists(filename0);
	t.result(!res);
	printf("result = %i%s\n", res? 1: 0, t.result_str(!res));

	wstring filename0_1 = test_dir + L"/" + f0_1;
	t.test_create_file(filename0_1);

	wstring filename1 = test_dir_sub_1_2 + L"/" + f1;
	t.test_create_file(filename1);

	wstring filename1_1 = test_dir_sub_1_2 + L"/" + f1_1;
	t.test_create_file(filename1_1);

	wstring filename2 = test_dir_sub_1_2 + L"/" + f2;
	t.test_create_file(filename2);

	wstring filename3 = test_dir_sub_3 + L"/" + f3;
	t.test_create_file(filename3);

	wstring filename4 = test_dir_sub_4 + L"/" + f4;
	t.test_create_file(filename4);

	printf("GetDirectoryFilesDetailed(\"%s\", {}, true)...\n",
			EnsureType<const char*>(convert_to_string(test_dir).c_str()));
	auto dir_info = GetDirectoryFilesDetailed(test_dir, {}, true);
	printf("Directory info:\n");
	PrintDirectoryContent(test_dir, dir_info, true);

	printf("GetDirectoryFilesDetailed(\"%s\", \"%s\", true)...\n",
			EnsureType<const char*>(convert_to_string(test_dir).c_str()),
			EnsureType<const char*>(convert_to_string(filter).c_str()));
	dir_info = GetDirectoryFilesDetailed(test_dir, filter, true);
	printf("Directory info:\n");
	PrintDirectoryContent(test_dir, dir_info, false);

	{
		printf("GetDirectoryContent(..., \"%s\", \"%s\")...\n",
				EnsureType<const char*>(convert_to_string(test_dir).c_str()),
				EnsureType<const char*>(convert_to_string(filter).c_str()));
		vector<string> files;
		vector<string> folders;
		GetDirectoryContent(files, folders, convert_to_string(test_dir), convert_to_string(filter));
		printf("result:\n");
		for (auto &n: files)
			printf("File: \"%s\"\n", EnsureType<const char*>(n.c_str()));
		for (auto &n: folders)
			printf("Directory: \"%s\"\n", EnsureType<const char*>(n.c_str()));
	}

	{
		printf("GetDirectoryFiles(\"%s\", \"%s\", true)...\n",
				EnsureType<const char*>(convert_to_string(test_dir).c_str()),
				EnsureType<const char*>(convert_to_string(filter).c_str()));
		auto files = GetDirectoryFiles(convert_to_string(test_dir), convert_to_string(filter), true);
		printf("result:\n");
		for (auto &n: files)
			printf("File: \"%s\"\n", EnsureType<const char*>(n.c_str()));
	}

	t.test_delete_file(filename0);
	t.test_delete_file(filename2);
	t.test_delete_file(filename3);
	t.test_delete_file(filename4);

	printf("Files 0, 2, 3, 4 deleted.\n");

	t.test_file_exists(filename0, false);
	t.test_file_exists(filename2, false);
	t.test_file_exists(filename3, false);
	t.test_file_exists(filename4, false);

	printf("GetDirectoryFilesDetailed(\"%s\", {}, true)...\n",
			EnsureType<const char*>(convert_to_string(test_dir).c_str()));
	dir_info = GetDirectoryFilesDetailed(test_dir, {}, true);
	printf("Directory info:\n");
	PrintDirectoryContent(test_dir, dir_info, false);

	printf("SetCurrentDirectory(\"%s\")...\n",
			EnsureType<const char*>(convert_to_string(test_dir).c_str()));
	res = SetCurrentDirectory(test_dir);
	t.result(res);
	printf("result = %i%s\n", res? 1: 0, t.result_str(res));

	printf("GetCurrentDirectory()...\n");
	wstring cur_dir = WGetCurrentDirectory();
	t.result(cur_dir == test_dir);
	printf("result = \"%s\"%s\n",
			EnsureType<const char*>(convert_to_string(cur_dir).c_str()),
			t.result_str(cur_dir == test_dir));

	printf("Test result: %s.\n",
			EnsureType<const char*>(t.total_result_str()));
}

//--------------------------------------------------------------

void TestFileSystemAscii()
{
	printf("Testing file system (ASCII)...\n");
	TestFileSystem(
			L"xrad_test",
			L"not-existing",
			L"1/2",
			L"u/3",
			L"u/4",
			L"file0_1.txtt",
			L"file_sub1.txt",
			L"file_sub1_1.txtt",
			L"file_sub2",
			L"file_sub3.x.dat",
			L"file_sub4.dat",
			L"*.txt;*.;*.x.dat");
}

//--------------------------------------------------------------

void TestFileSystemUnicode()
{
	printf("Testing file system (Unicode)...\n");
	TestFileSystem(
			L"xrad_test_u",
			L"not-existing",
			L"1/2",
			L"u/3-维基百科",
			L"u/4-\U0001F34C",
			L"file0_1.txtt",
			L"file_sub1.txt",
			L"file_sub1_1.txtt",
			L"file_sub2",
			L"file_sub3_维基百科.x.dat",
			L"file_sub4_\U0001F34C.dat",
			L"*.txt;*.;*.x.dat");
}

//--------------------------------------------------------------

void TestAll()
{
	TestFileNamePattern();
	TestFSPath();
	TestFileSystemAscii();
	TestFileSystemUnicode();
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
	--all - all tests
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
		else if (!strcmp(param_name, "--all"))
		{
			TestAll();
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
