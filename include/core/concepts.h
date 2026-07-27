#ifndef SYSCORE_CONCEPTS_H
#define SYSCORE_CONCEPTS_H

#include <concepts>
#include <string>
#include <type_traits>
#include <windows.h>

namespace SysCore::Concepts {

    // C++20 concept to verify handle trait validity for RAII handle wrappers
    template <typename Traits>
    concept HandleTraitsConcept = requires(HANDLE h) {
        { Traits::InvalidValue() } -> std::same_as<HANDLE>;
        { Traits::Close(h) } -> std::same_as<void>;
    };

    // C++20 concept for types that can be logged directly or formatted
    template <typename T>
    concept Loggable = requires(T a) {
        { std::to_string(a) } -> std::same_as<std::string>;
    } || std::convertible_to<T, std::string_view>;

    // C++20 concept for pointer types wrapped in RAII memory managers
    template <typename T>
    concept PointerType = std::is_pointer_v<T>;

} // namespace SysCore::Concepts

#endif // SYSCORE_CONCEPTS_H
