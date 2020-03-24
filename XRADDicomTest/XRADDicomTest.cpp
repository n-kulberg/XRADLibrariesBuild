#include "pre.h"

//#pragma warning(disable:4101)
//#pragma warning(disable:4189)
//#pragma warning(disable:4127)

#include "GenerateFigures.h"
#include "Tests.h"

#include <XRADDicom/XRADDicom.h>
#include <XRADDicomGUI/XRADDicomGUI.h>

#include <iostream>
#include <vld.h>

XRAD_USING

int xrad::xrad_main(int in_argc, char *in_argv[])
{

	try
	{
		printf("command string arguments:\n");
		for(int i = 0; i < in_argc; ++i)
		{
			printf("%d:\t%s\n", i, in_argv[i]);
		}

		fflush(stderr);
		fflush(stdout);

		do
		{
#define only_any_necessary_test 0
#if !only_any_necessary_test

			auto	datasource = []()
			{
				wstring foldername = GetFolderNameRead(L"Get DICOM directory");
//				Dicom::datasource_folder data_src(foldername, YesOrNo("Analyze subdirectories?"));
				constexpr auto default_mode = Dicom::datasource_folder::Mode::Default;
				Dicom::datasource_folder::Mode mode = GetButtonDecision(L"DICOM search mode",
						{
							MakeButton(L"Read and update index", make_fn([]()
									{ return Dicom::datasource_folder::Mode::Index; }))
									.SetDefault(Dicom::datasource_folder::Mode::Index == default_mode),
							MakeButton(L"Don't use index", make_fn([]()
									{ return Dicom::datasource_folder::Mode::NoIndex; }))
									.SetDefault(Dicom::datasource_folder::Mode::NoIndex == default_mode),
							MakeButton(L"Cancel", make_fn([]() -> Dicom::datasource_folder::Mode
									{ throw canceled_operation("Operation canceled."); }))
						})();
				Dicom::datasource_folder result(foldername, true, mode);
				return result;
			};

			auto option = GetButtonDecision("Choose option",
			{
/*
				MakeButton(L"Analyze DICOM open, view, modify, save", make_fn([]()
					{
						AnalyzeDicomDatasource(*GetDicomDataSource(), true, true, true);
					}))
				,*/ MakeButton("Analyze DICOM just view", make_fn([&datasource]()
					{
						AnalyzeDicomDatasource(datasource(), true, false, false);
					}))
				, MakeButton("Analyze DICOM DATASOURCE just view", make_fn([&datasource]()
					{
						Dicom::datasource_pacs data_src(L"10.1.2.155", 5104, L"PACS_AE", L"XRAD_SCU", 104, Dicom::e_request_t::cget);
						AnalyzeDicomDatasource(data_src, true, false, false);
					}))
				, MakeButton("Analyze DICOM just save", make_fn([&datasource]()
					{
						AnalyzeDicomDatasource(datasource(), false, false, true);
					}))
				, MakeButton("Analyze DICOM view save", make_fn([&datasource]()
					{
						AnalyzeDicomDatasource(datasource(), true, false, true);
					}))
				, MakeButton("Generate synthetic DICOM", make_fn([]()
					{
						TestSpongeSimulation();
					}))
				, MakeButton("UUID generation", make_fn([]()
					{
						UUID_generation();
					}))
				, MakeButton("any necessary test", make_fn([]()
					{
#endif
						any_necessary_test();
#if !only_any_necessary_test
					}))
				, MakeButton("Exit", function<void ()>())
			});
			if (!option)
				break;

			try
			{
				option();
			}
			catch (runtime_error &ex)
			{}
			catch(canceled_operation &ex){}
#endif
		} while (true);

	}

	catch(runtime_error &ex){}
	catch(canceled_operation &ex){}

	catch(quit_application &ex)
	{
		cout << "\n" << ex.what() << ", exit code = " << ex.exit_code;
		fflush(stdout);
		return ex.exit_code;
	}

	return EXIT_SUCCESS;
}
