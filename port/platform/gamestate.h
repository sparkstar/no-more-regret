/* The game's globals in the arena: each writable global of the game's
 * libraries is a reference to a place in the arena's part for them, which
 * gameGlobal gives as the program starts, so that the arena holds all of
 * the game's state and a copy of it is a snapshot (system/snapshot.h) on
 * every host. The template's argument is the type the global has; the code
 * that uses the global is as it was:
 *
 *     extern u16 &firstFreeItem;                            the header's
 *     u16 &firstFreeItem = gameGlobal<u16>();               zero
 *     int &speed = gameGlobal<int>(5);                      a constructor's arguments
 *     Rect &box = gameGlobal<Rect>({0, 0, 10, 10});         an aggregate's
 *     u8 (&table)[4] = gameGlobal<u8[4]>({1, 2, 3, 4});     the first elements, the rest zero
 *     char (&name)[80] = gameGlobal<char[80]>("usecode");   a string
 *
 * The places are given in the order the program's start binds the
 * references (a file's in the order of its definitions, the files' in no
 * order C++ sets: a global's initial value takes no other file's global),
 * zeroed first as static storage is; the objects are destroyed in the
 * reverse order at the program's end, as C++'s globals are. A constant is
 * no global of the arena: constexpr, or a table of functions that give the
 * places (savefields.h's SaveGlobal); bound to a const reference, a call
 * with a temporary is what GCC's -Wdangling-reference takes for one that
 * may dangle.
 *
 * A setting the player chose is no state of the game: gameSetting gives
 * its place outside the arena, in the settings' part, which a snapshot
 * does not hold, so that a rewind or a state's load keeps it. The same
 * forms as gameGlobal's:
 *
 *     i16 &frameSkip = gameSetting<i16>(0);
 *
 * A setting is bytes (trivially copyable): one that took the heap would
 * take it in the arena, and a snapshot put back would take back its blocks
 * and not it. */
#ifndef PORT_PLATFORM_GAMESTATE_H
#define PORT_PLATFORM_GAMESTATE_H
#include <cstddef>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

/* A global's place in the arena: bytes at alignment, zeroed; destroy (none
 * for an object without a destructor) at the program's end. At the
 * program's start only. */
[[nodiscard]] void *gameGlobalPlace(std::size_t bytes, std::size_t alignment, void (*destroy)(void *));
/* A setting's place in the settings' part, outside the arena: bytes at
 * alignment, zeroed. At the program's start only. */
[[nodiscard]] void *gameSettingPlace(std::size_t bytes, std::size_t alignment);

namespace gamestate {

/* The part a place is in: the arena's globals', or the settings'. */
enum class Part { GLOBALS, SETTINGS };

template <class T> void destroy(void *place)
{
    std::destroy_at(static_cast<T *>(place));
}

template <class T, Part part> void *placeFor()
{
    if constexpr (part == Part::SETTINGS) {
        static_assert(std::is_trivially_copyable_v<std::remove_all_extents_t<T>>,
                      "a setting is bytes: none of the heap, which is the arena's");
        return gameSettingPlace(sizeof(T), alignof(T));
    } else if constexpr (std::is_trivially_destructible_v<T>) {
        return gameGlobalPlace(sizeof(T), alignof(T), nullptr);
    } else {
        return gameGlobalPlace(sizeof(T), alignof(T), destroy<T>);
    }
}

/* The array of type T in place: each element value-initialized but the
 * first count, copies of initial's. */
template <class T> T &makeArray(void *place, const std::remove_extent_t<T> *initial, std::size_t count)
{
    using Item = std::remove_extent_t<T>;
    using Element = std::remove_all_extents_t<T>;
    if constexpr (std::is_trivially_copyable_v<Item> && std::is_trivially_default_constructible_v<Element>) {
        /* Value-initialized is zero, which the place is already. */
        if (count != 0) {
            std::memcpy(place, initial, count * sizeof(Item));
        }
    } else {
        static_assert(!std::is_array_v<Item>, "rows of a class type are not taken");
        Item *items = static_cast<Item *>(place);
        for (std::size_t i = 0; i < std::extent_v<T>; i++) {
            if (i < count) {
                ::new (static_cast<void *>(items + i)) Item(initial[i]);
            } else {
                ::new (static_cast<void *>(items + i)) Item();
            }
        }
    }
    return *std::launder(static_cast<T *>(place));
}

}

/* A global that is not an array: value-initialized, or made from the
 * arguments of one of its constructors. */
template <class T, class... Arguments>
    requires(!std::is_array_v<T>)
T &gameGlobal(Arguments &&...arguments)
{
    return *::new (gamestate::placeFor<T, gamestate::Part::GLOBALS>()) T(std::forward<Arguments>(arguments)...);
}

/* One that is not an array from a braced list (an aggregate's) or a value
 * of its type, taken by value: GCC's -Wdangling-reference takes a
 * temporary bound to a reference parameter for one the result may refer
 * to. */
template <class T>
    requires(!std::is_array_v<T>)
T &gameGlobal(T initial)
{
    return *::new (gamestate::placeFor<T, gamestate::Part::GLOBALS>()) T(std::move(initial));
}

/* An array, its elements value-initialized. */
template <class T>
    requires std::is_array_v<T>
T &gameGlobal()
{
    return gamestate::makeArray<T>(gamestate::placeFor<T, gamestate::Part::GLOBALS>(), nullptr, 0);
}

/* An array, its first elements from a braced list or a string, the rest
 * value-initialized. */
template <class T, std::size_t N>
    requires std::is_array_v<T>
T &gameGlobal(const std::remove_extent_t<T> (&initial)[N])
{
    static_assert(N <= std::extent_v<T>, "more initial elements than the array has");
    return gamestate::makeArray<T>(gamestate::placeFor<T, gamestate::Part::GLOBALS>(), initial, N);
}

/* The settings, the same four forms in the settings' part. */
template <class T, class... Arguments>
    requires(!std::is_array_v<T>)
T &gameSetting(Arguments &&...arguments)
{
    return *::new (gamestate::placeFor<T, gamestate::Part::SETTINGS>()) T(std::forward<Arguments>(arguments)...);
}

template <class T>
    requires(!std::is_array_v<T>)
T &gameSetting(T initial)
{
    return *::new (gamestate::placeFor<T, gamestate::Part::SETTINGS>()) T(std::move(initial));
}

template <class T>
    requires std::is_array_v<T>
T &gameSetting()
{
    return gamestate::makeArray<T>(gamestate::placeFor<T, gamestate::Part::SETTINGS>(), nullptr, 0);
}

template <class T, std::size_t N>
    requires std::is_array_v<T>
T &gameSetting(const std::remove_extent_t<T> (&initial)[N])
{
    static_assert(N <= std::extent_v<T>, "more initial elements than the array has");
    return gamestate::makeArray<T>(gamestate::placeFor<T, gamestate::Part::SETTINGS>(), initial, N);
}
#endif
