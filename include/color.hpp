#include <cstdint>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <limits>
#include <assert.h>
#include <type_traits>

struct color_t {
	std::uint8_t r;
	std::uint8_t g;
	std::uint8_t b;

};

template <
	class T,
	std::enable_if_t<std::is_integral_v<T>, int> = 0
>
color_t make_color(
	T r,
	T g,
	T b
)
{
	return color_t {
		static_cast<std::uint8_t>(r),
		static_cast<std::uint8_t>(g),
		static_cast<std::uint8_t>(b)
	};
}

template <
	class T,
	std::enable_if_t<std::is_floating_point_v<T>, int> = 0
>
color_t make_color(
	T r,
	T g,
	T b
)
{
	constexpr auto max = static_cast<double>(std::numeric_limits<std::uint8_t>::max());

	return color_t {
		static_cast<std::uint8_t>(max*std::clamp(r,0.0, 1.0)),
		static_cast<std::uint8_t>(max*std::clamp(g,0.0, 1.0)),
		static_cast<std::uint8_t>(max*std::clamp(b,0.0, 1.0))
	};
}

std::ostream& operator<<(std::ostream& lhs, const color_t& rhs) {
	lhs
		<< std::setw(2) << static_cast<unsigned int>(rhs.r)
		<< std::setw(2) << static_cast<unsigned int>(rhs.g)
		<< std::setw(2) << static_cast<unsigned int>(rhs.b);
	return lhs;
}

color_t fade(const color_t& begin, const color_t& end, double f) {
	f = std::clamp(f,0.0,1.0);
	return make_color(
		static_cast<std::uint8_t>(static_cast<double>(begin.r)*(1.0-f) + static_cast<double>(end.r)*f),
		static_cast<std::uint8_t>(static_cast<double>(begin.g)*(1.0-f) + static_cast<double>(end.g)*f),
		static_cast<std::uint8_t>(static_cast<double>(begin.b)*(1.0-f) + static_cast<double>(end.b)*f)
	);
}

color_t multi_fade(std::vector<color_t> colors, double f) {
	f = std::clamp(f,0.0,1.0);
	assert(colors.size() >= 2);
	const auto interval = 1.0/(colors.size()-1);
	const auto begin = static_cast<size_t>(f/interval);
	const auto end = begin + 1;
	f =
		(f - begin*interval)/interval;
	return fade(colors[begin],colors[end], f);
}
