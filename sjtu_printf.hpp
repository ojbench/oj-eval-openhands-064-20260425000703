

#pragma once
#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <ostream>
#include <sstream>
#include <span>
#include <string_view>
#include <vector>
#include <type_traits>

namespace sjtu {

using sv_t = std::string_view;

struct format_error : std::exception {
public:
    format_error(const char *msg = "invalid format") : msg(msg) {}
    auto what() const noexcept -> const char * override {
        return msg;
    }

private:
    const char *msg;
};

template <typename Tp>
struct formatter;

struct format_info {
    inline static constexpr auto npos = static_cast<std::size_t>(-1);
    std::size_t position; // where is the specifier
    std::size_t consumed; // how many characters consumed
};

template <typename... Args>
struct format_string {
public:
    // must be constructed at compile time, to ensure the format string is valid
    consteval format_string(const char *fmt);
    constexpr auto get_format() const -> std::string_view {
        return fmt_str;
    }
    constexpr auto get_index() const -> std::span<const format_info> {
        return fmt_idx;
    }

private:
    inline static constexpr auto Nm = sizeof...(Args);
    std::string_view fmt_str;            // the format string
    std::array<format_info, Nm> fmt_idx; // where are the specifiers
};

consteval auto find_specifier(sv_t &fmt) -> bool {
    do {
        if (const auto next = fmt.find('%'); next == sv_t::npos) {
            return false;
        } else if (next + 1 == fmt.size()) {
            throw format_error{"missing specifier after '%'"};
        } else if (fmt[next + 1] == '%') {
            // escape the specifier
            fmt.remove_prefix(next + 2);
        } else {
            fmt.remove_prefix(next + 1);
            return true;
        }
    } while (true);
}

template <typename... Args>
consteval auto compile_time_format_check(sv_t fmt, std::span<format_info> idx) -> void {
    std::size_t n = 0;
    auto parse_fn = [&fmt, &n, idx](const auto &parser) {
        const auto last_pos = fmt.begin();
        if (!find_specifier(fmt)) {
            // no specifier found
            idx[n++] = {
                .position = format_info::npos,
                .consumed = 0,
            };
            return;
        }

        const auto position = static_cast<std::size_t>(fmt.begin() - last_pos - 1);
        const auto consumed = parser.parse(fmt);

        idx[n++] = {
            .position = position,
            .consumed = consumed,
        };

        if (consumed > 0) {
            fmt.remove_prefix(consumed);
        } else if (fmt.starts_with("_")) {
            fmt.remove_prefix(1);
        } else {
            throw format_error{"invalid specifier"};
        }
    };

    (parse_fn(formatter<Args>{}), ...);
    if (find_specifier(fmt)) // extra specifier found
        throw format_error{"too many specifiers"};
}

template <typename... Args>
consteval format_string<Args...>::format_string(const char *fmt) :
    fmt_str(fmt), fmt_idx() {
    compile_time_format_check<Args...>(fmt_str, fmt_idx);
}

template <typename StrLike>
    requires(
        std::same_as<StrLike, std::string> ||      //
        std::same_as<StrLike, std::string_view> || //
        std::same_as<StrLike, char *> ||           //
        std::same_as<StrLike, const char *>        //
    )
struct formatter<StrLike> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        return fmt.starts_with("s") ? 1 : 0;
    }
    static auto format_to(std::ostream &os, const StrLike &val, sv_t fmt = "s") -> void {
        if (fmt.starts_with("s")) {
            os << static_cast<sv_t>(val);
        } else {
            throw format_error{};
        }
    }
};

// Specialization for integer types
template <std::integral Tp>
struct formatter<Tp> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("d")) {
            return 1;
        } else if (fmt.starts_with("u")) {
            return 1;
        } else if (fmt.starts_with("_")) {
            return 1;
        }
        return 0;
    }
    
    static auto format_to(std::ostream &os, const Tp &val, sv_t fmt = "") -> void {
        if (fmt.starts_with("d")) {
            os << static_cast<int64_t>(val);
        } else if (fmt.starts_with("u")) {
            os << static_cast<uint64_t>(val);
        } else if (fmt.starts_with("_")) {
            if constexpr (std::is_signed_v<Tp>) {
                os << static_cast<int64_t>(val);
            } else {
                os << static_cast<uint64_t>(val);
            }
        } else {
            throw format_error{};
        }
    }
};

// Specialization for vector
template <typename Tp>
struct formatter<std::vector<Tp>> {
    static constexpr auto parse(sv_t fmt) -> std::size_t {
        if (fmt.starts_with("_")) {
            return 1;
        }
        return 0;
    }
    
    static auto format_to(std::ostream &os, const std::vector<Tp> &val, sv_t fmt = "") -> void {
        if (fmt.starts_with("_")) {
            os << "[";
            for (size_t i = 0; i < val.size(); ++i) {
                if (i > 0) {
                    os << ",";
                }
                formatter<Tp>::format_to(os, val[i], "_");
            }
            os << "]";
        } else {
            throw format_error{};
        }
    }
};

template <typename... Args>
using format_string_t = format_string<std::decay_t<Args>...>;

template <typename... Args>
inline auto printf(format_string_t<Args...> fmt, const Args &...args) -> void {
    std::string result;
    std::ostringstream oss;
    const auto format = fmt.get_format();
    const auto indices = fmt.get_index();
    
    size_t pos = 0;
    size_t arg_idx = 0;
    
    for (const auto &info : indices) {
        // Output text before the specifier
        if (info.position != format_info::npos) {
            oss << format.substr(pos, info.position - pos);
            pos = info.position + 1 + info.consumed;
        } else {
            // No more specifiers, output the rest
            oss << format.substr(pos);
            break;
        }
        
        // Format the argument
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            ((I == arg_idx ? 
                formatter<std::decay_t<Args>>::format_to(oss, args, 
                    format.substr(info.position + 1, info.consumed)) : 
                void()), ...);
        }(std::index_sequence_for<Args...>{});
        
        ++arg_idx;
    }
    
    // Output any remaining text
    if (pos < format.size()) {
        oss << format.substr(pos);
    }
    
    std::cout << oss.str();
}

} // namespace sjtu

