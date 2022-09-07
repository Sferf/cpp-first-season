#include "DequeTests.hpp"
#include "TestLib.hpp"
#include "deque.h"

#include <algorithm>
#include <tuple>
#include <type_traits>
#include <vector>
#include <iterator>
#include <random>


namespace DequeTests {
    using Testing::make_simple_test;
    using Testing::make_pretty_test;
    using Testing::TestGroup;
    using groups_t = std::vector<TestGroup>;

    struct NotDefaultConstructible {
        NotDefaultConstructible() = delete;
        NotDefaultConstructible(int input): data(input) {}
        int data;

        auto operator<=>(const NotDefaultConstructible&) const = default;
    };

    struct CountedException : public std::exception {

    };

    template<int when_throw>
    struct Counted {
        inline static int counter = 0;

        Counted() {
            ++counter;
            if (counter == when_throw) {
                --counter;
                throw CountedException();
            }
        }

        Counted(const Counted&): Counted() { }

        ~Counted() {
            --counter;
        }
    };

    template<typename iter, typename T>
    struct CheckIter{
        using traits = std::iterator_traits<iter>;

        static_assert(std::is_same_v<std::remove_cv_t<typename traits::value_type>, std::remove_cv_t<T>>);
        static_assert(std::is_same_v<typename traits::pointer, T*>);
        static_assert(std::is_same_v<typename traits::reference, T&>);
        static_assert(std::is_same_v<typename traits::iterator_category, std::random_access_iterator_tag>);

        static_assert(std::is_same_v<decltype(std::declval<iter>()++), iter>);
        static_assert(std::is_same_v<decltype(++std::declval<iter>()), iter&>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() + 5), iter>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() += 5), iter&>);

        static_assert(std::is_same_v<decltype(std::declval<iter>() - std::declval<iter>()), typename traits::difference_type>);
        static_assert(std::is_same_v<decltype(*std::declval<iter>()), T&>);
        
        static_assert(std::is_same_v<decltype(std::declval<iter>() < std::declval<iter>()), bool>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() <= std::declval<iter>()), bool>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() > std::declval<iter>()), bool>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() >= std::declval<iter>()), bool>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() == std::declval<iter>()), bool>);
        static_assert(std::is_same_v<decltype(std::declval<iter>() != std::declval<iter>()), bool>);
    };

    TestGroup create_constructor_tests() {
        return { "constructors",
            make_pretty_test("default", [](auto& test){
                Deque<int> defaulted;
                test.check((defaulted.size() == 0));
                Deque<NotDefaultConstructible> without_default;
                test.check((without_default.size() == 0));
            }),
            make_pretty_test("copy", [](auto& test){
                Deque<NotDefaultConstructible> without_default;
                Deque<NotDefaultConstructible> copy = without_default;
                test.check((copy.size() == 0));
            }),
            make_pretty_test("with size", [](auto& test){
                int size = 17;
                int value = 14;
                Deque<int> simple(size);
                test.check((simple.size() == size_t(size)) && std::all_of(simple.begin(), simple.end(), [](int item){ return item == 0; }));
                Deque<NotDefaultConstructible> less_simple(size, value);
                test.check((less_simple.size() == size_t(size)) && std::all_of(less_simple.begin(), less_simple.end(), [&](const auto& item){ return item.data == value; }));
            }),
            make_pretty_test("assignment", [](auto& test){
                Deque<int> first(10, 10);
                Deque<int> second(9, 9);
                first = second;
                test.check((first.size() == second.size()) && (first.size() == 9) && std::equal(first.begin(), first.end(), second.begin()));
            }),
            make_simple_test("static asserts", []{
                using T1 = int;
                using T2 = NotDefaultConstructible;

                static_assert(std::is_default_constructible_v<Deque<T1>>, "should have default constructor");
                static_assert(std::is_default_constructible_v<Deque<T2>>, "should have default constructor");
                static_assert(std::is_copy_constructible_v<Deque<T1> >, "should have copy constructor");
                static_assert(std::is_copy_constructible_v<Deque<T2> >, "should have copy constructor");
                //static_assert(std::is_constructible_v<Deque<T1>, int>, "should have constructor from int");
                //static_assert(std::is_constructible_v<Deque<T2>, int>, "should have constructor from int");
                static_assert(std::is_constructible_v<Deque<T1>, int, const T1&>, "should have constructor from int and const T&");
                static_assert(std::is_constructible_v<Deque<T2>, int, const T2&>, "should have constructor from int and const T&");

                static_assert(std::is_copy_assignable_v<Deque<T1>>, "should have assignment operator");
                static_assert(std::is_copy_assignable_v<Deque<T2>>, "should have assignment operator");

                return true;       
            })
        };
    }

    TestGroup create_access_tests() {
        return { "access",
            make_pretty_test("operator[]", [](auto& test){
                Deque<size_t> defaulted(1300, 43);
                test.check((defaulted[0] == defaulted[1280]) && (defaulted[0] == 43));
                test.check((defaulted.at(0) == defaulted[1280]) && (defaulted.at(0) == 43));
                int caught = 0;

                try {
                    defaulted.at(size_t(-1));
                } catch (std::out_of_range& e) {
                    ++caught;
                }

                try {
                    defaulted.at(1300);
                } catch (std::out_of_range& e) {
                    ++caught;
                }

                test.check(caught == 2);
            }),
            make_simple_test("static asserts", []{
                Deque<size_t> defaulted;
                const Deque<size_t> constant;
                static_assert(std::is_same_v<decltype(defaulted[0]), size_t&>);
                static_assert(std::is_same_v<decltype(defaulted.at(0)), size_t&>);
                static_assert(std::is_same_v<decltype(constant[0]), const size_t&>);
                static_assert(std::is_same_v<decltype(constant.at(0)), const size_t&>);

                //static_assert(noexcept(defaulted[0]), "operator[] should not throw");
                static_assert(!noexcept(defaulted.at(0)), "at() can throw");

                return true;        
            })
        };
    }

    TestGroup create_iterator_tests() {
        return { "iterators",
            make_simple_test("static asserts", []{
                CheckIter<Deque<int>::iterator, int> iter;
                std::ignore = iter;
                CheckIter<decltype(std::declval<Deque<int>>().rbegin()), int> reverse_iter;
                std::ignore = reverse_iter;
                CheckIter<decltype(std::declval<Deque<int>>().cbegin()), const int> const_iter;
                std::ignore = const_iter;

                static_assert(std::is_convertible_v<
                        decltype(std::declval<Deque<int>>().begin()), 
                        decltype(std::declval<Deque<int>>().cbegin()) 
                        >, "should be able to construct const iterator from non const iterator");
                static_assert(!std::is_convertible_v<
                        decltype(std::declval<Deque<int>>().cbegin()), 
                        decltype(std::declval<Deque<int>>().begin()) 
                        >, "should NOT be able to construct iterator from const iterator");

                return true;
            }),
            make_pretty_test("arithmetic", [](auto& test){
                Deque<int> empty;
                test.check((empty.end() - empty.begin()) == 0);
                test.check((empty.begin() + 0 == empty.end()) && (empty.end() - 0 == empty.begin()));
                auto iter = empty.begin();
                test.check((iter++ == empty.begin()));
                Deque<int> one(1);
                auto iter2 = one.end();
                test.check(((--iter2) == one.begin()));

                test.check((empty.rend() - empty.rbegin()) == 0);
                test.check((empty.rbegin() + 0 == empty.rend()) && (empty.rend() - 0 == empty.rbegin()));
                auto r_iter = empty.rbegin();
                test.check((r_iter++ == empty.rbegin()));

                test.check((empty.cend() - empty.cbegin()) == 0);
                test.check((empty.cbegin() + 0 == empty.cend()) && (empty.cend() - 0 == empty.cbegin()));
                auto c_iter = empty.cbegin();
                test.check((c_iter++ == empty.cbegin()));

                Deque<int> d(1000, 3);
                test.check(size_t((d.end() - d.begin())) == d.size());
                test.check((d.begin() + d.size() == d.end()) && (d.end() - d.size() == d.begin()));
            }),
            make_pretty_test("comparison", [](auto& test){
                Deque<int> d(1000, 3);

                test.check(d.end() > d.begin());
                test.check(d.cend() > d.cbegin());
                test.check(d.rend() > d.rbegin());
                {

                  std::cout << (d.end() > d.begin());
                  std::cout << (d.cend() > d.cbegin());
                  std::cout << (d.rend() > d.rbegin());

                }
            }),
            make_pretty_test("algos", [](auto& test){
                Deque<int> d(1000, 3);

                std::iota(d.begin(), d.end(), 13);
                std::mt19937 g(31415);
                std::shuffle(d.begin(), d.end(), g);
                std::sort(d.rbegin(), d.rbegin() + 500);
                std::reverse(d.begin(), d.end());
                auto sorted_border = std::is_sorted_until(d.begin(), d.end());
                //std::copy(d.begin(), d.end(), std::ostream_iterator<int>(std::cout, " "));
                //std::cout << std::endl;
                test.check(sorted_border - d.begin() == 500);
            })
        };
    }

    TestGroup create_modification_tests() {
        return { "Modification",
            make_pretty_test("push and pop", [](auto& test){
                Deque<NotDefaultConstructible> d(10000, { 1 });
                auto start_size = d.size();
                
                auto middle = &(*(d.begin() + start_size / 2)); // 5000
                auto& middle_element = *middle;
                auto begin = &(*d.begin());
                auto end = &(*d.rbegin());

                auto middle2 = &(*((d.begin() + start_size / 2) + 2000)); // 7000
                
                // remove 400 elements 
                for (size_t i = 0; i < 400; ++i) {
                    d.pop_back();
                }
                
                // begin and middle pointers are still valid
                test.check(begin->data == 1);
                test.check(middle->data == 1);
                test.check(middle_element.data == 1);
                test.check(middle2->data == 1);

                end = &*d.rbegin();
                
                // 800 elemets removed in total
                for (size_t i = 0; i < 400; ++i) {
                    d.pop_front();
                }

                // and and middle iterators are still valid
                test.check(end->data == 1);
                test.check(middle->data == 1);
                test.check(middle_element.data == 1);
                test.check(middle2->data == 1);
                
                // removed 9980 items in total
                for (size_t i = 0; i < 4590; ++i) {
                    d.pop_front();
                    d.pop_back();
                }

                test.check(d.size() == 20);
                test.check(middle_element.data == 1);
                test.check(middle->data == 1 && middle->data == 1);
                test.check(std::all_of(d.begin(), d.end(), [](const auto& item) { return item.data == 1; } ));

                begin = &*d.begin();
                end = &*d.rbegin();

                for (size_t i = 0; i < 5500; ++i) {
                    d.push_back({2});
                    d.push_front({2});
                }

                test.check((*begin).data == 1);
                test.check((*end).data == 1);
                test.check(d.begin()->data == 2);
                test.check(d.size() == 5500 * 2 + 20);
                test.check(std::count(d.begin(), d.end(), NotDefaultConstructible{1}) == 20);
                test.check(std::count(d.begin(), d.end(), NotDefaultConstructible{2}) == 11000);
            }),
            make_pretty_test("insert and erase", [](auto& test){
                Deque<NotDefaultConstructible> d(10000, { 1 });
                auto start_size = d.size();
                
                d.insert(d.begin() + start_size / 2, NotDefaultConstructible{2});
                test.check(d.size() == start_size + 1);
                d.erase(d.begin() + start_size / 2 - 1);
                test.check(d.size() == start_size);

                test.check(size_t(std::count(d.begin(), d.end(), NotDefaultConstructible{1})) == start_size - 1);
                test.check(std::count(d.begin(), d.end(), NotDefaultConstructible{2}) == 1);

                Deque<NotDefaultConstructible> copy;
                for (const auto& item : d) {
                    copy.insert(copy.end(), item);
                }
                // std::copy(d.cbegin(), d.cend(), std::inserter(copy, copy.begin()));
                
                test.check(d.size() == copy.size());
                test.check(std::equal(d.begin(), d.end(), copy.begin()));
            }),
            make_pretty_test("exceptions", [](auto& test) {
                try {
                    Deque<Counted<17>> d(100);
                } catch (CountedException& e) {
                    test.check(Counted<17>::counter == 0);
                } catch (...) {
                    // should have caught same exception as thrown by Counted
                    test.fail();
                }
            })
        };
    }

    bool RunAll() {
        groups_t groups {};
        groups.push_back(create_constructor_tests());
        groups.push_back(create_access_tests());
        groups.push_back(create_iterator_tests());
        groups.push_back(create_modification_tests());

        bool res = true;
        for (auto& g : groups) {
            res &= g.run();
        }

        return res;
    }
}

