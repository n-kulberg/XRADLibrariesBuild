#include "pre.h"
#include "TestJson.h"
#include <fstream>
/*!
	\file
	\date 2018/07/05 17:30
	\author kulberg

	\brief 
*/

#include <XRADBasic/ThirdParty/nlohmann/json.hpp>

//	Краткие выводы по тестированию
//	1. Ввод-вывод через ostream. Юникод поддерживается только в виде utf-8, wchar_t не работает
//	2. Следить за наличием BOM в файле нужно самим
//	3. Принимает контейнеры XRAD, создает соответствующие вектора. 
//		При чтении превращает результат в vector от соответствующего значения.
//		Возможно, это повод сделать тем контейнерам конструктор от vector<T>
//	4. Почему работает п.3, пока неясно. При попытке повторить аналогичный класс 
//		на месте, ничего не получилось.
//	5.	Можно по желанию форматировать вывод отступами, управляется настройкой ofstream
//	6.	Имеется встроенный контроль типов данных, при попытке некорректного преобразования
//		дает исключение

using json = nlohmann::json;
//using json = nlohmann::basic_json<std::map, std::vector, std::wstring>; не работает

XRAD_BEGIN

void	GenerateJsonFile(string filename)
{
	json	j;
	json	j1;


//	wstring в ключе не проходит
// 	j1[L"unicode"] = L"Юникод";
// 	j1[L"维基"] = L"百科";

// 	j1["unicode"] = L"Юникод";
// 	j1["维基"] = L"百科";

	j1[u8"unicode"] = u8"Юникод";
	j1[u8"维基"] = u8"百科";

	j["double"] = 3.1415926;
	j["Doctor ID"] = j1;

	j["added"] = j;

	vector<string>	vs(3, "str");
	j["array_str"] = vs;

	vector<double>	vd(3, 2.5);
	j["array_d"] = vd;

	DataArray<double>	rfd(3, 2.5);
	j["function_d"] = rfd;// проходит

	point3_UI32 p(1, 2, 3);
	j["p3"] = p;// проходит

	j["sub"] = j;

	const auto cj(j);

	json cccj = j["sub1"];//несуществующее поле, должен вернуть 0
//	double s = cj["double"];// const, существующее поле, возвращает значение
	//json ccj = cj["sub1"];//const, несуществующий ключ - неопределенное поведение

 	//попытка понять, почему она принимает контейнеры XRAD, создан упрощенный класс, в который понемногу добавляются элементы, приближающие его к встроенным типам
	struct abra_kadabra
	{
		double d;
		abra_kadabra() : d(0){}
		abra_kadabra(double v) : d(v){}
		operator double &(){ return d; } //не помогло
		typedef double *iterator;
		iterator begin(){ return &d; }
		iterator end(){ return (&d)+1; }
		const iterator cbegin(){ return &d; }
		const iterator cend(){ return (&d)+1; }
	};
// 	j["abra_kadabra"] = abra_kadabra();


	ofstream	out_file;
	out_file.open(filename);
	out_file << "\xEF\xBB\xBF";//utf-8 BOM

//	nlohmann::detail::serializer<nlohmann::basic_json>	ser(out_file, "\t");
	
	out_file.width(1);//параметр потока должен быть установлен, чтобы вывод форматировался с отступами
	out_file.fill('\t');// отступы табуляцией

	out_file << j;
//	out_file.close();
}

void	TestJson()
{
	string	filename = "c:/temp/json.json";
	GenerateJsonFile(filename);


	ifstream	in_file;
	in_file.open("c:/temp/json.json");

	
	json	j1;
	in_file >> j1;
	
	in_file.clear();
	in_file.seekg(0, ios::beg);

	json j = json::parse(in_file);

	vector<double> vd = j["function_d"];
	//vector<double> vs = j["array_str"]; exception
	//string	s = j["array_s"]; exception

//	json j2(in_file);bad idea!

}


XRAD_END

