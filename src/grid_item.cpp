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

void litehtml::grid_item::place(pixel_t x, pixel_t y, pixel_t w, pixel_t h,
                                 const containing_block_context& self_size,
                                 formatting_context* fmt_ctx)
{
	pos_x = x;
	pos_y = y;
	width = w;
	height = h;

	// Create containing block context for the grid cell
	containing_block_context cell_ctx = self_size;
	cell_ctx.width.value = w;
	cell_ctx.width.type = containing_block_context::cbc_value_type_absolute;
	cell_ctx.height.value = h;
	cell_ctx.height.type = containing_block_context::cbc_value_type_absolute;

	// Render the element in its grid cell
	el->render(x, y, cell_ctx, fmt_ctx, false);

	// Set final position
	el->pos().x = x;
	el->pos().y = y;
}
