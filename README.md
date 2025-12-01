# format2
### A c++20 formatter that works for custom types having  operator &lt;&lt; and supports std manipulators

- You don’t need to write `std::formatter` for every (custom) type!
- Any type that works with `operator<<` can be formatted automatically.
- Supports C++ style manipulators we are used to like `std::hex`, `std::setw`, etc.
- %1, %2 … syntax works for manual (positional) indexing.
- %0 takes next unused parameter.
- Remaining parameters automatically append if no positional markers are used.
- Use of format string is optional. No need to use format string at all.
- Performance comparable to std::format, sometimes even faster.
- Only 225 lines of code.
