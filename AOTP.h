#pragma once

#include <typeindex>

/// Lowest Common Ancestor by http://stackoverflow.com/a/25760036/1536
namespace detail
{

	struct TypeIsNotPartOfTheHierarchy {};

	template<typename T>
	struct TypeWrapper
	{
		/*
		static_assert(!std::is_same<TypeIsNotPartOfTheHierarchy, T>::value,
		"using types of different type hierarchies.");
		*/
		using type = T;
	};

	template<typename StillCommonAncestor, typename TypeToCheck, typename... Ts>
	struct IsCommonAncestor;

	template<typename StillCommonAncestor, typename TypeToCheck>
	struct IsCommonAncestor<StillCommonAncestor, TypeToCheck>
	{
		static constexpr bool value = StillCommonAncestor::value;
	};

	template<typename StillCommonAncestor, typename TypeToCheck, typename T1, typename... Ts>
	struct IsCommonAncestor<StillCommonAncestor, TypeToCheck, T1, Ts...> :
		IsCommonAncestor
		<
			std::integral_constant
			<
				bool,
				std::conditional
				<
					std::is_base_of<TypeToCheck, T1>::value,
					std::true_type,
					std::false_type
				>::type::value && StillCommonAncestor::value
			>,
			TypeToCheck,
			Ts...
		>
	{};

	template<typename Pack, typename... Ts>
	struct LCA;

	template<typename... PackParams, typename T1>
	struct LCA<std::tuple<PackParams...>, T1> :
		std::conditional
		<
			IsCommonAncestor<std::true_type, T1, PackParams...>::value,
			TypeWrapper<T1>,
			TypeWrapper<TypeIsNotPartOfTheHierarchy>
		>::type
	{};

	template<typename... PackParams, typename T1, typename... Ts>
	struct LCA<std::tuple<PackParams...>, T1, Ts...> :
		std::conditional
		<
			IsCommonAncestor<std::true_type, T1, PackParams...>::value,
			TypeWrapper<T1>,
			LCA<std::tuple<PackParams...>, Ts...>
		>::type
	{};

}

template<typename... Ts>
struct LCA : detail::LCA<std::tuple<Ts...>, Ts...>
{};

/// variant with AOT Polymorphism support

template<typename... Ts>
struct variant_helper;

template<typename F, typename... Ts>
struct variant_helper<F, Ts...> {
	inline static void destroy(std::type_index id, void * data)
	{
		if (id == typeid(F))
			reinterpret_cast<F*>(data)->~F();
		else
			variant_helper<Ts...>::destroy(id, data);
	}

	inline static void move(std::type_index old_t, void * old_v, void * new_v)
	{
		if (old_t == typeid(F))
			new (new_v)F(std::move(*reinterpret_cast<F*>(old_v)));
		else
			variant_helper<Ts...>::move(old_t, old_v, new_v);
	}

	inline static void copy(std::type_index old_t, const void * old_v, void * new_v)
	{
		if (old_t == typeid(F))
			new (new_v)F(*reinterpret_cast<const F*>(old_v));
		else
			variant_helper<Ts...>::copy(old_t, old_v, new_v);
	}
};

template<> struct variant_helper<> {
	inline static void destroy(std::type_index id, void * data) { }
	inline static void move(std::type_index old_t, void * old_v, void * new_v) { }
	inline static void copy(std::type_index old_t, const void * old_v, void * new_v) { }
};

template<typename... Ts>
struct variant {
private:
	using data_t = typename std::aligned_union<1, Ts...>::type;

	using helper_t = variant_helper<Ts...>;
	static inline std::type_index invalid_type() {
		return typeid(void);
	}

	std::type_index type_id;
	data_t data;
public:
	variant() : type_id(invalid_type()) { }

	variant(const variant<Ts...>& old) : type_id(old.type_id)
	{
		helper_t::copy(old.type_id, &old.data, &data);
	}

	variant(variant<Ts...>&& old) : type_id(old.type_id)
	{
		helper_t::move(old.type_id, &old.data, &data);
	}

	template<typename T, typename... Args>
	static variant make(Args&&... args)
	{
		variant v;
		v.set<std::remove_reference_t<T>>(std::forward<Args>(args)...);
		return v;
	}


	// Serves as both the move and the copy asignment operator.
	variant<Ts...>& operator= (variant<Ts...> old)
	{
		std::swap(type_id, old.type_id);
		std::swap(data, old.data);

		return *this;
	}

	std::type_index get_type() const { return type_id; }

	template<typename T>
	bool is() const {
		return type_id == typeid(T);
	}

	bool valid() const {
		return type_id != invalid_type();
	}

	template<typename T, typename... Args>
	void set(Args&&... args)
	{
		// First we destroy the current contents
		helper_t::destroy(type_id, &data);
		new (&data) T(std::forward<Args>(args)...);
		type_id = typeid(T);
	}

	template<typename T>
	T& get()
	{
		// It is a dynamic_cast-like behaviour
		if (type_id == typeid(T))
			return *reinterpret_cast<T*>(&data);
		else
			throw std::bad_cast();
	}

	template<typename T>
	const T& get() const
	{
		// It is a dynamic_cast-like behaviour
		if (type_id == typeid(T))
			return *reinterpret_cast<const T*>(&data);
		else
			throw std::bad_cast();
	}

	~variant() {
		helper_t::destroy(type_id, &data);
	}

	/// AOTP

	typedef typename LCA<Ts...>::type lowest_common_ancestor_type;

	lowest_common_ancestor_type* operator->() { return reinterpret_cast<lowest_common_ancestor_type*>(&data); }
};
