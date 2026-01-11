#ifndef LITEHTML_GRID_ITEM_H
#define LITEHTML_GRID_ITEM_H

#include "formatting_context.h"
#include "render_item.h"

namespace litehtml
{
	/**
	 * Represents an item in a CSS Grid container
	 */
	class grid_item
	{
	public:
		std::shared_ptr<render_item> el;

		// Grid placement (1-based line numbers, 0 = auto)
		int col_start;
		int col_end;
		int row_start;
		int row_end;

		// Resolved placement (0-based indices into track arrays)
		int resolved_col_start;
		int resolved_col_end;
		int resolved_row_start;
		int resolved_row_end;

		// Content sizing
		pixel_t min_content_width;
		pixel_t max_content_width;
		pixel_t min_content_height;
		pixel_t max_content_height;

		// Final position and size
		pixel_t pos_x;
		pixel_t pos_y;
		pixel_t width;
		pixel_t height;

		int order;
		int src_order;

		explicit grid_item(std::shared_ptr<render_item>& _el) :
			el(_el),
			col_start(0),
			col_end(0),
			row_start(0),
			row_end(0),
			resolved_col_start(0),
			resolved_col_end(1),
			resolved_row_start(0),
			resolved_row_end(1),
			min_content_width(0),
			max_content_width(0),
			min_content_height(0),
			max_content_height(0),
			pos_x(0),
			pos_y(0),
			width(0),
			height(0),
			order(0),
			src_order(0)
		{}

		bool operator<(const grid_item& b) const
		{
			if (order < b.order) return true;
			if (order == b.order) return src_order < b.src_order;
			return false;
		}

		// Initialize the item and compute content sizes
		void init(const containing_block_context& self_size, formatting_context* fmt_ctx);

		// Get the number of columns this item spans
		int column_span() const { return resolved_col_end - resolved_col_start; }

		// Get the number of rows this item spans
		int row_span() const { return resolved_row_end - resolved_row_start; }

		// Place the item at the specified position
		void place(pixel_t x, pixel_t y, pixel_t w, pixel_t h,
				   const containing_block_context& self_size, formatting_context* fmt_ctx);
	};
}

#endif //LITEHTML_GRID_ITEM_H
