#ifndef TYPE_TRAITS_HPP
#define TYPE_TRAITS_HPP

#include <concepts>
#include <ostream>
#include <type_traits>

namespace cest {

template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream &>()
                                             << std::declval<const T &>())>>
    : std::true_type {};

template <typename T, typename = void> struct is_container : std::false_type {};

template <typename T>
struct is_container<T, std::void_t<typename T::value_type, typename T::iterator,
                                   decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end()),
                                   decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_container_v = is_container<T>::value;

template <typename T, typename MatcherTag>
struct has_matcher : std::false_type {};

template <typename T, typename MatcherTag>
concept HasMatcher = has_matcher<T, MatcherTag>::value;

} // namespace cest

#endif // TYPE_TRAITS_HPP