#pragma once
#include <ml/details/storage_iterator.hpp>

#include <algorithm>
#include <stdexcept>

namespace ml
{
  template<class ValueT, std::size_t StaticSize>
    requires (std::is_object_v<ValueT> && !std::is_const_v<ValueT> && !std::is_volatile_v<ValueT>)
  class static_vector {
  private:
    using storage_type = details::lazy_storage<ValueT>;

    std::size_t m_size {};
    storage_type m_storage[StaticSize];

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

    /**
     * Default constructor (1/8).
     * Initializes a static_vector of 0 elements.
     */
    static_vector () = default;

    /**
     * Size constructor without initial value (2/8).
     * Initializes a static_vector of @count elements.
     *
     * Throws:
     * - std::logic_error if count >= StaticSize.
     * - exceptions thrown by value_type's default constructor.
     * When an exception is thrown, leaves an empty static_vector.
     *
     * Mandates:
     * - std::default_initializable<value_type>
     *
     * Notes:
     * - passing a negative @count argument leaves the static_vector empty.
     */
    explicit constexpr static_vector (std::integral auto count) //
      noexcept (false)                                          //
      requires (std::default_initializable<value_type>)         //
    {
      emplace_back_many (count);
    }

    /**
     * Size constructor with initial value (3/8).
     * Initializes a static_vector of @count elements, copy-constructed from @init.
     *
     * Throws:
     * - std::logic_error if count >= StaticSize.
     * - exceptions thrown by value_type's copy constructor.
     * When an exception is thrown, leaves an empty static_vector.
     *
     * Mandates:
     * - std::copy_constructible<value_type>
     *
     * Notes:
     * - passing a negative @count argument leaves the static_vector empty.
     */
    constexpr static_vector (std::integral auto count, value_type const& init) //
      noexcept (false)                                                         //
      requires (std::copy_constructible<value_type>)                           //
    {
      emplace_back_many (count, init);
    }

    /**
     * Variadic constructor (4/8).
     * Initializes a static_vector of sizeof...(Args) elements, initialized from @args.
     *
     * Throws:
     * - exceptions thrown by value_type constructors.
     * When an exception is thrown, leaves an empty static_vector.
     *
     * Mandates:
     * - (std::constructible_from<value_type, Args> && ...)
     * - sizeof...(Args) <= StaticSize
     *
     * Notes:
     * - if an exception in thrown, rvalue arguments remain in moved-from state.
     * - static_vector{std::in_place, args...} looks like aggregate initialization,
     * but it is not. Lifetime extension doesn't apply, narrow conversion are allowed.
     */
    template<class... Args>
    constexpr static_vector (std::in_place_t, Args&&... args)               //
      noexcept ((std::is_nothrow_constructible_v<value_type, Args> && ...)) //
      requires ((std::constructible_from<value_type, Args> && ...))         //
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

    /**
     * Variadic constructor (5/8).
     * Initializes a static_vector of @count elements, initialized from @args.
     * If there are less arguments tha @count, the remaining elements are default constructed.
     *
     * Throws:
     * - exceptions thrown by value_type constructors.
     * When an exception is thrown, leaves an empty static_vector.
     *
     * Mandates:
     * - std::default_initializable<value_type>
     * - sizeof...(Args) <= StaticSize
     * - (std::constructible_from<value_type, Args> && ...)
     *
     * Notes:
     * - static_vector{std::in_place, args...} looks like aggregate initialization,
     * but it is not. Lifetime extension doesn't apply, narrow conversion are allowed.
     */
    template<class... Args>
      requires requires {
        requires std::default_initializable<value_type>;
        requires sizeof...(Args) <= StaticSize;
        requires (std::constructible_from<value_type, Args> && ...);
      }
    constexpr static_vector (std::integral auto count, std::in_place_t, Args&&... args) //
      noexcept (false)                                                                  //
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

    /**
     * Iterator and Sentinel constructor (6/8).
     *
     * Initializes a static_vector of std::ranges::distance(@first, @last) elements.
     * Each element is constructed from the corresponding element in [@first, @last).
     *
     * Throws:
     * - std::logic_error if std::ranges::distance(@first, @last) > StaticSize.
     * - exceptions thrown by value_type constructors.
     * When an exception is thrown, leaves an empty static_vector.
     *
     * Mandates:
     * - std::constructible_from<value_type, std::iter_reference_t<It>>
     */
    template<std::input_iterator It, std::sentinel_for<It> Sen>
      requires std::constructible_from<value_type, std::iter_reference_t<It>>
    constexpr static_vector (It first, Sen last) //
      noexcept (false)                           //
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

    /**
     * Range constructor (7/8).
     *
     * Initializes a static_vector of std::ranges::size(@rng) elements.
     * Each element is constructed from the corresponding element in @rng.
     *
     * Throws:
     * - std::logic_error if std::ranges::size(@rng) > StaticSize.
     * - exceptions thrown by value_type constructors.
     * When an exception is thrown, leaves an empty static_vector.
     *
     * Mandates:
     * - std::constructible_from<value_type, std::ranges::range_reference_t<Rng>>.
     */
    template<std::ranges::input_range Rng>
      requires requires {
        requires not std::same_as<std::remove_cvref_t<Rng>, static_vector>;
        requires not std::same_as<std::remove_cvref_t<Rng>, value_type>;
        requires std::constructible_from<value_type, std::ranges::range_reference_t<Rng>>;
      }
    constexpr static_vector (Rng&& rng) //
      noexcept (false)                  //
      : static_vector (std::ranges::begin (rng), std::ranges::end (rng))
    {}

    /**
     * C-Array move constructor (8/8).
     * Initializes a static_vector of @Count elements.
     * Each element is move-constructed from the corresponding element in @arr.
     *
     * Throws:
     * - exceptions thrown by value_type's move constructor.
     * When an exception is thrown, construction leaves an empty static_vector.
     *
     * Mandates:
     * - std::move_constructible<value_type>.
     * - @Count <= StaticSize.
     *
     * Notes:
     * - this constructor helps deduction of braced-init-list arguments when
     * static_vector class template arguments are provided.
     */
    template<size_type Count>
      requires requires {
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

    static_vector (static_vector const& other)                                                             //
      requires (std::copy_constructible<value_type> && std::is_trivially_copy_constructible_v<value_type>) //
    = default;

    constexpr static_vector (static_vector const& other)          //
      noexcept (std::is_nothrow_copy_constructible_v<value_type>) //
      requires (std::copy_constructible<value_type>)
      : static_vector (other.begin (), other.end ())
    {}

    // =============================================================================== //
    // --- Move constructor ---------------------------------------------------------- //
    // =============================================================================== //

    static_vector (static_vector&& other)                                                                  //
      requires (std::move_constructible<value_type> && std::is_trivially_move_constructible_v<value_type>) //
    = default;

    constexpr static_vector (static_vector&& other)               //
      noexcept (std::is_nothrow_move_constructible_v<value_type>) //
      requires (std::move_constructible<value_type>)              //
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

    constexpr static_vector& operator= (static_vector const& other)                           //
      requires (std::copyable<value_type> && std::is_trivially_copy_assignable_v<value_type>) //
    = default;

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

    constexpr static_vector& operator= (static_vector&& other)                               //
      requires (std::movable<value_type> && std::is_trivially_move_assignable_v<value_type>) //
    = default;

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
      return const_iterator (std::data (m_storage));
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
      return const_iterator (std::data (m_storage) + size ());
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
      return iterator (std::data (m_storage));
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
      return iterator (std::data (m_storage) + size ());
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

    /**
     * Copy constructs a new value at the end.
     *
     * Throws:
     * - std::logic_error if full()
     * - exceptions thrown by value_type's copy constructor.
     * When an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::copy_constructible<value_type>.
     *
     * Returns: a reference to the new element.
     */
    constexpr auto push_back (value_type const& init) //
      noexcept (false)                                //
      -> reference                                    //
      requires (std::copy_constructible<value_type>)  //
    {
      return emplace_back (init);
    }

    /**
     * Move constructs a new value at the end.
     *
     * Throws:
     * - std::logic_error if full()
     * - exceptions thrown by value_type's move constructor.
     * When an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::move_constructible<value_type>.
     *
     * Returns: a reference to the new element.
     */
    constexpr auto push_back (value_type&& init)     //
      noexcept (false)                               //
      -> reference                                   //
      requires (std::move_constructible<value_type>) //
    {
      return emplace_back (std::move (init));
    }

    /**
     * Constructs a new value in place at the end.
     * Forwards @args to the value_type constructor.
     *
     * Throws:
     * - std::logic_error if full()
     * - exceptions thrown by value_type's constructor.
     * When an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::constructible_from<value_type, Args...>.
     *
     * Returns: a reference to the new element.
     */
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
          return m_storage[m_size - 1].construct (std::forward<Args> (args)...);
        } catch (...) {
          pop_back ();
          throw;
        }
      }
    }

    /**
     * Removes the last element, or does nothing if already empty.
     */
    constexpr void pop_back () //
      noexcept                 //
    {
      if (!empty ()) {
        m_size--;
        m_storage[m_size].destroy ();
      }
    }

    /**
     * Copy constructs a new value after @pos.
     *
     * Throws:
     * - std::logic_error if full()
     * - exceptions thrown by value_type's copy constructor.
     * - exceptions thrown by std::ranges::swap.
     * When an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::copy_constructible<value_type>
     * - std::swappable<value_type>
     *
     * Returns: an iterator that points to the new element.
     */
    constexpr auto insert (iterator pos, value_type const& init)                   //
      noexcept (false)                                                             //
      requires (std::copy_constructible<value_type> && std::swappable<value_type>) //
    {
      emplace_back (init);
      std::ranges::rotate (pos, end () - 1, end ());
      return pos;
    }

    /**
     * Move constructs a new value after @pos.
     *
     * Throws:
     * - std::logic_error if full()
     * - exceptions thrown by value_type's move constructor.
     * - exceptions thrown by std::ranges::swap.
     * When an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::move_constructible<value_type>
     * - std::swappable<value_type>
     *
     * Returns: an iterator that points to the new element.
     */
    constexpr auto insert (iterator pos, value_type&& init)                        //
      noexcept (false)                                                             //
      requires (std::move_constructible<value_type> && std::swappable<value_type>) //
    {
      emplace_back (std::move (init));
      std::ranges::rotate (pos, end () - 1, end ());
      return pos;
    }

    /**
     * Erases the element pointed by @pos.
     *
     * Throws:
     * - exceptions thrown by value_type's move assignment.
     * When an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::movable<value_type>
     *
     * Returns: an iterator to the element after the erased element, or end().
     */
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

    /**
     * Erases all elements.
     */
    constexpr void clear () //
      noexcept              //
    {
      shrink_to (0);
    }

    /**
     * Erases @count elements, or all elements if size() < @count.
     */
    constexpr void shrink_by (std::integral auto count) //
      noexcept                                          //
    {
      while (std::cmp_less (0, count) && !empty ()) {
        --count;
        pop_back ();
      }
    }

    /**
     * Erases size() - @count elements, or none if size() <= @count.
     */
    constexpr void shrink_to (std::integral auto count) //
      noexcept                                          //
    {
      while (std::cmp_less (count, size ())) {
        pop_back ();
      }
    }

    /**
     * If size() < @count, emplaces @count - size()
     * default initialized elements at the end.
     * Otherwise, equivalent to shrink_to(@count).
     *
     * Throws:
     * - std::logic_error if @count >= StaticSize
     * - exceptions thrown by value_type's default constructor.
     * If an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::default_initializable<value_type>
     */
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

    /**
     * If size() < @count, emplaces @count - size()
     * copy constructed elements at the end.
     * Otherwise, equivalent to shrink_to(@count).
     * Inserted elements are copy constructed from @init.
     *
     * Throws:
     * - std::logic_error if @count >= StaticSize
     * - exceptions thrown by value_type's copy constructor.
     * If an exception is thrown, this function has no effect.
     *
     * Mandates:
     * - std::copy_constructible<value_type>
     */
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

    /**
     * Erases elements inside the range [@first, @last).
     *
     * Throws:
     * - exceptions thrown by value_type's move assignment.
     * If an exception is thrown, the static_vector object is left in a valid state.
     *
     * Mandates:
     * - std::movable<value_type>
     *
     * Returns:
     * an iterator to the first non erased element not before @first,
     * or end() if none exists.
     */
    constexpr auto erase (iterator first, iterator last)       //
      noexcept (std::is_nothrow_move_assignable_v<value_type>) //
      requires (std::movable<value_type>)                      //
    {
      std::ranges::move (last, end (), first);
      shrink_by (last - first);
      return first;
    }

    /**
     * Inserts @count elements after @pos, copy constructed from @init.
     *
     * Throws:
     * - std::logic_error if size() + count > StaticSize.
     * - exceptions thrown by value_type's copy constructor.
     * - exceptions thrown by value_type's swap.
     * If std::logic_error is thrown by exceeding maximum capacity, this function has no effect.
     * Otherwise, if an exception is thrown the static_vector object is left in a valid state.
     *
     * Mandates:
     * - std::movable<value_type>
     *
     * Returns:
     * an iterator to the first non erased element after @first, or end() if none exists.
     */
    constexpr auto insert (iterator pos, std::integral auto count, value_type const& init) //
      noexcept (false)                                                                     //
      requires (std::copyable<value_type>)                                                 //
    {
      emplace_back_many (count, init);
      std::ranges::rotate (pos, end () - count, end ());
      return pos;
    }

    /**
     * Inserts the range of elements [@first, @last) after @pos.
     *
     * Throws:
     * - std::logic_error if size() + std::ranges::distance(@first, @last) > StaticSize.
     * - exceptions thrown by value_type's constructor from the range's reference type.
     * - exceptions thrown by value_type's swap.
     * If std::logic_error is thrown by exceeding maximum capacity, this function has no effect.
     * Otherwise, if an exception is thrown the static_vector object is left in a valid state.
     *
     * Mandates:
     * - std::constructible_from<value_type, std::iter_reference_t<It>>
     * - std::swappable<value_type>
     *
     * Returns:
     * an iterator to the first inserted element, or @pos if none is inserted..
     */
    template<std::input_iterator It, std::sentinel_for<It> Sen>
      requires (std::constructible_from<value_type, std::iter_reference_t<It>> && std::swappable<value_type>) //
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

  // Deduce static_vector template parameters from cardinality of variadic
  // arguments.
  template<class Arg1, class... Args>
    requires ((std::common_with<Args, Arg1> && ...))
  static_vector (std::in_place_t, Arg1&&, Args&&...)
    -> static_vector<std::common_type_t<Arg1, Args...>, sizeof...(Args) + 1>;
} // namespace ml
