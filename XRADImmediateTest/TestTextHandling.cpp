#include "pre.h"

#include "TestTextHandling.h"

#include "TestHelpers.h"
#include <StringEncodeTest.h>
#include <StringConverters_MS_Test.h>
#include "QtStringConverters_Test.h"
#include <XRADBasic/Sources/Core/EscapeSequences.h>
#include <XRADSystem/TextFile.h>

#include <locale>

#ifdef XRAD_COMPILER_MSC
// For QtStringConverters_Test.h:
#include <XRADGUI/Sources/PlatformSpecific/MSVC/MSVC_XRADGUITestsLibLink.h>
#endif

XRAD_USING

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

template <class InternT, class ExternT>
class string_converter
{
	public:
		using intern_string_t = InternT;
		using extern_string_t = ExternT;
		using intern_char_t = typename intern_string_t::value_type;
		using extern_char_t = typename extern_string_t::value_type;
		using state_type = mbstate_t;

	public:
		static extern_string_t convert_fwd(const intern_string_t& str, const locale& loc, bool *error);
		static intern_string_t convert_rev(const extern_string_t& str, const locale& loc, bool *error);

	private:
		template <class CodeCvt, class FromString, class ToString, class Functor>
		static FromString convert(const ToString& src_str, const locale& loc, bool *error,
				Functor f);
};

template <class InternT, class ExternT>
auto string_converter<InternT, ExternT>::convert_fwd(const intern_string_t& str, const locale& loc,
		bool *error) -> extern_string_t
{
	using codecvt_t = codecvt<intern_char_t, extern_char_t, state_type>;
	return convert<codecvt_t, extern_string_t>(str, loc, error,
			[](const codecvt_t &cvt,
					state_type &state,
					const intern_char_t *from,
					const intern_char_t *from_end,
					const intern_char_t *&from_next,
					extern_char_t *to,
					extern_char_t *to_end,
					extern_char_t *&to_next)
			{
				return cvt.out(state, from, from_end, from_next, to, to_end, to_next);
			});
}

template <class InternT, class ExternT>
auto string_converter<InternT, ExternT>::convert_rev(const extern_string_t& str, const locale& loc,
		bool *error) -> intern_string_t
{
	using codecvt_t = codecvt<intern_char_t, extern_char_t, state_type>;
	return convert<codecvt_t, intern_string_t>(str, loc, error,
			[](const codecvt_t &cvt,
					state_type &state,
					const extern_char_t *from,
					const extern_char_t *from_end,
					const extern_char_t *&from_next,
					intern_char_t *to,
					intern_char_t *to_end,
					intern_char_t *&to_next)
			{
				return cvt.in(state, from, from_end, from_next, to, to_end, to_next);
			});
}

template <class InternT, class ExternT>
template <class CodeCvt, class FromString, class ToString, class Functor>
auto string_converter<InternT, ExternT>::convert(const ToString& from_str,
		const locale& loc, bool *error, Functor f) -> FromString
{
	using codecvt_t = CodeCvt;
	using from_string_t = ToString;
	using to_string_t = FromString;
	using from_char_t = typename from_string_t::value_type;
	using to_char_t = typename to_string_t::value_type;
	using state_type = typename codecvt_t::state_type;

	if (from_str.empty())
		return to_string_t();

	constexpr size_t max_initial_length = 4096;
	constexpr to_char_t default_char = to_char_t('?');

	const auto& cvt = use_facet<codecvt_t>(loc);
	// Преобразование может дать длину строки как больше, так и меньше исходной
	// (UTF-8 -> UTF-16, UTF-16 -> UTF-8).
	// Приблизительно оценить максимальную длину выходной строки для cvt.out(),
	// не выполняя конвертацию, можно с помощью вызова cvt.max_length().
	// Для cvt.in() такую оценку получить можно только с помощью cvt "обратного" типа.
	// Пока обойдемся грубой оценкой по длине исходной строки.
	size_t initial_length = from_str.length();
	if (initial_length > max_initial_length)
		initial_length = max_initial_length;
	vector<to_char_t> to_buf(initial_length, to_char_t(0));

	const auto *from = from_str.c_str();
	const auto *from_end = from + from_str.length();
	decltype(from) from_next = nullptr;
	auto *to = to_buf.data();
	auto *to_end = to + to_buf.size();
	decltype(to) to_next = nullptr;

	state_type state = {};
	size_t to_size = 0;
	for (;;)
	{
		auto status = f(cvt, state, from, from_end, from_next, to, to_end, to_next);
		switch (status)
		{
			case codecvt_t::noconv:
				if (to_buf.size() < from_str.length())
					to_buf.resize(from_str.length());
				to_size = from_str.length();
				copy(from_str.c_str(), from_str.c_str() + from_str.length(), to_buf.data());
				status = codecvt_t::ok;
				break;

			case codecvt_t::ok:
				to_size += to_next - to;
				if (from_next != from_end)
				{
					// Ошибка в библиотеках MSVC2015 для codecvt<u16char, u32char, ...>?
					// status == codecvt_t::ok, но преобразован не весь буфер, как в случае
					// codecvt_t::partial.
					from = from_next;
					status = codecvt_t::partial;
				}
				break;

			case codecvt_t::partial:
				to_size += to_next - to;
				if (from_next != from_end)
				{
					from = from_next;
				}
				else
				{
					// Входной буфер содержит в конце неполный символ.
					if (to_buf.size() < to_size + 1)
						to_buf.resize(to_size + 1);
					to_buf[to_size++] = default_char;
					if (error)
						*error = true;
					status = codecvt_t::ok;
				}
				break;

			case codecvt_t::error:
				to_size += to_next - to;
				if (to_buf.size() < to_size + 1)
					to_buf.resize(max(2*to_buf.size(), to_size + 1 + initial_length));
				to_buf[to_size++] = default_char;
				if (error)
					*error = true;
				if (from != from_end)
					from = from_next + 1;
				else
					status = codecvt_t::ok;
				break;
		}
		if (status == codecvt_t::ok)
			break;
		if (to_buf.size() < to_size + initial_length)
			to_buf.resize(2*to_buf.size());
		to = to_buf.data() + to_size;
		to_end = to_buf.data() + to_buf.size();
	}
	return to_string_t(to_buf.data(), to_size);
}

//--------------------------------------------------------------

void	TestCodeCvt()
{
	// Значения Locale name под Win10:
	// - "", "C",
	// - ".1251",
	// - ".1252",
	// - ".936".
	// UTF-8 (".65501") не поддерживается, вызывает исключение "bad locale name".
	//
	// Код, использующий codecvt<char16_t, char, mbstate_t> и codecvt<char32_t, char, mbstate_t>
	// в MSVC не линкуется.
	// Проблема актуальна в MSVC 2015, 2017 и, похоже, 2019:
	// - https://stackoverflow.com/questions/32055357/visual-studio-c-2015-stdcodecvt-with-char16-t-or-char32-t
	// - https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error?forum=vcgeneral
	// - https://developercommunityapi.westus.cloudapp.azure.com/content/problem/246272/msconnect-3118643-codecvt-library-issue.html
	//
	// С обратными преобразователями всё линкуется, но работают они неправильно.
	// Также неправильно работает преобразователь codecvt<char, wchar_t, mbstate_t>.
	// Замечание. Классы codecvt<char, T, S> с T != char не являются стандартными.
	auto loc = locale(GetString("Locale name", SavedGUIValue("")).c_str());
#pragma warning (push)
#pragma warning (disable: 4566)
	string str_c = "(0)ABIZabiz(1)Iıİi(2)АБЯабя(3)ΑΒΩαβω(4)维基百科(5)🍌(6)\U0001F34C(7)-";
	wstring str_w = L"(0)ABIZabiz(1)Iıİi(2)АБЯабя(3)ΑΒΩαβω(4)维基百科(5)🍌(6)\xD83C\xDF4C(7)\xD83C";
	u16string str_u16 = u"(0)ABIZabiz(1)Iıİi(2)АБЯабя(3)ΑΒΩαβω(4)维基百科(5)🍌(6)\xD83C\xDF4C(7)\xD83C";
	u32string str_u32 = U"(0)ABIZabiz(1)Iıİi(2)АБЯабя(3)ΑΒΩαβω(4)维基百科(5)🍌(6)\U0001F34C(7)\xD83C";
	string str_c_null = string("(0)AB(1)") + string(1, 0) + string("(2)ab(3)");
	wstring str_w_null = wstring(L"(0)AB(1)") + wstring(1, 0) + wstring(L"(2)ab(3)");
#pragma warning (pop)
	auto str_w_c_wc = string_converter<wstring, string>::convert_fwd(str_w, loc, nullptr);
	auto str_c_w_wc = string_converter<wstring, string>::convert_rev(str_c, loc, nullptr);
	auto str_w_c_cw = string_converter<string, wstring>::convert_rev(str_w, loc, nullptr);
	auto str_c_w_cw = string_converter<string, wstring>::convert_fwd(str_c, loc, nullptr);
	auto str_u32_u16_3216 = string_converter<u32string, u16string>::convert_fwd(str_u32, loc, nullptr);
	auto str_u16_u32_3216 = string_converter<u32string, u16string>::convert_rev(str_u16, loc, nullptr);
	auto str_u32_u16_1632 = string_converter<u16string, u32string>::convert_rev(str_u32, loc, nullptr);
	auto str_u16_u32_1632 = string_converter<u16string, u32string>::convert_fwd(str_u16, loc, nullptr);
	auto str_u32_32w = string_converter<u32string, wstring>::convert_fwd(str_u32, loc, nullptr);
	auto str_u16_16w = string_converter<u16string, wstring>::convert_fwd(str_u16, loc, nullptr);
	auto str_w_u32_w32 = string_converter<wstring, u32string>::convert_fwd(str_w, loc, nullptr);
	auto str_w_u16_w16 = string_converter<wstring, u16string>::convert_fwd(str_w, loc, nullptr);
	auto str_u16_w_w16 = string_converter<wstring, u16string>::convert_rev(str_u16, loc, nullptr);
	auto str_u16_c_c16 = string_converter<string, u16string>::convert_rev(str_u16, loc, nullptr);
	auto str_c_null_w_wc = string_converter<wstring, string>::convert_rev(str_c_null, loc, nullptr);
	auto str_w_null_c_wc = string_converter<wstring, string>::convert_fwd(str_w_null, loc, nullptr);
	ShowText(L"Test for std::codecvt",
			L"c:" + convert_to_wstring(str_c) + L"\n" +
			L"w:" + convert_to_wstring(str_w) + L"\n" +
			L"u16:" + convert_to_wstring(str_u16) + L"\n" +
			L"u32:" + convert_to_wstring(str_u32) + L"\n" +
			L"--\n" +
			L"w->c (w-c):" + convert_to_wstring(str_w_c_wc) + L"\n" +
			L"c->w (w-c):" + convert_to_wstring(str_c_w_wc) + L"\n" +
			L"w->c (c-w):" + convert_to_wstring(str_w_c_cw) + L"\n" +
			L"c->w (c-w):" + convert_to_wstring(str_c_w_cw) + L"\n" +
			L"u32->u16 (32-16):" + convert_to_wstring(str_u32_u16_3216) + L"\n" +
			L"u16->u32 (32-16):" + convert_to_wstring(str_u16_u32_3216) + L"\n" +
			L"u32->u16 (16-32):" + convert_to_wstring(str_u32_u16_1632) + L"\n" +
			L"u16->u32 (16-32):" + convert_to_wstring(str_u16_u32_1632) + L"\n" +
			L"u32->w (32-w):" + convert_to_wstring(str_u32_32w) + L"\n" +
			L"u16->w (16-w):" + convert_to_wstring(str_u16_16w) + L"\n" +
			L"w->u32 (w-32):" + convert_to_wstring(str_w_u32_w32) + L"\n" +
			L"w->u16 (w-16):" + convert_to_wstring(str_w_u16_w16) + L"\n" +
			L"u16->w (w-16):" + convert_to_wstring(str_u16_w_w16) + L"\n" +
			L"u16->c (c-16):" + convert_to_wstring(str_u16_c_c16) + L"\n" +
			L"--\n" +
			L"c_null:" + convert_to_wstring(encode_escape_sequences(str_c_null, {})) + L"\n" +
			L"c_null->w (w-c):" + convert_to_wstring(encode_escape_sequences(str_c_null_w_wc, {})) + L"\n" +
			L"w_null->c (w-c):" + convert_to_wstring(encode_escape_sequences(str_w_null_c_wc, {})) + L"\n" +
			L"--\n");
	return;
}

//--------------------------------------------------------------

} // namespace

namespace
{

//--------------------------------------------------------------

void	TestUnicode()
{
	{
		class ErrorReporter : public TestHelpers::ErrorReporter
		{
		public:
			virtual void ReportError(const string &error_message) override
			{
				printf("TestUnicode error: %s\n", error_message.c_str());
				Error(ssprintf("TestUnicode error:\n%s", error_message.c_str()));
			}
		};
		printf("Starting batch Unicode tests...\n");
		ErrorReporter error_reporter;
		StringEncodeTest::Test(&error_reporter);
#ifdef XRAD_USE_MS_VERSION
		StringConverters_MS_Test::Test(&error_reporter);
#endif
		QtStringConverters_Test::Test(&error_reporter);
		printf("Batch Unicode tests finished.\n");
	}
//	size_t	value = 1;
//	printf("%d", value);
//	text_encoding::file_type	enc = char_8bit;//на этом значении тест дает ошибку (см. ниже)
	text_encoding::file_type	enc = text_encoding::utf8;//на этом значении тест дает ошибку (см. ниже)
//	text_encoding::file_type	enc = utf16_le;
//	text_encoding::file_type	enc = utf16_be;

	ustring	ustr = convert_to_ustring(U"УТФ-8 из УТФ-32\r\n '🍌🌠🎄'");
	u32string u32str = convert_to_u32string(ustr);
	ShowString("u8->u32", u32str);
	ustring ustr2 = convert_to_ustring(L"УТФ-8 из УТФ-16.\r\n'🍌🌠🎄'");
	u32string	u32str2 = convert_to_u32string(ustr2);
	ShowString(L"u8->32->8", u32str2);
	ustring ustr3 = ssprintf(convert_to_ustring(L"результат ssprintf(ustring).\r\nEnd"));
	ShowString(convert_to_ustring(L"строка"), ssprintf(convert_to_ustring(
			L"ssprintf в УТФ-8. (%s)\r\nEnd" // использовано %s: строка будет преобразована в const char*
			), ustr3.c_str()));

	{
		text_file_writer	wfile;

		wfile.open_create(GetTempDirectory() + "/wtest.txt", enc);
		wfile.printf_(L"Однажды в студеную зимнюю пору (%ls)\r\n", L"'🍌🌠🎄'");//utf16

		wfile.printf_(convert_to_wstring(L"Я из лесу вышел. Был сильный мороз. (%ls)\r\n"), convert_to_wstring("УТФ-16 из разных типов").c_str());//utf16
		wfile.printf_("Гляжу, поднимается медленно в гору (%s)\r\n", "чистые 8 бит");//8bit
		wfile.printf_(convert_to_wstring(L"Лошадка, везущая хворосту воз. (%ls)\r\n"), convert_to_wstring("УТФ-8 из разных типов").c_str());
		wfile.printf_(L"И, шествуя важно в спокойствии чинном,\r\nЧитает она по-китайски стихи:\r\n(%ls)", L"维基百科");//krakozyb utf16
	}


	{
		text_file_reader	rfile;
		rfile.open(GetTempDirectory() + "/wtest.txt");
//		if(rfile.encoding()==text_encoding::encoding_unknown) rfile.set_encoding(text_encoding::encoding_char_8bit);
		wstring	content;
	//	DataArray<wchar_t>	content(rfile.size()/sizeof(wchar_t) + 1, 0);
		rfile.read(content);
		rfile.close();
		ShowText(L"just read", content, true);
	}
}

void TestCaseConversion()
{
	{
		wstring string_source = L"ABIZabizIıİiАБЯабяΑΒΩαβω维基百科";
		// Для турецких ıİ преобразование регистра не производится.
		// Для применения этих функций при сравнении имен файлов, имен тегов и т.п. с образцами
		// без учета регистра это поведение как раз то что нужно.
		// Разобраться с поведением этого преобразования на разных платформах.
		// См. также:
		// https://habr.com/ru/post/147387/
		wstring string_sample_upper = L"ABIZABIZIıİIАБЯАБЯΑΒΩΑΒΩ维基百科";
		wstring string_sample_lower = L"abizabiziıİiабяабяαβωαβω维基百科";
		wstring string_upper = get_upper(string_source);
		wstring string_lower = get_lower(string_source);
		ShowText(L"Case conversion",
				ssprintf(L"Source: %ls\n", EnsureType<const wchar_t*>(string_source.c_str())) +
				ssprintf(L"Upper result: %ls\n", EnsureType<const wchar_t*>(
						string_upper == string_sample_upper? L"OK": L"Failed")) +
				ssprintf(L"Upper S: %ls\n", EnsureType<const wchar_t*>(string_sample_upper.c_str())) +
				ssprintf(L"Upper R: %ls\n", EnsureType<const wchar_t*>(string_upper.c_str())) +
				ssprintf(L"Lower result: %ls\n", EnsureType<const wchar_t*>(
						string_lower == string_sample_lower? L"OK": L"Failed")) +
				ssprintf(L"Lower S: %ls\n", EnsureType<const wchar_t*>(string_sample_lower.c_str())) +
				ssprintf(L"Lower R: %ls\n", EnsureType<const wchar_t*>(string_lower.c_str())),
				true);
	}
}

//--------------------------------------------------------------

void TestLanguageSwitch()
{
	using namespace DynamicDialog;
	using DynamicDialog::Button;
	auto dialog = EnumDialog::Create(L"Language test", { MakeButton(L"OK", 0) });
	auto text_edit = dialog->CreateControl<TextEdit>(L"Sample text", L"");
	auto update_text = [text_edit]()
	{
		auto lang_id = GetLanguageId();
		auto text = ssprintf(L"lang_id = \"%ls\"\n", EnsureType<wstring>(convert_to_wstring(lang_id))) +
				tr_ru_en(L"Текст по-русски.\n", L"English text.\n");
		text_edit->SetValue(text);
	};
	update_text();
	string lang_id = LoadLanguageId();
	auto language_group = (EnumRadioButtonChoice::Create(L"Language (raw id)",
			{
				MakeButton("en", string(GetStdLanguageIdEn())),
				MakeButton("ru", string(GetStdLanguageIdRu())),
				MakeButton("foo", string("foo")),
				MakeButton("EN-US", string("EN-US")),
				MakeButton("RU-MO", string("RU-MO")),
				MakeButton("mn-Cyrl-MN", string("mn-Cyrl-MN")),
				MakeButton("<empty>", string("")),
			},
			&lang_id,
			Layout::Vertical,
			[&update_text](const string &v)
			{
				SaveLanguageId(v);
				update_text();
			}));
	dialog->AddControl(language_group);
	dialog->CreateControl<Button>(L"Update",
			[&update_text, &language_group, &lang_id]()
			{
				update_text();
				lang_id = LoadLanguageId();
				language_group->Update();
			});
	dialog->Show();
}

//--------------------------------------------------------------

void TestCaseChange()
{
	vector<wstring>	v =
	{
		L"жил бы цитрус в чащах юга? да, но фальшивый экземпляр",
		L"Zoë Saldaña played in La maldición del padre Cardona. Blumenstraße",
		L"'Ότι μὲν ὑμει̃σ, ὠ̃ ἄνδρες 'Αθηναι̃οι, πεπόνθατε ὑπὸ τω̃ν ἐμω̃ν",
		L"Οι σημαντικότερες μαρτυρίες που πιστοποιούν την ύπαρξη ζωής στον ελλαδικό χώρο από την Λίθινη εποχή",
		L"Չնայած այն հանգամանքին, որ մայրցամաքային Հունաստանի և Էգեյան ծովում գտնվող կղզիների"
	};

	for(auto &s: v)
	{
		auto S = get_upper(s);
		ShowText(L"Text sample", s + L"\n" + S);
	}

	// Проверка, что глобальная локаль не стала русской. В числе должна быть точка, а не запятая
	ShowString(L"Decimal dot", ssprintf(L"pi = %g", 3.1415926));
}

//--------------------------------------------------------------

void TestExtendedSprintf()
{
	string	s0 = " 'string to insert'";
	wstring	ws0 = L"'wstring to insert'";

	string	s1 = ssprintf_core("Pointer: %s", s0.c_str());
	string	s3 = ssprintf("Ext Pointer: %s", s0.c_str());
	string	s4 = ssprintf("Ext String: %s", s0);
	string	s5 = ssprintf("Ext wPointer: %s", ws0.c_str());
	string	s6 = ssprintf("Ext wString: %s", ws0);

	wstring	ws1 = ssprintf_core(L"wPointer: %ls", ws0.c_str());
	wstring	ws3 = ssprintf(L"Ext wPointer: %ls", ws0.c_str());
	wstring	ws4 = ssprintf(L"Ext wString: %ls", ws0);


	//неудавшаяся попытка вызвать warning C4477: format string '%d' requires an argument of type 'int', but variadic argument 1 has type 'double'
	//int	i;
	//double d;

	ForceDebugBreak();
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestTextHandling()
{
	for (;;)
	{
		using func = function<void()>;
		auto response = GetButtonDecision(L"Choose text test",
				{
					MakeButton(L"Unicode", func(TestUnicode)),
					MakeButton(L"Case conversion", func(TestCaseConversion)),
					MakeButton(L"Language switch", func(TestLanguageSwitch)),
					MakeButton(L"std::codecvt", func(TestCodeCvt)),
					MakeButton(L"Case change", func(TestCaseChange)),
					MakeButton(L"Extended sprintf", func(TestExtendedSprintf)),
					MakeButton(L"Exit", func())
				});
		if (!response)
			break;
		SafeExecute(response);
	}
}

//--------------------------------------------------------------
