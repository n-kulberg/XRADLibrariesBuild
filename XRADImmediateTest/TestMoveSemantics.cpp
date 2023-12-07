#include "pre.h"

#include "TestMoveSemantics.h"

#include <XRADBasic/Sources/Containers/ColorContainer.h>
#include <XRADBasic/BooleanFunctionTypes.h>

XRAD_USING

//--------------------------------------------------------------

template <class Array1D, class OtherArray1D>
pair<bool, string> TestMoveSemanticsHelper1D()
{
	const size_t size = 1024;
	Array1D array00(size);
	//array00 = size;
	//Array1D array01 = size;
	Array1D array1;
	array1.realloc(size);
	void *ptr1 = array1.data();
	Array1D array2c = array1;
	Array1D array2 = std::move(array1);
	void *ptr2 = array2.data();
	Array1D array3c;
	array3c = array2c;
	Array1D array3;
	array3 = std::move(array2);
	Array1D array4c;
	array4c = OtherArray1D(10);
	auto data_ptr = [](const Array1D &a) -> const void* { return a.size()? a.data(): nullptr; };
	bool is_good = ptr1 == ptr2 &&
			data_ptr(array1) == nullptr &&
			data_ptr(array2) == nullptr &&
			data_ptr(array3) == ptr2;
#if 0
	ShowString(L"Moved arrays 2D", ssprintf(
			L"array1 ptr=%p\n"
			L"array2 ptr=%p\n"
			L"array1: %zu (ptr=%p) ->\n"
			L"array2: %zu (ptr=%p) ->\n"
			L"array3: %zu (ptr=%p)",
			ptr1,
			ptr2,
			EnsureType<size_t>(array1.size()), data_ptr(array1),
			EnsureType<size_t>(array2.size()), data_ptr(array2),
			EnsureType<size_t>(array3.size()), data_ptr(array3)));
#endif
	return make_pair(is_good, ssprintf("Move result = %s, type = %s\n",
				is_good? "OK": "Error",
				EnsureType<const char*>(typeid(Array1D).name())));
}

template <class Array1D, class OtherArray1D>
pair<bool, string> TestMoveSemanticsHelper1DM()
{
	Array1D array01;
	(array01 = OtherArray1D(10)) *= 10; // Проверка возвращаемого типа для operator=.
	return TestMoveSemanticsHelper1D<Array1D, OtherArray1D>();
}

bool TestMoveSemantics1D()
{
	bool is_good = true;
	string message;
	auto test = [&is_good, &message](pair<bool, string> test_result)
	{
		if (!test_result.first)
		{
			is_good = false;
		}
		if (message.length())
			message += "\n";
		message += test_result.second;
	};
	using OtherArray1D = DataArray<int>;
	test(TestMoveSemanticsHelper1D<DataArray<int>, OtherArray1D>());
	test(TestMoveSemanticsHelper1D<AlgebraicStructures::GenericFieldElement<DataArray<double>, DataArray<double>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray>, OtherArray1D>());
	test(TestMoveSemanticsHelper1D<AlgebraicStructures::FieldElement<DataArray<double>, DataArray<double>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray, AlgebraicStructures::FieldTagScalar>, OtherArray1D>());
	test(TestMoveSemanticsHelper1D<Algebra1D<DataArray<double>, double, double, AlgebraicStructures::FieldTagScalar>, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<MathFunction<double, double, AlgebraicStructures::FieldTagScalar>, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<RealFunctionF32, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<ComplexFunctionF32, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<ColorFunctionF64, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<LinearVector<double, double, AlgebraicStructures::FieldTagScalar>, LinearVector<float, double, AlgebraicStructures::FieldTagScalar>>());
	test(TestMoveSemanticsHelper1DM<index_vector, index_vector>());
	test(TestMoveSemanticsHelper1DM<offset_vector, offset_vector>());
	test(TestMoveSemanticsHelper1DM<FilterKernelReal, FilterKernelReal>());
	test(TestMoveSemanticsHelper1DM<FilterKernelComplex, FilterKernelComplex>());
	test(TestMoveSemanticsHelper1D<BooleanFunctionLogical8, DataArray<int8_t>>());
	test(TestMoveSemanticsHelper1D<BooleanFunctionBitwise<int8_t>, DataArray<int8_t>>());

	ShowString("Move result 1D", ssprintf("Result = %s\n", is_good? "OK": "Error") + message);
	return is_good;
}



//--------------------------------------------------------------



string to_string_value(int v)
{
	return ssprintf("%i", v);
}

string to_string_value(double v)
{
	return ssprintf("%.3lf", v);
}

template <class PT, class ST>
string to_string_value(complex_sample<PT, ST> v)
{
	return to_string_value(v.re);
}

template <class RGB_TRAITS_T>
string to_string_value(const RGBColorSample<RGB_TRAITS_T> &v)
{
	return to_string_value(v.red());
}

template <class Array2D, class OtherArray2D>
pair<bool, string> TestMoveSemanticsHelper2D()
{
	const size_t base_size = 128;
	const size_t size_h = 2*base_size, size_v = base_size/2;
	Array2D array00(size_v, size_h);
	Array2D array1;
	array1.realloc(size_v, size_h, typename Array2D::value_type(-1));
	array1.at(0,0) = 1;
	array1.at(0,1) = 2;
	array1.at(1,0) = 20;
	const void *ptr1 = array1.data();
	const void *ptr1r = &array1.row(0);
	const void *ptr1c = &array1.col(0);
	Array2D array2c = array1;
	Array2D array2 = std::move(array1);
	const void *ptr2 = array2.data();
	const void *ptr2r = &array2.row(0);
	const void *ptr2c = &array2.col(0);
	Array2D array3c;
	array3c = array2c;
	Array2D array3;
	array3 = std::move(array2);
	Array2D array4c;
	array4c = OtherArray2D(10, 10);
	auto data_ptr = [](const Array2D &a) -> const void* { return !a.empty()? a.data(): nullptr; };
	auto data_ptr_row = [](const Array2D &a) -> const void* { return !a.empty()? &a.row(0): nullptr; };
	auto data_ptr_col = [](const Array2D &a) -> const void* { return !a.empty()? &a.col(0): nullptr; };
	bool is_good =
			ptr1 == ptr2 && ptr1r == ptr2r && ptr1c == ptr2c &&
			data_ptr(array1) == nullptr &&
			data_ptr(array2) == nullptr &&
			data_ptr(array3) == ptr2 && data_ptr_row(array3) == ptr2r && data_ptr_col(array3) == ptr2c;
#if 0
	auto to_string = [](const Array2D &a, size_t v, size_t h)
	{
		if (a.empty())
			return string("?");
		return to_string_value(a.at(v, h));
	};
	auto array_dump = [&data_ptr, &data_ptr_row, &data_ptr_col, &to_string](const Array2D &a) -> string
	{
		return ssprintf("(%zu, %zu) st(%zi, %zi) ptr(%p, %p, %p) [%s, %s, %s]",
				EnsureType<size_t>(a.hsize()),
				EnsureType<size_t>(a.vsize()),
				EnsureType<ptrdiff_t>(a.hstep()),
				EnsureType<ptrdiff_t>(a.vstep()),
				EnsureType<const void*>(data_ptr(a)),
				EnsureType<const void*>(data_ptr_row(a)),
				EnsureType<const void*>(data_ptr_col(a)),
				EnsureType<const char*>(to_string(a,0,0).c_str()),
				EnsureType<const char*>(to_string(a,0,1).c_str()),
				EnsureType<const char*>(to_string(a,1,0).c_str())
				);
	};
	ShowString("Moved arrays 2D", ssprintf(
			"type=%s\n"
			"array1 ptr=%p, %p, %p\n"
			"array2 ptr=%p, %p, %p\n"
			"array1: %s ->\n"
			"array2: %s ->\n"
			"array3: %s",
			EnsureType<const char*>(typeid(Array2D).name()),
			ptr1, ptr1r, ptr1c,
			ptr2, ptr2r, ptr2c,
			EnsureType<const char*>(array_dump(array1).c_str()),
			EnsureType<const char*>(array_dump(array2).c_str()),
			EnsureType<const char*>(array_dump(array3).c_str())));
#endif
	return make_pair(is_good, ssprintf("Move result = %s, type = %s\n",
				is_good? "OK": "Error",
				EnsureType<const char*>(typeid(Array2D).name())));
}

template <class Array2D, class OtherArray2D>
pair<bool, string> TestMoveSemanticsHelper2DM()
{
	Array2D array01;
	(array01 = OtherArray2D(10, 20, 1)) *= 10;
	return TestMoveSemanticsHelper2D<Array2D, OtherArray2D>();
}

bool TestMoveSemantics2D()
{
	bool is_good = true;
	string message;
	auto test = [&is_good, &message](pair<bool, string> test_result)
	{
		if (!test_result.first)
		{
			is_good = false;
		}
		if (message.length())
			message += "\n";
		message += test_result.second;
	};
	using OtherArray2D = DataArray2D<DataArray<int>>;
	test(TestMoveSemanticsHelper2D<DataArray2D<DataArray<int>>, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<AlgebraicStructures::GenericFieldElement<DataArray2D<DataArray<double>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray>, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<AlgebraicStructures::FieldElement<DataArray2D<DataArray<double>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray, AlgebraicStructures::FieldTagScalar>, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<Algebra2D<DataArray2D<DataArray<double>>, DataArray<double>, double, double, AlgebraicStructures::FieldTagScalar>, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<MathFunction2D<RealFunctionF32>, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<RealFunction2D_F32, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<ComplexFunction2D_F32, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<ColorImageF32, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<FIRFilter2DReal, FIRFilter2DReal>());
	test(TestMoveSemanticsHelper2D<FIRFilter2DComplex, FIRFilter2DComplex>());
	test(TestMoveSemanticsHelper2D<BooleanFunctionLogical2D<int8_t>, DataArray2D<DataArray<int8_t>>>());
	test(TestMoveSemanticsHelper2D<BooleanFunctionBitwise2D<int8_t>, DataArray2D<DataArray<int8_t>>>());

	ShowString("Move result 2D", ssprintf("Result = %s\n", is_good? "OK": "Error") + message);
	return is_good;
}



//--------------------------------------------------------------



string to_string_value(const index_vector &v)
{
	string result("(");
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i)
			result += ", ";
		result += ssprintf("%zu", EnsureType<size_t>(v[i]));
	}
	result += ")";
	return result;
}

string to_string_value(const offset_vector &v)
{
	string result("(");
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i)
			result += ", ";
		result += ssprintf("%ti", EnsureType<ptrdiff_t>(v[i]));
	}
	result += ")";
	return result;
}

template <class ArrayMD, class OtherArrayMD>
pair<bool, string> TestMoveSemanticsHelperMD()
{
	const size_t base_size = 4;
	const size_t size_0 = 8*base_size, size_1 = 4*base_size, size_2 = 2*base_size, size_3 = base_size;
	index_vector size {size_0, size_1, size_3, size_3};
	ArrayMD array00(size);
	ArrayMD array1;
	array1.realloc(size, typename ArrayMD::value_type(-1));
	array1.at({0, 0, 0, 0}) = 1;
	array1.at({0, 0, 0, 1}) = 2;
	array1.at({0, 0, 1, 0}) = 3;
	array1.at({0, 1, 0, 0}) = 4;
	array1.at({1, 0, 0, 0}) = 5;
	const void *ptr1 = &array1.at({0, 0, 0, 0});
	ArrayMD array2c = array1;
	ArrayMD array2 = std::move(array1);
	const void *ptr2 = &array2.at({0, 0, 0, 0});
	ArrayMD array3c;
	array3c = array2c;
	ArrayMD array3;
	array3 = std::move(array2);
	//ArrayMD array4c;
	//array4c = DataArrayMD<DataArray2D<DataArray<Array2D::value_type>>>({10, 10, 10, 10});
	auto data_ptr = [](const ArrayMD &a) -> const void* { return !a.empty()? &a.at({0, 0, 0, 0}): nullptr; };
	bool is_good =
			ptr1 == ptr2 &&
			data_ptr(array1) == nullptr &&
			data_ptr(array2) == nullptr &&
			data_ptr(array3) == ptr2;
#if 0
	auto to_string = [](const ArrayMD &a, const index_vector &index)
	{
		if (a.empty())
			return string("?");
		return to_string_value(a.at(index));
	};
	auto array_dump = [&data_ptr, &to_string](const ArrayMD &a) -> string
	{
		return ssprintf("%s st %s ptr(%p) [%s, %s, %s, %s, %s]",
				EnsureType<const char*>(to_string_value(a.sizes()).c_str()),
				EnsureType<const char*>(to_string_value(a.steps()).c_str()),
				EnsureType<const void*>(data_ptr(a)),
				EnsureType<const char*>(to_string(a,{0,0,0,0}).c_str()),
				EnsureType<const char*>(to_string(a,{0,0,0,1}).c_str()),
				EnsureType<const char*>(to_string(a,{0,0,1,0}).c_str()),
				EnsureType<const char*>(to_string(a,{0,1,0,0}).c_str()),
				EnsureType<const char*>(to_string(a,{1,0,0,0}).c_str())
				);
	};
	ShowString("Moved arrays MD", ssprintf(
			"type=%s\n"
			"array1 ptr=%p\n"
			"array2 ptr=%p\n"
			"array1: %s ->\n"
			"array2: %s ->\n"
			"array3: %s",
			EnsureType<const char*>(typeid(ArrayMD).name()),
			ptr1,
			ptr2,
			EnsureType<const char*>(array_dump(array1).c_str()),
			EnsureType<const char*>(array_dump(array2).c_str()),
			EnsureType<const char*>(array_dump(array3).c_str())));
#endif
	return make_pair(is_good, ssprintf("Move result = %s, type = %s\n",
				is_good? "OK": "Error",
				EnsureType<const char*>(typeid(ArrayMD).name())));
}

template <class ArrayMD, class OtherArrayMD>
pair<bool, string> TestMoveSemanticsHelperMDM()
{
	ArrayMD array01;
	(array01 = OtherArrayMD({4, 5, 6, 7}, 1)) *= 10;
	return TestMoveSemanticsHelperMD<ArrayMD, OtherArrayMD>();
}

bool TestMoveSemanticsMD()
{
	bool is_good = true;
	string message;
	auto test = [&is_good, &message](pair<bool, string> test_result)
	{
		if (!test_result.first)
		{
			is_good = false;
		}
		if (message.length())
			message += "\n";
		message += test_result.second;
	};
	//ComplexFunctionMD_F32;
	using OtherArrayMD = DataArrayMD<DataArray2D<DataArray<int>>>;
	test(TestMoveSemanticsHelperMD<DataArrayMD<DataArray2D<DataArray<int>>>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMD<AlgebraicStructures::GenericFieldElement<DataArrayMD<DataArray2D<DataArray<double>>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMD<AlgebraicStructures::FieldElement<DataArrayMD<DataArray2D<DataArray<double>>>, DataArrayMD<DataArray2D<DataArray<double>>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray, AlgebraicStructures::FieldTagScalar>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMD<AlgebraMD<DataArrayMD<DataArray2D<DataArray<double>>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::FieldTagScalar>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<MathFunctionMD<RealFunction2D_F32>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<RealFunctionMD_F32, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<ComplexFunctionMD_F32, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<ColorImageMD_F32, OtherArrayMD>());
	ShowString("Move result MD", ssprintf("Result = %s\n", is_good? "OK": "Error") + message);
	return is_good;
}

//--------------------------------------------------------------

void TestMoveSemantics()
{
	for (;;)
	{
		using func = function<void ()>;
		auto action = GetButtonDecision(L"Choose test",
				{
					MakeButton(L"Move semantics 1D", func(TestMoveSemantics1D)),
					MakeButton(L"Move semantics 2D", func(TestMoveSemantics2D)),
					MakeButton(L"Move semantics MD", func(TestMoveSemanticsMD)),
					MakeButton(L"OK", func())
				});
		if (!action)
			break;
		SafeExecute(action);
	}
}

//--------------------------------------------------------------
