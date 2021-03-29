#pragma once
#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

namespace ml::details
{
	template<class ValueT>
		requires requires
		{
			requires (std::is_object_v<ValueT>);
			requires (!std::is_const_v<ValueT>);
			requires (!std::is_volatile_v<ValueT>);
		}
	union lazy_initialized_storage final
	{
	private:
		struct empty_type
		{};

		[[no_unique_address]] //
		empty_type m_empty {};
		[[no_unique_address]] //
		ValueT m_value;

	public:
		using value_type = ValueT;
		using pointer = ValueT*;
		using const_pointer = ValueT const*;
		using reference = ValueT&;
		using const_reference = ValueT const&;

		lazy_initialized_storage () = default;

		template<class T>
		constexpr explicit(!std::convertible_to<T, ValueT>)			//
			lazy_initialized_storage (T&& init)										//
			noexcept (std::is_nothrow_constructible_v<ValueT, T>) //
			requires (std::constructible_from<ValueT, T>)					//
			: m_value (std::forward<T> (init))
		{}

		template<class... Args>
		constexpr explicit																						//
			lazy_initialized_storage (std::in_place_t, Args&&... args)	//
			noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
			requires (std::constructible_from<ValueT, Args...>)					//
			: m_value (std::forward<Args> (args)...)
		{}

		constexpr auto data () //
			noexcept						 //
			-> ValueT*					 //
		{
			return std::addressof (m_value);
		}

		constexpr auto data () const //
			noexcept									 //
			-> ValueT const*					 //
		{
			return std::addressof (m_value);
		}

		constexpr auto value () & //
			noexcept								//
			-> ValueT&							//
		{
			return m_value;
		}

		constexpr auto value () const& //
			noexcept										 //
			-> ValueT const&						 //
		{
			return m_value;
		}

		constexpr auto value () &&																//
			noexcept (std::is_nothrow_move_constructible_v<ValueT>) //
			-> ValueT																								//
			requires (std::move_constructible<ValueT>)							//
		{
			return std::move (m_value);
		}

		constexpr auto value () const&& = delete;

		constexpr void destroy () //
			noexcept								//
		{
			std::ranges::destroy_at (data ());
			std::ranges::construct_at (std::addressof (m_empty));
		}

		template<class... Args>
		constexpr void construct (Args&&... args)											//
			noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
			requires (std::constructible_from<ValueT, Args...>)					//
		{
			std::ranges::destroy_at (std::addressof (m_empty));
			std::ranges::construct_at (data (), std::forward<Args> (args)...);
		}

		template<class... Args>
		constexpr void reconstruct (Args&&... args)										//
			noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
			requires (std::constructible_from<ValueT, Args...>)					//
		{
			destroy ();
			construct (std::forward<Args> (args)...);
		}
	};

	template<class T>
	lazy_initialized_storage (T&&) -> lazy_initialized_storage<std::remove_cvref_t<T>>;

} // namespace ml::details
