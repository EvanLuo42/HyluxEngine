/// @file
/// @brief Stand-in for `void` in templated async APIs. Future<T> requires a value type;
///        Unit fills that slot when there is nothing meaningful to carry. Use it whenever
///        you would write `Future<void>` if the language allowed it.

#pragma once

namespace Hylux
{

/// @brief Empty value carrier for "nothing to return" in Future / Promise chains.
struct Unit
{
};

} // namespace Hylux
