#include "types.h"
#include "render_grid.h"
#include "html_tag.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace litehtml
{

std::shared_ptr<render_item> render_item_grid::init()
{
	// Initialize like block, then collect grid items
	auto ret = render_item_block::init();

	// Grid items are direct children
	m_items.clear();
	int src_order = 0;
	for (auto& child : children())
	{
		// Skip absolutely positioned and fixed elements
		if (child->src_el()->css().get_position() == element_position_absolute ||
			child->src_el()->css().get_position() == element_position_fixed)
		{
			continue;
		}

		auto item = std::make_shared<grid_item>(child);
		item->src_order = src_order++;
		m_items.push_back(item);
	}

	return ret;
}

// Helper to parse a single track value (e.g., "1fr", "100px", "auto")
grid_track render_item_grid::parse_single_track(const string& token)
{
	grid_track track;

	if (token.empty())
	{
		track.type = grid_track::sizing_auto;
		return track;
	}

	// Check for "fr" unit
	size_t fr_pos = token.find("fr");
	if (fr_pos != string::npos)
	{
		track.type = grid_track::sizing_fr;
		string num = token.substr(0, fr_pos);
		track.value = num.empty() ? 1.0f : std::stof(num);
		return track;
	}

	// Check for percentage
	size_t pct_pos = token.find('%');
	if (pct_pos != string::npos)
	{
		track.type = grid_track::sizing_percentage;
		track.value = std::stof(token.substr(0, pct_pos));
		return track;
	}

	// Check for "auto"
	if (token == "auto")
	{
		track.type = grid_track::sizing_auto;
		return track;
	}

	// Check for "min-content"
	if (token == "min-content")
	{
		track.type = grid_track::sizing_min_content;
		return track;
	}

	// Check for "max-content"
	if (token == "max-content")
	{
		track.type = grid_track::sizing_max_content;
		return track;
	}

	// Try to parse as fixed length (px, em, rem, etc.)
	char* endptr;
	float val = std::strtof(token.c_str(), &endptr);
	if (endptr != token.c_str())
	{
		track.type = grid_track::sizing_fixed;
		track.value = val;
		return track;
	}

	// Default to auto
	track.type = grid_track::sizing_auto;
	return track;
}

// Parse minmax(min, max) - returns a track using the max value for flexible sizing
grid_track render_item_grid::parse_minmax(const string& content)
{
	// Find the comma separating min and max
	size_t comma_pos = content.find(',');
	if (comma_pos == string::npos)
	{
		return parse_single_track(content);
	}

	string min_str = content.substr(0, comma_pos);
	string max_str = content.substr(comma_pos + 1);

	// Trim whitespace
	while (!min_str.empty() && (min_str.front() == ' ' || min_str.front() == '\t')) min_str.erase(0, 1);
	while (!min_str.empty() && (min_str.back() == ' ' || min_str.back() == '\t')) min_str.pop_back();
	while (!max_str.empty() && (max_str.front() == ' ' || max_str.front() == '\t')) max_str.erase(0, 1);
	while (!max_str.empty() && (max_str.back() == ' ' || max_str.back() == '\t')) max_str.pop_back();

	// Parse min for the min_size constraint
	grid_track min_track = parse_single_track(min_str);
	// Parse max for the main sizing behavior
	grid_track max_track = parse_single_track(max_str);

	// Use max track type but store min value for constraints
	if (min_track.type == grid_track::sizing_fixed)
	{
		max_track.min_size = (pixel_t)min_track.value;
	}

	return max_track;
}

// Expand repeat(count, tracks) into individual tracks
void render_item_grid::expand_repeat(const string& content, std::vector<grid_track>& tracks, auto_repeat_info& auto_info)
{
	// Find the comma separating count and track list
	size_t comma_pos = content.find(',');
	if (comma_pos == string::npos)
	{
		return;
	}

	string count_str = content.substr(0, comma_pos);
	string track_list = content.substr(comma_pos + 1);

	// Trim whitespace
	while (!count_str.empty() && (count_str.front() == ' ' || count_str.front() == '\t')) count_str.erase(0, 1);
	while (!count_str.empty() && (count_str.back() == ' ' || count_str.back() == '\t')) count_str.pop_back();
	while (!track_list.empty() && (track_list.front() == ' ' || track_list.front() == '\t')) track_list.erase(0, 1);
	while (!track_list.empty() && (track_list.back() == ' ' || track_list.back() == '\t')) track_list.pop_back();

	// Parse the track list inside repeat() first
	std::vector<grid_track> repeated_tracks;
	auto_repeat_info dummy_info;  // Nested repeats shouldn't have auto-fill/auto-fit
	parse_track_list(track_list, repeated_tracks, dummy_info);

	// Handle auto-fill and auto-fit
	if (count_str == "auto-fill" || count_str == "auto-fit")
	{
		// Store auto-repeat info for later resolution during layout
		auto_info.is_auto_repeat = true;
		auto_info.is_auto_fit = (count_str == "auto-fit");
		auto_info.repeat_tracks = repeated_tracks;

		// Calculate minimum track size for auto-repeat calculation
		pixel_t min_size = 0;
		for (const auto& t : repeated_tracks)
		{
			if (t.type == grid_track::sizing_fixed)
			{
				min_size += (pixel_t)t.value;
			}
			else if (t.min_size > 0)
			{
				min_size += t.min_size;
			}
			else
			{
				// For auto/fr tracks, use a reasonable minimum
				min_size += 100;  // Default minimum for flexible tracks
			}
		}
		auto_info.min_track_size = min_size > 0 ? min_size : 100;

		// Add one placeholder track - will be replaced during resolve_auto_repeat
		for (const auto& t : repeated_tracks)
		{
			tracks.push_back(t);
		}
		return;
	}

	// Fixed repeat count
	int repeat_count = 1;
	try
	{
		repeat_count = std::stoi(count_str);
	}
	catch (...)
	{
		repeat_count = 1;
	}

	if (repeat_count <= 0 || repeat_count > 100)
	{
		repeat_count = 1; // Sanity check
	}

	// Add repeated tracks
	for (int i = 0; i < repeat_count; i++)
	{
		for (const auto& t : repeated_tracks)
		{
			tracks.push_back(t);
		}
	}
}

// Resolve auto-fill/auto-fit count based on available space
void render_item_grid::resolve_auto_repeat(std::vector<grid_track>& tracks, auto_repeat_info& auto_info, pixel_t available_space, pixel_t gap)
{
	if (!auto_info.is_auto_repeat || auto_info.repeat_tracks.empty())
	{
		return;
	}

	// Calculate how many repetitions fit
	// Formula: count = floor((available_space + gap) / (track_size + gap))
	pixel_t track_size = auto_info.min_track_size;
	pixel_t space_per_track = track_size + gap;

	int count = 1;
	if (space_per_track > 0)
	{
		count = (int)((available_space + gap) / space_per_track);
	}

	// Ensure at least 1 track
	if (count < 1) count = 1;
	// Reasonable upper limit
	if (count > 100) count = 100;

	// Clear existing tracks and add the resolved number
	tracks.clear();
	for (int i = 0; i < count; i++)
	{
		for (const auto& t : auto_info.repeat_tracks)
		{
			tracks.push_back(t);
		}
	}

	// For auto-fit, empty tracks will be collapsed during sizing
	// (This is handled by checking if any items span a track)
}

// Parse a track list (handles nested functions)
void render_item_grid::parse_track_list(const string& template_str, std::vector<grid_track>& tracks, auto_repeat_info& auto_info)
{
	if (template_str.empty() || template_str == "none")
	{
		return;
	}

	size_t pos = 0;
	size_t len = template_str.length();

	while (pos < len)
	{
		// Skip whitespace
		while (pos < len && (template_str[pos] == ' ' || template_str[pos] == '\t'))
		{
			pos++;
		}

		if (pos >= len) break;

		// Check for function (repeat, minmax, fit-content, etc.)
		size_t func_start = pos;
		while (pos < len && template_str[pos] != '(' && template_str[pos] != ' ' && template_str[pos] != '\t')
		{
			pos++;
		}

		string token = template_str.substr(func_start, pos - func_start);

		if (pos < len && template_str[pos] == '(')
		{
			// This is a function - find matching closing parenthesis
			pos++; // Skip '('
			int paren_depth = 1;
			size_t content_start = pos;

			while (pos < len && paren_depth > 0)
			{
				if (template_str[pos] == '(') paren_depth++;
				else if (template_str[pos] == ')') paren_depth--;
				pos++;
			}

			string content = template_str.substr(content_start, pos - content_start - 1);

			if (token == "repeat")
			{
				expand_repeat(content, tracks, auto_info);
			}
			else if (token == "minmax")
			{
				tracks.push_back(parse_minmax(content));
			}
			else if (token == "fit-content")
			{
				// fit-content(length) - treat as auto with max constraint
				grid_track track;
				track.type = grid_track::sizing_auto;
				grid_track limit = parse_single_track(content);
				if (limit.type == grid_track::sizing_fixed)
				{
					track.max_size = (pixel_t)limit.value;
				}
				tracks.push_back(track);
			}
			else
			{
				// Unknown function - treat as auto
				grid_track track;
				track.type = grid_track::sizing_auto;
				tracks.push_back(track);
			}
		}
		else if (!token.empty())
		{
			// Simple token
			tracks.push_back(parse_single_track(token));
		}
	}
}

void render_item_grid::parse_track_template(const string& template_str, std::vector<grid_track>& tracks)
{
	tracks.clear();
	auto_repeat_info dummy;
	parse_track_list(template_str, tracks, dummy);
}

void render_item_grid::size_tracks(std::vector<grid_track>& tracks, pixel_t available_space, bool is_column)
{
	if (tracks.empty())
	{
		return;
	}

	pixel_t used_space = 0;
	float total_fr = 0;

	// First pass: calculate fixed and percentage sizes, sum up fr values
	for (auto& track : tracks)
	{
		switch (track.type)
		{
			case grid_track::sizing_fixed:
				track.base_size = (pixel_t)track.value;
				used_space += track.base_size;
				break;

			case grid_track::sizing_percentage:
				track.base_size = (pixel_t)(available_space * track.value / 100.0f);
				used_space += track.base_size;
				break;

			case grid_track::sizing_auto:
			case grid_track::sizing_min_content:
			case grid_track::sizing_max_content:
				// Use content-based sizing from items
				track.base_size = std::max(track.min_size, (pixel_t)0);
				used_space += track.base_size;
				break;

			case grid_track::sizing_fr:
				total_fr += track.value;
				track.base_size = 0;
				break;
		}
	}

	// Distribute remaining space to fr tracks
	if (total_fr > 0)
	{
		pixel_t free_space = available_space - used_space;
		distribute_fr_space(tracks, free_space);
	}
}

void render_item_grid::distribute_fr_space(std::vector<grid_track>& tracks, pixel_t free_space)
{
	if (free_space <= 0)
	{
		return;
	}

	float total_fr = 0;
	for (const auto& track : tracks)
	{
		if (track.type == grid_track::sizing_fr)
		{
			total_fr += track.value;
		}
	}

	if (total_fr <= 0)
	{
		return;
	}

	pixel_t fr_unit = (pixel_t)(free_space / total_fr);

	for (auto& track : tracks)
	{
		if (track.type == grid_track::sizing_fr)
		{
			track.base_size = (pixel_t)(track.value * fr_unit);
		}
	}
}

void render_item_grid::calculate_track_positions(std::vector<grid_track>& tracks, pixel_t gap, pixel_t start)
{
	pixel_t pos = start;
	for (size_t i = 0; i < tracks.size(); i++)
	{
		tracks[i].position = pos;
		pos += tracks[i].base_size;
		if (i < tracks.size() - 1)
		{
			pos += gap;
		}
	}
}

void render_item_grid::place_items()
{
	int num_cols = (int)m_columns.size();
	if (num_cols == 0) num_cols = 1;

	// Estimate initial rows needed
	int num_rows = std::max((int)m_rows.size(), (int)m_items.size() / num_cols + 1);

	// Create occupancy grid - dynamically expandable by adding rows
	std::vector<std::vector<bool>> occupied(num_rows, std::vector<bool>(num_cols, false));

	// Lambda to expand grid if needed
	auto ensure_row = [&occupied, num_cols](int row) {
		while (row >= (int)occupied.size()) {
			occupied.push_back(std::vector<bool>(num_cols, false));
		}
	};

	// Lambda to check if a cell range is available
	auto is_available = [&occupied, &ensure_row, num_cols](int row_start, int row_end, int col_start, int col_end) -> bool {
		ensure_row(row_end - 1);
		for (int r = row_start; r < row_end; r++) {
			for (int c = col_start; c < col_end; c++) {
				if (c >= num_cols) return false;
				if (occupied[r][c]) return false;
			}
		}
		return true;
	};

	// Lambda to mark cells as occupied
	auto mark_occupied = [&occupied, &ensure_row](int row_start, int row_end, int col_start, int col_end) {
		ensure_row(row_end - 1);
		for (int r = row_start; r < row_end; r++) {
			for (int c = col_start; c < col_end; c++) {
				occupied[r][c] = true;
			}
		}
	};

	// First pass: mark cells occupied by explicitly placed items
	for (auto& item : m_items)
	{
		if (item->col_start > 0 || item->row_start > 0)
		{
			// Item has explicit placement (resolved in grid_item::init())
			mark_occupied(item->resolved_row_start, item->resolved_row_end,
			              item->resolved_col_start, item->resolved_col_end);
		}
	}

	// Second pass: auto-place remaining items
	int cursor_col = 0;
	int cursor_row = 0;

	for (auto& item : m_items)
	{
		// Skip explicitly placed items
		if (item->col_start > 0 || item->row_start > 0)
		{
			continue;
		}

		int span_cols = item->resolved_col_end - item->resolved_col_start;
		int span_rows = item->resolved_row_end - item->resolved_row_start;
		if (span_cols < 1) span_cols = 1;
		if (span_rows < 1) span_rows = 1;

		// Find next available slot that can fit the item
		bool placed = false;
		while (!placed)
		{
			if (cursor_col + span_cols <= num_cols &&
			    is_available(cursor_row, cursor_row + span_rows, cursor_col, cursor_col + span_cols))
			{
				// Found a slot
				item->resolved_col_start = cursor_col;
				item->resolved_col_end = cursor_col + span_cols;
				item->resolved_row_start = cursor_row;
				item->resolved_row_end = cursor_row + span_rows;

				mark_occupied(cursor_row, cursor_row + span_rows, cursor_col, cursor_col + span_cols);
				placed = true;

				// Advance cursor past this item
				cursor_col += span_cols;
			}
			else
			{
				// Move to next position
				cursor_col++;
				if (cursor_col >= num_cols)
				{
					cursor_col = 0;
					cursor_row++;
				}
			}

			// Safety: prevent infinite loop
			if (cursor_row > 1000) break;
		}
	}
}

void render_item_grid::auto_place_item(grid_item& item)
{
	// Simple auto-placement: find first empty cell
	// More sophisticated algorithms would handle spanning items
	// For now, just use cursor position
}

pixel_t render_item_grid::_render_content(pixel_t x, pixel_t y, bool /*second_pass*/,
										   const containing_block_context& self_size,
										   formatting_context* fmt_ctx)
{
	// Get gap values
	m_column_gap = (pixel_t)css().get_column_gap().val();
	m_row_gap = (pixel_t)css().get_row_gap().val();

	// Reset auto-repeat info
	m_col_auto_repeat = auto_repeat_info();
	m_row_auto_repeat = auto_repeat_info();

	// Parse track templates with auto-repeat info collection
	m_columns.clear();
	parse_track_list(css().get_grid_template_columns(), m_columns, m_col_auto_repeat);

	m_rows.clear();
	parse_track_list(css().get_grid_template_rows(), m_rows, m_row_auto_repeat);

	// Resolve auto-fill/auto-fit for columns based on available width
	pixel_t available_width = self_size.render_width;
	resolve_auto_repeat(m_columns, m_col_auto_repeat, available_width, m_column_gap);

	// If no columns defined, create one auto column
	if (m_columns.empty())
	{
		grid_track auto_col;
		auto_col.type = grid_track::sizing_auto;
		m_columns.push_back(auto_col);
	}

	// Ensure we have enough rows for all items
	int max_row = 1;
	for (const auto& item : m_items)
	{
		max_row = std::max(max_row, item->resolved_row_end);
	}
	while ((int)m_rows.size() < max_row)
	{
		grid_track auto_row;
		auto_row.type = grid_track::sizing_auto;
		m_rows.push_back(auto_row);
	}

	// Initialize items and get their content sizes
	for (auto& item : m_items)
	{
		item->init(self_size, fmt_ctx);

		// Contribute to track min sizes
		int col = item->resolved_col_start;
		int row = item->resolved_row_start;

		if (col >= 0 && col < (int)m_columns.size() && item->column_span() == 1)
		{
			m_columns[col].min_size = std::max(m_columns[col].min_size, item->min_content_width);
		}
		if (row >= 0 && row < (int)m_rows.size() && item->row_span() == 1)
		{
			m_rows[row].min_size = std::max(m_rows[row].min_size, item->min_content_height);
		}
	}

	// Place items (resolve auto-placement)
	place_items();

	// Size tracks (available_width already calculated above)
	pixel_t total_col_gap = m_columns.size() > 1 ? (m_columns.size() - 1) * m_column_gap : 0;
	size_tracks(m_columns, available_width - total_col_gap, true);

	// Calculate column positions
	calculate_track_positions(m_columns, m_column_gap, x);

	// Now size rows based on item heights after column sizing
	for (auto& item : m_items)
	{
		// Re-render with proper column width to get accurate height
		int col_start = item->resolved_col_start;
		int col_end = item->resolved_col_end;

		if (col_start >= 0 && col_end <= (int)m_columns.size())
		{
			pixel_t cell_width = 0;
			for (int c = col_start; c < col_end; c++)
			{
				cell_width += m_columns[c].base_size;
				if (c < col_end - 1) cell_width += m_column_gap;
			}

			containing_block_context cell_ctx = self_size;
			cell_ctx.width.value = cell_width;
			cell_ctx.width.type = containing_block_context::cbc_value_type_absolute;
			cell_ctx.height.type = containing_block_context::cbc_value_type_auto;

			item->el->render(0, 0, cell_ctx, fmt_ctx, false);
			item->min_content_height = item->el->height();
			item->max_content_height = item->el->height();

			int row = item->resolved_row_start;
			if (row >= 0 && row < (int)m_rows.size() && item->row_span() == 1)
			{
				m_rows[row].min_size = std::max(m_rows[row].min_size, item->min_content_height);
			}
		}
	}

	// Size rows - for now, just use content sizes
	pixel_t total_row_gap = m_rows.size() > 1 ? (m_rows.size() - 1) * m_row_gap : 0;
	for (auto& row : m_rows)
	{
		if (row.type == grid_track::sizing_auto ||
			row.type == grid_track::sizing_min_content ||
			row.type == grid_track::sizing_max_content)
		{
			row.base_size = row.min_size;
		}
		else if (row.type == grid_track::sizing_fixed)
		{
			row.base_size = (pixel_t)row.value;
		}
		else if (row.type == grid_track::sizing_percentage)
		{
			// Percentage heights need explicit container height
			row.base_size = row.min_size;
		}
		else if (row.type == grid_track::sizing_fr)
		{
			// Fr rows will be sized after we know total height
			row.base_size = row.min_size;
		}
	}

	// Calculate row positions
	calculate_track_positions(m_rows, m_row_gap, y);

	// Place each item in its grid cell
	for (auto& item : m_items)
	{
		int col_start = item->resolved_col_start;
		int col_end = std::min(item->resolved_col_end, (int)m_columns.size());
		int row_start = item->resolved_row_start;
		int row_end = std::min(item->resolved_row_end, (int)m_rows.size());

		if (col_start < 0 || row_start < 0 || col_start >= (int)m_columns.size() || row_start >= (int)m_rows.size())
		{
			continue;
		}

		pixel_t cell_x = m_columns[col_start].position;
		pixel_t cell_y = m_rows[row_start].position;

		pixel_t cell_width = 0;
		for (int c = col_start; c < col_end; c++)
		{
			cell_width += m_columns[c].base_size;
			if (c < col_end - 1) cell_width += m_column_gap;
		}

		pixel_t cell_height = 0;
		for (int r = row_start; r < row_end; r++)
		{
			cell_height += m_rows[r].base_size;
			if (r < row_end - 1) cell_height += m_row_gap;
		}

		item->place(cell_x, cell_y, cell_width, cell_height, self_size, fmt_ctx);
	}

	// Calculate total grid height
	pixel_t total_height = 0;
	if (!m_rows.empty())
	{
		total_height = m_rows.back().position + m_rows.back().base_size - y;
	}

	return total_height;
}

} // namespace litehtml
