#ifndef LH_CSS_INTERPOLATION_H
#define LH_CSS_INTERPOLATION_H

#include "types.h"
#include "web_color.h"
#include "css_length.h"
#include <cmath>

namespace litehtml
{
	// Easing function types
	enum class easing_type
	{
		linear,
		ease,
		ease_in,
		ease_out,
		ease_in_out,
		cubic_bezier,
		step_start,
		step_end,
		steps
	};

	// Cubic bezier timing function
	struct cubic_bezier
	{
		float x1 = 0, y1 = 0, x2 = 1, y2 = 1;

		cubic_bezier() = default;
		cubic_bezier(float x1, float y1, float x2, float y2)
			: x1(x1), y1(y1), x2(x2), y2(y2) {}

		// Calculate output for input t (0-1)
		float calculate(float t) const;
	};

	// Easing function representation
	struct easing_function
	{
		easing_type type = easing_type::ease;
		cubic_bezier bezier;
		int steps = 1;
		bool step_start = false;

		easing_function() = default;
		easing_function(easing_type t) : type(t) {}

		// Parse timing function from string (e.g., "ease-in-out", "cubic-bezier(0.1, 0.7, 1.0, 0.1)")
		static easing_function parse(const string& str);

		// Apply easing to time t (0-1), returns progress (0-1)
		float apply(float t) const;
	};

	// Interpolation helpers
	namespace interpolate
	{
		// Linear interpolation
		template<typename T>
		T lerp(const T& a, const T& b, float t)
		{
			return a + (b - a) * t;
		}

		// Interpolate colors (in RGBA space)
		web_color color(const web_color& from, const web_color& to, float t);

		// Interpolate lengths
		css_length length(const css_length& from, const css_length& to, float t);

		// Interpolate float values
		inline float number(float from, float to, float t)
		{
			return from + (to - from) * t;
		}

		// Interpolate pixel values
		inline pixel_t pixels(pixel_t from, pixel_t to, float t)
		{
			return (pixel_t)(from + (to - from) * t);
		}
	}

	// Pre-defined easing curves
	namespace easing
	{
		inline float linear(float t) { return t; }
		float ease(float t);
		float ease_in(float t);
		float ease_out(float t);
		float ease_in_out(float t);
	}

} // namespace litehtml

#endif // LH_CSS_INTERPOLATION_H
