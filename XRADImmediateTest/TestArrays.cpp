// file TestArrays.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestArrays.h"
#include "TrackedObject.h"
#include <XRADBasic/Containers.h>
#include <numeric>

XRAD_BEGIN

#pragma warning(disable:4101) // Unreferenced local variable.
#pragma warning(disable:4189) // Local variable is initialized but not referenced

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

void TestArrayCompileTime()
{
	int x[3] = {1, 2, 3};
	DataArray<int> a1(3, 0);
	DataArray<int> a2(&x[0], x+3);
	DataArray<int> a3(x, 3);
	//DataArray<int> a4(1.5, 0); // должно вызвать ошибку компиляции
	//DataArray<int> a5(1.5, (size_t)0); // должно вызвать ошибку компиляции
	//DataArray<int> a5(1.5, 2.5); // должно вызвать ошибку компиляции
}

//--------------------------------------------------------------

void TestArrays1D()
{
	AlgebraicStructures::FieldTraits<int> fti;
//	AlgebraicStructures::FieldTraits<bool const volatile> ftb;
//	AlgebraicStructures::FieldTraits<const bool> ftcb;

//	some<int> value;

	MathFunction<complexF32, double, AlgebraicStructures::FieldTagComplex> f1(100, complexF32(0));
	MathFunction<const complexF32, double, AlgebraicStructures::FieldTagComplex> f1c(100/*, complexF32(0)*/);
	f1 = f1*2;//ok
	f1 = f1*complexF32(2);//ok
//	f1 = f1c*complexF32(2);// ошибка, подхватился скалярный шаблон
//	f1 = f1c%complexF32(2);// ошибка, подхватился скалярный шаблон
//	f1 = f1c*2;// еще ошибка, но какая-то другая. ее распутывал пока.

	RealFunctionF32 x(10, 1.1f);
	RealFunctionF32 y;
	y = x;
	y.fill(5);
	RealFunctionF32 z(x.size());
	double a = 0.1;
	z.mix(x, y, reference_wrapper<double>(a), 0.9);
#if XRAD_CFG_ARRAYS_INTERACTIONS_VER >= 2
	auto co1 = conversion_offset<int32_t, uint8_t>();
	Functors::absolute_value func;
	auto ac = func(complexF32(1, 2));
#endif
	ComplexFunctionF32 f{1, 2};
	int breakpoint = 0;
	{
		DataArray<TrackedObject> fi(8);
		for (size_t i = 0; i < fi.size(); ++i)
			fi[i].value = (int)i;
		auto fi_ref_1 = fi.GetDataFragment(0, fi.size());
		auto fi_ref_it = fi_ref_1.begin();
		auto fi_ref_rev_it = fi_ref_1.rbegin();
		for (size_t i = 0; i < fi.size(); ++i)
		{
			auto correct_value = fi[i].value;
			if (fi_ref_1[i].value != correct_value ||
					fi_ref_it->value != correct_value)
			{
				Error("DataArray access error.");
				break;
			}
			auto correct_rev_value = fi[fi.size() - 1 - i].value;
			if (fi_ref_rev_it->value != correct_rev_value)
			{
				Error("DataArray access error (reverse iterator).");
				break;
			}
			++fi_ref_it;
			--fi_ref_it;
			++fi_ref_it;
			++fi_ref_rev_it;
			--fi_ref_rev_it;
			++fi_ref_rev_it;
		}

		auto fi_ref_2 = fi.GetDataFragment(0, fi.size());
		fi_ref_2.reverse();
		fi_ref_it = fi_ref_2.begin();
		fi_ref_rev_it = fi_ref_2.rbegin();
		for (size_t i = 0; i < fi.size(); ++i)
		{
			auto correct_value = fi[fi.size() - 1 - i].value;
			if (fi_ref_2[i].value != correct_value ||
					fi_ref_it->value != correct_value)
			{
				Error("DataArray::reverse() error.");
				break;
			}
			auto correct_rev_value = fi[i].value;
			if (fi_ref_rev_it->value != correct_rev_value)
			{
				Error("DataArray::reverse() access error (reverse iterator).");
				break;
			}
			++fi_ref_it;
			--fi_ref_it;
			++fi_ref_it;
			++fi_ref_rev_it;
			--fi_ref_rev_it;
			++fi_ref_rev_it;
		}

		auto fi_ref_30 = fi.GetDataFragment(0, fi.size(), 2);
		auto fi_ref_3 = fi_ref_30.GetDataFragment(0, fi_ref_30.size(), 2); // Дважды применяем шаг, отличный от 1
		fi_ref_it = fi_ref_3.begin();
		fi_ref_rev_it = fi_ref_3.rbegin();
		for (size_t i = 0; i < fi_ref_3.size(); ++i)
		{
			auto correct_value = fi[i * 4].value;
			if (fi_ref_3[i].value != correct_value ||
					fi_ref_it->value != correct_value)
			{
				Error("DataArray with step error.");
				break;
			}
			auto correct_rev_value = fi[fi.size() - (i + 1) * 4].value;
			if (fi_ref_rev_it->value != correct_rev_value)
			{
				Error("DataArray with step access error (reverse iterator).");
				break;
			}
			++fi_ref_it;
			--fi_ref_it;
			++fi_ref_it;
			++fi_ref_rev_it;
			--fi_ref_rev_it;
			++fi_ref_rev_it;
		}

		auto fi_ref_40 = fi.GetDataFragment(0, fi.size(), 1); // 0, 1, 2... 7
		fi_ref_40.reverse(); // 7, 6, 5... 0
		auto fi_ref_41 = fi_ref_40.GetDataFragment(0, fi_ref_40.size(), 2); // 7, 5, 3, 1
		fi_ref_41.reverse(); // 1, 3, 5, 7
		auto fi_ref_4 = fi_ref_41.GetDataFragment(0, fi_ref_41.size(), 2); // 1, 5
		fi_ref_4.reverse(); // 5, 1
		fi_ref_it = fi_ref_4.begin();
		fi_ref_rev_it = fi_ref_4.rbegin();
		for (size_t i = 0; i < fi_ref_4.size(); ++i)
		{
			auto correct_value = fi[fi.size() - 3 - i * 4].value;
			if (fi_ref_4[i].value != correct_value ||
					fi_ref_it->value != correct_value)
			{
				Error("DataArray with step::reverse() error.");
				break;
			}
			auto correct_rev_value = fi[i * 4 + 1].value;
			if (fi_ref_rev_it->value != correct_rev_value)
			{
				Error("DataArray with step::reverse() access error (reverse iterator).");
				break;
			}
			++fi_ref_it;
			--fi_ref_it;
			++fi_ref_it;
			++fi_ref_rev_it;
			--fi_ref_rev_it;
			++fi_ref_rev_it;
		}

#ifdef _DEBUG
		// Проверка проверок итераторов
		{
			auto it_1_b = fi_ref_1.begin();
			try
			{
				--it_1_b;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_2_b = fi_ref_2.begin();
			try
			{
				--it_2_b;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_3_b = fi_ref_3.begin();
			try
			{
				--it_3_b;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_4_b = fi_ref_4.begin();
			try
			{
				--it_4_b;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}

			auto it_1_e = fi_ref_1.end();
			try
			{
				++it_1_e;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_2_e = fi_ref_2.end();
			try
			{
				++it_2_e;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_3_e = fi_ref_3.end();
			try
			{
				++it_3_e;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_4_e = fi_ref_4.end();
			try
			{
				++it_4_e;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}

			auto it_1_rb = fi_ref_1.rbegin();
			try
			{
				--it_1_rb;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_2_rb = fi_ref_2.rbegin();
			try
			{
				--it_2_rb;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_3_rb = fi_ref_3.rbegin();
			try
			{
				--it_3_rb;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_4_rb = fi_ref_4.rbegin();
			try
			{
				--it_4_rb;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}

			auto it_1_re = fi_ref_1.rend();
			try
			{
				++it_1_re;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_2_re = fi_ref_2.rend();
			try
			{
				++it_2_re;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_3_re = fi_ref_3.rend();
			try
			{
				++it_3_re;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
			auto it_4_re = fi_ref_4.rend();
			try
			{
				++it_4_re;
				Error(L"Ошибка проверки диапазона итератора.");
			}
			catch(...)
			{}
		}
#endif
	}
	if (TrackedObject::tracker_size())
	{
		Error("DataArray operations caused memory leak.");
	}
	{
		FixedSizeArray<TrackedObject, 8> fi;
		for (size_t i = 0; i < fi.size(); ++i)
			fi[i].value = (int)i;
		auto it = fi.make_iterator(4);
		if (it->value != 4)
		{
			Error("FixesSizeArray::make_iterator error.");
		}
	}
	if (TrackedObject::tracker_size())
	{
		Error("FixedSizeArray operations caused memory leak.");
	}

	{
		// Тест метода GetDataComponent().
		ComplexFunctionF64 g(10);
		int counter = 1;
		for (size_t i = 0; i < g.size(); ++i)
		{
			g[i] = complexF64(counter, 2*counter);
			++counter;
		}
		RealFunctionF64 g_re = g.GetDataComponent<RealFunctionF64>(
				[](complexF64 *data) { return &data->re; });
		g_re.fill(11);
		auto g_im = g.GetDataComponent<RealFunctionF64>([](complexF64 *data) { return &data->im; });
		g_im.fill(12);
		auto g_im_d = g.GetDataComponent([](complexF64 *data) { return &data->im; });
		DataArray<double> *g_im_d_check = &g_im_d;
		g_im_d_check->fill(13);
		const auto &g_const = g;
		auto g_const_re = g_const.GetDataComponent<RealFunctionF64::invariable>(
				[](const complexF64 *data) { return &data->re; });
		auto g_const_im = g_const.GetDataComponent(
				[](const complexF64 *data) { return &data->im; });
		auto re_mean = AverageValue(g_const_re);
		auto im_mean = AverageValue(g_const_im);
		auto g_const_re_2 = g.GetDataComponent([](const complexF64 *data) { return &data->re; });
		// Error:
		// auto g_nonconst_re_2 = g_const.GetDataComponent([](complexF64 *data) { return &data->re; });

		ComplexFunctionF64::invariable g_invar;
		g_invar.UseData(g);
		auto g_invar_im = g_invar.GetDataComponent(
				[](const complexF64 *data) { return &data->im; });
	}
}

//--------------------------------------------------------------

void TestArrays2D()
{
	size_t tracked_count = TrackedObject::tracker_size();
	{
		DataArray2D<DataArray<TrackedObject>> fi01(1, 2);
		for (size_t i = 0; i < fi01.sizes(0); ++i)
		{
			for (size_t j = 0; j < fi01.sizes(1); ++j)
			{
				fi01.at(i, j).value = int(i * 100 + j);
			}
		}
		DataArray2D<DataArray<TrackedObject>> fi02(std::move(fi01));
		DataArray2D<DataArray<TrackedObject>> fi03;
		fi03 = std::move(fi02);

		DataArray2D<DataArray<TrackedObject>> fi(fi03);
		fi.realloc(5, 7);
		for (size_t i = 0; i < fi.sizes(0); ++i)
		{
			for (size_t j = 0; j < fi.sizes(1); ++j)
			{
				fi.at(i, j).value = int(i * 1000 + j);
			}
		}

		{
			auto fi_ref = fi.GetDataFragment(0, 0, fi.sizes(0), fi.sizes(1));
			fi_ref.transpose();
			auto fi_ref_it_1 = fi_ref.begin();
			for (size_t i = 0; i < fi_ref.sizes(0); ++i)
			{
				bool do_break = false;
				auto fi_ref_it_2 = fi_ref_it_1->begin();
				for (size_t j = 0; j < fi_ref.sizes(1); ++j)
				{
					auto correct_value = fi.at(j, i).value;
					if (fi_ref.at(i, j).value != correct_value ||
							fi_ref.row(i)[j].value != correct_value ||
							fi_ref.col(j)[i].value != correct_value ||
							fi_ref_it_2->value != correct_value)
					{
						Error(L"DataArray2D::transpose() error.");
						do_break = true;
						break;
					}
					++fi_ref_it_2;
				}
				if (do_break)
					break;
				++fi_ref_it_1;
				--fi_ref_it_1;
				++fi_ref_it_1;
			}

			fi_ref = fi.GetDataFragment(0, 0, fi.sizes(0), fi.sizes(1));
			fi_ref.hflip();
			fi_ref_it_1 = fi_ref.begin();
			for (size_t i = 0; i < fi_ref.sizes(0); ++i)
			{
				bool do_break = false;
				auto fi_ref_it_2 = fi_ref_it_1->begin();
				for (size_t j = 0; j < fi_ref.sizes(1); ++j)
				{
					auto correct_value = fi.at(i, fi.sizes(1) - 1 - j).value;
					if (fi_ref.at(i, j).value != correct_value ||
							fi_ref.row(i)[j].value != correct_value ||
							fi_ref.col(j)[i].value != correct_value ||
							fi_ref_it_2->value != correct_value)
					{
						Error(L"DataArray2D::hflip() error.");
						do_break = true;
						break;
					}
					++fi_ref_it_2;
				}
				if (do_break)
					break;
				++fi_ref_it_1;
				--fi_ref_it_1;
				++fi_ref_it_1;
			}
#ifdef _DEBUG
			try
			{
				++fi_ref_it_1; // Должен вызвать ошибку.
				Error(L"Проверка диапазона итератора не работает.");
			}
			catch (...)
			{
				// Должны попасть сюда, это норма.
			}
#endif

			fi_ref = fi.GetDataFragment(0, 0, fi.sizes(0), fi.sizes(1));
			fi_ref.vflip();
			fi_ref_it_1 = fi_ref.begin();
			for (size_t i = 0; i < fi_ref.sizes(0); ++i)
			{
				bool do_break = false;
				auto fi_ref_it_2 = fi_ref_it_1->begin();
				for (size_t j = 0; j < fi_ref.sizes(1); ++j)
				{
					auto correct_value = fi.at(fi.sizes(0) - 1 - i, j).value;
					if (fi_ref.at(i, j).value != correct_value ||
							fi_ref.row(i)[j].value != correct_value ||
							fi_ref.col(j)[i].value != correct_value ||
							fi_ref_it_2->value != correct_value)
					{
						Error(L"DataArray2D::vflip() error.");
						do_break = true;
						break;
					}
					++fi_ref_it_2;
				}
				if (do_break)
					break;
				++fi_ref_it_1;
				--fi_ref_it_1;
				++fi_ref_it_1;
			}
#ifdef _DEBUG
			try
			{
				++fi_ref_it_1;
				Error(L"Проверка диапазона итератора не работает.");
			}
			catch (...)
			{
				// Должны попасть сюда, это норма.
			}
#endif
			int breakpoint2 = 0;
		}
		int breakpoint = 0;
	}
	if (TrackedObject::tracker_size() != tracked_count)
	{
		Error("DataArray2D operations: memory leak detected.");
	}

	{
		// Тест метода GetDataComponent().
		ComplexFunction2D_F64 g(10, 15);
		int counter = 1;
		for (size_t i = 0; i < g.sizes(0); ++i)
		{
			for (size_t j = 0; j < g.sizes(1); ++j)
			{
				g.at(i, j) = complexF64(counter, 2*counter);
				++counter;
			}
		}
		RealFunction2D_F64 g_re = g.GetDataComponent<RealFunction2D_F64>(
				[](complexF64 *data) { return &data->re; });
		g_re.fill(11);
		auto g_im = g.GetDataComponent<RealFunction2D_F64>([](complexF64 *data) { return &data->im; });
		g_im.fill(12);
		auto g_im_d = g.GetDataComponent([](complexF64 *data) { return &data->im; });
		DataArray2D<DataArray<double>> *g_im_d_check = &g_im_d;
		g_im_d_check->fill(13);
		const auto &g_const = g;
		auto g_const_re = g_const.GetDataComponent<RealFunction2D_F64::invariable>(
				[](const complexF64 *data) { return &data->re; });
		auto g_const_im = g_const.GetDataComponent(
				[](const complexF64 *data) { return &data->im; });
		auto re_mean = AverageValue(g_const_re);
		auto im_mean = AverageValue(g_const_im);
		auto g_const_re_2 = g.GetDataComponent([](const complexF64 *data) { return &data->re; });
		// Error:
		// auto g_nonconst_re_2 = g_const.GetDataComponent([](complexF64 *data) { return &data->re; });

		ComplexFunction2D_F64::invariable g_invar;
		g_invar.UseData(g);
		auto g_invar_im = g_invar.GetDataComponent(
				[](const complexF64 *data) { return &data->im; });
	}

	{
		RealFunction2D_F32 rf2(3, 4);
		rf2.fill(0);
		rf2.row(0).fill(1);
		rf2.row(1).fill(2);
		CutHistogramEdges(rf2, range1_F64(0.1, 0.1));
		ColorImageI32 image(100, 200);
		image.fill(ColorSampleI32(40, 80, 255));
		DisplayMathFunction2D(image, "image");
	}
}

//--------------------------------------------------------------

void TestArraysMD()
{
	ComplexFunctionMD_F64 f({2, 3, 4, 5});
	index_vector v(4);
	int sum = 0;
	int sum_sq = 0;
	int counter = 1;
	for (size_t i = 0; i < f.sizes(0); ++i)
	{
		v[0] = i;
		for (size_t j = 0; j < f.sizes(1); ++j)
		{
			v[1] = j;
			for (size_t k = 0; k < f.sizes(2); ++k)
			{
				v[2] = k;
				for (size_t l = 0; l < f.sizes(3); ++l)
				{
					v[3] = l;
					sum += counter;
					sum_sq += counter * counter;
					f.at(v) = counter++;
				}
			}
		}
	}
	ComplexFunctionMD_F64 g;
	MakeCopy(g, f);
#if XRAD_CFG_ARRAYS_INTERACTIONS_VER >= 2
	auto sum_g = ElementSum(g);
	auto sum_sq_g = ElementSumTransformed(g, [](const complexF64 &v) { return v * v; });
	g += f;
	auto sum_g2 = ElementSum(g);
	g *= 0.5;
	auto sum_g3 = ElementSum(g);
	index_vector max_pos;
	auto max_val = MaxValueTransformed(f, Functors::real_part(), &max_pos);
	index_vector min_pos;
	auto min_val = MinValueTransformed(f, Functors::real_part(), &min_pos);
#endif
	/*roll(f, offset_vector({1, 0, 0, 0}));
	roll(f, offset_vector({0, 1, 0, 0}));
	roll(f, offset_vector({0, 0, 2, 0}));
	roll(f, offset_vector({0, 0, 0, 3}));
	roll(g, offset_vector({1, 1, 2, 3}));*/

	{
		// Тест метода GetDataComponent().
		RealFunctionMD_F64 g_re = g.GetDataComponent<RealFunctionMD_F64>(
				[](complexF64 *data) { return &data->re; });
		g_re.fill(11);
		auto g_im = g.GetDataComponent<RealFunctionMD_F64>([](complexF64 *data) { return &data->im; });
		g_im.fill(12);
		auto g_im_d = g.GetDataComponent([](complexF64 *data) { return &data->im; });
		DataArrayMD<DataArray2D<DataArray<double>>> *g_im_d_check = &g_im_d;
		g_im_d_check->fill(13);
		const auto &g_const = g;
		auto g_const_re = g_const.GetDataComponent<RealFunctionMD_F64::invariable>(
				[](const complexF64 *data) { return &data->re; });
		auto g_const_im = g_const.GetDataComponent(
				[](const complexF64 *data) { return &data->im; });
		auto re_mean = AverageValue(g_const_re);
		auto im_mean = AverageValue(g_const_im);
		auto g_const_re_2 = g.GetDataComponent([](const complexF64 *data) { return &data->re; });
		// Error:
		// auto g_nonconst_re_2 = g_const.GetDataComponent([](complexF64 *data) { return &data->re; });

		ComplexFunctionMD_F64::invariable g_invar;
		g_invar.UseData(g);
		auto g_invar_im = g_invar.GetDataComponent(
				[](const complexF64 *data) { return &data->im; });
	}

	int breakpoint = 0;
}

//--------------------------------------------------------------

class TestHistogramsHelper
{
	public:
		TestHistogramsHelper()
		{
			array_MD.realloc(index_vector({2, 3, 4}), 0);
			array_MD.GetRow({0, 0, slice_mask(0)}).CopyData(DataArray<double>({-0.9, -0.9, -0.9, -0.9}));
			array_MD.GetRow({0, 1, slice_mask(0)}).CopyData(DataArray<double>({0.1, 0.4, 0.6, 0.9}));
			array_MD.GetRow({0, 2, slice_mask(0)}).CopyData(DataArray<double>({1.1, 1.4, 1.6, 1.9}));
			array_MD.GetRow({1, 0, slice_mask(0)}).CopyData(DataArray<double>({7.001, 9.999, 11.5, 11.5}));
			array_MD.GetRow({1, 1, slice_mask(0)}).CopyData(DataArray<double>({14.1, 14.5, 14.5, 14.9}));
			array_MD.GetRow({1, 2, slice_mask(0)}).CopyData(DataArray<double>({16.5, 16.5, 16.5, 16.5}));

			array_2D.realloc(array_MD.sizes(0)*array_MD.sizes(1), array_MD.sizes(2));
			for (size_t i = 0; i < array_MD.sizes(0); ++i)
			{
				for (size_t j = 0; j < array_MD.sizes(1); ++j)
				{
					size_t row_index = i * array_MD.sizes(1) + j;
					array_2D.row(row_index).CopyData(array_MD.GetRow({i, j, slice_mask(0)}));
				}
			}

			array_1D.realloc(array_2D.sizes(0) * array_2D.sizes(1));
			for (size_t i = 0; i < array_2D.sizes(0); ++i)
			{
				size_t index = i * array_2D.sizes(1);
				array_1D.GetDataFragment(index, index + array_2D.sizes(1)).CopyData(array_2D.row(i));
			}
		}
	private:
		template <class Array>
		void TestHistogramsT(Array array)
		{
			auto h_0_16_16_1 = ComputeHistogramRaw(array, {0, 16}, 16);
			static_assert(std::is_same<decltype(h_0_16_16_1), DataArray<size_t>>::value, "ComputeHistogramRaw: Wrong result type.");
			if (RealFunction<size_t, ptrdiff_t>(h_0_16_16_1) != sample_histogram_0_16_16)
			{
				Error(L"ComputeHistogramRaw: Wrong result values.");
			}
			auto h_0_16_16_2 = ComputeHistogramRaw<RealFunctionUI32>(array, {0, 16}, 16);
			static_assert(std::is_same<decltype(h_0_16_16_2), RealFunctionUI32>::value, "ComputeHistogramRaw: Wrong result type.");
			if (h_0_16_16_2 != sample_histogram_0_16_16)
			{
				Error(L"ComputeHistogramRaw: Wrong result values.");
			}
			auto h_m2_18_20 = ComputeHistogramRaw<RealFunctionUI32>(array, {-2, 18}, 20);
			if (h_m2_18_20 != sample_histogram_m2_18_20)
			{
				Error(L"ComputeHistogramRaw: Wrong result values.");
			}
			auto h_0_16_8 = ComputeHistogramRaw<RealFunctionUI32>(array, {0, 16}, 8);
			if (h_0_16_8 != sample_histogram_0_16_8)
			{
				Error(L"ComputeHistogramRaw: Wrong result values.");
			}
			auto h_m3_13_8 = ComputeHistogramRaw<RealFunctionUI32>(array, {-3, 13}, 8);
			if (h_m3_13_8 != sample_histogram_m3_13_8)
			{
				Error(L"ComputeHistogramRaw: Wrong result values.");
			}
		}
	public:
		void Test()
		{
			TestHistogramsT(array_1D);
			TestHistogramsT(array_2D);
			TestHistogramsT(array_MD);
		}
	private:
		DataArrayMD<DataArray2D<DataArray<double>>> array_MD;
		DataArray2D<DataArray<double>> array_2D;
		DataArray<double> array_1D;
		RealFunctionUI32 sample_histogram_m2_18_20 = {0, 4, 4, 4, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 4, 0, 4, 0};
		RealFunctionUI32 sample_histogram_0_16_16 = {8, 4, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 4, 4};
		RealFunctionUI32 sample_histogram_0_16_8 = {12, 0, 0, 1, 1, 2, 0, 8};
		RealFunctionUI32 sample_histogram_m3_13_8 = {0, 8, 4, 0, 0, 1, 1, 10};
};

//--------------------------------------------------------------

void TestHistograms()
{
	TestHistogramsHelper helper;
	helper.Test();
}

//--------------------------------------------------------------

void TestComplexContainersMD()
{
	ComplexFunctionMD_F32 f({2, 3, 4, 5});
	index_vector v(4);
	int sum = 0;
	int sum_sq = 0;
	int counter = 1;
	for (size_t i = 0; i < f.sizes(0); ++i)
	{
		v[0] = i;
		for (size_t j = 0; j < f.sizes(1); ++j)
		{
			v[1] = j;
			for (size_t k = 0; k < f.sizes(2); ++k)
			{
				v[2] = k;
				for (size_t l = 0; l < f.sizes(3); ++l)
				{
					v[3] = l;
					sum += counter;
					sum_sq += counter * counter;
					f.at(v) = complexF32(counter, 2*counter);
					++counter;
				}
			}
		}
	}
	auto subset = f.GetSubset<ComplexFunctionMD_F32>({0, slice_mask(0), slice_mask(1), slice_mask(2)});
	DisplayMathFunction3D(subset, L"ComplexFunctionMD_F32");
	DisplayMathFunction3D(subset.real(), L"ComplexFunctionMD_F32.re");
}

//--------------------------------------------------------------

void TestComplexContainers()
{
	TestComplexContainersMD();
}

//--------------------------------------------------------------

template <class Array>
void check_sums(const Array &array, double sum, double sum_sq,
		const wstring &err_message)
{
	auto array_sum = ElementSum(array);
	auto array_sum_sq = SquareElementSum(array);
	if (array_sum != sum || array_sum_sq != sum_sq)
	{
		Error(err_message);
	}
}

template <class Array>
void check_rgb_sums(const Array &array, vector<double> sum, vector<double> sum_sq,
		const wstring &err_message)
{
	if (sum.size() != 3 || sum_sq.size() != 3)
		throw invalid_argument("Check_rgb_sums: Invalid sum size.");
	auto array_sum = ElementSum(array);
	auto array_sum_sq = ElementSumTransformed(array, [](const typename Array::value_type &v)
			{ return typename Array::value_type(square(v.red()), square(v.green()), square(v.blue())); });
	vector<wstring> mismatches;
	if (array_sum.red() != sum[0] || array_sum_sq.red() != sum_sq[0])
		mismatches.push_back(L"R");
	if (array_sum.green() != sum[1] || array_sum_sq.green() != sum_sq[1])
		mismatches.push_back(L"G");
	if (array_sum.blue() != sum[2] || array_sum_sq.blue() != sum_sq[2])
		mismatches.push_back(L"B");
	if (mismatches.empty())
		return;
	Error(ssprintf(L"%ls (%ls)",
			EnsureType<const wchar_t*>(err_message.c_str()),
			EnsureType<const wchar_t*>(std::accumulate(mismatches.begin(), mismatches.end(),
					wstring()).c_str())));
}

//--------------------------------------------------------------

namespace ColorContainers
{

using ColorContainer1D_F32 = ColorFunctionF32;

using ColorContainer2D_F32 = ColorImageF32;

using ColorContainerMD_F32 = ColorContainer<MathFunctionMD<ColorContainer2D_F32>, RealFunctionMD_F32>;

} // namespace ColorContainers

//--------------------------------------------------------------

void TestColorContainers1D()
{
	using namespace ColorContainers;

	ColorContainer1D_F32 f(10);
	int i_sum = 0;
	int i_sum_sq = 0;
	int counter = 1;
	for (size_t i = 0; i < f.size(); ++i)
	{
		i_sum += counter;
		i_sum_sq += counter * counter;
		f[i] = ColorSampleF32(counter, 2*counter, 3*counter);
		++counter;
	}
	double d_R_sum = i_sum;
	double d_R_sum_sq = i_sum_sq;
	double d_G_sum = 2*d_R_sum;
	double d_G_sum_sq = 4*d_R_sum_sq;
	double d_B_sum = 3*d_R_sum;
	double d_B_sum_sq = 9*d_R_sum_sq;
	check_rgb_sums(f, {d_R_sum, d_G_sum, d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArray1D operations (1).");

	auto f_red = f.red();
	check_sums(f_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (2.0).");

	f_red.fill(7);
	d_R_sum = 7 * f.element_count();
	d_R_sum_sq = 7*7 * f.element_count();
	check_rgb_sums(f, {d_R_sum, d_G_sum, d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArray1D operations (2.1).");
	check_sums(f_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (2.2).");

	ColorContainer1D_F32::plane_ref f_get_red;
	f.GetRed(f_get_red);
	check_sums(f_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (2.3).");

	const auto &f_const = f;
	auto f_const_red = f_const.red();
	check_sums(f_const_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (3.1-R).");
	auto f_const_green = f_const.green();
	check_sums(f_const_green, d_G_sum, d_G_sum_sq,
			L"Invalid ColorArray1D operations (3.1-G).");
	auto f_const_blue = f_const.blue();
	check_sums(f_const_blue, d_B_sum, d_B_sum_sq,
			L"Invalid ColorArray1D operations (3.1-B).");


	ColorContainer1D_F32::plane_ref_invariable f_const_get_red;
	f_const.GetRed(f_const_get_red);
	f.GetRed(f_const_get_red);
	check_sums(f_const_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (3.2).");

	ColorContainer1D_F32::invariable f_invar;
	f_invar.UseData(f);
	auto f_invar_red = f_invar.red();
	check_sums(f_invar_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (4.1).");

	f_invar.GetRed(f_const_get_red);
	check_sums(f_const_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray1D operations (4.2).");

	inverse(f);

	check_rgb_sums(f, {-d_R_sum, -d_G_sum, -d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArray1D operations (5).");

	inverse(f);

	DisplayMathFunction(f, 0, 1, L"ColorArray1D_F32");
	DisplayMathFunction(f.green(), 0, 1, L"ColorArray1D_F32.green");
	DisplayMathFunction(f.blue(), 0, 1, L"ColorArray1D_F32.blue");

	ColorFunction<const ColorSampleF32, double> const_f;
	const_f.UseData(f);
	auto const_f_g = const_f.green();
}

//--------------------------------------------------------------

void TestColorContainers2D()
{
	using namespace ColorContainers;

	ColorContainer2D_F32 f(10, 15);
	// Массив должен быть небольшой, чтобы суммы квадратов во float считались без потери точности.
	int sum = 0;
	int sum_sq = 0;
	int counter = 1;
	for (size_t i = 0; i < f.sizes(0); ++i)
	{
		for (size_t j = 0; j < f.sizes(1); ++j)
		{
			sum += counter;
			sum_sq += counter * counter;
			f.at(i, j) = ColorSampleF32(counter, 2*counter, 3*counter);
			++counter;
		}
	}
	double d_R_sum = sum;
	double d_R_sum_sq = sum_sq;
	double d_G_sum = 2*d_R_sum;
	double d_G_sum_sq = 4*d_R_sum_sq;
	double d_B_sum = 3*d_R_sum;
	double d_B_sum_sq = 9*d_R_sum_sq;
	check_rgb_sums(f, {d_R_sum, d_G_sum, d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArray2D operations (1).");

	auto f_red = f.red();
	check_sums(f_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (2.0).");

	f_red.fill(7);
	d_R_sum = 7 * f.element_count();
	d_R_sum_sq = 7*7 * f.element_count();
	check_rgb_sums(f, {d_R_sum, d_G_sum, d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArray2D operations (2.1).");
	check_sums(f_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (2.2).");

	ColorContainer2D_F32::plane_ref f_get_red;
	f.GetRed(f_get_red);
	check_sums(f_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (2.3).");

	const auto &f_const = f;
	auto f_const_red = f_const.red();
	check_sums(f_const_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (3.1-R).");
	auto f_const_green = f_const.green();
	check_sums(f_const_green, d_G_sum, d_G_sum_sq,
			L"Invalid ColorArray2D operations (3.1-G).");
	auto f_const_blue = f_const.blue();
	check_sums(f_const_blue, d_B_sum, d_B_sum_sq,
			L"Invalid ColorArray2D operations (3.1-B).");


	ColorContainer2D_F32::plane_ref_invariable f_const_get_red;
	f_const.GetRed(f_const_get_red);
	f.GetRed(f_const_get_red);
	check_sums(f_const_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (3.2).");

	ColorContainer2D_F32::invariable f_invar;
	f_invar.UseData(f);
	auto f_invar_red = f_invar.red();
	check_sums(f_invar_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (4.1).");

	f_invar.GetRed(f_const_get_red);
	check_sums(f_const_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArray2D operations (4.2).");

	inverse(f);

	check_rgb_sums(f, {-d_R_sum, -d_G_sum, -d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArray2D operations (5).");

	inverse(f);

	DisplayMathFunction2D(f, L"ColorImage2D_F32");
	DisplayMathFunction2D(f.green(), L"ColorImage2D_F32.green");
	DisplayMathFunction2D(f.blue(), L"ColorImage2D_F32.blue");

	ColorImage<const ColorSampleF32, double> const_f;
	const_f.UseData(f);
	auto const_f_g = const_f.green();

	int breakpoint_place = 0;
}

//--------------------------------------------------------------

void TestColorContainersMD()
{
	using namespace ColorContainers;

	ColorContainerMD_F32 f({2, 3, 4, 5});
	index_vector v(4);
	int sum = 0;
	int sum_sq = 0;
	int counter = 1;
	for (size_t i = 0; i < f.sizes(0); ++i)
	{
		v[0] = i;
		for (size_t j = 0; j < f.sizes(1); ++j)
		{
			v[1] = j;
			for (size_t k = 0; k < f.sizes(2); ++k)
			{
				v[2] = k;
				for (size_t l = 0; l < f.sizes(3); ++l)
				{
					v[3] = l;
					sum += counter;
					sum_sq += counter * counter;
					f.at(v) = ColorSampleF32(counter, 2*counter, 3*counter);
					++counter;
				}
			}
		}
	}
	double d_R_sum = sum;
	double d_R_sum_sq = sum_sq;
	double d_G_sum = 2*d_R_sum;
	double d_G_sum_sq = 4*d_R_sum_sq;
	double d_B_sum = 3*d_R_sum;
	double d_B_sum_sq = 9*d_R_sum_sq;
	check_rgb_sums(f, {d_R_sum, d_G_sum, d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArrayMD operations (1).");

	auto f_red = f.red();
	check_sums(f_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (2.0).");

	f_red.fill(7);
	d_R_sum = 7 * f.element_count();
	d_R_sum_sq = 7*7 * f.element_count();
	check_rgb_sums(f, {d_R_sum, d_G_sum, d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArrayMD operations (2.1).");
	check_sums(f_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (2.2).");

	ColorContainerMD_F32::plane_ref f_get_red;
	f.GetRed(f_get_red);
	check_sums(f_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (2.3).");

	const auto &f_const = f;
	auto f_const_red = f_const.red();
	check_sums(f_const_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (3.1-R).");
	auto f_const_green = f_const.green();
	check_sums(f_const_green, d_G_sum, d_G_sum_sq,
			L"Invalid ColorArrayMD operations (3.1-G).");
	auto f_const_blue = f_const.blue();
	check_sums(f_const_blue, d_B_sum, d_B_sum_sq,
			L"Invalid ColorArrayMD operations (3.1-B).");


	ColorContainerMD_F32::plane_ref_invariable f_const_get_red;
	f_const.GetRed(f_const_get_red);
	f.GetRed(f_const_get_red);
	check_sums(f_const_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (3.2).");

	ColorContainerMD_F32::invariable f_invar;
	f_invar.UseData(f);
	auto f_invar_red = f_invar.red();
	check_sums(f_invar_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (4.1).");

	f_invar.GetRed(f_const_get_red);
	check_sums(f_const_get_red, d_R_sum, d_R_sum_sq,
			L"Invalid ColorArrayMD operations (4.2).");

	inverse(f);

	check_rgb_sums(f, {-d_R_sum, -d_G_sum, -d_B_sum}, {d_R_sum_sq, d_G_sum_sq, d_B_sum_sq},
			L"Invalid ColorArrayMD operations (5).");

	inverse(f);

	auto subset = f.GetSubset<ColorContainerMD_F32>({0, slice_mask(0), slice_mask(1), slice_mask(2)});
	DisplayMathFunction3D(subset, L"ColorImageMD_F32");
	DisplayMathFunction3D(subset.green(), L"ColorImageMD_F32.green");
	DisplayMathFunction3D(subset.blue(), L"ColorImageMD_F32.blue");

	ColorContainerMD_F32::invariable const_f;
	const_f.UseData(f);
	auto const_f_g = const_f.green();

	int breakpoint_place = 0;
}

//--------------------------------------------------------------

void TestColorContainers()
{
	TestColorContainers1D();
	TestColorContainers2D();
	TestColorContainersMD();
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestArrays()
{
	for (;;)
	{
		auto answer = GetButtonDecision(L"Test arrays",
				{
					MakeButton(L"1D", TestArrays1D),
					MakeButton(L"2D", TestArrays2D),
					MakeButton(L"MD", TestArraysMD),
					MakeButton(L"Histogram", TestHistograms),
					MakeButton(L"Complex", TestComplexContainers),
					MakeButton(L"Color", TestColorContainers),
					MakeButton(L"Cancel", (void (*)())nullptr)
				});
		if (!answer)
			break;
		SafeExecute(answer);
	}
}

//--------------------------------------------------------------

XRAD_END

//--------------------------------------------------------------
