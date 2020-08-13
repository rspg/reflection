#pragma once

#include <xstring>
#include <tuple>

namespace rtti
{
	namespace impl
	{
		template<size_t Index, typename T = void, typename ...Rest>
		struct type_element
			: public type_element<Index - 1, Rest...>
		{};
		template<typename T, typename ...Rest>
		struct type_element<0, T, Rest...>
		{
			using type = T;
		};

		template<typename ...T>
		struct type_list
		{
			static constexpr size_t size = sizeof...(T);
			template<size_t Index> using at = typename type_element<Index, T...>::type;

			template<typename ...Appending>
			using add_front = type_list<Appending..., T...>;
			template<typename ...Appending>
			using add_back = type_list<T..., Appending...>;

			template<template<typename...> typename Type>
			using expand = Type<T...>;

			template<typename L, size_t Max = size, size_t Index = 0>
			static constexpr size_t find()
			{
				if constexpr (Max > Index)
				{
					if constexpr (std::is_same_v<L, at<Index>>)
						return Index;
					else if constexpr (Max > Index + 1)
						return find<L, Max, Index + 1>();
				}
				return SIZE_MAX;
			}
		};


		template<typename Class, typename Result, typename... Args>
		struct invoker_traits_impl
		{
			using result_type = Result;
			using arguments_type = type_list<Args...>;
		};

		template<typename Class, typename Result, typename... Args>
		constexpr auto invoker_traits_functor_impl(Result(Class::*)(Args...)const)
		{
			return invoker_traits_impl<Class, Result, Args...>{};
		}
		template<typename Result, typename... Args>
		constexpr auto invoker_traits_function_impl(Result(*)(Args...))
		{
			return invoker_traits_impl<void, Result, Args...>{};
		}

		template<typename Fn, typename = void> struct invoker_traits;

		template<typename Fn>
		struct invoker_traits<Fn, std::void_t<decltype(&Fn::operator())>> : public decltype(invoker_traits_functor_impl(&Fn::operator()))
		{};
		template<typename Fn>
		struct invoker_traits<Fn, std::enable_if_t<std::is_function_v<std::remove_pointer_t<Fn>>>> : public decltype(invoker_traits_function_impl(static_cast<Fn>(nullptr)))
		{};

		template<typename T>
		constexpr const char* generate_type_name()
		{
#if defined(__clang__)
			return __PRETTY_FUNCTION__; 
#elif defined(_MSC_VER)
			return __FUNCSIG__;
#else
			#error "is not supported."
#endif
		}

		template<typename T>
		constexpr std::string_view get_type_name()
		{
			constexpr auto name = std::string_view(generate_type_name<T>());
#if defined(__clang__)
			constexpr auto begin = name.find("[T = ") + 5;
			constexpr auto end = name.rfind(']');
#elif defined(_MSC_VER)
			constexpr auto begin = name.find('<') + 1;
			constexpr auto end = name.rfind('>');
#else
			#error "is not supported."
#endif
			return name.substr(begin, end - begin);
		}

		using type_id_t = unsigned long long;

		constexpr type_id_t hash(std::string_view cname)
		{
			type_id_t result = 1125899906842597ULL;
			for (auto i = cname.begin(); i != cname.end(); ++i)
			{
				result &= std::numeric_limits<type_id_t>::max() >> 5;
				result = (31 * result + *i);
			}
			return (type_id_t)result;
		}

		template<typename T> constexpr type_id_t get_type_id()
		{
			return hash(get_type_name<T>());
		}

		struct noncopyable
		{
			constexpr noncopyable() {};
			constexpr noncopyable(const noncopyable&) = delete;
			constexpr noncopyable& operator=(const noncopyable&) = delete;
		};

		template<typename Iterator>
		class iterator_range
		{
		public:
			using iterator = Iterator;

			constexpr iterator_range()
			{}
			constexpr iterator_range(iterator begin_itr, iterator end_itr)
				: m_begin(begin_itr)
				, m_end(end_itr)
			{}

			constexpr iterator begin() const { return m_begin; }
			constexpr iterator end() const { return m_end; }
			constexpr size_t size() const { return m_end - m_begin; }
		private:
			iterator	m_begin = nullptr;
			iterator	m_end = nullptr;
		};

		template<typename View, typename... Types>
		class view_mapped_array
		{
		private:
			using instance_type = std::tuple<Types...>;
			using view_array_type = std::array<View, sizeof...(Types)>;
		public:
			using value_type = View;
			using instance_value_type = type_list<Types...>;
			using iterator = const value_type*;

			constexpr view_mapped_array(Types&&... args)
				: m_instances{ args... }
				, m_views{ make_view_array(std::make_index_sequence<sizeof...(Types)>()) }
			{}
			constexpr view_mapped_array(const view_mapped_array& r)
				: m_instances(r.m_instances)
				, m_views{ make_view_array(std::make_index_sequence<sizeof...(Types)>()) }
			{}
			view_mapped_array& operator=(const view_mapped_array&) = delete;

			// instance method
			template<size_t Index>
			constexpr const auto& at() const
			{
				return std::get<Index>(m_instances);
			}

			// views
			constexpr size_t size() const { return m_views.size(); }
			constexpr iterator begin() const { return m_views.data(); }
			constexpr iterator end() const { return m_views.data() + size(); }

		private:
			instance_type		m_instances;
			view_array_type		m_views;

			template<size_t... Indices>
			constexpr view_array_type make_view_array(std::index_sequence<Indices...>)
			{
				return { std::get<Indices>(m_instances)... };
			}

		protected:
			constexpr const instance_type& instances() const { return m_instances; }
		};
		template<typename View>
		class view_mapped_array<View>
		{
		public:
			using value_type = View;
			using instance_value_type = type_list<>;
			using iterator = const value_type*;

			constexpr view_mapped_array() {}
			constexpr size_t size() const { return 0; }
			constexpr iterator begin() const { return nullptr; }
			constexpr iterator end() const { return nullptr; }
			constexpr iterator_range<iterator> range() const
			{
				return { begin(), end() };
			}
		};

		template<typename Iterator, typename Pred>
		constexpr Iterator constexpr_find_if(Iterator begin, Iterator end, Pred&& pred)
		{
			for (auto itr = begin; itr != end; ++itr)
				if (pred(*itr)) return itr;
			return end;
		}


		template<
			template<typename...> typename T,
			typename... Types1, size_t... Indices1,
			typename... Types2, size_t... Indices2> constexpr auto concat_array(const T<Types1...>& a1, std::index_sequence<Indices1...>, const T<Types2...>& a2, std::index_sequence<Indices2...>)
		{
			return T{ std::move(a1.template at<Indices1>())..., std::move(a2.template at<Indices2>())... };
		}

		template<
			template<typename...> typename T,
			typename... Types1,
			typename... Types2> constexpr auto concat_array(const T<Types1...>& a1, const T<Types2...>& a2)
		{
			return concat_array(a1, std::make_index_sequence<sizeof...(Types1)>(), a2, std::make_index_sequence<sizeof...(Types2)>());
		}

		class property_view;
		class attribute_view;
		class type_view;

		template<typename T, typename = void>
		struct has_runtime_type
			: public std::false_type
		{};
		template<typename T>
		struct has_runtime_type<T, std::enable_if_t<std::is_same_v<
			decltype(reinterpret_cast<std::remove_pointer_t<std::decay_t<T>>*>(0)->rtti_type_view()), const type_view&>>>
			: public std::true_type
		{};
		template<typename T> constexpr bool has_runtime_type_v = has_runtime_type<T>::value;

		template<typename T> const type_view& get_type_view();
		template<typename T> const type_view& get_type_view(T&& object)
		{
			if constexpr (has_runtime_type_v<T>)
			{
				const type_view& get_type_view_once_pointer(const type_view&);
				const type_view& get_type_view_const(const type_view&);

				if constexpr (std::is_pointer_v<std::decay_t<T>>)
				{
					if(object == nullptr)
					{
						return get_type_view<std::decay_t<T>>();
					}
					else
					{
						auto result = &object->rtti_type_view();
						if constexpr (std::is_const_v<std::remove_pointer_t<std::decay_t<T>>>)
							result = &get_type_view_const(*result);
						return get_type_view_once_pointer(*result);
					}
				}
				else
					return object.rtti_type_view();
			}
			else
			{
				return get_type_view<std::decay_t<T>>();
			}
		}

		class value;
		template<typename T> const T* value_cast(const value&);
		template<typename T> T* value_cast_object(const value&);

		class value
		{
			template<typename T> friend const T* value_cast(const value&);
			template<typename T> friend T* value_cast_object(const value&);
		public:
			value()
			{}
			value(value&& value)
				: m_type(value.m_type)
				, m_storage(value.m_storage)
			{
				value.m_type = nullptr;
			}
			~value()
			{
				reset();
			}

			template<typename T>
			value(T&& value) { assign(std::forward<T>(value)); }

			template<typename T>
			value& operator=(T&& value) { assign(std::forward<T>(value)); return *this; }
			value& operator=(value&& val) 
			{
				m_type = val.m_type;
				m_storage = val.m_storage;
				val.m_type = nullptr;
				return *this;
			}

			void reset()
			{
				if (m_type == nullptr)
					return;

				if (!m_storage.small.is_small)
				{
					m_storage.large->destructor(m_storage.large->buffer);
					free(m_storage.large);
				}
			}

			bool has_value() const { return m_type; }

			const type_view& type() const
			{
				if (m_type)
					return *m_type;
				return get_type_view<void>();
			}

		private:
			struct small_value
			{
				char buffer[sizeof(void*) + 15];
				bool is_small : 1;
			};
			struct large_value
			{
				void(*destructor)(void*);
				char buffer[1];
			};
			union storage
			{
				small_value  small;
				large_value* large;
			};

			const type_view*	m_type = nullptr;
			storage				m_storage;

			template<typename T>
			void assign(T&& v)
			{
				reset();

				using value_type = std::decay_t<T>;
				static_assert(!std::is_same_v<value_type, value>);
				static_assert(std::is_copy_constructible_v<value_type>);

				constexpr size_t value_size = sizeof(value_type);

				void* addr;
				if constexpr (value_size <= sizeof(m_storage.small.buffer) && std::is_trivially_destructible_v<value_type>)
				{
					addr = m_storage.small.buffer;
					m_storage.small.is_small = true;
				}
				else
				{
					m_storage.large = reinterpret_cast<large_value*>(malloc(value_size + sizeof(large_value)));
					m_storage.large->destructor = [](void* p) {
						reinterpret_cast<value_type*>(p)->~value_type();
					};
					addr = m_storage.large->buffer;
					m_storage.small.is_small = false;
				}

				m_type = &get_type_view(*(new(addr) value_type(std::forward<T>(v))));
			}

			void* buffer_address() const
			{
				return m_storage.small.is_small ? const_cast<char*>(m_storage.small.buffer) : m_storage.large->buffer;
			}
		};

		using arguments = const std::initializer_list<value>&;

		class attribute
		{};

		class attribute_view
		{
		public:
			template<typename T>
			constexpr attribute_view(const T& instance)
				: m_instance(instance)
				, m_type_id(get_type_id<T>())
			{}

			constexpr type_id_t type_id() const { return m_type_id; }

			template<typename T>
			const T* cast() const
			{
				if (is_same_type<T>())
					return static_cast<const T*>(&m_instance);
				return nullptr;
			}

			template<typename T>
			bool is_same_type() const
			{
				return m_type_id == get_type_id<T>();
			}

		private:
			const attribute& m_instance;
			type_id_t m_type_id;
		};

		class attribute_iterable
			: public iterator_range<view_mapped_array<attribute_view>::iterator>
		{
			using base_class = iterator_range<view_mapped_array<attribute_view>::iterator>;
		public:

			constexpr attribute_iterable()
			{}
			constexpr attribute_iterable(iterator begin, iterator end)
				: base_class(begin, end)
			{}

			template<typename T>
			bool is() const { return is(get_type_id<T>()); }

			template<typename T>
			const T* get() const
			{
				auto v = get(get_type_id<T>());
				return v ? v->template cast<T>() : nullptr;
			}

		protected:

			constexpr bool is(type_id_t type_id) const
			{
				return constexpr_find_if(begin(), end(), [type_id](const auto& i)
					{
						return i.type_id() == type_id;
					}) != end();
			}

			constexpr const attribute_view* get(type_id_t type_id) const
			{
				auto itr = constexpr_find_if(begin(), end(), [type_id](const auto& i)
					{
						return i.type_id() == type_id;
					});
				return itr == end() ? nullptr : itr;
			}
		};

		template<typename ...Types>
		class attribute_array
			: public view_mapped_array<attribute_view, Types...>
		{
			using base_class = view_mapped_array<attribute_view, Types...>;
		public:
			using iterator = typename base_class::iterator;

			constexpr attribute_array(Types&&... args)
				: base_class(std::forward<Types>(args)...)
			{}
			constexpr attribute_array(const attribute_array& r)
				: base_class(r)
			{}

			constexpr attribute_iterable iterable() const
			{
				return { base_class::begin(), base_class::end() };
			}

			template<typename T>
			constexpr bool is() const
			{
				return base_class::instance_value_type::template find<T>() != SIZE_MAX;
			}
			template<typename T>
			constexpr const T* get() const
			{
				constexpr size_t index = base_class::instance_value_type::template find<T>();
				if constexpr (index != SIZE_MAX)
					return &std::get<index>(base_class::instances());
				return nullptr;
			}
		};

		class index_base
		{
		public:
			virtual size_t operator[](size_t rank) const = 0;
			virtual size_t get_rank() const = 0;

			template<typename Tuple>
			Tuple make_tuple() const
			{
				return make_tuple_impl<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>());
			}

		private:
			template<typename Tuple, size_t... Indices>
			Tuple make_tuple_impl(std::index_sequence<Indices...>) const
			{
				return Tuple{ (*this)[Indices]... };
			}
		};

		template<typename ...Types>
		class index : public index_base
		{
		public:
			static constexpr size_t rank = sizeof...(Types);

			index(Types... args)
				: m_index{ ((size_t)args)... }
			{}

			virtual size_t operator[](size_t i) const override
			{
				return i < rank ? m_index[i] : 0;
			}
			virtual size_t get_rank() const override
			{
				return rank;
			}

		private:
			std::array<size_t, rank>	m_index;
		};
		template<>
		class index<> : public index_base
		{
		public:
			static constexpr size_t rank = 0;
			virtual size_t operator[](size_t i) const override
			{
				return 0;
			}
			virtual size_t get_rank() const override
			{
				return rank;
			}
		};

		template<size_t Ranks, typename... Types>
		struct index_tuple
			: public index_tuple<Ranks - 1, size_t, Types...>
		{};
		template<typename... Types>
		struct index_tuple<0, Types...>
		{
			using type = typename std::tuple<Types...>;
		};
		template<size_t Ranks> using index_tuple_t = typename index_tuple<Ranks>::type;

		template<typename T = void, typename Value = int, size_t Ranks = 0>
		struct property_invoker
		{
			using index_type = index_tuple_t<Ranks>;
			using value_type = Value;
			using object_type = T;

			value_type get(const object_type* object, const index_type& index) const { return {}; }
			void set(object_type* object, const value_type& value, const index_type& index) const {}
			value_type* ref(object_type* object, const index_type& index) const { return {}; }
			const value_type* cref(const object_type* object, const index_type& index) const { return {}; }
			bool is_read_only() const { return false; }
		};

		template<typename Class, typename Value, typename BaseClass = property_invoker<Class, std::remove_cv_t<Value>>>
		struct property_member_invoker : public BaseClass
		{
			using index_type = typename BaseClass::index_type;
			using value_type = typename BaseClass::value_type;
			using object_type = typename BaseClass::object_type;
			Value(Class::* m_member);

			constexpr property_member_invoker(Value(Class::* member))
				: m_member(member)
			{}

			value_type get(const object_type* object, const index_type& index) const
			{
				return object->*m_member;
			}
			void set(object_type* object, const value_type& value, const index_type& index) const
			{
				if constexpr (!is_read_only())
				{
					(object->*m_member) = value;
				}
			}
			value_type* ref(object_type* object, const index_type& index) const
			{
				return &(object->*m_member);
			}
			const value_type* cref(const object_type* object, const index_type& index) const
			{
				return &(object->*m_member);
			}
			static constexpr bool is_read_only() { return !std::is_copy_assignable_v<Value>; }
		};

		template<typename Class, typename Value, typename BaseClass = property_invoker<Class, std::remove_cv_t<std::remove_all_extents_t<Value>>, std::rank_v<Value>>>
		struct property_member_indexed_invoker : public BaseClass
		{
			static_assert(std::is_array_v<Value>);

			using index_type = typename BaseClass::index_type;
			using value_type = typename BaseClass::value_type;
			using object_type = typename BaseClass::object_type;
			Value(Class::* m_member);

			constexpr property_member_indexed_invoker(Value(Class::* member))
				: m_member(member)
			{}

			template<size_t Rank, typename T>
			static auto& get(T& valueArray, const index_type& index)
			{
				if constexpr (Rank == std::rank_v<Value>)
					return valueArray;
				else
					return get<Rank + 1>(valueArray[std::get<Rank>(index)], index);
			}
			value_type get(const object_type* object, const index_type& index) const
			{
				return get<0>(object->*m_member, index);
			}

			template<size_t Rank, typename T>
			static value_type& set(T& valueArray, const index_type& index)
			{
				if constexpr (Rank == std::rank_v<Value>)
					return valueArray;
				else
					return set<Rank + 1>(valueArray[std::get<Rank>(index)], index);
			}
			void set(object_type* object, const value_type& value, const index_type& index) const
			{
				if constexpr (!is_read_only())
				{
					set<0>(object->*m_member, index) = value;
				}
			}

			value_type* ref(object_type* object, const index_type& index) const
			{
				return &get<0>(object->*m_member, index);
			}
			const value_type* cref(const object_type* object, const index_type& index) const
			{
				return &get<0>(object->*m_member, index);
			}

			static constexpr bool is_read_only() { return !std::is_copy_assignable_v<std::remove_all_extents_t<Value>>; }
		};

		template<typename Class, typename Value, typename BaseClass = property_invoker<Class, Value>>
		struct property_method_invoker : public BaseClass
		{
			using index_type = typename BaseClass::index_type;
			using value_type = typename BaseClass::value_type;
			using object_type = typename BaseClass::object_type;
			using getter_type = Value(Class::*)() const;
			using setter_type = void(Class::*)(const Value&);
			getter_type		m_getter;
			setter_type		m_setter;

			constexpr property_method_invoker(getter_type getter, setter_type setter)
				: m_getter(getter)
				, m_setter(setter)
			{}

			value_type get(const object_type* object, const index_type& index) const
			{
				return (object->*m_getter)();
			}
			void set(object_type* object, const value_type& value, const index_type& index) const
			{
				if (!is_read_only())
					(object->*m_setter)(value);
			}

			constexpr bool is_read_only() const { return m_setter == nullptr; }
		};

		template<typename Class, typename Value, class BaseClass = property_invoker<Class, Value>>
		struct property_delegate_invoker : public  BaseClass
		{
			using index_type = typename BaseClass::index_type;
			using value_type = typename BaseClass::value_type;
			using object_type = typename BaseClass::object_type;
			using getter_type = Value(*)(const Class&);
			using setter_type = void(*)(Class&, const Value&);
			getter_type		m_getter;
			setter_type		m_setter;

			constexpr property_delegate_invoker(getter_type getter, setter_type setter)
				: m_getter(getter)
				, m_setter(setter)
			{}

			value_type get(const object_type* object, const index_type& index) const
			{
				return m_getter(*object);
			}
			void set(object_type* object, const value_type& value, const index_type& index) const
			{
				if (!is_read_only())
					m_setter(*object, value);
			}

			constexpr bool is_read_only() const { return m_setter == nullptr; }
		};

		template<typename Type = void, typename Invoker = property_invoker<>, typename Attributes = type_list< >>
		class property
		{
		public:
			using attribute_set = typename Attributes::template expand<attribute_array>;
			using invoker_type = Invoker;
			using value_type = typename invoker_type::value_type;
			using index_type = typename invoker_type::index_type;
			using object_type = typename invoker_type::object_type;

			// constructor
			constexpr property(std::string_view name)
				: m_name(name)
				, m_display_name(name)
			{}
			constexpr property(std::string_view name, std::string_view display_name, const attribute_set& attributes, const Invoker& invoker)
				: m_name(name)
				, m_display_name(display_name)
				, m_attributes(attributes)
				, m_invoker(invoker)
			{}

			// chain construction
			template<typename Class, typename Value>
			constexpr auto member(Value(Class::* getter)() const, void(Class::* setter)(const Value&)) const
			{
				auto invoker = property_method_invoker<Class, Value>{ getter, setter };
				return property<Value, decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}
			template<typename Class, typename Value>
			constexpr auto member(Value(Class::* p)) const
			{
				if constexpr (std::is_array_v<Value>)
					return member_array<Class, Value>(p);
				else
					return member_scalar<Class, Value>(p);
			}
			template<typename Class, typename Value>
			constexpr auto delegate(Value(*getter)(const Class&), void(*setter)(Class&, const Value&)) const
			{
				auto invoker = property_delegate_invoker<Class, Value>{ getter, setter };
				return property<Value, decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}
			template<typename Class, typename Value>
			constexpr auto delegate(Value(*getter)(const Class&)) const
			{
				auto invoker = property_delegate_invoker<Class, Value>{ getter, nullptr };
				return property<Value, decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}

			constexpr auto display_name(std::string_view name) const
			{
				return property<Type, Invoker, Attributes>(m_name, name, m_attributes, m_invoker);
			}
			template<typename ...Args>
			constexpr auto attributes(Args&&... args) const
			{
				return property<Type, Invoker, type_list<Args...>>(m_name, m_display_name, { std::forward<Args>(args)... }, m_invoker);
			}

			value_type get(const object_type* object, const index_type& idx = {}) const
			{
				return m_invoker.get(object, idx);
			}
			void set(object_type* object, const value_type& value, const index_type& idx = {}) const
			{
				m_invoker.set(object, value, idx);
			}
			value_type* ref(object_type* object, const index_type& idx = {}) const
			{
				return m_invoker.ref(object, idx);
			}
			const value_type* cref(const object_type* object, const index_type& idx = {}) const
			{
				return m_invoker.cref(object, idx);
			}

			constexpr std::string_view name() const { return m_name; }
			constexpr std::string_view display_name() const { return m_display_name; }
			constexpr bool is_read_only() const { return m_invoker.is_read_only(); }
			constexpr const attribute_set& attributes() const { return m_attributes; }

		protected:
			std::string_view		m_name;
			std::string_view		m_display_name;
			attribute_set			m_attributes;
			Invoker					m_invoker;

			template<typename Class, typename Value>
			constexpr auto member_array(Value(Class::* pmember)) const
			{
				using value_t = std::remove_extent_t<Value>;

				auto invoker = property_member_indexed_invoker<Class, Value>(pmember);
				return property<value_t, decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}

			template<typename Class, typename Value>
			constexpr auto member_scalar(Value(Class::* pmember)) const
			{
				auto invoker = property_member_invoker<Class, Value>(pmember);
				return property<Value, decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}
		};

		class property_view
		{
		public:
			template<typename T>
			constexpr property_view(const T& instance)
				: m_instance(&instance)
				, m_name(instance.name())
				, m_display_name(instance.display_name())
				, m_attributes(instance.attributes().iterable())
				, m_getter([](const void* instance, const value& object, const index_base& index) -> value
					{ 
						if (auto p = value_cast_object<const typename T::object_type>(object))
							return reinterpret_cast<const T*>(instance)->get(p, index.make_tuple<typename T::index_type>());
						return {};
					})
				, m_setter([](const void* instance, const value& object, const value& value, const index_base& index)
					{ 
						if (auto p = value_cast_object<typename T::object_type>(object))
						{
							if (auto v = value_cast<typename T::value_type>(value))
								reinterpret_cast<const T*>(instance)->set(p, *v, index.make_tuple<typename T::index_type>());
						}
					})
				, m_refer([](const void* instance, const value& object, const index_base& index) -> value
					{ 
						if (auto p = value_cast_object<typename T::object_type>(object))
							return reinterpret_cast<const T*>(instance)->ref(p, index.make_tuple<typename T::index_type>());
						return {};
					})
				, m_crefer([](const void* instance, const value& object, const index_base& index) -> value
					{ 
						if (auto p = value_cast_object<const typename T::object_type>(object))
							return reinterpret_cast<const T*>(instance)->cref(p, index.make_tuple<typename T::index_type>());
						return {};
					})
				, m_view_getter(get_type_view<typename T::value_type>)
			{
			}

			constexpr std::string_view name() const { return m_name; }
			constexpr std::string_view display_name() const { return m_display_name; }
			constexpr attribute_iterable attributes() const { return m_attributes; }
			value get(const value& object, const index_base& idx= index<>()) const
			{
				return m_getter(m_instance, object, idx);
			}
			void set(const value& object, const value& value, const index_base& idx = index<>()) const
			{
				m_setter(m_instance, object, value, idx);
			}
			value ref(const value& object, const index_base& idx = index<>()) const
			{
				return m_refer(m_instance, object, idx);
			}
			value cref(const value& object, const index_base& idx = index<>()) const
			{
				return m_crefer(m_instance, object, idx);
			}
			inline const type_view& value_type() const;

		private:
			const void* m_instance;
			std::string_view		m_name;
			std::string_view		m_display_name;
			attribute_iterable		m_attributes;
			value(*m_getter)(const void*, const value&, const index_base&);
			void(*m_setter)(const void*, const value&, const value&, const index_base&);
			value(*m_refer)(const void*, const value&, const index_base&);
			value(*m_crefer)(const void*, const value&, const index_base&);
			const type_view&(*m_view_getter)();
		};


		class property_iterable
			: public iterator_range<view_mapped_array<property_view>::iterator>
		{
			using base_class = iterator_range<view_mapped_array<property_view>::iterator>;
		public:

			constexpr property_iterable()
			{}
			constexpr property_iterable(iterator begin, iterator end)
				: base_class(begin, end)
			{}

			constexpr const property_view* get(std::string_view name) const
			{
				return constexpr_find_if(base_class::begin(), base_class::end(), [name](const auto& i)
					{
						return name.compare(i.name()) == 0;
					});
			}
		};

		template<typename ...Types>
		class property_array
			: public view_mapped_array<property_view, Types...>
		{
			using base_class = view_mapped_array<property_view, Types...>;
		public:
			using iterator = typename base_class::iterator;

			constexpr property_array(Types&&... args)
				: base_class(std::forward<Types>(args)...)
			{}
			constexpr property_array(const property_array& r)
				: base_class(r)
			{}

			constexpr property_iterable iterable() const
			{
				return { base_class::begin(), base_class::end() };
			}

			constexpr const property_view* get(std::string_view name) const
			{
				return iterable().get(name);
			}
		};

		class type_iterator
		{
		public:
			constexpr type_iterator()
			{}
			constexpr type_iterator(const type_view& (*getter)(size_t index), size_t index)
				: m_getter(getter)
				, m_index(index)
			{}

			constexpr const type_view& operator*() const
			{
				return m_getter(m_index);
			}
			constexpr const type_view* operator->() const
			{
				return &m_getter(m_index);
			}
			constexpr int operator-(const type_iterator& q) const
			{
				return (int)(m_index - q.m_index);
			}
			constexpr type_iterator operator++()
			{
				auto tmp = *this;
				++m_index;
				return tmp;
			}
			constexpr type_iterator& operator++(int)
			{
				++m_index;
				return *this;
			}

			constexpr bool operator==(const type_iterator& q) const
			{
				return m_getter == q.m_getter && m_index == q.m_index;
			}
			constexpr bool operator!=(const type_iterator& q) const
			{
				return !operator==(q);
			}

		private:
			const type_view& (*m_getter)(size_t index) = nullptr;
			size_t m_index = 0;
		};
		class type_iterable
		{
		public:
			constexpr type_iterable()
			{}
			constexpr type_iterable(type_iterator begin, type_iterator end)
				: m_begin(begin)
				, m_end(end)
			{}

			type_iterator begin()  const { return m_begin; }
			type_iterator end() const { return m_end; }
			size_t size() const { return m_end - m_begin; }
		private:
			type_iterator	m_begin;
			type_iterator	m_end;
		};

		template<typename... Types>
		class type_array
		{
		public:
			constexpr type_iterable iterable() const { return { begin(), end() }; }
			constexpr type_iterator begin() const { return { view, 0 }; }
			constexpr type_iterator end() const { return { view, size() }; }
			constexpr size_t size() const { return sizeof...(Types); }
		private:
			constexpr static const type_view& view(size_t index)
			{
				const type_view* views[] = { &get_type_view<Types>()... };
				return *views[index];
			}
		};
		template<>
		class type_array<>
		{
		public:
			constexpr type_iterable iterable() const { return { }; }
			constexpr type_iterator begin() const { return { nullptr, 0 }; }
			constexpr type_iterator end() const { return { nullptr, size() }; }
			constexpr size_t size() const { return 0; }
		};


		template<typename Result = void, typename Arglist = type_list<>>
		struct method_invoker
		{
			using result_type = Result;
			using object_type = const void;
			using argument_list = Arglist;
			Result invoke(object_type*) const { ; }
		};
		template<typename Class, typename Result, typename ...Args>
		struct method_member_invoker : public method_invoker<Result, type_list<Args...>>
		{
			using invoker_type = Result(Class::*)(Args...);
			using object_type = Class;
			using result_type = Result;

			invoker_type	m_invoker;

			constexpr method_member_invoker(invoker_type p)
				: m_invoker(p)
			{}

			result_type invoke(object_type* object, const Args&... args) const
			{
				if constexpr (std::is_void_v<Result>)
					(object->*m_invoker)(args...);
				else
					return (object->*m_invoker)(args...);
				return {};
			}
		};
		template<typename Fn, typename Result, typename Arglist>
		struct method_delegate_invoker : public method_invoker<Result, Arglist>
		{
			using invoker_type = Fn;
			using object_type = const void;
			using result_type = Result;

			invoker_type m_invoker;

			constexpr method_delegate_invoker(invoker_type&& p)
				: m_invoker(p)
			{}

			template<typename... Args>
			result_type invoke(object_type*, const Args&... args) const
			{
				if constexpr (std::is_void_v<Result>)
					m_invoker(args...);
				else
					return m_invoker(args...);
				return {};
			}
		};

		template<typename Invoker = method_invoker<>, typename Attributes = type_list<>>
		class method
		{
		public:
			using argument_list = typename Invoker::argument_list;
			using arguments_set = typename Invoker::argument_list::template expand<type_array>;
			using attribute_set = typename Attributes::template expand<attribute_array>;
			using result_type = typename Invoker::result_type;
			using object_type = typename Invoker::object_type;

			constexpr method(std::string_view name)
				: m_name(name)
				, m_display_name(name)
			{}
			constexpr method(std::string_view name, std::string_view display_name, const attribute_set& attributes, const Invoker& invoker)
				: m_name(name)
				, m_display_name(display_name)
				, m_attributes(attributes)
				, m_invoker(invoker)
			{}

			template<typename Class, typename Result, typename ...Args>
			constexpr auto invoker(Result(Class::*p)(Args...)) const
			{
				auto invoker = method_member_invoker<Class, Result, Args...>(p);
				return method<decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}

			template<typename Fn>
			constexpr auto invoker(Fn&& fn) const
			{
				using traits = invoker_traits<Fn>;
				using invoker_type = method_delegate_invoker<Fn, typename traits::result_type, typename traits::arguments_type>;
				
				invoker_type invoker(std::forward<Fn>(fn));
				return method<decltype(invoker), Attributes>(m_name, m_display_name, m_attributes, invoker);
			}

			constexpr auto display_name(std::string_view name) const
			{
				return method<Invoker, Attributes>(m_name, name, m_attributes, m_invoker);
			}

			template<typename... Args>
			constexpr auto attributes(Args&&... args)
			{
				return method<Invoker, type_list<Args...>>(m_name, m_display_name, { std::forward<Args>(args)... }, m_invoker);
			}

			template<typename... Args>
			result_type invoke(object_type* object, const Args&... args) const
			{
				return m_invoker.invoke(object, args...);
			}

			constexpr std::string_view name() const { return m_name; }
			constexpr std::string_view display_name() const { return m_display_name; }
			constexpr const arguments_set& arguments_type() const { return m_arguments;  }
			constexpr const attribute_set& attributes() const { return m_attributes; }
		private:
			std::string_view	m_name;
			std::string_view	m_display_name;
			arguments_set		m_arguments;
			attribute_set		m_attributes;
			Invoker				m_invoker;
		};

		class method_view
		{
		public:
			template<typename T>
			constexpr method_view(const T& instance)
				: m_instance(&instance)
				, m_name(instance.name())
				, m_display_name(instance.display_name())
				, m_arguments_type(instance.arguments_type().iterable())
				, m_attributes(instance.attributes().iterable())
				, m_invoker([](const void* instance, const value& object, arguments args)
					{ return method_view::invokeT(reinterpret_cast<const T*>(instance), object, args, std::make_index_sequence<T::argument_list::size>()); })
				, m_result_type(get_type_view<typename T::result_type>)
			{}

			constexpr std::string_view name() const { return m_name; }
			constexpr std::string_view display_name() const { return m_display_name; }
			constexpr attribute_iterable attributes() const { return m_attributes; }
			constexpr type_iterable arguments_type() const { return m_arguments_type; }

			value invoke(const value& object, arguments args) const
			{
				return m_invoker(m_instance, object, args);
			}

			const type_view& result_type() const
			{
				return m_result_type();
			}

		private:
			const void* m_instance;
			std::string_view		m_name;
			std::string_view		m_display_name;
			type_iterable			m_arguments_type;
			attribute_iterable		m_attributes;
			value(*m_invoker)(const void*, const value&, arguments) = nullptr;
			const type_view& (*m_result_type)() = nullptr;

			template<typename T, size_t... Indices>
			static value invokeT(const T* instance, const value& object, arguments args, std::index_sequence<Indices...>)
			{
				if (auto p = value_cast_object<typename T::object_type>(object))
				{
					auto values = std::make_tuple(
						value_cast<typename T::argument_list::template at<Indices>>(*(args.begin() + Indices))...);
					if ((std::get<Indices>(values) && ...))
						return instance->invoke(p, *std::get<Indices>(values)...);
				}
				return {};
			}
		};

		class method_iterable
			: public iterator_range<view_mapped_array<method_view>::iterator>
		{
			using base_class = iterator_range<view_mapped_array<method_view>::iterator>;
		public:
			constexpr method_iterable()
			{}
			constexpr method_iterable(iterator begin, iterator end)
				: base_class(begin, end)
			{}

			constexpr const method_view* get(std::string_view name) const
			{
				return constexpr_find_if(base_class::begin(), base_class::end(), [name](const auto& i)
					{
						return name.compare(i.name()) == 0;
					});
			}
		};

		template<typename ...Types>
		class method_array
			: public view_mapped_array<method_view, Types...>
		{
			using base_class = view_mapped_array<method_view, Types...>;
		public:
			using iterator = typename base_class::iterator;

			constexpr method_array(Types&&... args)
				: base_class(std::forward<Types>(args)...)
			{}
			constexpr method_array(const method_array& r)
				: base_class(r)
			{}

			constexpr method_iterable iterable() const
			{
				return { base_class::begin(), base_class::end() };
			}

			constexpr const method_view* get(std::string_view name) const
			{
				return iterable().get(name);
			}
		};


		template<typename... Args>
		class constructor
		{
		public:
			using argument_list = type_list<Args...>;
			using arguments_set = typename argument_list::template expand<type_array>;

			template<typename T> static T* instantiate(const Args&... args)
			{
				return new T(args...);
			}

			constexpr const arguments_set& arguments_type() const { return m_arguments; }

		private:
			arguments_set	m_arguments;
		};

		class constructor_view
		{
		public:
			template<typename T>
			constexpr constructor_view(const T& instance)
				: m_arguments_type(instance.arguments_type().iterable())
			{}

			constexpr type_iterable arguments_type() const { return m_arguments_type; }
		private:
			type_iterable			m_arguments_type;
		};

		class constructor_iterable
			: public iterator_range<view_mapped_array<constructor_view>::iterator>
		{
			using base_class = iterator_range<view_mapped_array<constructor_view>::iterator>;
		public:
			constexpr constructor_iterable()
			{}
			constexpr constructor_iterable(iterator begin, iterator end)
				: base_class(begin, end)
			{}
		};

		template<typename T = void, typename ...Types>
		class constructor_array
			: public view_mapped_array<constructor_view, Types...>
		{
			using base_class = view_mapped_array<constructor_view, Types...>;
		public:
			using iterator = typename base_class::iterator;

			constexpr constructor_array(Types&&... args)
				: base_class(std::forward<Types>(args)...)
			{}
			constexpr constructor_array(const constructor_array& r)
				: base_class(r)
			{}

			constexpr constructor_iterable iterable() const
			{
				return { base_class::begin(), base_class::end() };
			}

			value instantiate(arguments args) const
			{
				return instantiate_match<Types...>(args);
			}

		private:
			template<typename Ctor = void, typename... Rest>
			T* instantiate_match(arguments args) const
			{
				if (auto p = invoke_ctor<Ctor>(args, std::make_index_sequence<Ctor::argument_list::size>()))
					return p;
				return instantiate_match<Rest...>(args);
			}
			template<>
			T* instantiate_match<>(arguments args) const
			{
				return nullptr;
			}
			template<typename Ctor, size_t... Indices>
			T* invoke_ctor(arguments args, std::index_sequence<Indices...>) const
			{
				if (args.size() != sizeof...(Indices))
					return nullptr;

				auto values = std::make_tuple(value_cast<typename Ctor::argument_list::template at<Indices> >(*(args.begin() + Indices))...);
				if ((std::get<Indices>(values) && ...))
					return Ctor::template instantiate<T>(*std::get<Indices>(values)...);
				return nullptr;
			}
		};
		template<>
		class constructor_array<void>
		{
		public:
			constexpr constructor_iterable iterable() const{ return { }; }
			value instantiate(arguments args) const { return {}; }
		};

		class type_view : noncopyable
		{
		public:

			template<typename T>
			constexpr type_view(const T&)
				: m_name(T::name())
				, m_display_name(T::display_name())
				, m_id(T::id())
				, m_bases(T::bases().iterable())
				, m_constructors(T::constructors().iterable())
				, m_properties(T::properties().iterable())
				, m_methods(T::methods().iterable())
				, m_attributes(T::attributes().iterable())
				, m_constructor([](arguments args) { return T::instantiate(args); })
				, m_has_description(T::has_description())
				, m_is_const(T::is_const())
				, m_is_volatile(T::is_volatile())
				, m_is_reference(T::is_reference())
				, m_is_pointer(T::is_pointer())
				, m_rank(T::rank())
				, m_decay_type(get_type_view<typename T::decay_type>)
				, m_unconst_type(get_type_view<typename T::unconst_type>)
				, m_const_type(get_type_view<typename T::const_type>)
				, m_unpointer_type(get_type_view<typename T::unpointer_type>)
				, m_once_pointer_type(get_type_view<typename T::once_pointer_type>)
			{}

			constexpr std::string_view name() const { return m_name; }
			constexpr std::string_view display_name() const { return m_display_name; }
			constexpr type_id_t id() const { return m_id; }
			constexpr type_iterable bases() const { return m_bases; }
			constexpr constructor_iterable constructors() const { return m_constructors; }
			constexpr property_iterable properties() const { return m_properties; }
			constexpr method_iterable methods() const { return m_methods; }
			constexpr attribute_iterable attributes() const { return m_attributes; }
			constexpr bool has_description() const { return m_has_description; }
			constexpr bool is_const() const { return m_is_const; }
			constexpr bool is_volatile() const { return m_is_volatile; }
			constexpr bool is_reference() const { return m_is_reference; }
			constexpr bool is_pointer() const { return m_is_pointer; }
			constexpr size_t rank() const { return m_rank; }
			constexpr bool operator==(const type_view& q) const { return id() == q.id(); }
			constexpr bool operator!=(const type_view& q) const { return !operator==(q); }
			template<typename T>
			constexpr bool is() const { return *this == get_type_view<T>(); }
			value instantiate(arguments args) const
			{
				return m_constructor(args);
			}

			const type_view& decay_type() const { return m_decay_type(); }
			const type_view& unconst_type() const { return m_unconst_type(); }
			const type_view& const_type() const { return m_const_type(); }
			const type_view& unpointer_type() const { return m_unpointer_type(); }
			const type_view& once_pointer_type() const { return m_once_pointer_type(); }
		private:
			std::string_view m_name;
			std::string_view m_display_name;
			type_id_t	m_id = 0;
			type_iterable m_bases;
			constructor_iterable m_constructors;
			property_iterable m_properties;
			method_iterable m_methods;
			attribute_iterable m_attributes;
			value(*m_constructor)(arguments) = nullptr;
			bool	m_has_description = false;
			bool	m_is_const = false;
			bool	m_is_volatile = false;
			bool	m_is_reference = false;
			bool	m_is_pointer = false;
			size_t	m_rank = 0;
			const type_view& (*m_decay_type)();
			const type_view& (*m_unconst_type)();
			const type_view& (*m_const_type)();
			const type_view& (*m_unpointer_type)();
			const type_view& (*m_once_pointer_type)();
		};

		inline const type_view& property_view::value_type() const
		{
			return m_view_getter();
		}

		class type_view_chain : noncopyable
		{
		public:
			type_view_chain(const type_view& r)
				: m_type_view(r)
			{
				if (sm_root == nullptr)
					sm_root = this;
				if(sm_tail != nullptr)
					sm_tail->m_next = this;
				sm_tail = this;
			}

			const type_view& view() const { return m_type_view; }
			const type_view_chain* next() const { return m_next; }
			static const type_view_chain* root() { return sm_root; }
		private:
			const type_view&	m_type_view;
			type_view_chain*	m_next;
			inline static type_view_chain*	sm_root;
			inline static type_view_chain*	sm_tail;
		};

		template<typename C> struct meta {};

		template<typename C>
		class type_impl
		{
		public:
			using target_type = C;
			using decay_type = std::decay_t<C>;
			using const_type = std::add_const_t<C>;
			using unconst_type = std::remove_const_t<C>;
			using unpointer_type = std::remove_pointer_t<C>;
			using once_pointer_type = std::conditional_t<std::is_pointer_v<C>, C, std::add_pointer_t<C>>;

			static constexpr std::string_view name() { return get_type_name<C>(); }
			static constexpr std::string_view display_name() { return name(); }
			static constexpr type_array<> bases() { return {}; }
			static constexpr constructor_array<> constructors() { return {}; }
			static constexpr property_array<> properties() { return {}; }
			static constexpr method_array<> methods() { return {}; }
			static constexpr attribute_array<> attributes() { return {}; }
			static constexpr type_id_t id() { return get_type_id<C>(); }
			static constexpr bool is_const() { return std::is_const_v<C>;  }
			static constexpr bool is_volatile() { return std::is_volatile_v<C>; }
			static constexpr bool is_reference() { return std::is_reference_v<C>; }
			static constexpr bool is_pointer() { return std::is_pointer_v<C>; }
			static constexpr size_t rank() { return std::rank_v<C>; }
			static value instantiate(arguments) { return {}; }
		};
		template<typename C, typename = void>
		class type : public type_impl<C>
		{
		public:
			static constexpr bool has_description() { return false; }
		};
		template<typename C>
		class type<C, std::void_t<decltype(meta<std::remove_cv_t<C>>::description)>> : public type_impl<C>
		{
		public:
			using meta_type = meta<std::remove_cv_t<C>>;
			static constexpr std::string_view display_name() { return meta_type::description.display_name(); }
			static constexpr auto& bases() { return meta_type::description.bases(); }
			static constexpr auto& constructors() { return meta_type::description.constructors(); }
			static constexpr auto& properties() { return meta_type::description.properties(); }
			static constexpr auto& methods() { return meta_type::description.methods(); }
			static constexpr auto& attributes() { return meta_type::description.attributes(); }
			static constexpr bool has_description() { return true; }
			static value instantiate(arguments args) { return meta_type::description.instantiate(args); }
		};

		template<typename C, typename Bases = type_list<>, typename Constructors = type_list<>, typename Properties = type_list<>, typename Methods = type_list<>, typename Attributes = type_list<>>
		class meta_description
		{
			static_assert(std::is_same_v<C, std::remove_cv_t<std::decay_t<C>>>, "typename C must not have qualifier.");
		public:
			template<typename... Args> using type_constructor_array = constructor_array<C, Args...>;
			using base_set = typename Bases::template expand<type_array>;
			using constructor_set = typename Constructors::template expand<type_constructor_array>;
			using property_set = typename Properties::template expand<property_array>;
			using method_set = typename Methods::template expand<method_array>;
			using attribute_set = typename Attributes::template expand<attribute_array>;

			constexpr meta_description(std::string_view display_name, const constructor_set& constructors, const property_set& properties, const method_set& methods, const attribute_set& attributes)
				: m_display_name(display_name)
				, m_constructors(constructors)
				, m_properties(properties)
				, m_methods(methods)
				, m_attributes(attributes)
			{}

			template<typename... Types>
			static constexpr auto initialize(std::string_view name)
			{
				static_assert(Bases::size == 0);
				static_assert(Constructors::size == 0);
				static_assert(Properties::size == 0);
				static_assert(Attributes::size == 0);
				static_assert(Methods::size == 0);

				auto all_properties = gather_properties(type<Types>()...);
				auto all_attributes = gather_attributes(type<Types>()...);
				auto all_methods = gather_methods(type<Types>()...);
				return meta_description<C,
					type_list<Types...>,
					Constructors,
					typename decltype(all_properties)::instance_value_type,
					typename decltype(all_methods)::instance_value_type,
					typename decltype(all_attributes)::instance_value_type
				>(name, constructor_set(), all_properties, all_methods, all_attributes);
			}

			template<typename... Types>
			constexpr auto bases() const
			{
				static_assert(Bases::size == 0);
				static_assert(Constructors::size == 0);
				static_assert(Properties::size == 0);
				static_assert(Attributes::size == 0);
				static_assert(Methods::size == 0);

				auto all_properties = gather_properties(type<Types>()...);
				auto all_attributes = gather_attributes(type<Types>()...);
				auto all_methods = gather_methods(type<Types>()...);
				return meta_description<C,
					type_list<Types...>,
					Constructors,
					typename decltype(all_properties)::instance_value_type,
					typename decltype(all_methods)::instance_value_type,
					typename decltype(all_attributes)::instance_value_type
				>(m_display_name, m_constructors, all_properties, all_methods, all_attributes);
			}

			constexpr auto display_name(std::string_view display_name) const
			{
				return meta_description<C, Bases, Constructors, Properties, Methods, Attributes>(display_name, m_constructors, m_properties, m_methods, m_attributes);
			}

			template<typename ...Args>
			constexpr auto constructors(Args&&... args) const
			{
				auto all = type_constructor_array<Args...>{ std::forward<Args>(args)... };
				return meta_description<C, Bases, typename decltype(all)::instance_value_type, Properties, Methods, Attributes>(m_display_name, all, m_properties, m_methods, m_attributes);
			}

			template<typename... Args>
			constexpr auto properties(Args&&... args) const
			{
				auto all = concat_array(m_properties, property_array{ std::forward<Args>(args)... });
				return meta_description<C, Bases, Constructors, typename decltype(all)::instance_value_type, Methods, Attributes>(m_display_name, m_constructors, all, m_methods, m_attributes);
			}

			template<typename... Args>
			constexpr auto methods(Args&&... args) const
			{
				auto all = concat_array(m_methods, method_array{ std::forward<Args>(args)... });
				return meta_description<C, Bases, Constructors, Properties, typename decltype(all)::instance_value_type, Attributes>(m_display_name, m_constructors, m_properties, all, m_attributes);
			}

			template<typename... Args>
			constexpr auto attributes(Args&&... args) const
			{
				auto all = concat_array(m_attributes, attribute_array{ std::forward<Args>(args)... });
				return meta_description<C, Bases, Constructors, Properties, Methods, typename decltype(all)::instance_value_type>(m_display_name, m_constructors, m_properties, m_methods, all);
			}

			constexpr std::string_view display_name() const { return m_display_name; }
			constexpr const base_set& bases() const { return m_bases; }
			constexpr const constructor_set& constructors() const { return m_constructors; }
			constexpr const property_set& properties() const { return m_properties; }
			constexpr const method_set& methods() const { return m_methods; }
			constexpr const attribute_set& attributes() const { return m_attributes; }
			value instantiate(arguments args) const { return m_constructors.instantiate(args); }
		private:
			std::string_view				m_display_name;
			base_set						m_bases;
			constructor_set					m_constructors;
			property_set					m_properties;
			method_set						m_methods;
			attribute_set					m_attributes;

		protected:

			template<typename Type, typename... Types>
			static constexpr auto gather_properties(Type t, Types... types)
			{
				return concat_array(t.properties(), gather_properties(types...));
			}
			static constexpr auto gather_properties()
			{
				return property_set{};
			}

			template<typename Type, typename... Types>
			static constexpr auto gather_methods(Type t, Types... types)
			{
				return concat_array(t.methods(), gather_methods(types...));
			}
			static constexpr auto gather_methods()
			{
				return method_set{};
			}

			template<typename Type, typename... Types>
			static constexpr auto gather_attributes(Type t, Types... types)
			{
				return concat_array(t.attributes(), gather_attributes(types...));
			}
			static constexpr auto gather_attributes()
			{
				return attribute_set{};
			}
		};

		template<typename T>
		inline const type_view type_view_instance = { type<T>() };

		template<typename T>
		inline const type_view& get_type_view()
		{
			return type_view_instance<T>;
		}

		inline const type_view& get_type_view_once_pointer(const type_view& type)
		{
			return type.once_pointer_type();
		}
		inline const type_view& get_type_view_const(const type_view& type)
		{
			return type.const_type();
		}

		template<typename T> 
		const T* value_cast(const value& v)
		{
			static_assert(!std::is_reference_v<T>);

			if (!v.has_value())
				return nullptr;

			if (v.type() == get_type_view<T>())
				return reinterpret_cast<T*>(v.buffer_address());

			if constexpr (std::is_pointer_v<T> && std::is_const_v<std::remove_pointer_t<T>>)
			{
				using attempt_type = std::remove_const_t<std::remove_pointer_t<T>>*;
				if (v.type() == get_type_view<attempt_type>())
					return reinterpret_cast<attempt_type*>(v.buffer_address());
			}
			
			return nullptr;
		}

		template<typename To> To* object_cast(void* ptr, const type_view& from_type);
		template<typename T> T* value_cast_object(const value& v)
		{
			static_assert(std::is_class_v<T> || std::is_void_v<T>);

			if (!v.has_value())
				return nullptr;

			using value_type = std::remove_const_t<T>;
			constexpr bool is_void = std::is_void_v<value_type>;

			if (v.type().is_pointer())
			{
				if (!v.type().unpointer_type().is_const() || std::is_const_v<T>)
				{
					if constexpr (is_void)
						return *reinterpret_cast<void**>(v.buffer_address());
					else
						return object_cast<value_type>(
							*reinterpret_cast<void**>(v.buffer_address()), v.type().unpointer_type().unconst_type());
				}
			}
			else
			{
				// If value has a instanced T, return the address of storage whitch it has directly.
				// This case are allowed when T has const qualifer.
				if constexpr (std::is_const_v<T>)
				{
					if constexpr (is_void)
						return reinterpret_cast<const void*>(v.buffer_address());
					else
						return object_cast<value_type>(
							reinterpret_cast<void*>(v.buffer_address()), v.type().unpointer_type().unconst_type());
				}
			}

			return nullptr;
		}

		inline bool is_base_of(const type_view& base_type, const type_view& derived_type)
		{
			if (base_type.id() == derived_type.id())
				return true;

			for (auto& derived_base : derived_type.bases())
			{
				if (is_base_of(base_type, derived_base))
					return true;
			}

			return false;
		}

		template<typename Base>
		inline bool is_base_of(const type_view& derived_type)
		{
			return is_base_of(get_type_view<std::decay_t<Base>>(), derived_type);
		}

		template<typename To>
		inline To* object_cast(void* ptr, const type_view& from_type)
		{
			if (is_base_of(get_type_view<To>(), from_type))
				return reinterpret_cast<To*>(ptr);
			return nullptr;
		}

		template<typename To, typename From>
		inline To* object_cast(From* object)
		{
			if constexpr (std::is_convertible_v<From*, To*>)
				return static_cast<To*>(object);
			else if (is_base_of<To>(object->rtti_type_view()))
				return reinterpret_cast<To*>(object);
			return nullptr;
		}
	}

	using attribute = impl::attribute;
	using type_view = impl::type_view;
	using property_view = impl::property_view;
	using type_id_t = impl::type_id_t;
	using type_iterable = impl::type_iterable;
	using property_iterable = impl::property_iterable;
	using method_iterable = impl::method_iterable;
	using attribute_iterable = impl::attribute_iterable;
	using value = impl::value;

	template<typename ...Types>
	class index : public impl::index<Types...>
	{
	public:
		index(Types... indices)
			: impl::index<Types...>(indices...)
		{}
	};

#define rtti_class_decl(Class)				\
	friend struct ::rtti::impl::meta<Class>;	\
	static const ::rtti::impl::type_view_chain Rtti;	\
	virtual const ::rtti::impl::type_view& rtti_type_view() const { return Rtti.view(); }

#define rtti_class_impl(Class, ...)				\
	template<> struct ::rtti::impl::meta<Class> {	\
		inline static constexpr auto description = ::rtti::impl::meta_description<Class>::initialize<>(get_type_name<Class>()) __VA_ARGS__; };  \
	const ::rtti::impl::type_view_chain Class::Rtti = { ::rtti::impl::get_type_view<Class>() }

#define rtti_impl(Class, ...)					\
	template<> struct ::rtti::impl::meta<Class> {	\
		inline static constexpr auto description = ::rtti::impl::meta_description<Class>::initialize<>(get_type_name<Class>()) __VA_ARGS__; };  \
	namespace rtti_types::Class##_def{			\
		inline const ::rtti::impl::type_view_chain Rtti = { rtti::impl::get_type_view<::Class>() }; }


	template<typename T> inline const T* value_cast(const impl::value& v)
	{
		return impl::value_cast<T>(v);
	}
	template<typename T> inline const T& value_cast(const impl::value& v, const T& default_value)
	{
		if (auto p = impl::value_cast<T>(v))
			return *p;
		return default_value;
	}

	template<typename T> inline constexpr auto value_cast_object = impl::value_cast_object<T>;

	template<typename Base> inline constexpr auto is_base_of = impl::is_base_of<Base>;
	
	template<typename T, typename Object>
	inline auto object_cast(Object* object)
	{
		static_assert(std::is_class_v<Object>, "The typename Object must be a class type.");
		static_assert(std::is_class_v<T> && !std::is_const_v<T>, "The typename T must be a class type and without const qualifer.");

		using target_type = std::remove_cv_t<T>;
		if constexpr (std::is_const_v<Object>)
			return impl::object_cast<const target_type>(object);
		else
			return impl::object_cast<target_type>(object);
	}

	template<typename Fn>
	inline void visit_all_types(Fn&& f)
	{
		auto chain = impl::type_view_chain::root();
		while (chain)
		{
			if (!f(chain->view()))
				break;
			chain = chain->next();
		}
	}

	template<typename T>
	inline const impl::type_view& get_type_view()
	{
		return impl::get_type_view<T>();
	}
	inline const impl::type_view* get_type_view(const char* type_name)
	{
		auto type_id = impl::hash(type_name);
		const impl::type_view* result = nullptr;
		visit_all_types([&](auto& view) {
			if (view.id() == type_id)
			{
				result = &view;
				return false;
			}
			return true;
		});
		return result;
	}

	template<typename T> using type = impl::type<T>;
}

