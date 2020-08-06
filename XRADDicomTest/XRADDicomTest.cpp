#include "pre.h"

//#pragma warning(disable:4101)
//#pragma warning(disable:4189)
//#pragma warning(disable:4127)

#include "GenerateFigures.h"
#include "Tests.h"

#include <XRADDicom/XRADDicom.h>
#include <XRADDicomGUI/XRADDicomGUI.h>

#include <iostream>
#ifdef XRAD_COMPILER_MSC
#include <vld.h>
#endif // XRAD_COMPILER_MSC

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
			auto	datasource = []()
			{
				wstring foldername = GetFolderNameRead(L"Get DICOM directory");
//				Dicom::datasource_folder data_src(foldername, YesOrNo("Analyze subdirectories?"));
				constexpr auto default_mode = Dicom::datasource_folder::mode_t::default_mode;
				Dicom::datasource_folder::mode_t mode = GetButtonDecision(L"DICOM search mode",
						{
							MakeButton(L"Read and update index", make_fn([]()
									{ return Dicom::datasource_folder::mode_t::read_and_update_index; }))
									.SetDefault(Dicom::datasource_folder::mode_t::read_and_update_index == default_mode),
							MakeButton(L"Don't use index", make_fn([]()
									{ return Dicom::datasource_folder::mode_t::no_index; }))
									.SetDefault(Dicom::datasource_folder::mode_t::no_index == default_mode),
							MakeButton(L"Cancel", make_fn([]() -> Dicom::datasource_folder::mode_t
									{ throw canceled_operation("Operation canceled."); }))
						})();
				Dicom::datasource_folder result(foldername, true, mode);
				return result;
			};

			auto option = GetButtonDecision("Choose option",
			{
				MakeButton("Analyze DICOM just view", make_fn([&datasource]()
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
						any_necessary_test();
					}))
				, MakeButton("Exit", function<void ()>())
			});
			if (!option)
				break;

			try
			{
				option();
			}
			catch(...)
			{
				Error(GetExceptionStringOrRethrow());
			}
		} while (true);

	}

	catch(runtime_error &){}
	catch(canceled_operation &){}
	catch(quit_application &ex)
	{
		cout << "\n" << ex.what() << ", exit code = " << ex.exit_code;
		fflush(stdout);
		return ex.exit_code;
	}

	return EXIT_SUCCESS;
}
