#include "grid_item.h"
#include "types.h"

void litehtml::grid_item::init(const litehtml::containing_block_context& self_size,
                                litehtml::formatting_context* fmt_ctx)
{
	if (!el) return;

	el->calc_outlines(self_size.render_width);
	order = el->css().get_order();

	// Get grid placement from CSS properties
	col_start = el->css().get_grid_column_start();
	col_end = el->css().get_grid_column_end();
	row_start = el->css().get_grid_row_start();
	row_end = el->css().get_grid_row_end();

	// Handle negative values (span N is stored as -N)
	// For now, resolve simple cases
	if (col_start > 0)
	{
		resolved_col_start = col_start - 1; // Convert to 0-based
	}
	else
	{
		resolved_col_start = 0; // Auto-placement will handle this
	}

	if (col_end > 0)
	{
		resolved_col_end = col_end - 1; // Convert to 0-based
	}
	else if (col_end < 0)
	{
		// Span: col_end = col_start + span
		resolved_col_end = resolved_col_start + (-col_end);
	}
	else
	{
		// Auto: span 1 from start
		resolved_col_end = resolved_col_start + 1;
	}

	if (row_start > 0)
	{
		resolved_row_start = row_start - 1; // Convert to 0-based
	}
	else
	{
		resolved_row_start = 0; // Auto-placement will handle this
	}

	if (row_end > 0)
	{
		resolved_row_end = row_end - 1; // Convert to 0-based
	}
	else if (row_end < 0)
	{
		// Span: row_end = row_start + span
		resolved_row_end = resolved_row_start + (-row_end);
	}
	else
	{
		// Auto: span 1 from start
		resolved_row_end = resolved_row_start + 1;
	}

	// Ensure valid ranges
	if (resolved_col_end <= resolved_col_start)
	{
		resolved_col_end = resolved_col_start + 1;
	}
	if (resolved_row_end <= resolved_row_start)
	{
		resolved_row_end = resolved_row_start + 1;
	}

	// Calculate content sizes by doing a test render
	// This is similar to how flexbox calculates base size
	containing_block_context child_ctx = self_size;
	child_ctx.width.type = containing_block_context::cbc_value_type_auto;
	child_ctx.height.type = containing_block_context::cbc_value_type_auto;

	// Render to get intrinsic size
	el->render(0, 0, child_ctx, fmt_ctx, false);

	min_content_width = el->width();
	max_content_width = el->width();
	min_content_height = el->height();
	max_content_height = el->height();
}

void litehtml::grid_item::place(pixel_t cell_x, pixel_t cell_y, pixel_t cell_w, pixel_t cell_h,
                                 const containing_block_context& self_size,
                                 formatting_context* fmt_ctx,
                                 flex_align_items container_justify_items,
                                 flex_align_items container_align_items)
{
	// Get item's self-alignment (overrides container's default)
	flex_align_items justify = el->src_el()->css().get_justify_self();
	flex_align_items align = el->src_el()->css().get_flex_align_self();

	// If auto, use container's value
	if (justify == flex_align_items_auto)
		justify = container_justify_items;
	if (align == flex_align_items_auto)
		align = container_align_items;

	// Check alignment for each axis independently
	bool width_is_auto = el->src_el()->css().get_width().is_predefined();
	bool height_is_auto = el->src_el()->css().get_height().is_predefined();

	bool shrink_width = (justify & 0xFF) != flex_align_items_stretch &&
	                    (justify & 0xFF) != flex_align_items_normal &&
	                    width_is_auto;
	bool shrink_height = (align & 0xFF) != flex_align_items_stretch &&
	                     (align & 0xFF) != flex_align_items_normal &&
	                     height_is_auto;

	pixel_t item_w = cell_w;
	pixel_t item_h = cell_h;

	if (shrink_width || shrink_height)
	{
		// Measure content size - use cell dimensions as max available space
		// size_mode_content makes the element shrink to fit its content
		pixel_t content_w = el->render(0, 0, self_size.new_width(cell_w,
			containing_block_context::size_mode_content), fmt_ctx, false);
		pixel_t content_h = el->height();

		// Apply content size only to axes that need it
		if (shrink_width)
		{
			item_w = content_w;
		}
		if (shrink_height)
		{
			item_h = content_h;
		}
	}

	// Calculate horizontal offset based on justify
	pixel_t offset_x = 0;
	switch (justify & 0xFF)  // Mask off overflow flags
	{
		case flex_align_items_center:
			offset_x = (cell_w - item_w) / 2;
			break;
		case flex_align_items_end:
		case flex_align_items_flex_end:
		case flex_align_items_self_end:
			offset_x = cell_w - item_w;
			break;
		case flex_align_items_start:
		case flex_align_items_flex_start:
		case flex_align_items_self_start:
		case flex_align_items_stretch:
		case flex_align_items_normal:
		default:
			offset_x = 0;
			break;
	}

	// Calculate vertical offset based on align
	pixel_t offset_y = 0;
	switch (align & 0xFF)  // Mask off overflow flags
	{
		case flex_align_items_center:
			offset_y = (cell_h - item_h) / 2;
			break;
		case flex_align_items_end:
		case flex_align_items_flex_end:
		case flex_align_items_self_end:
			offset_y = cell_h - item_h;
			break;
		case flex_align_items_start:
		case flex_align_items_flex_start:
		case flex_align_items_self_start:
		case flex_align_items_stretch:
		case flex_align_items_normal:
		default:
			offset_y = 0;
			break;
	}

	// Set final position and size
	pos_x = cell_x + offset_x;
	pos_y = cell_y + offset_y;
	width = item_w;
	height = item_h;

	// Final render with correct size
	// Use size_mode flags to force exact dimensions for stretch alignment
	int size_mode = 0;
	if ((justify & 0xFF) == flex_align_items_stretch ||
	    (justify & 0xFF) == flex_align_items_normal)
	{
		size_mode |= containing_block_context::size_mode_exact_width;
	}
	if ((align & 0xFF) == flex_align_items_stretch ||
	    (align & 0xFF) == flex_align_items_normal)
	{
		size_mode |= containing_block_context::size_mode_exact_height;
	}

	// Render at (0, 0) to establish size (like flexbox does)
	// For stretch: subtract content offset (cell size is border-box, need content-box)
	// For shrink-to-fit: content_w is already the correct border-box from measurement
	pixel_t render_w = shrink_width ? item_w : (item_w - el->content_offset_width());
	pixel_t render_h = shrink_height ? item_h : (item_h - el->content_offset_height());
	containing_block_context final_ctx = self_size.new_width_height(render_w, render_h, size_mode);
	el->render(0, 0, final_ctx, fmt_ctx, false);

	// Explicitly set position (like flexbox does via set_main_position/set_cross_position)
	el->pos().x = pos_x + el->content_offset_left();
	el->pos().y = pos_y + el->content_offset_top();
}
