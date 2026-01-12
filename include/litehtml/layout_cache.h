#ifndef LH_LAYOUT_CACHE_H
#define LH_LAYOUT_CACHE_H

#include "types.h"
#include <cstdint>

namespace litehtml
{

// Damage flags for incremental layout - indicates what needs to be recalculated
// Based on Servo's damage system and Chromium's LayoutNG constraint diffing
enum class damage_flags : uint32_t
{
	none            = 0x00,

	// Repaint only - visual properties changed (color, background, etc.)
	// No layout recalculation needed
	repaint         = 0x01,

	// Reflow self - this element's layout needs recalculation
	// Position/size may change but children don't need full relayout
	reflow_self     = 0x02,

	// Reflow children - children need to be laid out again
	// Parent's intrinsic size may change
	reflow_children = 0x04,

	// Reflow all - full subtree needs layout
	// Used when structure changes or inherited properties change
	reflow_all      = reflow_self | reflow_children,

	// Width changed - affects intrinsic width calculations
	width_changed   = 0x08,

	// Height changed - affects height calculations
	height_changed  = 0x10,

	// Position changed - element position changed
	position_changed = 0x20,

	// Content changed - text content or replaced content changed
	content_changed = 0x40,
};

inline damage_flags operator|(damage_flags a, damage_flags b)
{
	return static_cast<damage_flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline damage_flags operator&(damage_flags a, damage_flags b)
{
	return static_cast<damage_flags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline damage_flags& operator|=(damage_flags& a, damage_flags b)
{
	a = a | b;
	return a;
}

inline bool has_flag(damage_flags flags, damage_flags flag)
{
	return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// Block-level width cache for min/max content width calculations
// Inspired by LayoutNG's measure pass caching
struct block_width_cache
{
	// Cached intrinsic widths
	pixel_t min_content_width;
	pixel_t max_content_width;

	// The containing block width used for last calculation
	// (needed to invalidate percentage-based values)
	pixel_t cached_containing_width;

	// Cache validity flags
	bool min_content_valid;
	bool max_content_valid;

	// Generation counter for invalidation
	uint32_t generation;

	block_width_cache() :
		min_content_width(0),
		max_content_width(0),
		cached_containing_width(-1),
		min_content_valid(false),
		max_content_valid(false),
		generation(0)
	{}

	void invalidate()
	{
		min_content_valid = false;
		max_content_valid = false;
		generation++;
	}

	bool is_valid_for_width(pixel_t containing_width) const
	{
		// Cache is invalid if containing width changed (affects percentages)
		if (cached_containing_width != containing_width)
			return false;
		return min_content_valid && max_content_valid;
	}

	void set_min_content(pixel_t width, pixel_t containing_width)
	{
		min_content_width = width;
		cached_containing_width = containing_width;
		min_content_valid = true;
	}

	void set_max_content(pixel_t width, pixel_t containing_width)
	{
		max_content_width = width;
		cached_containing_width = containing_width;
		max_content_valid = true;
	}
};

// Layout result cache for LayoutNG-style constraint caching
// Caches the result of layout for a given set of constraints
struct layout_result_cache
{
	// Input constraints that produced this result
	pixel_t input_available_width;
	pixel_t input_available_height;
	uint32_t input_size_mode;

	// Cached output
	pixel_t output_width;
	pixel_t output_height;
	pixel_t output_min_width;

	// Validity
	bool valid;
	uint32_t generation;

	layout_result_cache() :
		input_available_width(-1),
		input_available_height(-1),
		input_size_mode(0),
		output_width(0),
		output_height(0),
		output_min_width(0),
		valid(false),
		generation(0)
	{}

	void invalidate()
	{
		valid = false;
		generation++;
	}

	bool matches(pixel_t available_width, pixel_t available_height, uint32_t size_mode) const
	{
		if (!valid) return false;
		// Exact match required for now - could add fuzzy matching later
		return input_available_width == available_width &&
		       input_available_height == available_height &&
		       input_size_mode == size_mode;
	}

	void store(pixel_t available_width, pixel_t available_height, uint32_t size_mode,
	           pixel_t width, pixel_t height, pixel_t min_width)
	{
		input_available_width = available_width;
		input_available_height = available_height;
		input_size_mode = size_mode;
		output_width = width;
		output_height = height;
		output_min_width = min_width;
		valid = true;
	}
};

// Global layout generation counter for cache invalidation
// Incremented on each full layout pass
class layout_generation
{
public:
	static uint32_t current() { return s_generation; }
	static void increment() { ++s_generation; }
	static void reset() { s_generation = 0; }

private:
	static inline uint32_t s_generation = 0;
};

// Layout cache statistics for profiling
struct layout_cache_stats
{
	static inline uint64_t layout_cache_hits = 0;
	static inline uint64_t layout_cache_misses = 0;
	static inline uint64_t width_cache_hits = 0;
	static inline uint64_t width_cache_misses = 0;

	static void reset()
	{
		layout_cache_hits = 0;
		layout_cache_misses = 0;
		width_cache_hits = 0;
		width_cache_misses = 0;
	}

	static void print_stats()
	{
		if (layout_cache_hits + layout_cache_misses > 0 || width_cache_hits + width_cache_misses > 0)
		{
			double layout_hit_rate = (layout_cache_hits + layout_cache_misses > 0)
				? 100.0 * layout_cache_hits / (layout_cache_hits + layout_cache_misses)
				: 0.0;
			double width_hit_rate = (width_cache_hits + width_cache_misses > 0)
				? 100.0 * width_cache_hits / (width_cache_hits + width_cache_misses)
				: 0.0;

			printf("[Layout Cache] Layout: %lu hits, %lu misses (%.1f%% hit rate)\n",
			       layout_cache_hits, layout_cache_misses, layout_hit_rate);
			printf("[Layout Cache] Width:  %lu hits, %lu misses (%.1f%% hit rate)\n",
			       width_cache_hits, width_cache_misses, width_hit_rate);
		}
	}
};

} // namespace litehtml

#endif // LH_LAYOUT_CACHE_H
