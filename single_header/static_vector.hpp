#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <iterator>
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
    constexpr explicit(!std::convertible_to<T, ValueT>)     //
      lazy_initialized_storage (T&& init)                   //
      noexcept (std::is_nothrow_constructible_v<ValueT, T>) //
      requires (std::constructible_from<ValueT, T>)         //
      : m_value (std::forward<T> (init))
    {}
    template<class... Args>
    constexpr explicit                                            //
      lazy_initialized_storage (std::in_place_t, Args&&... args)  //
      noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
      requires (std::constructible_from<ValueT, Args...>)         //
      : m_value (std::forward<Args> (args)...)
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
    constexpr auto value () & //
      noexcept                //
      -> ValueT&              //
    {
      return m_value;
    }
    constexpr auto value () const& //
      noexcept                     //
      -> ValueT const&             //
    {
      return m_value;
    }
    constexpr auto value () &&                                //
      noexcept (std::is_nothrow_move_constructible_v<ValueT>) //
      -> ValueT                                               //
      requires (std::move_constructible<ValueT>)              //
    {
      return std::move (m_value);
    }
    constexpr auto value () const&& = delete;
    constexpr void destroy () //
      noexcept                //
    {
      std::ranges::destroy_at (data ());
      std::ranges::construct_at (std::addressof (m_empty));
    }
    template<class... Args>
    constexpr void construct (Args&&... args)                     //
      noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
      requires (std::constructible_from<ValueT, Args...>)         //
    {
      std::ranges::destroy_at (std::addressof (m_empty));
      std::ranges::construct_at (data (), std::forward<Args> (args)...);
    }
    template<class... Args>
    constexpr void reconstruct (Args&&... args)                   //
      noexcept (std::is_nothrow_constructible_v<ValueT, Args...>) //
      requires (std::constructible_from<ValueT, Args...>)         //
    {
      destroy ();
      construct (std::forward<Args> (args)...);
    }
  };
  template<class T>
  lazy_initialized_storage (T&&) -> lazy_initialized_storage<std::remove_cvref_t<T>>;
} // namespace ml::details
namespace ml::details
{
  template<class StorageType>
  class storage_iterator;
  template<class StorageType>
  class storage_iterator
  {
  private:
    friend storage_iterator<std::remove_const_t<StorageType>>;
    friend storage_iterator<std::add_const_t<StorageType>>;
    [[no_unique_address]] //
    StorageType* m_ptr = nullptr;

  public:
    // ========================================================================== //
    // --- Type aliases --------------------------------------------------------- //
    // ========================================================================== //
    using value_type = typename std::remove_const_t<StorageType>::value_type;
    using pointer = std::conditional_t<std::is_const_v<StorageType>, value_type const*, value_type*>;
    using reference = std::conditional_t<std::is_const_v<StorageType>, value_type const&, value_type&>;
    using difference_type = std::ptrdiff_t;
    using iterator_tag = std::random_access_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;
    static_assert (std::is_same_v<std::remove_const_t<StorageType>, lazy_initialized_storage<value_type>>);
    // ========================================================================== //
    // --- Conversions- --------------------------------------------------------- //
    // ========================================================================== //
    constexpr operator storage_iterator<std::add_const_t<StorageType>> () const //
      noexcept                                                                  //
      requires (std::is_const_v<StorageType>)                                   //
    {
      return storage_iterator<std::add_const_t<StorageType>> (m_ptr);
    }
    // ========================================================================== //
    // --- Constructors --------------------------------------------------------- //
    // ========================================================================== //
    storage_iterator () = default;
    constexpr explicit                      //
      storage_iterator (StorageType* first) //
      noexcept                              //
      : m_ptr (first)
    {}
    // ========================================================================== //
    // --- Comparisons ---------------------------------------------------------- //
    // ========================================================================== //
    bool operator== (storage_iterator const&) const = default;
    auto operator<=> (storage_iterator const&) const = default;
    // ========================================================================== //
    // --- Incrementable interface ---------------------------------------------- //
    // ========================================================================== //
    constexpr auto operator++ () //
      noexcept                   //
      -> storage_iterator&       //
    {
      ++m_ptr;
      return *this;
    }
    constexpr auto operator++ (int) //
      noexcept                      //
      -> storage_iterator           //
    {
      auto copy = *this;
      ++*this;
      return copy;
    }
    // ========================================================================== //
    // --- Readable interface --------------------------------------------------- //
    // ========================================================================== //
    constexpr auto operator* () const //
      noexcept                        //
      -> reference                    //
    {
      return m_ptr->value ();
    }
    // ========================================================================== //
    // --- Bidirectional interface ---------------------------------------------- //
    // ========================================================================== //
    constexpr auto operator-- () //
      noexcept                   //
      -> storage_iterator&       //
    {
      --m_ptr;
      return *this;
    }
    constexpr auto operator-- (int) //
      noexcept                      //
      -> storage_iterator           //
    {
      auto copy = *this;
      --*this;
      return *this;
    }
    // ========================================================================== //
    // --- Random access interface ---------------------------------------------- //
    // ========================================================================== //
    constexpr auto operator+= (std::integral auto delta) //
      noexcept                                           //
      -> storage_iterator                                //
    {
      m_ptr += delta;
      return *this;
    }
    constexpr auto operator-= (std::integral auto delta) //
      noexcept                                           //
      -> storage_iterator                                //
    {
      m_ptr -= delta;
      return *this;
    }
    friend constexpr auto operator+ (std::integral auto delta, storage_iterator iter) //
      noexcept                                                                        //
      -> storage_iterator                                                             //
    {
      iter += delta;
      return iter;
    }
    friend constexpr auto operator+ (storage_iterator iter, std::integral auto delta) //
      noexcept                                                                        //
      -> storage_iterator                                                             //
    {
      iter += delta;
      return iter;
    }
    friend constexpr auto operator- (std::integral auto delta, storage_iterator iter) //
      noexcept                                                                        //
      -> storage_iterator                                                             //
    {
      iter -= delta;
      return iter;
    }
    friend constexpr auto operator- (storage_iterator iter, std::integral auto delta) //
      noexcept                                                                        //
      -> storage_iterator                                                             //
    {
      iter -= delta;
      return iter;
    }
    friend constexpr auto operator- (storage_iterator lhs, storage_iterator rhs) //
      noexcept                                                                   //
      -> difference_type                                                         //
    {
      return lhs.m_ptr - rhs.m_ptr;
    }
    constexpr auto operator[] (std::integral auto index) //
      noexcept                                           //
      -> reference                                       //
    {
      return m_ptr[index].value ();
    }
    constexpr auto operator-> () const //
			noexcept												 //
      -> pointer                       //
    {
      return m_ptr->data ();
    }
  };
  template<class StorageType>
  storage_iterator (StorageType*) -> storage_iterator<StorageType>;
} // namespace ml::details
#include <stdexcept>
namespace ml
{
  template<class ValueT, std::size_t StaticSize>
    requires requires
    {
      requires (std::is_object_v<ValueT>);
      requires (!std::is_const_v<ValueT>);
      requires (!std::is_volatile_v<ValueT>);
    }
  class static_vector
  {
  private:
    using storage_type = details::lazy_initialized_storage<ValueT>;
    std::size_t m_size {};
    std::array<storage_type, StaticSize> m_storage {};

  public:
    // =============================================================================== //
    // --- Types aliases ------------------------------------------------------------- //
    // =============================================================================== //
    using value_type = ValueT;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using iterator = details::storage_iterator<storage_type>;
    using const_iterator = details::storage_iterator<storage_type const>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    // =============================================================================== //
    // --- Constructors -------------------------------------------------------------- //
    // =============================================================================== //
    constexpr static_vector ()                                       //
      noexcept (std::is_nothrow_default_constructible_v<value_type>) //
      : m_size (0), m_storage {}
    {}
    explicit constexpr static_vector (std::integral auto count) //
      noexcept (false)                                          //
      requires (std::default_initializable<value_type>)         //
      : static_vector ()
    {
      emplace_back_many (count);
    }
    constexpr static_vector (std::integral auto count, value_type const& init) //
      noexcept (false)                                                         //
      requires (std::copy_constructible<value_type>)                           //
      : static_vector ()
    {
      emplace_back_many (count, init);
    }
    template<class... Args>
    constexpr static_vector (std::in_place_t, Args&&... args)               //
      noexcept ((std::is_nothrow_constructible_v<value_type, Args> && ...)) //
      requires ((std::constructible_from<value_type, Args> && ...))         //
      : static_vector ()
    {
      constexpr bool is_noexcept = (std::is_nothrow_constructible_v<value_type, Args> && ...);
      if constexpr (is_noexcept) {
        (static_cast<void> (emplace_back (std::forward<Args> (args))), ...);
      } else {
        try {
          (static_cast<void> (emplace_back (std::forward<Args> (args))), ...);
        } catch (...) {
          clear ();
          throw;
        }
      }
    }
    template<class... Args>
      requires requires
      {
        requires std::default_initializable<value_type>;
        requires sizeof...(Args) <= StaticSize;
        requires (std::constructible_from<value_type, Args> && ...);
      }
    constexpr static_vector (std::integral auto count, std::in_place_t, Args&&... args) //
      noexcept (false)                                                                  //
      : static_vector ()
    {
      try {
        (static_cast<void> (emplace_back (std::forward<Args> (args))), ...);
        if (std::cmp_less (sizeof...(Args), count)) {
          emplace_back_many (static_cast<size_type> (count) - sizeof...(Args));
        }
      } catch (...) {
        clear ();
        throw;
      }
    }
    template<std::input_iterator It, std::sentinel_for<It> Sen>
      requires std::constructible_from<value_type, std::iter_reference_t<It>>
    constexpr static_vector (It first, Sen last) //
      noexcept (false)                           //
      : static_vector ()
    {
      try {
        while (first != last) {
          emplace_back (*first);
          ++first;
        }
      } catch (...) {
        clear ();
        throw;
      }
    }
    template<std::ranges::input_range Rng>
      requires requires
      {
        requires not std::same_as<std::remove_cvref_t<Rng>, static_vector>;
        requires not std::same_as<std::remove_cvref_t<Rng>, value_type>;
        requires std::constructible_from<value_type, std::ranges::range_reference_t<Rng>>;
      }
    constexpr static_vector (Rng&& rng) //
      noexcept (false)                  //
      : static_vector (std::ranges::begin (rng), std::ranges::end (rng))
    {}
    template<size_type Count>
      requires requires
      {
        requires (std::move_constructible<value_type>);
        requires (Count <= StaticSize);
      }
    constexpr static_vector (value_type (&&arr)[Count])           //
      noexcept (std::is_nothrow_move_constructible_v<value_type>) //
      : static_vector (std::move_iterator (arr), std::move_sentinel (arr + Count))
    {}
    // =============================================================================== //
    // --- Copy constructor ---------------------------------------------------------- //
    // =============================================================================== //
    constexpr static_vector (static_vector const& other) //
      noexcept                                           //
      requires (std::copy_constructible<value_type>&& std::is_trivially_copy_constructible_v<value_type>) = default;
    constexpr static_vector (static_vector const& other)          //
      noexcept (std::is_nothrow_copy_constructible_v<value_type>) //
      requires (std::copy_constructible<value_type>)
      : static_vector (other.begin (), other.end ())
    {}
    // =============================================================================== //
    // --- Move constructor ---------------------------------------------------------- //
    // =============================================================================== //
    constexpr static_vector (static_vector&& other)                                                       //
      noexcept                                                                                            //
      requires (std::move_constructible<value_type>&& std::is_trivially_move_constructible_v<value_type>) //
      = default;
    constexpr static_vector (static_vector&& other)               //
      noexcept (std::is_nothrow_move_constructible_v<value_type>) //
      requires (std::move_constructible<value_type>)              //
      : static_vector ()
    {
      if constexpr (std::is_nothrow_move_constructible_v<value_type> || !std::copy_constructible<value_type>) {
        std::ranges::construct_at (this, std::move_iterator (other.begin ()), std::move_sentinel (other.end ()));
      } else {
        std::ranges::construct_at (this, other.begin (), other.end ());
      }
    }
    // =============================================================================== //
    // --- Copy assignment ----------------------------------------------------------- //
    // =============================================================================== //
    constexpr static_vector& operator= (static_vector const& other) //
      noexcept (std::is_nothrow_copy_constructible_v<value_type>&& std::is_nothrow_copy_assignable_v<value_type>) //
      requires (std::copyable<value_type>) //
    {
      if constexpr (std::is_nothrow_copy_constructible_v<value_type> && std::is_nothrow_copy_assignable_v<value_type>) {
        if (size () <= other.size ()) {
          std::ranges::copy (other.begin (), other.begin () + size (), begin ());
          emplace_back_range (other.begin () + size (), other.end ());
        } else {
          std::ranges::copy (other.begin (), other.end (), begin ());
          shrink_to (other.size ());
        }
        return *this;
      } else {
        // this is O(capacity() + other.capacity()) instead of O(size() + other.size())
        auto copy = *other;
        *this = std::move (copy);
        return *this;
      }
    }
    // =============================================================================== //
    // --- Move assignment ----------------------------------------------------------- //
    // =============================================================================== //
    constexpr static_vector& operator= (static_vector&& other) //
      noexcept (std::is_nothrow_move_assignable_v<value_type>  //
          && std::is_nothrow_move_constructible_v<value_type>) //
      requires (std::movable<value_type>)                      //
    {
      if (size () <= other.size ()) {
        std::ranges::move (other.begin (), other.begin () + size (), begin ());
        emplace_back_range (std::move_iterator (other.begin () + size ()), std::move_sentinel (other.end ()));
      } else {
        std::ranges::move (other.begin (), other.end (), begin ());
        shrink_to (other.size ());
      }
      return *this;
    }
    // =============================================================================== //
    // --- Destructors---------------------------------------------------------------- //
    // =============================================================================== //
    constexpr ~static_vector ()                               //
      noexcept                                                //
      requires (std::is_trivially_destructible_v<value_type>) //
      = default;
    constexpr ~static_vector () //
      noexcept
    {
      clear ();
    }
    // =============================================================================== //
    // --- Comparisons --------------------------------------------------------------- //
    // =============================================================================== //
    template<class OtherT, std::size_t OtherSize>
      requires (std::equality_comparable_with<value_type const&, OtherT const&>) //
    friend constexpr bool operator== (static_vector const& lhs, static_vector<OtherT, OtherSize> const& rhs) //
      noexcept (noexcept (std::declval<value_type const&> () == std::declval<OtherT const&> ())) //
    {
      return std::ranges::equal (lhs, rhs);
    }
    template<class U, std::size_t M>
      requires (std::three_way_comparable_with<value_type const&, U const&>)                     //
    friend constexpr auto operator<=> (static_vector const& lhs, static_vector<U, M> const& rhs) //
      noexcept (noexcept (std::declval<value_type const&> () <=> std::declval<U const&> ()))     //
      -> std::compare_three_way_result_t<value_type const&, U const&>                            //
    {
      return std::lexicographical_compare_three_way (lhs.begin (), lhs.end (), rhs.begin (), rhs.end ());
    }
    // =============================================================================== //
    // --- Single element access ----------------------------------------------------- //
    // =============================================================================== //
    template<std::integral T>
    [[nodiscard]] constexpr auto at (T index) & //
      noexcept (false)                          //
      -> reference                              //
    {
      if (std::cmp_less (index, 0) || std::cmp_less_equal (size (), index)) {
        throw std::out_of_range ("static_vector index out of range.");
      } else {
        return m_storage[static_cast<size_type> (index)].value ();
      }
    }
    template<std::integral T>
    [[nodiscard]] constexpr auto at (T index) &&     //
      noexcept (false)                               //
      -> value_type                                  //
      requires (std::move_constructible<value_type>) //
    {
      return std::move (at (index));
    }
    template<std::integral T>
    [[nodiscard]] constexpr auto at (T index) const& //
      noexcept (false)                               //
      -> const_reference                             //
    {
      if (std::cmp_less (index, 0) || std::cmp_less_equal (size (), index)) {
        throw std::out_of_range ("static_vector index out of range.");
      } else {
        return m_storage[static_cast<size_type> (index)].value ();
      }
    }
    constexpr void at (int) const&& = delete;
    [[nodiscard]] constexpr auto front () & //
      noexcept (false)                      //
      -> reference                          //
    {
      return at (0u);
    }
    [[nodiscard]] constexpr auto front () &&         //
      noexcept (false)                               //
      -> value_type                                  //
      requires (std::move_constructible<value_type>) //
    {
      return std::move (front ());
    }
    [[nodiscard]] constexpr auto front () const& //
      noexcept (false)                           //
      -> const_reference                         //
    {
      return at (0u);
    }
    constexpr void front () const&& = delete;
    [[nodiscard]] constexpr auto back () & //
      noexcept (false)                     //
      -> reference                         //
    {
      return at (size () - 1);
    }
    [[nodiscard]] constexpr auto back () &&          //
      noexcept (false)                               //
      -> value_type                                  //
      requires (std::move_constructible<value_type>) //
    {
      return std::move (back ());
    }
    [[nodiscard]] constexpr auto back () const& //
      noexcept (false)                          //
      -> const_reference                        //
    {
      return at (size () - 1);
    }
    constexpr void back () const&& = delete;
    template<std::integral T>
    [[nodiscard]] constexpr auto operator[] (T index) & //
      noexcept (false)                                  //
      -> reference                                      //
    {
      return at (index);
    }
    template<std::integral T>
    [[nodiscard]] constexpr auto operator[] (T index) const& //
      noexcept (false)                                       //
      -> const_reference                                     //
    {
      return at (index);
    }
    template<std::integral T>
    constexpr auto operator[] (T index) &&           //
      noexcept (false)                               //
      -> value_type                                  //
      requires (std::move_constructible<value_type>) //
    {
      return std::move (at (index));
    }
    template<std::integral T>
    constexpr void operator[] (T index) const&& = delete;
    [[nodiscard]] constexpr auto index_of (iterator pos) const //
      noexcept                                                 //
      -> size_type                                             //
    {
      return pos - begin ();
    }
    [[nodiscard]] constexpr auto index_of (const_iterator pos) const //
      noexcept                                                       //
      -> size_type                                                   //
    {
      return pos - cbegin ();
    }
    // =============================================================================== //
    // --- Size inspection ----------------------------------------------------------- //
    // =============================================================================== //
    [[nodiscard]] constexpr auto size () const //
      noexcept                                 //
      -> size_type                             //
    {
      return m_size;
    }
    [[nodiscard]] friend constexpr auto size (static_vector const& self) //
      noexcept                                                           //
      -> size_type                                                       //
    {
      return self.size ();
    }
    [[nodiscard]] constexpr auto ssize () const //
      noexcept                                  //
      -> difference_type                        //
    {
      return static_cast<difference_type> (size ());
    }
    [[nodiscard]] friend constexpr auto ssize (static_vector const& self) //
      noexcept                                                            //
      -> difference_type                                                  //
    {
      return self.ssize ();
    }
    [[nodiscard]] static constexpr auto max_size () //
      noexcept                                      //
      -> size_type                                  //
    {
      return StaticSize;
    }
    [[nodiscard]] static constexpr auto capacity () //
      noexcept (true)                               //
      -> size_type                                  //
    {
      return StaticSize;
    }
    [[nodiscard]] constexpr auto empty () const //
      noexcept                                  //
      -> bool                                   //
    {
      return size () == 0;
    }
    [[nodiscard]] friend constexpr auto empty (static_vector const& self) //
      noexcept                                                            //
      -> bool                                                             //
    {
      return self.empty ();
    }
    [[nodiscard]] constexpr auto full () const //
      noexcept                                 //
      -> bool
    {
      return size () == max_size ();
    }
    // =============================================================================== //
    // --- Iterator access ----------------------------------------------------------- //
    // =============================================================================== //
    // Note: friend functions are declared as templates to avoid implicit conversions.
    // Implicit conversion checks during overload resolution might trigger cyclic evaluation of standard concepts.
    [[nodiscard]] constexpr auto cbegin () const //
      noexcept                                   //
      -> const_iterator                          //
    {
      return const_iterator (m_storage.data ());
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto cbegin (T const& self) //
      noexcept                                                 //
      -> const_iterator                                        //
    {
      return self.cbegin ();
    }
    [[nodiscard]] constexpr auto cend () const //
      noexcept                                 //
      -> const_iterator                        //
    {
      return const_iterator (m_storage.data () + size ());
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto cend (T const& self) //
      noexcept                                               //
      -> const_iterator                                      //
    {
      return self.cend ();
    }
    [[nodiscard]] constexpr auto begin () //
      noexcept                            //
      -> iterator                         //
    {
      return iterator (m_storage.data ());
    }
    [[nodiscard]] constexpr auto begin () const //
      noexcept                                  //
      -> const_iterator                         //
    {
      return cbegin ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto begin (T const& self) //
      noexcept                                                //
      -> const_iterator                                       //
    {
      return self.begin ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto begin (T& self) //
      noexcept                                          //
      -> iterator                                       //
    {
      return self.begin ();
    }
    [[nodiscard]] constexpr auto end () //
      noexcept                          //
      -> iterator                       //
    {
      return iterator (m_storage.data () + size ());
    }
    [[nodiscard]] constexpr auto end () const //
      noexcept                                //
      -> const_iterator                       //
    {
      return cend ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto end (T const& self) //
      noexcept                                              //
      -> const_iterator                                     //
    {
      return self.end ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto end (T& self) //
      noexcept                                        //
      -> iterator                                     //
    {
      return self.end ();
    }
    // =============================================================================== //
    // --- Reverse iterator access --------------------------------------------------- //
    // =============================================================================== //
    // Note: friend functions are declared as templates to avoid implicit conversions.
    // Implicit conversion checks during overload resolution might trigger cyclic evaluation of standard concepts.
    [[nodiscard]] constexpr auto crbegin () const //
      noexcept                                    //
      -> const_reverse_iterator                   //
    {
      return const_reverse_iterator (cend ());
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto crbegin (T const& self) //
      noexcept                                                  //
      -> const_reverse_iterator                                 //
    {
      return self.crbegin ();
    }
    [[nodiscard]] constexpr auto crend () const //
      noexcept                                  //
      -> const_reverse_iterator                 //
    {
      return const_reverse_iterator (cbegin ());
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto crend (T const& self) //
      noexcept                                                //
      -> const_reverse_iterator                               //
    {
      return self.crend ();
    }
    [[nodiscard]] constexpr auto rbegin () //
      noexcept                             //
      -> reverse_iterator                  //
    {
      return reverse_iterator (end ());
    }
    [[nodiscard]] constexpr auto rbegin () const //
      noexcept                                   //
      -> const_iterator                          //
    {
      return crbegin ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto rbegin (T& self) //
      noexcept                                           //
      -> reverse_iterator                                //
    {
      return self.rbegin ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto rbegin (T const& self) //
      noexcept                                                 //
      -> const_reverse_iterator                                //
    {
      return self.rbegin ();
    }
    [[nodiscard]] constexpr auto rend () //
      noexcept                           //
      -> reverse_iterator                //
    {
      return reverse_iterator (begin ());
    }
    [[nodiscard]] constexpr auto rend () const //
      noexcept                                 //
      -> const_iterator                        //
    {
      return crend ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto rend (T& self) //
      noexcept                                         //
      -> reverse_iterator                              //
    {
      return self.rend ();
    }
    template<std::same_as<static_vector> T>
    [[nodiscard]] friend constexpr auto rend (T const& self) //
      noexcept                                               //
      -> const_reverse_iterator                              //
    {
      return self.rend ();
    }
    // =============================================================================== //
    // --- Single element modifiers -------------------------------------------------- //
    // =============================================================================== //
    constexpr auto push_back (value_type const& init) //
      noexcept (false)                                //
      -> reference                                    //
      requires (std::copy_constructible<value_type>)  //
    {
      return emplace_back (init);
    }
    constexpr auto push_back (value_type&& init)     //
      noexcept (false)                               //
      -> reference                                   //
      requires (std::move_constructible<value_type>) //
    {
      return emplace_back (std::move (init));
    }
    template<class... Args>
      requires (std::constructible_from<value_type, Args...>) //
    constexpr auto emplace_back (Args&&... args)              //
      noexcept (false)                                        //
      -> reference                                            //
    {
      if (full ()) {
        throw std::logic_error ("invalid emplace_back on full static_vector.");
      } else {
        try {
          m_size++;
          m_storage[m_size - 1].construct (std::forward<Args> (args)...);
          return m_storage[m_size - 1].value ();
        } catch (...) {
          pop_back ();
          throw;
        }
      }
    }
    constexpr void pop_back () //
      noexcept                 //
    {
      if (!empty ()) {
        m_size--;
        m_storage[m_size].destroy ();
      }
    }
    constexpr auto insert (iterator pos, value_type const& init)                  //
      noexcept (false)                                                            //
      requires (std::copy_constructible<value_type>&& std::swappable<value_type>) //
    {
      emplace_back (init);
      std::ranges::rotate (pos, end () - 1, end ());
      return pos;
    }
    constexpr auto insert (iterator pos, value_type&& init)                       //
      noexcept (false)                                                            //
      requires (std::move_constructible<value_type>&& std::swappable<value_type>) //
    {
      emplace_back (std::move (init));
      std::ranges::rotate (pos, end () - 1, end ());
      return pos;
    }
    constexpr auto erase (iterator pos)                        //
      noexcept (std::is_nothrow_move_assignable_v<value_type>) //
      requires (std::movable<value_type>)                      //
    {
      if (pos != end ()) {
        std::ranges::move (pos + 1, end (), pos);
        pop_back ();
        return pos;
      }
    }
    // =============================================================================== //
    // --- Multiple element modifiers ------------------------------------------------ //
    // =============================================================================== //
    constexpr void clear () //
      noexcept              //
    {
      shrink_to (0);
    }
    constexpr void shrink_by (std::integral auto count) //
      noexcept                                          //
    {
      while (std::cmp_less (0, count) && !empty ()) {
        --count;
        pop_back ();
      }
    }
    constexpr void shrink_to (std::integral auto count) //
      noexcept                                          //
    {
      while (std::cmp_less (count, size ())) {
        pop_back ();
      }
    }
    constexpr void resize (std::integral auto count)    //
      noexcept (false)                                  //
      requires (std::default_initializable<value_type>) //
    {
      if (std::cmp_less_equal (count, size ())) {
        shrink_to (count);
      } else {
        emplace_back_many (count);
      }
    }
    constexpr void resize (std::integral auto count, value_type const& init) //
      noexcept (false)                                                       //
      requires (std::copy_constructible<value_type>)                         //
    {
      if (std::cmp_less_equal (count, size ())) {
        shrink_to (count);
      } else {
        emplace_back_many (static_cast<size_type> (count) - size (), init);
      }
    }
    constexpr auto erase (iterator first, iterator last)       //
      noexcept (std::is_nothrow_move_assignable_v<value_type>) //
      requires (std::movable<value_type>)                      //
    {
      std::ranges::move (last, end (), first);
      shrink_by (last - first);
      return first;
    }
    constexpr auto insert (iterator pos, std::integral auto count, value_type const& init) //
      noexcept (false)                                                                     //
      requires (std::copyable<value_type>)                                                 //
    {
      emplace_back_many (count, init);
      std::ranges::rotate (pos, end () - count, end ());
      return pos;
    }
    template<std::input_iterator It, std::sentinel_for<It> Sen>
      requires (std::constructible_from<value_type, std::iter_reference_t<It>>&& std::swappable<value_type>) //
    constexpr auto insert (iterator pos, It first, Sen last) //
      noexcept (false)                                       //
    {
      auto inserted = emplace_back_range (std::move (first), std::move (last));
      std::ranges::rotate (pos, end () - inserted, end ());
      return pos;
    }
    template<class... Args>
      requires (std::constructible_from<value_type, Args const&...>)                 //
    constexpr void emplace_back_many (std::integral auto count, Args const&... args) //
      noexcept (false)                                                               //
    {
      auto const previous_size = size ();
      try {
        for (size_type i = 0; std::cmp_less (i, count); ++i)
          emplace_back (args...);
      } catch (...) {
        shrink_to (previous_size);
        throw;
      }
    }
    template<std::input_iterator It, std::sentinel_for<It> Sen>
      requires (std::constructible_from<value_type, std::iter_reference_t<It>>) //
    constexpr auto emplace_back_range (It first, Sen last)                      //
      noexcept (false)                                                          //
      -> size_type                                                              //
    {
      auto const previous_size = size ();
      try {
        for (; first != last; ++first)
          emplace_back (*first);
        return size () - previous_size;
      } catch (...) {
        shrink_to (previous_size);
        throw;
      }
    }
    template<std::ranges::input_range Rng>
      requires (std::constructible_from<value_type, std::ranges::range_reference_t<Rng>>) //
    constexpr auto emplace_back_range (Rng&& range)                                       //
      noexcept (false)                                                                    //
    {
      return emplace_back_range (std::ranges::begin (range), std::ranges::end (range));
    }
    template<size_type Count>
      requires (Count <= StaticSize)                              //
    constexpr auto emplace_back_range (value_type (&&arr)[Count]) //
      noexcept (false)                                            //
      -> size_type                                                //
    {
      return emplace_back_range (std::move_iterator (arr), std::move_sentinel (arr + Count));
    }
  };
  // Deduce static_vector template parameters from C array size.
  template<class ValueT, auto Count>
  static_vector (ValueT (&&arr)[Count]) -> static_vector<ValueT, Count>;
  template<class ValueT, auto Count>
  static_vector (ValueT const (&arr)[Count]) -> static_vector<ValueT, Count>;
  // Deduce static_vector template parameters from std::array template arguments.
  template<class ValueT, std::size_t Count>
  static_vector (std::array<ValueT, Count>&&) -> static_vector<ValueT, Count>;
  template<class ValueT, std::size_t Count>
  static_vector (std::array<ValueT, Count> const&) -> static_vector<ValueT, Count>;
  // Deduce static_vector template parameters from cardinality of variadic
  // arguments.
  template<class Arg1, class... Args>
    requires ((std::common_with<Args, Arg1> && ...))
  static_vector (std::in_place_t, Arg1&&, Args&&...)
  ->static_vector<std::common_type_t<Arg1, Args...>, sizeof...(Args) + 1>;
} // namespace ml
