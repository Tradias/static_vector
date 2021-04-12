#include <ml/static_vector/static_vector.hpp>

#include <span>

namespace ml
{
  namespace tests
  {
    // Test deduction guides
    inline void test1 ()
    {
      // Deduction from a single c-array argument.
      constexpr int c_array[] {1, 2, 3};

      constexpr auto t1 = static_vector (c_array);
      static_assert (std::same_as<decltype (t1), static_vector<int, 3> const>);

      constexpr auto t2 = static_vector ({1.0f, 2.0f, 3.0f});
      static_assert (std::same_as<decltype (t2), static_vector<float, 3> const>);

      // Deduction from std::array argument
      constexpr auto std_array = std::array {1, 2, 3};
      constexpr auto t3 = static_vector (std_array);
      static_assert (std::same_as<decltype (t3), static_vector<int, 3> const>);

      constexpr auto t4 = static_vector (std::array {1, 2, 3});
      static_assert (std::same_as<decltype (t4), static_vector<int, 3> const>);

      // Deduction from variadic arguments.
      constexpr auto t5 = static_vector (std::in_place, 1, 2, 3);
      static_assert (std::same_as<decltype (t5), static_vector<int, 3> const>);

      constexpr auto t6 = static_vector (std::in_place, 1, 2, 3.0);
      static_assert (std::same_as<decltype (t6), static_vector<double, 3> const>);

      // narrowing conversions are unfortunately allowed.
      constexpr auto t7 = static_vector {std::in_place, 1u, 2, -1};
      static_assert (std::same_as<decltype (t7), static_vector<unsigned, 3> const>);
    }

    // Test constructors
    inline void test2 ()
    {
      // default constructor
      constexpr auto t1 = [] () { return static_vector<int, 5> (); }();
      static_assert (t1.empty ());

      // size constructor
      constexpr auto t2 = [] () { return static_vector<int, 5> (3); }();
      static_assert (t2 == static_vector ({0, 0, 0}));

      // size and initial value
      constexpr auto t3 = [] () { return static_vector<int, 5> (3, 1); }();
      static_assert (t3 == static_vector ({1, 1, 1}));

      // variadic constructor
      constexpr auto t4 = [] () { return static_vector<int, 5> (std::in_place, 1, 2); }();
      static_assert (t4.size () == 2);
      static_assert (t4 == static_vector ({1, 2}));

      // variadic constructor with fixed size
      constexpr auto t5 = [] () { return static_vector<int, 5> (4, std::in_place, 1, 2); }();
      static_assert (t5 == static_vector ({1, 2, 0, 0}));

      // iterator and sentinel constructor
      constexpr auto t6 = [] () {
        int array[] {1, 2, 3, 4};
        return static_vector<int, 5> (array, array + 2);
      }();
      static_assert (t6 == static_vector ({1, 2}));

      // range constructor
      constexpr auto t7 = [] () {
        int array[] {1, 2, 3};
        auto span = std::span<int> (array);
        return static_vector<int, 5> (span);
      }();
      static_assert (t7 == static_vector ({1, 2, 3}));

      // lvalue c-array constructor with fixed type
      constexpr auto t8 = [] () {
        int array[] {4, 5};
        return static_vector<int, 5> (array);
      }();
      static_assert (t8 == static_vector ({4, 5}));

      // rvalue c-array constructor with fixed type
      constexpr auto t9 = [] () { return static_vector<int, 5> ({3, 4}); }();
      static_assert (t9 == static_vector ({3, 4}));

      // lvalue c-array converting constructor
      constexpr auto t10 = [] () {
        double array[] {1, 2};
        return static_vector<int, 5> (array);
      }();
      static_assert (t10 == static_vector ({1, 2}));
    }

    // Test accessors
    inline void test3 ()
    {
      constexpr auto t1 = static_vector ({1, 2, 3, 4, 5});
      static_assert (t1.at (0) == 1);
      static_assert (t1.at (4) == 5);
      static_assert (t1[4] == 5);
      static_assert (t1.front () == 1);
      static_assert (t1.back () == 5);
      static_assert (!t1.empty ());
      static_assert (t1.size () == 5);
      static_assert (t1.ssize () == 5);

      constexpr auto t2 = [] () {
        auto result = static_vector ({1, 2, 3, 4, 5});
        result.clear ();
        return result;
      }();
      static_assert (t2.empty ());
    }

    // Test iterators
    inline void test4 ()
    {
      // begin works
      constexpr auto t1 = [] () {
        auto vector = static_vector<int, 5> ({1, 2, 3});
        auto first = vector.begin ();
        return *first;
      }();
      static_assert (t1 == 1);

      // prev(end()) works
      constexpr auto t2 = [] () {
        auto vector = static_vector<int, 5> ({1, 2, 3});
        return *std::prev (vector.end ());
      }();
      static_assert (t2 == 3);

      // increment works
      constexpr auto t3 = [] () {
        auto vector = static_vector<int, 5> ({1, 2, 3});
        auto it = vector.begin ();
        ++it;
        return *it;
      }();
      static_assert (t3 == 2);

      // iteration works
      constexpr auto t4 = [] () {
        int result = 0;
        auto vector = static_vector<int, 5> ({1, 2, 3});
        for (auto& x : vector)
          result += x;
        return result;
      }();
      static_assert (t4 == 6);

      // const iteration works
      constexpr auto t5 = [] () {
        int result = 0;
        auto vector = static_vector<int, 5> ({1, 2, 3});
        for (auto it = vector.cbegin (); it != vector.cend (); ++it)
          result += *it;
        return result;
      }();
      static_assert (t5 == 6);

      // TODO: reverse iteration works
      constexpr auto t6 = [] () {
        int result = 0;
        auto vector = static_vector<int, 5> ({1, 2, 3});
        for (auto it = vector.rbegin (); it != vector.rend (); ++it)
          result = result * result + *it;
        return result;
      }();
      // (((0^2)+3)^2+2)^2+1 = ((3^2)+2)^2)+1 = (11^2)+1 =
      static_assert (t6 == 122);
    }

    // Test modifiers
    inline void test5 ()
    {
      constexpr auto t1 = [] () {
        auto vector = static_vector<int, 5> ();
        vector.push_back (1);
        vector.push_back (2);
        vector.emplace_back (4);
        vector.pop_back ();
        vector.push_back (3);
        return vector;
      }();
      static_assert (t1 == static_vector {std::in_place, 1, 2, 3});

      constexpr auto t2 = [] () {
        auto vector = static_vector ({1, 2, 3, 4, 5});
        vector.pop_back ();
        vector.pop_back ();
        vector.pop_back ();
        return vector;
      }();
      static_assert (t2 == static_vector ({1, 2}));

      constexpr auto t3 = [] () {
        auto vector = static_vector ({1, 2, 3, 4, 5});
        vector.shrink_by (3);
        return vector;
      }();
      static_assert (t3.size () == 2);
      static_assert (t3 == static_vector ({1, 2}));

      constexpr auto t4 = [] () {
        auto vector = static_vector ({1, 2, 3, 4, 5});
        vector.shrink_to (2);
        return vector;
      }();
      static_assert (t4.size () == 2);
      static_assert (t4 == static_vector ({1, 2}));

      constexpr auto t5 = [] () {
        auto vector = static_vector<int, 7> ();
        vector.emplace_back_many (2, 1);
        vector.emplace_back_range ({1, 2});
        int array[] {3, 4};
        vector.emplace_back_range (array, array + 2);
        return vector;
      }();
      static_assert (t5.size () == 6);
      static_assert (t5 == static_vector ({1, 1, 1, 2, 3, 4}));

      constexpr auto t6 = [] () {
        auto vector = static_vector<int, 6> ({1, 2, 3});
        vector.resize (5, 100);
        return vector;
      }();
      static_assert (t6 == static_vector ({1, 2, 3, 100, 100}));

      constexpr auto t7 = [] () {
        auto vector = static_vector<int, 6> ({1, 2, 3});
        vector.resize (1);
        return vector;
      }();
      static_assert (t7 == static_vector ({1}));

      constexpr auto t8 = [] () {
        auto vector = static_vector<int, 8> ({1, 2, 3});
        vector.insert (vector.begin (), 100);
        vector.insert (vector.end (), 100);
        auto curr = vector.insert (vector.begin () + 2, 50);
        curr = vector.insert (curr, 4);
        vector.insert (curr, 5);
        return vector;
      }();
      static_assert (t8 == static_vector ({100, 1, 5, 4, 50, 2, 3, 100}));

      constexpr auto t9 = [] () {
        auto vector = static_vector<int, 6> ({1, 2, 3, 4, 5, 6});
        vector.erase (vector.begin () + 1, vector.begin () + 3);
        return vector;
      }();
      static_assert (t9 == static_vector ({1, 4, 5, 6}));
    }

    // Test copy/move assignment
    inline void test6 ()
    {
      constexpr auto t1 = [] () {
        auto result = static_vector ({1, 2, 3, 4, 5});
        result = static_vector ({1, 2, 3});
        return result;
      }();
      static_assert (t1 == static_vector ({1, 2, 3}));

      constexpr auto t2 = [] () {
        auto result = static_vector ({1, 2, 3, 4, 5});
        auto other = static_vector ({1, 2, 3});
        result = other;
        return result;
      }();
      static_assert (t2 == static_vector ({1, 2, 3}));
    }

    // Test three way comparison
    inline void test7 ()
    {
      static_assert (static_vector ({1, 2, 3}) < static_vector ({10}));
      static_assert (static_vector ({10}) > static_vector ({1, 2, 3}));
      static_assert (static_vector ({1, 2, 3}) < static_vector ({1, 2, 3, 1}));
    }

  } // namespace tests
} // namespace ml

int main ()
{}
