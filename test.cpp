// rtti.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include "pch.h"
#include "rtti.h"
#include <iostream>
#include <vector>
#include <array>
#include <any>
#include <string>


#define CATCH_CONFIG_MAIN
#include "catch.hpp"


template<typename T> auto initializer();


rtti_impl(std::string,
	.display_name("std::string")
	.properties(
		property("c_str").delegate(
			+[](const std::string& o) { return o.c_str(); })));

struct Base
{
	Base() {}
	Base(int val) { m_b_v0 = val; }
	virtual~Base() {}

	int		m_b_v0 = 11;
	std::string		m_string = "abcd";
	int			m_array[2][4] = {
		{ 11, 12, 13, 14 },
		{ 21, 22, 23, 24 },
	};

	int m_modify_by_method = 50;
	int get() const { return m_modify_by_method; }
	void set(const int& v) { m_modify_by_method = v; }

	int method(int arg) { return arg*20; }

	rtti_class_decl(Base);
};
rtti_class_impl(Base,
	.display_name("Base")
	.constructors(
		constructor<>(),
		constructor<int>())
	.properties(
		property("b_v0").member(&Base::m_b_v0),
		property("string").member(&Base::m_string),
		property("array").member(&Base::m_array),
		property("method").member(&Base::get, &Base::set),
		property("delegate").delegate(
			+[](const Base& o) { return o.m_b_v0 * 10; },
			+[](Base& o, const int& value) { o.m_b_v0 = value / 10; }))
	.methods(
		method("method").invoker(&Base::method),
		method("delegate").invoker(+[](int value) { return value*30; })));

struct MyAttribute : public rtti::attribute
{
	std::string_view description;

	constexpr MyAttribute(const char* desc)
		: description(desc)
	{}
};

struct MyClass : public Base
{
	int		m_v0 = 22;

	rtti_class_decl(MyClass);
};
rtti_class_impl(MyClass,
	.bases<Base>()
	.display_name("MyClass")
	.attributes(
		MyAttribute("myattribute"))
	.properties(
		property("v0").member(&MyClass::m_v0)));

struct MyClass2
{
	int			m_value = 33;
};
rtti_impl(MyClass2,
	.display_name("MyClass2")
	.properties(
		property("value").member(&MyClass2::m_value)));


const auto& void_type = rtti::get_type_view<void>();


TEST_CASE("value", "[rtti]")
{
	SECTION("cast")
	{
		rtti::value v;

		int i = 10;

		v = i;
		CHECK(rtti::value_cast<int>(v, 0) == 10);

		v = &i;
		CHECK(*rtti::value_cast<int*>(v, nullptr) == 10);
		CHECK(*rtti::value_cast<const int*>(v, nullptr) == 10);

		v = const_cast<const int*>(&i);
		CHECK(*rtti::value_cast<const int*>(v, nullptr) == 10);

		const int ci = 20;
		v = ci;
		CHECK(rtti::value_cast<int>(v, 0) == 20);

		int ai[2] = {11, 12};
		v = ai;
		CHECK(rtti::value_cast<int*>(v, nullptr)[1] == 12);

		int ai2[2][2] = { { 11, 12 }, { 21, 22} };
		v = ai2;
		CHECK(rtti::value_cast<int(*)[2]>(v, nullptr)[1][1] == 22);

		std::string str = "abcd";
		v = str;
		CHECK(rtti::value_cast<std::string>(v, "").compare("abcd") == 0);
	}

	SECTION("cast for object")
	{
		MyClass object;

		rtti::value v = object;
		CHECK(rtti::value_cast_object<MyClass>(v) == nullptr);
		CHECK(rtti::value_cast_object<const MyClass>(v));

		v = &object;
		CHECK(rtti::value_cast_object<MyClass>(v));
		CHECK(rtti::value_cast_object<const MyClass>(v));

		v = const_cast<const MyClass*>(&object);
		CHECK(rtti::value_cast_object<MyClass>(v) == nullptr);
		CHECK(rtti::value_cast_object<const MyClass>(v));

		CHECK(rtti::value_cast_object<const Base>(v));

		v = static_cast<Base*>(&object);
		CHECK(rtti::value_cast_object<Base>(v));
		CHECK(rtti::value_cast_object<MyClass>(v));

		Base object2;
		v = &object2;
		CHECK(rtti::value_cast_object<MyClass>(v) == nullptr);
	}
}

TEST_CASE("property", "[rtti]")
{
	SECTION("member")
	{
		Base object;

		auto property = object.rtti_type_view().properties().get("b_v0");
		CHECK(property);

		CHECK(rtti::value_cast<int>(property->get(&object), 0) == 11);

		property->set(&object, 101);
		CHECK(object.m_b_v0 == 101);

		auto p0 = rtti::value_cast<int*>(property->ref(&object), nullptr);
		CHECK(p0);
		CHECK(*p0 == 101);
		
		auto p1 = rtti::value_cast<const int*>(
			property->cref(const_cast<const Base*>(&object)), nullptr);
		CHECK(p1);
		CHECK(*p1 == 101);

		*p0 = 2020;
		CHECK(*p1 == 2020);
	}

	SECTION("array member")
	{
		Base object;

		auto property = object.rtti_type_view().properties().get("array");
		CHECK(property);

		CHECK(rtti::value_cast<int>(property->get(&object, rtti::index { 1, 1 }), 0) == 22);

		property->set(&object, 99, rtti::index { 1, 3 });
		CHECK(rtti::value_cast<int>(property->get(&object, rtti::index { 1, 3 }), 0) == 99);

		auto p0 = rtti::value_cast<int*>(property->ref(&object, rtti::index { 0, 2 }), nullptr);
		CHECK(p0);
		*p0 = 111;

		auto value = property->cref(const_cast<const Base*>(&object), rtti::index{ 0, 2 });
		CHECK(rtti::value_cast<int*>(value, nullptr) == nullptr);
		CHECK(*rtti::value_cast<const int*>(value, nullptr) == 111);
	}

	SECTION("object member")
	{
		Base object;

		auto prop_str = object.rtti_type_view().properties().get("string");
		auto& prop_str_type = prop_str->value_type();

		auto c_str_prop = prop_str_type.properties().get("c_str");
		auto vstr = c_str_prop->get(prop_str->ref(&object));
		auto pstr = rtti::value_cast<const char*>(vstr, "");
		CHECK(strcmp(pstr, "abcd") == 0);
	}

	SECTION("method")
	{
		Base object;

		auto prop_method = object.rtti_type_view().properties().get("method");
		CHECK(rtti::value_cast(prop_method->get(&object), 0) == 50);

		prop_method->set(&object, 150);
		CHECK(rtti::value_cast(prop_method->get(&object), 0) == 150);
	}

	SECTION("delegate")
	{
		Base object;

		auto property = object.rtti_type_view().properties().get("delegate");
		CHECK(property);

		CHECK(rtti::value_cast<int>(property->get(&object), 0) == 110);

		property->set(&object, 1010);
		CHECK(rtti::value_cast<int>(property->get(&object), 0) == 1010);
	}

	SECTION("derivered class property")
	{
		MyClass object;

		auto property = object.rtti_type_view().properties().get("b_v0");
		CHECK(property);

		CHECK(rtti::value_cast<int>(property->get(&object), 0) == 11);

		property->set(&object, 101);
		CHECK(object.m_b_v0 == 101);

		auto property2 = object.rtti_type_view().properties().get("v0");
		CHECK(property2);

		CHECK(rtti::value_cast<int>(property2->get(&object), 0) == 22);

		property2->set(&object, 301);
		CHECK(object.m_v0 == 301);
	}

	SECTION("dependency descripted class")
	{
		MyClass2 object;
		auto& type = rtti::get_type_view<decltype(object)>();

		auto property = type.properties().get("value");
		CHECK(property);

		CHECK(rtti::value_cast<int>(property->get(&object), 0) == 33);

		property->set(&object, 101);
		CHECK(object.m_value == 101);
	}
}

TEST_CASE("method", "[rtti]")
{
	Base object;

	auto method0 = object.rtti_type_view().methods().get("method");
	auto result = method0->invoke(&object, { 30 });
	CHECK(rtti::value_cast<int>(result, 0) == 600);

	auto method1 = object.rtti_type_view().methods().get("delegate");
	result = method1->invoke(nullptr, { 40 });
	CHECK(rtti::value_cast<int>(result, 0) == 1200);

	CHECK(method1->result_type().is<int>());

	auto args = method1->arguments_type();
	CHECK(args.size() == 1);
	CHECK(args.begin()->is<int>());
}

TEST_CASE("construction", "[rtti]")
{
	auto& base_type = rtti::get_type_view<Base>();
	
	auto p0 = rtti::value_cast_object<Base>(base_type.instantiate({}));
	CHECK(p0);
	CHECK(p0->m_b_v0 == 11);
	delete p0;
	
	auto p1 = rtti::value_cast_object<Base>(base_type.instantiate({99}));
	CHECK(p1);
	CHECK(p1->m_b_v0 == 99);
	delete p1;

	auto p2 = rtti::value_cast_object<Base>(base_type.instantiate({ nullptr }));
	CHECK(p2 == nullptr);
}

TEST_CASE("meta", "[rtti]")
{
	SECTION("type")
	{
		auto& type_int = rtti::get_type_view<int>();
		CHECK(!type_int.is_const());
		CHECK(!type_int.is_volatile());
		CHECK(!type_int.is_pointer());
		CHECK(!type_int.is_reference());
		CHECK(type_int.rank() == 0);

		auto& type_const_int = rtti::get_type_view<const int>();
		CHECK(type_const_int.is_const());
		CHECK(!type_const_int.is_volatile());
		CHECK(!type_const_int.is_pointer());
		CHECK(!type_const_int.is_reference());
		CHECK(type_const_int.rank() == 0);

		auto& type_int_reference = rtti::get_type_view<int&>();
		CHECK(!type_int_reference.is_const());
		CHECK(!type_int_reference.is_volatile());
		CHECK(!type_int_reference.is_pointer());
		CHECK(type_int_reference.is_reference());
		CHECK(type_int_reference.rank() == 0);

		auto& type_int_const_reference = rtti::get_type_view<const int&>();
		CHECK(!type_int_const_reference.is_const());
		CHECK(!type_int_const_reference.is_volatile());
		CHECK(!type_int_const_reference.is_pointer());
		CHECK(type_int_const_reference.is_reference());
		CHECK(type_int_const_reference.rank() == 0);

		auto& type_int_pointer = rtti::get_type_view<int*>();
		CHECK(!type_int_pointer.is_const());
		CHECK(!type_int_pointer.is_volatile());
		CHECK(type_int_pointer.is_pointer());
		CHECK(!type_int_pointer.is_reference());
		CHECK(type_int_pointer.rank() == 0);

		auto& type_int_const_pointer = rtti::get_type_view<const int*>();
		CHECK(!type_int_const_pointer.is_const());
		CHECK(!type_int_const_pointer.is_volatile());
		CHECK(type_int_const_pointer.is_pointer());
		CHECK(!type_int_const_pointer.is_reference());
		CHECK(type_int_const_pointer.rank() == 0);

		auto& type_int_array = rtti::get_type_view<int[1][1]>();
		CHECK(!type_int_array.is_const());
		CHECK(!type_int_array.is_volatile());
		CHECK(!type_int_array.is_pointer());
		CHECK(!type_int_array.is_reference());
		CHECK(type_int_array.rank() == 2);

		auto& type_array_reference = rtti::get_type_view<int(&)[1][1]>();
		CHECK(!type_array_reference.is_const());
		CHECK(!type_array_reference.is_volatile());
		CHECK(!type_array_reference.is_pointer());
		CHECK(type_array_reference.is_reference());
		CHECK(type_array_reference.rank() == 0);
	}

	SECTION("type comparison")
	{
		auto& type_int = rtti::get_type_view<int>();
		auto& type_const_int = rtti::get_type_view<const int>();
		auto& type_int_array = rtti::get_type_view<int[1][1]>();

		// type comparison
		CHECK(type_int != type_const_int);
		CHECK(type_int == type_const_int.unconst_type());
		CHECK(type_int.const_type() == type_const_int);
		CHECK(type_int_array.decay_type() == rtti::get_type_view<int(*)[1]>());
	}

	SECTION("attributes")
	{
		auto& type = rtti::get_type_view<MyClass>();
		CHECK(type.display_name().compare("MyClass") == 0);

		CHECK(type.bases().size() == 1);
		for (auto& base : type.bases())
		{
			CHECK(base.display_name().compare("Base") == 0);
		}

		auto props = type.properties();
		CHECK(type.properties().size() == 6);

		auto prop_b_v0 = props.get("b_v0");
		CHECK(prop_b_v0);
		CHECK(prop_b_v0->value_type().name().compare("int") == 0);

		auto prop_v0 = props.get("v0");
		CHECK(prop_v0);
		CHECK(prop_v0->value_type().name().compare("int") == 0);

		auto attr = type.attributes().get<MyAttribute>();
		CHECK(attr);
		CHECK(attr->description.compare("myattribute") == 0);
	}

	SECTION("visit")
	{
		int count = 0;
		rtti::visit_all_types([&](const rtti::type_view& type) 
		{
				++count;
				return true;
		});
		CHECK(count == 4);
	}
}

TEST_CASE("cast", "[rtti]")
{
	MyClass myclass;
	Base base;

	MyClass* cast0 = rtti::object_cast<MyClass>(&base);
	CHECK(cast0 == nullptr);
	
	Base* cast1 = rtti::object_cast<Base>(&myclass);
	CHECK(cast1 != nullptr);

	MyClass* cast2 = rtti::object_cast<MyClass>(cast1);
	CHECK(cast2 != nullptr);

	const Base* cast3 = rtti::object_cast<Base>((const MyClass*)&myclass);
	CHECK(cast3 != nullptr);

	const MyClass* cast4 = rtti::object_cast<MyClass>(cast3);
	CHECK(cast4 != nullptr);
}