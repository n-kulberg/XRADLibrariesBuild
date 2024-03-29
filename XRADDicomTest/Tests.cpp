﻿/*!
	\file
	\date 10/5/2018 6:19:48 PM
	\author Kovbas (kovbas)
*/
#include "pre.h"
#include "Tests.h"

#include <XRADDicom/XRADDicom.h>
#include <XRADDicomGUI/XRADDicomGUI.h>
#include <XRADSystem/CFile.h>

#include <sstream>
#include <string>

#include "RadonTest.h"

XRAD_USING

//	Функция, которая слегка портит томограмму.
//	Тест, что измененные данные действительно сохранены
void ModifyTomogram(TomogramAcquisition &tomogram)
{
	RealFunctionF32 row;
	for (size_t i = 0; i < tomogram.sizes(0); ++i)
	{
		size_t white_row_num = RandomUniformF64(0, tomogram.sizes(1));
		tomogram.slices().GetRow(row, { i, white_row_num, slice_mask(0) });

		row.fill(1000);
	}
}


// исследование источников данных dicom
void AnalyzeDicomDatasource(const Dicom::datasource_t &dcm_datasrc, bool display_loaded_data, bool modify, bool save_to_file)
{
	Dicom::filter_t filter;

	switch (dcm_datasrc.type())
	{
	case Dicom::datasource_t::pacs:
	{
		Dicom::elemsmap_t filter_set;
#if 1
		filter_set.insert({ Dicom::tag_e::e_study_id, L"16228" });
		filter_set.insert({ Dicom::tag_e::e_accession_number, L"30038531" });
		filter_set.insert({ Dicom::tag_e::e_series_number, L"601" });
#else
		filter_set.insert({ Dicom::tag_e::e_study_id, L"9669" });
		filter_set.insert({ Dicom::tag_e::e_accession_number, L"9669" });
		filter_set.insert({ Dicom::tag_e::e_series_number, L"2" });
#endif
		filter.push_back(filter_set);
	}
		break;

	case Dicom::datasource_t::folder:
		break;

	case Dicom::datasource_t::file:
		break;
	}


#if !defined(XRAD_DEBUG)  || 1 //чтобы можно было легко включать и отключать в режиме откладки
	//FilterDicoms(studies_heap, RemoveAuxiliaryTomogramImages());
	//FilterDicoms(studies_heap, KeepOnlyCTFilter());
	//FilterDicoms(studies_heap, RemoveMultiframeFilter());
#endif


	Dicom::patients_loader studies_heap;
// 	studies_heap = GetDicomStudiesHeap(dcm_datasrc, MakeDicomInstanceFilters(filter, modality_is_ct()), GUIProgressProxy());
	studies_heap = GetDicomStudiesHeap(dcm_datasrc, MakeDicomInstanceFilters(), GUIProgressProxy());


	if (studies_heap.empty())
	{
		Error(L"Datasource '" + dcm_datasrc.print() + L"' contains no acquisitions");
	}
	else do
	{
		try
		{
			//выбираем сборку, с которой будем работать
			Dicom::acquisition_loader &chosen_acquisition = SelectSeriesInteractive(studies_heap);

			// создаём переменную для одной сборки и заполняем её
			ProcessAcquisition_ptr proc_acquisition{ CreateProcessAcquisition(chosen_acquisition, GUIProgressProxy()) };
			ProcessAcquisitionOpenClose prcAcq(*proc_acquisition);

			if (display_loaded_data)
				//отображаем созданную сборку
				DisplayProcessAcquisition(*proc_acquisition, L"Original " + proc_acquisition->modality() + L" acquisition");

			if (modify)
			{
				if (Dicom::is_modality_tomogram(proc_acquisition->modality()))
				{
					ModifyTomogram(dynamic_cast<TomogramAcquisition&>(*proc_acquisition));
				}

				DisplayProcessAcquisition(*proc_acquisition, L"Modified tomogram " + proc_acquisition->modality() + L" acquisition");
			}

			if (save_to_file)
			{
				//записывать ли?
				e_saving_decision_options_t saving_decision = e_saving_decision_options_t(GetButtonDecision(L"Write modified series?", saving_decision_options()));
				//saving_decision_options_enum saving_decision = e_save_to_old_file;
				if (saving_decision != e_saving_decision_options_exit)
				{
					wstring folder_path = L"c:/temp/write_dicom";
					//wstring folder_path = GetFolderNameSave(L"Выберите каталог для сохранения обработанной серии");
					//wstring folder_path = chosen_series.front()->id.study_id() + L"_" + string_to_wstring(chosen_series.front()->series_description, e_decode_literals);


					// выбираем кодировать изображение или нет
					Dicom::e_compression_type_t compression = Decide(L"Choose image format", {MakeButton(L"Uncompressed", Dicom::e_uncompressed), MakeButton(L"Compressed", Dicom::e_jpeg_lossless)});

					//auto	common_data_object = chosen_acquisition.front();
					//-common_data_object->collect_data(Dicom::source::e_load_minimal_cache);

					// сохраняем файл
					proc_acquisition->save_to_file(folder_path, saving_decision, compression, GUIProgressProxy());
				}
			}
		}
		catch (canceled_operation &)
		{
			// нажатие кнопки cancel в любом диалоговом окне, включая прогресс.
			// (например, отмена ошибочного выбранной команды без необходимости
			// дожидаться ее завершения)
		}
		catch (quit_application &)
		{
			// команда принудительного выхода из программы
			throw;
		}
		catch (exception &ex)
		{
			Error(ex.what());
		}
	} while (YesOrNo("Analyze other series?", false));
}

//todo (Kovbas) хотелось бы анонимизатор иметь в качестве библиотеки, чтобы можно было использовать её в разных проектах
void AnonymizeDicomFolder(const Dicom::datasource_t &dcm_data_src_p)
{
	Dicom::patients_loader studies_heap;
	studies_heap = GetDicomStudiesHeap(dcm_data_src_p, MakeDicomInstanceFilters(), GUIProgressProxy());
	wstring save_folder_name = GetFolderNameWrite(L"Выберите каталог для сохранения анонимизированных данных");
	if (!DirectoryExists(save_folder_name))
	{
		CreatePath(save_folder_name);
	}
	//Dicom::anonymizer_settings anonymizer_settings;
	//TODO перерабатывать
	// 	Dicom::PatientsProcessorRecursive processor(make_shared<Dicom::InstanceAnonymizer>(save_folder_name, original_folder_name, anonymizer_settings));
	// 	processor.Apply(studies_heap, GUIProgressProxy());
}

vector<Dicom::tag_t> fillTagListToDownload_IDs()
{
	return vector<Dicom::tag_t>
	{
		//IDs
		Dicom::e_patient_id,
			//study IDs
			Dicom::e_study_id,
			Dicom::e_accession_number,
			Dicom::e_study_instance_uid,
			//series IDs
			Dicom::e_series_instance_uid,
			Dicom::e_series_number,
			Dicom::e_modality,
			//instance IDs
			Dicom::e_instance_number,
			Dicom::e_sop_instance_uid
	};
}
void delete_duplicates_tags(vector<Dicom::tag_t> &tagList_p)
{
	//delete duplicates tags
	for (auto el = tagList_p.begin(); el != tagList_p.end(); ++el)
	{
		auto el1 = el;
		++el1;
		while (el1 != tagList_p.end())
		{
			if (*el == *el1) el1 = tagList_p.erase(el1);
			else ++el1;
		}
	}
}
vector<Dicom::tag_t> fillTagListToDownload_burashov()
{
	vector<Dicom::tag_t> tagListToDownload{ fillTagListToDownload_IDs() };

	vector<Dicom::tag_t> vecTmp
	{
		0x00081030, //Study description 0008, 1030 (SPINE)
		0x00180015,//	Body part Examined 0018, 0015 (L SPINE)
		0x00181030,//	ProtocolName 0018, 1030 (Lumbar Spine HCT)
		0x00080100,//	CodeValue 0008, 0100 (KT04)
	};

	tagListToDownload.insert(tagListToDownload.end(), vecTmp.begin(), vecTmp.end());
	delete_duplicates_tags(tagListToDownload);
	return tagListToDownload;
}



void any_necessary_test()
{
#if 0
	//ShowString(L"prompt", L"This functions is empty now. But don't delete it.");
	Dicom::patients_loader studies_heap;
	studies_heap = GetDicomStudiesHeap(Dicom::datasource_folder(wstring(L"D:/tmp/_dcm_tst/multiframe"), false), Dicom::filter_t(), GUIProgressProxy());



#else
	/*
		Получить кучу дайкомов, открыть файлы, забрать из них данные и положить в таблицу, разделённую табами или точкой с запятой
	*/
	Dicom::datasource_folder dcm_data_src{ GetFolderNameRead(L"Get DICOM directory"), YesOrNo("Analyze subdirectories?") };
	wstring report_dst = GetFileNameWrite(L"Choose file to save report");

	std::vector<wstring> filenames = GetDirectoryFiles(dcm_data_src.path()+ L" ", L" ", dcm_data_src.analyze_subfolders(), GUIProgressProxy());
//	std::vector<wstring> subfolders;
//	GetDirectoryFiles
//	GetDicomFolderFileList(subfolders, filenames, dcm_data_src.path(), dcm_data_src.analyze_subfolders(), GUIProgressProxy());
	if (filenames.empty() )
	{
		ShowString(L"Папка пуста", L"Папка не содержит исследования, которые возможно обработать.");
		return;
	}
	//fill tag list to fill a table
	auto tagListToDownload{ fillTagListToDownload_burashov() };

	wstring columns_delimeter{ L"\t" };
	//open file to write
	shared_cfile file(report_dst, L"wb");

	//make header
	auto it_file = filenames.begin();
	for (size_t i = 0; i < filenames.size(); ++i, ++it_file )
	{
		wstring txt{ L"" };
		//auto container = Dicom::MakeContainer(new Dicom::instancestorage_file(*it_file));
		auto container = Dicom::MakeContainer();
		if (!container->try_open_instancestorage(new Dicom::instancestorage_file(*it_file))) continue;
		if (container->is_dicomdir()) continue;
		if (container->is_multiframe()) continue;

		for (auto el : tagListToDownload)
			txt += Dicom::get_tag_as_string(el) + Dicom::get_tagname(el) + columns_delimeter;
		txt += L"path" + columns_delimeter + L"filename";
		txt += L"\n";
		file.write(txt.c_str(), sizeof(wchar_t), txt.size());
		break;
	}

	//fill fields
//	it_fold = subfolders.begin();
	it_file = filenames.begin();
	wstring prompt{L"Collecting tags data"};
	GUIProgressBar progress;
	progress.start(prompt, filenames.size());
	for (size_t i = 0; i < filenames.size(); ++i, ++it_file )
	{
		wstring txt{ L"" };
		auto container = Dicom::MakeContainer();
		if (!container->try_open_instancestorage(new Dicom::instancestorage_file(*it_file))) continue;
		if (container->is_dicomdir()) continue;

		for (auto el : tagListToDownload)
			txt += container->get_wstring(Dicom::tag_e(el))  + columns_delimeter;

		//path and file name
		txt += *it_file + columns_delimeter;
		txt += L"\n";
		file.write(txt.c_str(), sizeof(wchar_t), txt.size());
		++progress;
	}
	progress.end();

	ShowText(L"Process result", convert_to_wstring("The folder ") + dcm_data_src.path()
		+ L" was handled.\nThe report file:\n" + report_dst);

#endif
}
