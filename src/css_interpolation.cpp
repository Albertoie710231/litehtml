#include "html.h"
#include "css_interpolation.h"
#include <algorithm>

namespace litehtml
{

// Cubic bezier implementation
float cubic_bezier::calculate(float t) const
{
	// Simple implementation using Newton-Raphson method
	// For a proper implementation, see https://github.com/nicksay/cubic-bezier-talk/blob/main/demo.html

	// Handle edge cases
	if (t <= 0) return 0;
	if (t >= 1) return 1;

	// For linear (0,0,1,1), just return t
	if (x1 == 0 && y1 == 0 && x2 == 1 && y2 == 1)
		return t;

	// Find x value using Newton-Raphson (approximate solution for parametric t)
	float guess = t;
	for (int i = 0; i < 8; i++)
	{
		float x = 3 * (1 - guess) * (1 - guess) * guess * x1 +
		          3 * (1 - guess) * guess * guess * x2 +
		          guess * guess * guess;

		float dx = 3 * (1 - guess) * (1 - guess) * x1 +
		           6 * (1 - guess) * guess * (x2 - x1) +
		           3 * guess * guess * (1 - x2);

		if (std::abs(dx) < 1e-6) break;

		guess -= (x - t) / dx;
		guess = std::clamp(guess, 0.0f, 1.0f);
	}

	// Calculate y value at parameter guess
	return 3 * (1 - guess) * (1 - guess) * guess * y1 +
	       3 * (1 - guess) * guess * guess * y2 +
	       guess * guess * guess;
}

// Parse timing function from string
easing_function easing_function::parse(const string& str)
{
	easing_function func;
	string s = lowcase(str);

	if (s == "linear")
	{
		func.type = easing_type::linear;
	}
	else if (s == "ease")
	{
		func.type = easing_type::ease;
		func.bezier = cubic_bezier(0.25f, 0.1f, 0.25f, 1.0f);
	}
	else if (s == "ease-in")
	{
		func.type = easing_type::ease_in;
		func.bezier = cubic_bezier(0.42f, 0.0f, 1.0f, 1.0f);
	}
	else if (s == "ease-out")
	{
		func.type = easing_type::ease_out;
		func.bezier = cubic_bezier(0.0f, 0.0f, 0.58f, 1.0f);
	}
	else if (s == "ease-in-out")
	{
		func.type = easing_type::ease_in_out;
		func.bezier = cubic_bezier(0.42f, 0.0f, 0.58f, 1.0f);
	}
	else if (s == "step-start")
	{
		func.type = easing_type::step_start;
		func.steps = 1;
		func.step_start = true;
	}
	else if (s == "step-end")
	{
		func.type = easing_type::step_end;
		func.steps = 1;
		func.step_start = false;
	}
	else if (s.substr(0, 13) == "cubic-bezier(")
	{
		func.type = easing_type::cubic_bezier;
		// Parse cubic-bezier(x1, y1, x2, y2)
		size_t start = 13;
		size_t end = s.find(')');
		if (end != string::npos)
		{
			string params = s.substr(start, end - start);
			float values[4] = {0, 0, 1, 1};
			int idx = 0;
			size_t pos = 0;
			while (pos < params.size() && idx < 4)
			{
				size_t comma = params.find(',', pos);
				if (comma == string::npos) comma = params.size();
				string val = params.substr(pos, comma - pos);
				// Trim whitespace
				size_t vstart = val.find_first_not_of(" \t");
				size_t vend = val.find_last_not_of(" \t");
				if (vstart != string::npos && vend != string::npos)
				{
					values[idx++] = strtof(val.substr(vstart, vend - vstart + 1).c_str(), nullptr);
				}
				pos = comma + 1;
			}
			func.bezier = cubic_bezier(values[0], values[1], values[2], values[3]);
		}
	}
	else if (s.substr(0, 6) == "steps(")
	{
		func.type = easing_type::steps;
		// Parse steps(n, start|end)
		size_t start = 6;
		size_t end = s.find(')');
		if (end != string::npos)
		{
			string params = s.substr(start, end - start);
			size_t comma = params.find(',');
			if (comma != string::npos)
			{
				func.steps = atoi(params.substr(0, comma).c_str());
				string dir = params.substr(comma + 1);
				size_t dstart = dir.find_first_not_of(" \t");
				if (dstart != string::npos)
				{
					func.step_start = (dir.substr(dstart) == "start");
				}
			}
			else
			{
				func.steps = atoi(params.c_str());
			}
		}
	}

	return func;
}

// Apply easing function
float easing_function::apply(float t) const
{
	t = std::clamp(t, 0.0f, 1.0f);

	switch (type)
	{
	case easing_type::linear:
		return t;

	case easing_type::ease:
	case easing_type::ease_in:
	case easing_type::ease_out:
	case easing_type::ease_in_out:
	case easing_type::cubic_bezier:
		return bezier.calculate(t);

	case easing_type::step_start:
		return t > 0 ? 1.0f : 0.0f;

	case easing_type::step_end:
		return t >= 1 ? 1.0f : 0.0f;

	case easing_type::steps:
		if (steps <= 0) return t;
		if (step_start)
		{
			return std::ceil(t * steps) / steps;
		}
		else
		{
			return std::floor(t * steps) / steps;
		}
	}

	return t;
}

// Interpolate colors
web_color interpolate::color(const web_color& from, const web_color& to, float t)
{
	return web_color(
		(byte)(from.red + (to.red - from.red) * t),
		(byte)(from.green + (to.green - from.green) * t),
		(byte)(from.blue + (to.blue - from.blue) * t),
		(byte)(from.alpha + (to.alpha - from.alpha) * t)
	);
}

// Interpolate lengths
css_length interpolate::length(const css_length& from, const css_length& to, float t)
{
	// If both have same units, interpolate directly
	if (from.units() == to.units() && !from.is_predefined() && !to.is_predefined())
	{
		return css_length(from.val() + (to.val() - from.val()) * t, from.units());
	}
	// Otherwise, return from for t < 0.5, to for t >= 0.5
	return t < 0.5f ? from : to;
}

// Pre-defined easing curves
float easing::ease(float t)
{
	cubic_bezier b(0.25f, 0.1f, 0.25f, 1.0f);
	return b.calculate(t);
}

float easing::ease_in(float t)
{
	cubic_bezier b(0.42f, 0.0f, 1.0f, 1.0f);
	return b.calculate(t);
}

float easing::ease_out(float t)
{
	cubic_bezier b(0.0f, 0.0f, 0.58f, 1.0f);
	return b.calculate(t);
}

float easing::ease_in_out(float t)
{
	cubic_bezier b(0.42f, 0.0f, 0.58f, 1.0f);
	return b.calculate(t);
}

} // namespace litehtml
