#ifndef LITEHTML_RENDER_GRID_H
#define LITEHTML_RENDER_GRID_H

#include "render_block.h"
#include "grid_item.h"
#include <vector>

namespace litehtml
{
	// Represents a grid track (column or row)
	struct grid_track
	{
		enum sizing_type
		{
			sizing_fixed,       // Fixed size in pixels
			sizing_percentage,  // Percentage of container
			sizing_fr,          // Fraction of remaining space
			sizing_auto,        // Size to content
			sizing_min_content, // Minimum content size
			sizing_max_content  // Maximum content size
		};

		sizing_type type = sizing_auto;
		float value = 0;        // px, %, or fr multiplier
		pixel_t base_size = 0;  // Computed size
		pixel_t position = 0;   // Cumulative position (start of track)
		pixel_t min_size = 0;   // Minimum size from content
		pixel_t max_size = 0;   // Maximum size from content
	};

	class render_item_grid : public render_item_block
	{
		std::vector<grid_track> m_columns;
		std::vector<grid_track> m_rows;
		std::vector<std::shared_ptr<grid_item>> m_items;
		pixel_t m_column_gap = 0;
		pixel_t m_row_gap = 0;

		// Parse track template string (e.g., "100px 1fr 2fr auto")
		void parse_track_template(const string& template_str, std::vector<grid_track>& tracks);

		// Parse a single track value (e.g., "1fr", "100px", "auto")
		grid_track parse_single_track(const string& token);

		// Parse minmax(min, max) function
		grid_track parse_minmax(const string& content);

		// Expand repeat(count, tracks) into individual tracks
		void expand_repeat(const string& content, std::vector<grid_track>& tracks);

		// Parse a track list (handles nested functions like repeat, minmax)
		void parse_track_list(const string& template_str, std::vector<grid_track>& tracks);

		// Size tracks based on content and available space
		void size_tracks(std::vector<grid_track>& tracks, pixel_t available_space, bool is_column);

		// Distribute remaining space to fr tracks
		void distribute_fr_space(std::vector<grid_track>& tracks, pixel_t free_space);

		// Calculate track positions
		void calculate_track_positions(std::vector<grid_track>& tracks, pixel_t gap, pixel_t start);

		// Place items into the grid
		void place_items();

		// Auto-place an item (find next available slot)
		void auto_place_item(grid_item& item);

		pixel_t _render_content(pixel_t x, pixel_t y, bool second_pass,
								const containing_block_context& self_size,
								formatting_context* fmt_ctx) override;

	public:
		explicit render_item_grid(std::shared_ptr<element> src_el) :
			render_item_block(std::move(src_el))
		{}

		std::shared_ptr<render_item> clone() override
		{
			return std::make_shared<render_item_grid>(src_el());
		}

		std::shared_ptr<render_item> init() override;
	};
}

#endif //LITEHTML_RENDER_GRID_H
