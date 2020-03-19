﻿/*!
	* \file Tests.h
	* \date 10/5/2018 6:19:48 PM
	*
	* \author Kovbas (kovbas)
	*
	* \brief
	*
	* TODO: long description
	*
	* \note
*/
#ifndef Tests_h__
#define Tests_h__

//#include <XRADDicom/DicomClasses/DataContainers/datasource.h>
#include <XRADDicom/XRADDicom.h>

XRAD_USING

void AnalyzeDicomDatasource(const Dicom::datasource_t &dcm_datasrc, bool display_loaded_data = true, bool modify = false, bool save_to_file = false);

void UUID_generation();

void any_necessary_test();

#endif // Tests_h__