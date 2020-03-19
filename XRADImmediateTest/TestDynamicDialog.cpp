// file TestDynamicDialog.cpp
//--------------------------------------------------------------
#include "pre.h"

#include "TestDynamicDialog.h"
#include <XRAD/GUI/DynamicDialog.h>
#include <XRADBasic/Sources/Core/EscapeSequences.h>

XRAD_BEGIN

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

struct DialogTestData
{
	shared_ptr<DynamicDialog::Dialog> dialog;
	vector<shared_ptr<DynamicDialog::Button>> buttons;
	vector<shared_ptr<DynamicDialog::CheckBox>> checkboxes;
	vector<shared_ptr<DynamicDialog::RadioButton>> radiobuttons;
	vector<shared_ptr<DynamicDialog::RadioButtonContainer>> radiobutton_containers;
	vector<shared_ptr<DynamicDialog::ComboBox>> comboboxes;
	vector<shared_ptr<DynamicDialog::StringEdit>> string_edits;
	vector<function<string ()>> functors;
	function<string ()> rb_fp_value = make_fn([]() { return string("RB Functor choice: default."); });
	function<string ()> combo_fp_value = make_fn([]()
			{ return string("Combo Functor choice: default."); });

	vector<shared_ptr<DynamicDialog::RadioButtonContainer>> rbc;
	bool clear_rb = false;
	bool reuse = false;
};

//--------------------------------------------------------------

class AllocCounter
{
	public:
		AllocCounter() { ++object_count; }
		AllocCounter(const AllocCounter &) = delete;
		AllocCounter& operator=(const AllocCounter &) = delete;
		~AllocCounter() { --object_count; }
	public:
		static size_t ObjectCount() { return object_count; }
	private:
		static size_t object_count;
};

size_t AllocCounter::object_count = 0;

template <class T>
class AllocCounterT: private AllocCounter
{
	public:
		T value;

		template <class... Args>
		AllocCounterT(Args&&... args):
			value(std::forward<Args>(args)...)
		{
		}
};

template <class T, class... Args>
shared_ptr<T> make_shared_x(Args&&... args)
{
	auto p = make_shared<AllocCounterT<T>>(std::forward<Args>(args)...);
	return shared_ptr<T>(p, &p->value);
}

#define make_shared make_shared_x

//--------------------------------------------------------------

// Проверка компилируемости всех методов шаблона EnumRadioButtonChoiceImpl:
template class DynamicDialog::EnumRadioButtonChoiceImpl<string>; // тип с операцией сравнения (==)
template class DynamicDialog::EnumRadioButtonChoiceImpl<function<void ()>>; // тип без ==

// Проверка компилируемости всех методов шаблона ValueEnumRadioButtonChoiceImpl:
template class DynamicDialog::ValueEnumRadioButtonChoiceImpl<string>; // тип с операцией сравнения (==)
template class DynamicDialog::ValueEnumRadioButtonChoiceImpl<function<void ()>>; // тип без ==

// Проверка компилируемости всех методов шаблона EnumComboBoxImpl:
template class DynamicDialog::EnumComboBoxImpl<string>; // тип с операцией сравнения (==)
template class DynamicDialog::EnumComboBoxImpl<function<void ()>>; // тип без ==

// Проверка компилируемости всех методов шаблона ValueEnumComboBoxImpl:
template class DynamicDialog::ValueEnumComboBoxImpl<string>; // тип с операцией сравнения (==)
template class DynamicDialog::ValueEnumComboBoxImpl<function<void ()>>; // тип без ==

// Проверка компилируемости всех методов шаблона ValueNumberEdit:
template class DynamicDialog::ValueNumberEdit<double>;

//--------------------------------------------------------------

shared_ptr<DialogTestData> CreateTestDialog()
{
	using namespace DynamicDialog;
	using DynamicDialog::Button;

	auto dialog_test_data = make_shared<DialogTestData>();

	auto dialog = OKCancelDialog::Create(L"Math Dynamic dialog test");
	dialog_test_data->dialog = dialog;
	dialog_test_data->functors.push_back([dialog]()
			{
				const char *result_str = "Invalid";
				switch (dialog->Choice())
				{
					case OKCancelDialog::Result::OK: result_str = "OK"; break;
					case OKCancelDialog::Result::Cancel: result_str = "Cancel"; break;
					case OKCancelDialog::Result::UserDefined: result_str = "<UserDefined>"; break;
				}
				return ssprintf("OKCancedDialog result: %s.",
						EnsureType<const char*>(result_str));
			});

	auto gross_container = make_shared<ControlContainer>(Layout::Horizontal);
	dialog->AddControl(gross_container);
	auto first_container = make_shared<ControlContainer>();
	gross_container->AddControl(first_container);

	// [X] Option 1
	//   +- Group ----
	//   ! [ ] Option 1.1
	//   ! [ ] Option 1.2 -> text
	//   ! ----
	//   ! Test text label...
	auto checkbox_1 = first_container->CreateControl<CheckBox>(L"Option 1", true);
	dialog_test_data->checkboxes.push_back(checkbox_1);
	auto op_1_sub_container = make_shared<ControlContainer>(L"Group");
	auto ui_checkbox = op_1_sub_container->CreateControl<CheckBox>(L"Option 1.1", false);
	dialog_test_data->checkboxes.push_back(ui_checkbox);
	auto ui_checkbox_1_2 = op_1_sub_container->CreateControl<CheckBox>(L"Option 1.2 -> text", false);
	dialog_test_data->checkboxes.push_back(ui_checkbox_1_2);
	op_1_sub_container->CreateControl<Separator>();
	auto op_1_text = op_1_sub_container->CreateControl<TextLabel>(L"Test text label.\nSecond line.");
	ui_checkbox_1_2->SetOnCheckedChanged([w_label = to_weak_ptr(op_1_text)](bool c)
			{
				if (auto label = w_label.lock())
				{
					label->SetText(c ? L"3 line text;\nline 2;\nline 3." :
							L"Достаточно длинный однострочный текст.");
				}
			});
	checkbox_1->SetSubControl(op_1_sub_container);

	// [ ] Option 2
	//   [X] Option 2.1
	//     [X] Option 2.1.1
	auto checkbox_2_1 = make_shared<CheckBox>(L"Option 2.1: ShowString on checked",
			SavedGUIValue(true),
			[](bool checked)
			{
				if (checked)
				{
					ShowString(L"CheckBox", L"Test");
				}
			});
	dialog_test_data->checkboxes.push_back(checkbox_2_1);
	auto checkbox_2_1_0 = checkbox_2_1->CreateSubControl<CheckBox>(L"Option 2.1.0: ignored", true);
			// Будет перезаписано checkbox_2_1_1.
	dialog_test_data->checkboxes.push_back(checkbox_2_1_0);
	auto checkbox_2_1_1 = checkbox_2_1->CreateSubControl<CheckBox>(
			L"Option 2.1.1: throw on checked", true,
			[](bool checked)
			{
				if (checked)
					throw runtime_error("Throw from checkbox");
			}); // Перезаписываем checkbox_2_1_0.
	dialog_test_data->checkboxes.push_back(checkbox_2_1_1);
	auto checkbox_2 = make_shared<CheckBox>(L"Option 2 -> Option 1", SavedGUIValue(false),
			[checkbox_1](bool checked)
			{
				checkbox_1->SetChecked(checked);
			});
	first_container->AddControl(checkbox_2);
	dialog_test_data->checkboxes.push_back(checkbox_2);
	checkbox_2->SetSubControl(checkbox_2_1);

	// [ ] Toggle loop
	auto checkbox_toggle_loop = first_container->CreateControl<CheckBox>(L"Toggle loop", false);
	// Внимание! В обработчик нельзя захватывать shared_ptr на самого себя, т.к. такой объект
	// никогда не будет удален.
	unsigned int counter = 0;
	checkbox_toggle_loop->SetOnCheckedChanged(
			[checkbox = checkbox_toggle_loop.get(), &counter](bool checked)
			{
				++counter;
				if (counter & 0xFFF) // Защита от зацикливания (переполнения стека)
					checkbox->SetChecked(!checked);
			});
	dialog_test_data->checkboxes.push_back(checkbox_toggle_loop);

	// [Many toggles]
	auto many_toggles_button = first_container->CreateControl<Button>(L"Many toggles",
			[checkbox_1]()
			{
				// Проверка, что UI не зависает при постоянных обращениях из рабочего потока.
				for (size_t i = 0; i < 10000; ++i)
				{
					checkbox_1->SetChecked(i & 0x1);
				}
			});
	dialog_test_data->buttons.push_back(many_toggles_button);

	// ( ) Radio 1
	//   +- Radio group ----
	//   ! ( ) Radio 1.1  ( ) Radio 1.2
	//   +----
	// (*) Radio 2
	//   +- Radio group 2 ----
	//   ! ( ) Radio 2.1     (*) Radio 2.2
	//   !   [ ] Uncheck RB    Comment
	//   !   [ ] Result
	//   +----
	auto first_radio_group = first_container->CreateControl<RadioButtonContainer>(
			saved_default_value);
	auto radio_1 = first_radio_group->CreateRadioButton(L"Radio 1", false);
	dialog_test_data->radiobuttons.push_back(radio_1);

	auto radio_1_1 = make_shared<RadioButton>(L"Radio 1.1", false);
	dialog_test_data->radiobuttons.push_back(radio_1_1);
	auto checkbox_r_1_1_1 = radio_1_1->CreateSubControl<CheckBox>(L"Radio 1.1 sub", false);
	dialog_test_data->checkboxes.push_back(checkbox_r_1_1_1);
	auto radio_1_2 = make_shared<RadioButton>(L"Radio 1.2", false);
	dialog_test_data->radiobuttons.push_back(radio_1_2);
	auto rb_1_sub_container = radio_1->CreateSubControl<RadioButtonContainer>(L"Radio group",
			vector<shared_ptr<RadioButton>>({radio_1_1, radio_1_2}),
			SavedGUIValue(nullptr),
			Layout::Horizontal);
	dialog_test_data->radiobutton_containers.push_back(rb_1_sub_container);

	auto cb_result = make_shared<CheckBox>(L"Result", false);
	auto radio_2_1 = make_shared<RadioButton>(L"Radio 2.1", false,
			[cb_result](bool checked)
			{
				cb_result->SetChecked(checked);
			});
	auto radio_2_1_sub_container = make_shared<ControlContainer>();
	auto radio_2_1_clear_button = make_shared<CheckBox>(L"Uncheck RB", false,
			[dtd = dialog_test_data.get()](bool checked)
			{
				if (checked)
				{
					for (auto &rbc: dtd->rbc)
						rbc->SetChoice(nullptr);
				}
				dtd->clear_rb = checked;
			});
	dialog_test_data->checkboxes.push_back(radio_2_1_clear_button);
	radio_2_1_sub_container->AddControl(radio_2_1_clear_button);
	radio_2_1_sub_container->AddControl(cb_result);
	radio_2_1->SetSubControl(radio_2_1_sub_container);
	auto radio_2_2 = make_shared<RadioButton>(L"Radio 2.2", false);
	radio_2_2->SetSubControl(make_shared<TextLabel>(L"Comment"));
	auto rb_2_sub_container = make_shared<RadioButtonContainer>(L"Radio group 2",
			saved_default_value,
			Layout::Horizontal);
	rb_2_sub_container->AddControl(radio_2_1);
	rb_2_sub_container->AddControl(radio_2_2);
	rb_2_sub_container->SetChoice(radio_2_2.get());
	dialog_test_data->rbc.push_back(rb_2_sub_container);
	dialog_test_data->radiobuttons.push_back(radio_2_1);
	dialog_test_data->radiobuttons.push_back(radio_2_2);
	dialog_test_data->radiobutton_containers.push_back(rb_2_sub_container);

	auto radio_2 = first_radio_group->CreateRadioButton(L"Radio 2", true);
	dialog_test_data->radiobuttons.push_back(radio_2);
	radio_2->SetSubControl(rb_2_sub_container);
	dialog_test_data->rbc.push_back(first_radio_group);

	// [ ] Reuse structures
	auto checkbox_reuse = first_container->CreateControl<ValueCheckBox>(L"Reuse structures (bool*)",
			SavedGUIValue(&dialog_test_data->reuse));
	dialog_test_data->checkboxes.push_back(checkbox_reuse);

	// [Change checkbox]
	first_container->CreateControl<Button>(L"Change checkbox",
			[checkbox_reuse, p_value = &dialog_test_data->reuse]()
			{
				*p_value = !*p_value;
				checkbox_reuse->Update();
			},
			Button::ActionPolicy::None);

	// [Инструкция]
	auto instructions_button = first_container->CreateControl<Button>(L"Инструкция",
			[]()
			{
				ShowString(L"Dynamic Dialog",
						L"1. Нажать кнопку \"Инструкция\".\n"
						L"2. Не закрывая окна инструкции, нажать кнопку \"Инструкция\" два раза.\n"
						L"3. Закрыть окно инструкции.\n"
						L"4. Появится первое дополнительное окно инструкции. Закрыть его.\n"
						L"5. Появится второе дополнительное окно инструкции. Закрыть его.\n"
						);
			},
			Button::ActionPolicy::None);
	dialog_test_data->buttons.push_back(instructions_button);

	// [Вложенный диалог (block, dep)]
	auto sub_dialog_button_functor = make_shared<function<string ()>>();
	auto sub_dialog_button = first_container->CreateControl<Button>(
			L"Вложенный диалог (block, dep)",
			[sub_dialog_button_functor]()
			{
				auto dialog = EnumDialog::Create(L"Functor dialog",
						{
							MakeButton(L"DF1", make_fn([]() { return string("DF1"); })),
							MakeButton(L"DF2", make_fn([]() { return string("DF2"); })).SetDefault(),
							MakeButton(L"Cancel", make_fn([]() { return string("Cancel"); })).SetCancel(),
						},
						make_fn([]() { return string("User defined"); }));
				dialog->AddControl(make_shared<TextLabel>(convert_to_wstring(*sub_dialog_button_functor?
						(*sub_dialog_button_functor)(): string("<null functor>"))));
				dialog->AddControl(make_shared<Button>(L"User defined"));
				dialog->AddEnumButton(make_shared<Button>(L"DF3"), make_fn([]() { return string("DF3"); }));
				dialog->Show();
				auto choice = dialog->Choice();
				if (!choice)
					ShowString(L"Choice", L"<null functor>");
				else
					ShowString(L"Choice", convert_to_wstring(choice()));
			});
	dialog_test_data->buttons.push_back(sub_dialog_button);

	// [Вложенный диалог (hide)]
	auto sub_dialog_button_hide = first_container->CreateControl<Button>(
			L"Вложенный диалог (hide)",
			[]()
			{
				enum class TestOKCancelEnum { UserDefined, Cancel, OK, Other };
				auto dialog = OKCancelDialog::Create<TestOKCancelEnum>(L"Вложенный диалог (hide)");
				OKCancelDialog::dialog_t<TestOKCancelEnum> *dlg_type_check = dialog.get();
				(void)dlg_type_check;
				dialog->AddEnumButton(make_shared<Button>(L"Other"), TestOKCancelEnum::Other);
				dialog->Show();
				const wchar_t *id = L"???";
				switch (dialog->Choice())
				{
					case TestOKCancelEnum::UserDefined: id = L"User defined"; break;
					case TestOKCancelEnum::Cancel: id = L"Cancel"; break;
					case TestOKCancelEnum::OK: id = L"OK"; break;
					case TestOKCancelEnum::Other: id = L"Other"; break;
				}
				ShowString(L"Dialog result", id);
			},
			Button::ActionPolicy::Hide);
	dialog_test_data->buttons.push_back(sub_dialog_button_hide);

	gross_container->CreateControl<Separator>();

	auto second_container = gross_container->CreateControl<ControlContainer>();

	enum class Choices1 { N1, N2 };
	auto rb_e_choice = EnumRadioButtonChoice::CreateX(L"Enum choice",
			{
				MakeButton(L"N1", Choices1::N1),
				MakeButton(L"N2", Choices1::N2)
			},
			Choices1::N2,
			Layout::Vertical,
			[](Choices1 c)
			{
				string c_str = "Invalid";
				switch (c)
				{
					case Choices1::N1: c_str = "N1"; break;
					case Choices1::N2: c_str = "N2"; break;
				}
				printf("Enum choice changed: %s\n", EnsureType<const char*>(c_str.c_str()));
			});
	second_container->AddControl(rb_e_choice);
	dialog_test_data->radiobutton_containers.push_back(rb_e_choice->InternalContainer());
	dialog_test_data->functors.push_back(make_fn([rb_e_choice]()
			{
				string c_str = "Invalid";
				switch (rb_e_choice->Choice())
				{
					case Choices1::N1: c_str = "N1"; break;
					case Choices1::N2: c_str = "N2"; break;
				}
				return ssprintf("Enum choice: %s.", EnsureType<const char*>(c_str.c_str()));
			}));

	auto rb_f_value = make_shared<function<string ()>>(
			make_fn([]() { return string("Functor choice: default."); }));
			// Это значение хранится в dialog_test_data->functors, см. ниже.
	auto rb_f_choice = EnumRadioButtonChoice::Create(L"Functor choice",
			{
				MakeButton(L"F1", make_fn([]() { return string("Functor choice: F1."); })),
				MakeButton(L"F2", make_fn([]() { return string("Functor choice: F2."); }))
			},
			SavedGUIValue(rb_f_value.get()),
			Layout::Horizontal,
			[](function<string ()> f)
			{
				string c_str = "(null functor)";
				if (f)
					c_str = "Functor: " + f();
				printf("Functor choice changed: \"%s\"\n", EnsureType<const char*>(c_str.c_str()));
			});
	second_container->AddControl(rb_f_choice);
	dialog_test_data->radiobutton_containers.push_back(rb_f_choice->InternalContainer());
	// Важно! Здесь захватывается и хранится rb_f_value, на который ссылается rb_f_choice.
	dialog_test_data->functors.push_back(make_fn([rb_f_value]()
			{
				string c_str = "(null functor)";
				auto f = *rb_f_value;
				if (f)
					c_str = "Functor: " + f();
				return ssprintf("Functor choice: \"%s\".", EnsureType<const char*>(c_str.c_str()));
			}));

	auto rb_fp_choice = EnumRadioButtonChoice::Create(
			{
				MakeButton(L"FP1 (🐻)",
						make_fn([]() { return convert_to_string(L"Functor choice: FP1 (🐻)."); })),
				MakeButton(L"FP2 (🐻🐻)",
						make_fn([]() { return convert_to_string(L"Functor choice: FP2 (🐻🐻)."); }))
			},
			SavedGUIValue(&dialog_test_data->rb_fp_value),
			Layout::Horizontal,
			[](function<string ()> f)
			{
				string c_str = "(null functor)";
				if (f)
					c_str = "Functor: " + f();
				printf("RB Functor pointer choice changed: \"%s\"\n", EnsureType<const char*>(c_str.c_str()));
			});
	second_container->AddControl(rb_fp_choice);
	dialog_test_data->radiobutton_containers.push_back(rb_fp_choice->InternalContainer());
	dialog_test_data->functors.push_back(make_fn([&fp_value = dialog_test_data->rb_fp_value]()
			{
				string c_str = "(null functor)";
				if (fp_value)
					c_str = "Functor: " + fp_value();
				return ssprintf("RB Functor pointer choice: \"%s\".", EnsureType<const char*>(c_str.c_str()));
			}));

	// Combo
	// [Item  [V]]
	auto combo = make_shared<ComboBox>(L"Combo",
			vector<wstring>({
				L"Item 1",
				L"Item 2",
				L"Item 3",
			}), 1,
			[](size_t item_index)
			{
				printf("Combo item changed: %zu.\n", EnsureType<size_t>(item_index));
			},
			[](size_t item_index, DialogResultCode)
			{
				printf("Combo item applied: %zu.\n", EnsureType<size_t>(item_index));
			});
	second_container->AddControl(combo);
	dialog_test_data->comboboxes.push_back(combo);

	// [Item  [V]]
	auto combo_2 = second_container->CreateControl<ComboBox>(
			vector<wstring>({
				L"Item 1",
				L"Item 2",
				L"Item 3",
				L"Item 4",
				L"Item 5",
			}),
			SavedGUIValue(-1),
			[combo](size_t item_index)
			{
				printf("Combo 2 item changed: %zu.\n", EnsureType<size_t>(item_index));
				combo->SetChoice(item_index - 1);
			});
	dialog_test_data->comboboxes.push_back(combo_2);

	auto combo_e_value = make_shared<Choices1>(Choices1::N2);
			// Это значение хранится в dialog_test_data->functors, см. ниже.
	auto combo_e_choice = EnumComboBox::Create(L"Enum combo",
			{
				MakeButton(L"N1", Choices1::N1),
				MakeButton(L"N2", Choices1::N2)
			},
			combo_e_value.get(),
			[rb_e_choice](Choices1 c)
			{
				string c_str = "Invalid";
				switch (c)
				{
					case Choices1::N1: c_str = "N1"; break;
					case Choices1::N2: c_str = "N2"; break;
				}
				printf("Combo Enum choice changed: %s\n", EnsureType<const char*>(c_str.c_str()));
				rb_e_choice->SetChoice(c);
			});
	second_container->AddControl(combo_e_choice);
	dialog_test_data->comboboxes.push_back(combo_e_choice->InternalComboBox());
	// Важно! Здесь захватывается и хранится combo_e_value, на который ссылается combo_e_choice.
	dialog_test_data->functors.push_back(make_fn([combo_e_value]()
			{
				string c_str = "Invalid";
				switch (*combo_e_value)
				{
					case Choices1::N1: c_str = "N1"; break;
					case Choices1::N2: c_str = "N2"; break;
				}
				return ssprintf("Combo Enum choice: %s.", EnsureType<const char*>(c_str.c_str()));
			}));

	// [Change choice]
	second_container->CreateControl<Button>(L"Change choice",
			[combo_e_choice, combo_e_value]()
			{
				switch (*combo_e_value)
				{
					case Choices1::N1:
						*combo_e_value = Choices1::N2;
						break;
					case Choices1::N2:
						*combo_e_value = Choices1::N1;
						break;
				}
				combo_e_choice->Update();
			});

	auto combo_fp_choice = EnumComboBox::Create(
			{
				MakeButton(L"FP1 (🐻)",
						make_fn([]() { return convert_to_string(L"Functor choice: FP1 (🐻)."); })),
				MakeButton(L"FP2 (🐻🐻)",
						make_fn([]() { return convert_to_string(L"Functor choice: FP2 (🐻🐻)."); }))
			},
			SavedGUIValue(&dialog_test_data->combo_fp_value),
			[](function<string ()> f)
			{
				string c_str = "(null functor)";
				if (f)
					c_str = "Functor: " + f();
				printf("Combo Functor pointer choice changed: \"%s\"\n",
						EnsureType<const char*>(c_str.c_str()));
			});
	second_container->AddControl(combo_fp_choice);
	dialog_test_data->comboboxes.push_back(combo_fp_choice->InternalComboBox());
	dialog_test_data->functors.push_back(make_fn([&fp_value = dialog_test_data->combo_fp_value]()
			{
				string c_str = "(null functor)";
				if (fp_value)
					c_str = "Functor: " + fp_value();
				return ssprintf("Combo Functor pointer choice: \"%s\".",
						EnsureType<const char*>(c_str.c_str()));
			}));

	*sub_dialog_button_functor = [combo_fp_choice]()
			{
				auto f = combo_fp_choice->Choice();
				if (f)
					return f();
				return string("(null choice)");
			};
	sub_dialog_button->SetValueDependencies({combo_fp_choice});

	//second_container->AddControl(make_shared<Stretch>());

	auto third_container = gross_container->CreateControl<ControlContainer>();

	auto edit_cb = third_container->CreateControl<CheckBox>(L"Edits", SavedGUIValue(true));
	dialog_test_data->checkboxes.push_back(edit_cb);
	auto edit_container = edit_cb->CreateSubControl<ControlContainer>();

	auto string_edit_v = edit_container->CreateControl<StringEdit>(L"Test string-v", L"value-v");
	dialog_test_data->string_edits.push_back(string_edit_v);

	auto string_edit_h_value = make_shared<wstring>(L"value-h");
			// Это значение хранится в dialog_test_data->functors, см. ниже.
	auto string_edit_h = edit_container->CreateControl<ValueStringEdit>(L"Test string-h",
			SavedGUIValue(&*string_edit_h_value),
			Layout::Horizontal);
	dialog_test_data->string_edits.push_back(string_edit_h);
	// Важно! Здесь захватывается и хранится string_edit_h_value, на который ссылается string_edit_h.
	dialog_test_data->functors.push_back([string_edit_h_value]()
			{
				return convert_to_string(ssprintf(L"String edit h: \"%ls\".",
						EnsureType<const wchar_t*>(string_edit_h_value->c_str())));
			});

	// [Change string]
	edit_container->CreateControl<Button>(L"Change string",
			[string_edit_h, string_edit_h_value]()
			{
				*string_edit_h_value = *string_edit_h_value + L"+";
				string_edit_h->Update();
			});

	auto number_edit_h = edit_container->CreateControl<NumberEdit<long long>>(
			L"Test number-h",
			SavedGUIValue(19), -100, 1000,
			out_of_range_prohibited,
			Layout::Horizontal,
			[](long long v)
			{
				printf("Test number-h changed: %lli.\n",
						EnsureType<long long>(v));
			});
	auto number_edit_v = edit_container->CreateControl<NumberEdit<long long>>(
			L"Test number-v (any range)",
			21, 10, -10,
			out_of_range_allowed,
			Layout::Vertical,
			[number_edit_h](long long v)
			{
				printf("Test number-v changed: %lli.\n",
						EnsureType<long long>(v));
				number_edit_h->SetValue(v);
			});
	dialog_test_data->functors.push_back([number_edit_v]()
			{
				return convert_to_string(ssprintf(L"Number edit v: %lli.",
						EnsureType<long long>(number_edit_v->Value())));
			});
	dialog_test_data->functors.push_back([number_edit_h]()
			{
				return convert_to_string(ssprintf(L"Number edit h: %lli.",
						EnsureType<long long>(number_edit_h->Value())));
			});

	auto number_edit_i_h_value_first = make_shared<int>(1);
	auto number_edit_i_h_value_last = make_shared<int>(1);

	auto number_edit_i_h_first = edit_container->CreateControl<ValueNumberEdit<int>>(
			L"Test number int-h[",
			SavedGUIValue(&*number_edit_i_h_value_first), -100, *number_edit_i_h_value_last,
			out_of_range_prohibited,
			Layout::Horizontal);
	auto number_edit_i_h_last = edit_container->CreateControl<ValueNumberEdit<int>>(
			L"Test number int-h]",
			SavedGUIValue(&*number_edit_i_h_value_last), *number_edit_i_h_value_first, 200,
			out_of_range_prohibited,
			Layout::Horizontal,
			[w_first = to_weak_ptr(number_edit_i_h_first)](int v)
			{
				printf("Test number int-h-last changed: %i.\n",
						EnsureType<int>(v));
				if (auto first = w_first.lock())
				{
					first->SetMinMax(first->Min(), v);
				}
			});
	number_edit_i_h_first->SetOnValueChanged(
			[w_last = to_weak_ptr(number_edit_i_h_last)](int v)
			{
				printf("Test number int-h-first changed: %i.\n",
						EnsureType<int>(v));
				if (auto last = w_last.lock())
				{
					last->SetMinMax(v, last->Max());
				}
			});

	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([number_edit_i_h_value_first]()
			{
				return convert_to_string(ssprintf(L"Number edit int h first: %i.",
						EnsureType<int>(*number_edit_i_h_value_first)));
			});
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([number_edit_i_h_value_last]()
			{
				return convert_to_string(ssprintf(L"Number edit int h last: %i.",
						EnsureType<int>(*number_edit_i_h_value_last)));
			});

	// [Change number]
	edit_container->CreateControl<Button>(L"Change number",
			[number_edit_i_h_last, number_edit_i_h_value_last]()
			{
				--*number_edit_i_h_value_last;
				number_edit_i_h_last->Update();
			});

	auto number_edit_u_h_value = make_shared<unsigned int>(10);
	auto number_edit_u_h = edit_container->CreateControl<ValueNumberEdit<unsigned int>>(
			L"Test number uint-h",
			SavedGUIValue(&*number_edit_u_h_value), 1, 300,
			out_of_range_prohibited,
			Layout::Horizontal,
			[](unsigned int v)
			{
				printf("Test number uint-h changed: %u.\n",
						EnsureType<unsigned int>(v));
			});
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([number_edit_u_h_value]()
			{
				return convert_to_string(ssprintf(L"Number edit uint h: %u.",
						EnsureType<unsigned int>(*number_edit_u_h_value)));
			});

	auto number_edit_llu_h_value = make_shared<unsigned long long>(20);
	auto number_edit_llu_h = edit_container->CreateControl<ValueNumberEdit<unsigned long long>>(
			L"Test number ulonglong-h",
			SavedGUIValue(&*number_edit_llu_h_value), 1, 300000,
			out_of_range_prohibited,
			Layout::Horizontal,
			[](unsigned long long v)
			{
				printf("Test number ulonglong-h changed: %llu.\n",
						EnsureType<unsigned long long>(v));
			});
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([number_edit_llu_h_value]()
			{
				return convert_to_string(ssprintf(L"Number edit ulonglong h: %llu.",
						EnsureType<unsigned long long>(*number_edit_llu_h_value)));
			});

	auto number_edit_d_h_value = make_shared<double>(pi());
	auto number_edit_d_h = edit_container->CreateControl<ValueNumberEdit<double>>(
			L"Test number double-h",
			SavedGUIValue(&*number_edit_d_h_value), -1.23456781234567812345678e10, 1e20,
			out_of_range_prohibited,
			Layout::Horizontal,
			[](double v)
			{
				printf("Test number double-h changed: %lg.\n",
						EnsureType<double>(v));
			});
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([number_edit_d_h_value]()
			{
				return convert_to_string(ssprintf(L"Number edit double h: %lg.",
						EnsureType<double>(*number_edit_d_h_value)));
			});

	auto number_edit_f_h_value = make_shared<float>(euler_e());
	auto number_edit_f_h = edit_container->CreateControl<ValueNumberEdit<float>>(
			L"Test number float-h",
			SavedGUIValue(&*number_edit_f_h_value), -1.23456781234567812345678e10, 1e20,
			out_of_range_allowed,
			Layout::Horizontal,
			[](float v)
			{
				printf("Test number float-h changed: %g.\n",
						EnsureType<float>(v));
			});
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([number_edit_f_h_value]()
			{
				return convert_to_string(ssprintf(L"Number edit float h: %g.",
						EnsureType<float>(*number_edit_f_h_value)));
			});

	auto file_name_edit_load_label = make_shared<TextLabel>(L"");
	auto file_name_edit_load = edit_container->CreateControl<FileLoadEdit>(
			L"Load file",
			SavedGUIValue(WGetApplicationPath()),
			L"*.*");
	file_name_edit_load->SetOnValueChanged(
			[file_name_edit_load_label,
					w_file_name_edit_load = to_weak_ptr(file_name_edit_load)](const wstring &v)
			{
				auto file_name_edit_load = w_file_name_edit_load.lock();
				if (!file_name_edit_load)
					return;
				file_name_edit_load_label->SetText(ssprintf(L"%ls: %ls",
						EnsureType<const wchar_t*>(file_name_edit_load->FileNameValid()?
								L"Valid": L"Invalid"),
						EnsureType<const wchar_t*>(v.c_str())));
			});
	edit_container->AddControl(file_name_edit_load_label);
	dialog_test_data->functors.push_back([file_name_edit_load]()
			{
				return convert_to_string(ssprintf(L"FileNameEdit(\"%ls\"): \"%ls\".",
						EnsureType<const wchar_t*>(file_name_edit_load->Caption().c_str()),
						EnsureType<const wchar_t*>(file_name_edit_load->Value().c_str())));
			});

	auto file_name_edit_save_value = make_shared<wstring>(WGetApplicationPath() + L".tmp");
	auto file_name_edit_save = edit_container->CreateControl<ValueFileSaveEdit>(
			L"Save file",
			SavedGUIValue(file_name_edit_save_value.get()),
			L"*.*",
			Layout::Horizontal);
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([file_name_edit_save, file_name_edit_save_value]()
			{
				return convert_to_string(ssprintf(L"FileNameEdit(\"%ls\"): \"%ls\".",
						EnsureType<const wchar_t*>(file_name_edit_save->Caption().c_str()),
						EnsureType<const wchar_t*>(file_name_edit_save_value->c_str())));
			});

	auto file_name_edit_read_dir_value = make_shared<wstring>(WGetApplicationDirectory());
	auto file_name_edit_read_dir = edit_container->CreateControl<ValueDirectoryReadEdit>(
			L"Read directory",
			SavedGUIValue(file_name_edit_read_dir_value.get()),
			Layout::Horizontal);
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([file_name_edit_read_dir, file_name_edit_read_dir_value]()
			{
				return convert_to_string(ssprintf(L"FileNameEdit(\"%ls\"): \"%ls\".",
						EnsureType<const wchar_t*>(file_name_edit_read_dir->Caption().c_str()),
						EnsureType<const wchar_t*>(file_name_edit_read_dir_value->c_str())));
			});

	auto file_name_edit_write_dir_value = make_shared<wstring>(WGetTempDirectory());
	auto file_name_edit_write_dir = edit_container->CreateControl<ValueDirectoryWriteEdit>(
			L"Write directory",
			SavedGUIValue(file_name_edit_write_dir_value.get()),
			Layout::Horizontal);
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([file_name_edit_write_dir, file_name_edit_write_dir_value]()
			{
				return convert_to_string(ssprintf(L"FileNameEdit(\"%ls\"): \"%ls\".",
						EnsureType<const wchar_t*>(file_name_edit_write_dir->Caption().c_str()),
						EnsureType<const wchar_t*>(file_name_edit_write_dir_value->c_str())));
			});

	auto text_edit_value = make_shared<wstring>(L"Line 1\r\nLine 2\rLine 3\nLine 4");
	auto text_edit_lock = make_shared<bool>(false);
	auto text_edit_enc = make_shared<StringEdit>(L"As one line encoded",
			L"",
			Layout::Vertical);
	auto text_edit = edit_container->CreateControl<ValueTextEdit>(L"Multiline text",
			SavedGUIValue(text_edit_value.get()),
			Layout::Vertical,
			[text_edit_enc, text_edit_lock](const wstring &v)
			{
				if (!*text_edit_lock)
				{
					*text_edit_lock = true;
					text_edit_enc->SetValue(encode_escape_sequences(v, L""));
					*text_edit_lock = false;
				}
				printf("Multiline text (encoded):\n%s\n",
						EnsureType<const char*>(convert_to_string(encode_escape_sequences(v, L"")).c_str()));
			});
	// Важно! Здесь захватывается и хранится shared_ptr с переменной, к которой привязан контрол.
	dialog_test_data->functors.push_back([text_edit_value]()
			{
				return convert_to_string(ssprintf(L"Multiline text: \"%ls\".",
						EnsureType<const wchar_t*>(encode_escape_sequences(*text_edit_value, L"").c_str())));
			});
	text_edit_enc->SetValue(encode_escape_sequences(text_edit->Value(), L""));
	text_edit_enc->SetOnValueChanged(
			[w_text_edit = to_weak_ptr(text_edit), text_edit_lock](const wstring &v)
			{
				if (auto text_edit = w_text_edit.lock())
				{
					if (!*text_edit_lock)
					{
						*text_edit_lock = true;
						text_edit->SetValue(decode_escape_sequences(v));
						*text_edit_lock = false;
					}
				}
			});
	edit_container->AddControl(text_edit_enc);

	// [User OK] [User Cancel]
	auto button_layout = dialog->CreateControl<ControlContainer>(Layout::Horizontal);
	button_layout->CreateControl<Stretch>();
	auto button = button_layout->CreateControl<Button>(L"User OK");
	button->SetDefault();
	dialog_test_data->buttons.push_back(button);
	button = button_layout->CreateControl<Button>(L"User Cancel", DialogResultCode::Rejected);
	dialog_test_data->buttons.push_back(button);
	button = make_shared<Button>(L"User OK: enum + dep");
	button->SetValueDependencies({edit_container, rb_fp_choice});
	dialog->AddEnumButton(button, OKCancelDialog::Result::OK, button_layout);
	dialog_test_data->buttons.push_back(button);
	button_layout->CreateControl<Stretch>();

	return dialog_test_data;
}

//--------------------------------------------------------------

void ShowTestDialogResult(const DialogTestData &dialog_test_data)
{
	wstring result;
	wstring result_name = L"(Unknown)";
	DynamicDialog::DialogResultCode result_code = DynamicDialog::DialogResultCode::Rejected;
	if (!dialog_test_data.dialog->GetResult(&result_code))
	{
		result_name = L"NULL";
	}
	else
	{
		for (auto &b: dialog_test_data.buttons)
		{
			if (dialog_test_data.dialog->GetResult() == &*b)
			{
				result_name = b->Caption();
			}
		}
	}
	result += ssprintf(L"dialog[%ls] button = %ls, code = %ls\n",
			EnsureType<const wchar_t*>(convert_to_wstring(
					typeid(*dialog_test_data.dialog).name()).c_str()),
			EnsureType<const wchar_t*>(result_name.c_str()),
			EnsureType<const wchar_t*>(result_code == decltype(result_code)::Accepted ? L"Accepted":
					result_code == decltype(result_code)::Rejected ? L"Rejected" : L"Invalid"));
	for (auto &c: dialog_test_data.checkboxes)
	{
		result += ssprintf(L"checkbox[%ls][\"%ls\"] = %i\n",
				EnsureType<const wchar_t*>(convert_to_wstring(typeid(*c).name()).c_str()),
				EnsureType<const wchar_t*>(c->Caption().c_str()),
				c->Checked()? 1: 0);
	}
	for (auto &c: dialog_test_data.radiobuttons)
	{
		result += ssprintf(L"radiobutton[%ls][\"%ls\"] = %i\n",
				EnsureType<const wchar_t*>(convert_to_wstring(typeid(*c).name()).c_str()),
				EnsureType<const wchar_t*>(c->Caption().c_str()),
				c->Checked()? 1: 0);
	}
	for (auto &c: dialog_test_data.radiobutton_containers)
	{
		auto choice = c->Choice();
		wstring choice_str = choice ? choice->Caption() : L"(null)";
		result += ssprintf(L"radiobutton_container[%ls][\"%ls\"] = %ls\n",
				EnsureType<const wchar_t*>(convert_to_wstring(typeid(*c).name()).c_str()),
				EnsureType<const wchar_t*>(c->Caption().c_str()),
				EnsureType<const wchar_t*>(choice_str.c_str()));
	}
	for (auto &c: dialog_test_data.comboboxes)
	{
		size_t choice = c->Choice();
		wstring choice_str = choice < c->Items().size() ? c->Items()[choice] : wstring(L"(null)");
		result += ssprintf(L"comboboxes[%ls][\"%ls\"] = [%zu] %ls\n",
				EnsureType<const wchar_t*>(convert_to_wstring(typeid(*c).name()).c_str()),
				EnsureType<const wchar_t*>(c->Caption().c_str()),
				EnsureType<size_t>(choice),
				EnsureType<const wchar_t*>(choice_str.c_str()));
	}
	for (auto &c: dialog_test_data.string_edits)
	{
		wstring value = c->Value();
		result += ssprintf(L"string_edits[%ls][\"%ls\"] = %ls\n",
				EnsureType<const wchar_t*>(convert_to_wstring(typeid(*c).name()).c_str()),
				EnsureType<const wchar_t*>(c->Caption().c_str()),
				EnsureType<const wchar_t*>(value.c_str()));
	}
	for (auto &c: dialog_test_data.functors)
	{
		result += ssprintf(L"functor = \"%ls\"\n",
				EnsureType<const wchar_t*>(convert_to_wstring(c()).c_str()));
	}
	ShowString(L"DD Result", result);
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestDynamicDialog()
{
	size_t start_oc = AllocCounter::ObjectCount();
	printf("Object count at start = %zu\n", start_oc);
	auto dialog_test_data = CreateTestDialog();
	ShowTestDialogResult(*dialog_test_data);
	for (;;)
	{
		auto dialog = dialog_test_data->dialog.get();

		dialog->Show();

		ShowTestDialogResult(*dialog_test_data);
		if (!dialog->GetResult())
			break;
		if (!dialog_test_data->reuse)
			break;

		if (dialog_test_data->clear_rb)
		{
			for (auto &rbc: dialog_test_data->rbc)
				rbc->SetChoice(nullptr);
		}
	}
	dialog_test_data.reset();
	size_t end_oc = AllocCounter::ObjectCount();
	printf("Object count at end = %zu\n", end_oc);
	if (end_oc != start_oc)
	{
		Error("Memory leak in dialog data detected (shared_ptr self ownership).");
	}
}

//--------------------------------------------------------------

XRAD_END

//--------------------------------------------------------------
