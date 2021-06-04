#include "pre.h"
#include <XRADBasic/Sources/Utils/RadonTransform.h>
#include <XRADDicomGUI/XRADDicomGUI.h>
#include <XRADDicom/XRADDicom.h>

//TODO избыточное включение

XRAD_BEGIN

const double gantry_threshold = -1500;//эмпирическое число такое, что больше гентри (2048), но гарантированно меньше любого искаженного "воздуха"

float air_value = -1000;






auto	LoadOriginal(wstring folder_name)
{	
	wstring root_folder_name = GetFolderNameRead(L"Choose dicom folder");

	Dicom::datasource_folder datasource(root_folder_name, true, Dicom::datasource_folder::mode_t::read_and_update_index);

	auto studies_heap = GetDicomStudiesHeap(datasource, MakeDicomInstanceFilters(), GUIProgressProxy());

	auto acquisition = SelectSeriesInteractive(studies_heap);

	
	ProcessAcquisition_ptr acq{CreateProcessAcquisition(acquisition, GUIProgressProxy())};
	ProcessAcquisitionOpenClose control(*acq);

	DisplayProcessAcquisition(*acq, L"Tomogram");

	auto	&ct = dynamic_cast<CTAcquisition&>(*acq);

	RealFunction2D_F32	slice;
	auto	slices = ct.slices();
	auto	gantry_to_air = [](auto& x){return x = in_range(x, -4000, gantry_threshold) ? air_value : x;};
	ApplyFunction(slices, gantry_to_air);
	return slices;
}

template<class ARR, class FUNCTION>
void	TestRadonTransformCorrectness(const ARR &original, const ARR &radon, const ARR &reverse, FUNCTION f)
{
	size_t	answer = 0;

	while(answer != 4)
	{
		answer = GetButtonDecision("Choose option", {"Original", "Reverse Radon", "Difference", "Radon", "Exit"});
		switch(answer)
		{
			case 0:
				f(original, "Original");
				break;
			case 1:
				f(reverse, "Reverse Radon");
				break;
			case 2:
				f(reverse-original, "Difference");
				break;
			case 3:
				f(radon, "Radon");
				break;
			default:
				break;
		}
	}
}


void CTRadonTest()
{
	InitRadonTransform();

	size_t enlarge_factor = GetUnsigned("Enlarge factor", 2, 1, 16);
	size_t radon_size_factor = 2;

	RealFunction2D_F32 original;

	wstring	folder_name;

	auto original_md = LoadOriginal(folder_name);

	original_md -= air_value;

	original.MakeCopy(original_md.GetSlice({0, slice_mask(0), slice_mask(1)}));


	

	size_t	n_radon_angles = radon_size_factor*original.hsize();
	size_t	n_radon_samples = radon_size_factor*original.vsize();
	size_t	n_slices = original_md.sizes(0);

	RealFunction2D_F32 radon_data(n_radon_angles, n_radon_samples);
	RealFunctionMD_F32 radon_md({n_slices, n_radon_angles, n_radon_samples});

	RealFunctionMD_F32 reverse_md(original_md.sizes());

	GUIProgressBar	pb;
	pb.start("Radon transform", n_slices);
	for(size_t i = 0; i < n_slices; ++i)
	{
		auto original = original_md.GetSlice({i, slice_mask(0), slice_mask(1)});
		auto radon = radon_md.GetSlice({i, slice_mask(0), slice_mask(1)});
		auto reverse = reverse_md.GetSlice({i, slice_mask(0), slice_mask(1)});
		MakeIsotropic(original);
 		RadonTransformForward(radon, original, enlarge_factor, pb.substep(i, i+0.5));
		RadonTransformReverse(reverse, radon, enlarge_factor, pb.substep(i+0.5, i+1));

		if(CapsLock())
		{
			TestRadonTransformCorrectness(original, radon, reverse, [](const auto& arr, string title){DisplayMathFunction2D(arr, title);});
		}

	}
	pb.end();

	TestRadonTransformCorrectness(original_md, radon_md, reverse_md, [](const auto& arr, string title){DisplayMathFunction3D(arr, title);});

}

XRAD_END