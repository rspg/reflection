#pragma once

#include <functional>

#define RTTI(Class)
#define RTTI_IMPL(Class)
#define PROPERTY(Name, )
#define PROPERTY_OBJECT(Name, Type)
#define PROPERTY_POINTER(Name, Type)


class property
{
public:
	enum attribute
	{
		attribute_readonly,
		attribute_noserialize,
		attribute_hide,
	};
	enum type
	{
		type_int,
		type_unsigned_int,
		type_float,
		type_double,
		type_object,
		type_pointer,
	};

	void get(void* p, size_t index = 0)
	{
		if (m_customGetter)
			m_customGetter(p, index);
		property_access_provider::get(type(), p, index);
	}
	void set(const void* p, size_t index = 0)
	{
		if (m_customSetter)
			m_customSetter(p, index);
		property_access_provider::set(type(), p, index);
	}

	const char* name();
	const char* virtual_name();

	bool is_attribute();

	type type();

private:
	std::function<void(void*, size_t)>			m_customGetter;
	std::function<void(const void*, size_t)>	m_customSetter;
	std::function<size_t()>						m_count;
};

class metadata
{
public:
	const char* name();
	const char* virtual_name();
	const metadata* parent();

	is_convertable(const metadata&);

	count();
	members();
};

