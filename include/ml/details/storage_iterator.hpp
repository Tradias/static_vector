#pragma once
#include <ml/details/lazy_initialized_storage.hpp>

#include <iterator>


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
