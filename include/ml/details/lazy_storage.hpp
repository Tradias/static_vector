#pragma once
#include <concepts>
#include <memory>
#include <utility>

namespace ml::details
{
  struct DefaultInit {};
  struct Uninit : DefaultInit {};

  template<class ValueT>
  union lazy_storage final {
  private:
    ValueT m_value;

  public:
    using value_type = ValueT;
    using pointer = ValueT*;
    using const_pointer = ValueT const*;
    using reference = ValueT&;
    using const_reference = ValueT const&;

    lazy_storage ()
      requires (std::is_trivially_default_constructible_v<ValueT>)
    = default;

    consteval lazy_storage (DefaultInit) noexcept (std::is_nothrow_default_constructible_v<ValueT>)
      : m_value ()
    {}

    constexpr lazy_storage (Uninit) noexcept
    {}

    constexpr lazy_storage () noexcept (noexcept (lazy_storage (Uninit {}))) : lazy_storage (Uninit {})
    {}

    constexpr ~lazy_storage ()                            //
      noexcept                                            //
      requires (std::is_trivially_destructible_v<ValueT>) //
    = default;

    constexpr ~lazy_storage () noexcept
    {}

    template<class T>
    explicit constexpr lazy_storage (T&& init)              //
      noexcept (std::is_nothrow_constructible_v<ValueT, T>) //
      requires (std::constructible_from<ValueT, T>)         //
      : m_value (std::forward<T> (init))
    {}

    constexpr auto data () //
      noexcept             //
      -> ValueT*           //
    {
      return std::addressof (m_value);
    }

    constexpr auto data () const //
      noexcept                   //
      -> ValueT const*           //
    {
      return std::addressof (m_value);
    }

    constexpr auto value () //
      noexcept              //
      -> ValueT&            //
    {
      return m_value;
    }

    constexpr auto value () const //
      noexcept                    //
      -> ValueT const&            //
    {
      return m_value;
    }

    constexpr void destroy () //
      noexcept                //
    {
      if constexpr (!std::is_trivially_destructible_v<value_type>) {
        std::ranges::destroy_at (data ());
      }
    }

    template<class... Args>
    constexpr auto construct (Args&&... args)                     //
      noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
      -> ValueT&                                                  //
    { return *std::ranges::construct_at (data (), std::forward<Args> (args)...); }
  };

} // namespace ml::details
