#ifndef IG_8bc3e50758094a5ea5f236c1beff5ede
#define IG_8bc3e50758094a5ea5f236c1beff5ede

#pragma once

#include <sstream>                      // std::stringstream
#include <iomanip>                      // manipulators
#include <array>
#include <charconv>                     // to_chars

namespace format2
{

namespace priv
{

    // In format string a single parameter as given by index.
    // Eg index = 3: replace %3 with p_to.
    // Or replace the first found %0 with p_to, but not when its a manipulator.
    // Note: format string cannot use %10, %11 etc, will then first replace %1.
    inline void DoParameterReplace(std::string& p_format, int p_num_params, const std::string* p_params)
    {
        bool positional_found = false;      // %1 %2 etc found
        int zindex = 0;                     // how many %0 found
        size_t pos = 0;
        while (zindex < p_num_params && !positional_found)
        {
            pos = p_format.find('%', pos);
            while (pos != std::string::npos && pos < (p_format.length() - 1))
            {
                auto idx_num = p_format[pos + 1] - '0';             // 0-9
                if (idx_num == 0 && zindex < p_num_params)          // %0 found
                {
                    p_format.replace(pos, 2, p_params[zindex]);
                    pos += p_params[zindex].length();
                    zindex++;
                }
                else if (idx_num > 0 && idx_num <= p_num_params)       // %1 - %9 found
                {
                    p_format.replace(pos, 2, p_params[idx_num - 1]);
                    pos += p_params[idx_num - 1].length();
                    positional_found = true;
                }
                else
                {
                    pos++;          // skip other usages of %
                }
                pos = p_format.find('%', pos);
            }

            // append remaining parameter, but not when %1-%9 found
            // (can cause hangups when remaining parameter also contains %1-%9)
            if (!positional_found && zindex < p_num_params)
            {
                pos = p_format.length();
                p_format += p_params[zindex];
                zindex++;
            }
        }
    }


    // A string/string_view or const char *
    template<typename T>
    concept StringLike = std::constructible_from<std::string_view, T> && !std::is_null_pointer<T>();


    template<typename T>
    concept IsManipulator =
        std::same_as<const T&, const decltype(std::hex)&> ||
        std::same_as<const T&, const decltype(std::dec)&> ||
        std::same_as<const T&, const decltype(std::oct)&> ||
        std::same_as<const T&, const decltype(std::setw(0))&> ||
        std::same_as<const T&, const decltype(std::setprecision(0))&> ||
        std::same_as<const T&, const decltype(std::setfill(0))&>  ||
        std::same_as<const T&, const decltype(std::setbase(0))&>;


    // Can it use std::to_chars?
    template <class T>
    concept CanUseToChars = requires(T v, char* c) { std::to_chars(c, c, v); } ;



    // It is a manipulator. Handle it plus make sure p_manip_found is true; do not increase index.
    template <class TData> requires(IsManipulator<TData>)
    void ToFmtString(std::stringstream& p_stream, std::string*, unsigned& , bool& p_manip_found, const TData& p_data)
    {
        p_stream.str("");       // makes result empty but keeps manipulators
        p_stream << p_data;
        p_manip_found = true;
    }


    // TData is already a string like. Can therefore move assign (when no manipulor was used).
    // Performance.
    template <class TData> requires(StringLike<TData>)
    inline void ToFmtString(std::stringstream& p_stream, std::string* p_to_what, unsigned& p_index, bool& p_manip_found, TData p_data)
    {
        if (!p_manip_found)
        {
            p_to_what[p_index] = std::string(std::move(p_data));
        }
        else
        {
            p_stream.str("");       // makes result empty but keeps manipulators
            p_stream << p_data;
            p_to_what[p_index] = p_stream.str();
        }
        p_index++;
    }




    // Can use to_chars on TData (when no manipulator was used).
    // Performance.
    // dont use to_chars on char -> will print as number.
    template <class TData> requires (!std::same_as<TData, char>) && CanUseToChars<TData>
    inline void ToFmtString(std::stringstream& p_stream, std::string* p_to_what, unsigned& p_index, bool& p_manip_found, const TData &p_data)
    {
        if (!p_manip_found)
        {
            char buffer[64];
            auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), p_data);
            *ptr = 0;       // zero-terminate to_chars doesn't do that
            p_to_what[p_index] = buffer;
            // OR: p_to_what[p_index] = std::string(buffer, ptr);
        }
        else
        {
            p_stream.str("");       // makes result empty but keeps manipulators
            p_stream << p_data;
            p_to_what[p_index] = p_stream.str();
        }
        p_index++;
    }

    // It is not a manipulator, not already a string-like and can not use to_chars.
    // Can only try streaming as last resort then.
    template <class TData> requires(
        !IsManipulator<TData> &&
        !StringLike<TData> &&
        (std::same_as<TData, char> || !CanUseToChars<TData>))
    void ToFmtString(std::stringstream& p_stream, std::string* p_to_what, unsigned& p_index, bool&, const TData& p_data)
    {
        p_stream.str("");       // makes result empty but keeps manipulators
        p_stream << p_data;
        p_to_what[p_index++] = p_stream.str();
    }


    namespace
    {
        thread_local std::stringstream ss;      // thread_local(=static) makes it twice as fast,
                                                // Note when inside function below is stored multiple times (template!) (avoid memory overhead)
        thread_local bool manip_found = false;  // keep together with stream
    }

}           // priv



template <typename ... TParams>
[[nodiscard]]
std::string Format(std::string p_format, TParams && ... p_params)
{
    unsigned index = 0;
    if(priv::manip_found)
    {
        priv::ss = std::stringstream{};       // need to reset manipulators (this is slow)
        priv::manip_found = false;
    }
    std::array<std::string, sizeof ... (p_params)> params;
    ((priv::ToFmtString(priv::ss, params.data(), index, priv::manip_found, std::forward<TParams>(p_params))), ...);

    priv::DoParameterReplace(p_format, index, params.data());
    return p_format;
}


template <typename ... TParams>
[[nodiscard]]
std::string Format(std::string_view p_format, TParams && ... p_params)
{
    return Format(std::string(p_format), std::forward<TParams>(p_params) ...);
}


template <typename ... TParams>
[[nodiscard]]
std::string Format(const char* p_format, TParams && ... p_params)
{
    return Format(std::string(p_format), std::forward<TParams>(p_params) ...);
}


template <typename ... TParams>
[[nodiscard]]
std::string Format(TParams && ... p_params)
{
    return Format("", std::forward<TParams>(p_params) ...);
}


[[nodiscard]]
inline std::string Format()
{
    return "";
}

}

#endif       // (include guard)
