#include "html.h"
#include "css_properties.h"
#include "css_transform.h"
#include <cmath>
#include <sstream>
#include "document.h"
#include "html_tag.h"
#include "document_container.h"
#include "types.h"

#define offset(member) ((uint_ptr)&this->member - (uint_ptr)this)
//#define offset(func)	[](const css_properties& css) { return css.func; }

void litehtml::css_properties::compute(const html_tag* el, const document::ptr& doc)
{
	m_color = el->get_property<web_color>(_color_, true, web_color::black, offset(m_color));
	m_accent_color = el->get_property<web_color>(_accent_color_, true, web_color(0x00, 0x66, 0xCC), offset(m_accent_color));  // Default blue accent
	m_caret_color = el->get_property<web_color>(_caret_color_, true, web_color::current_color, offset(m_caret_color));

	m_el_position	 = (element_position)	el->get_property<int>( _position_,		false,	element_position_static,	 offset(m_el_position));
	m_display		 = (style_display)		el->get_property<int>( _display_,			false,	display_inline,			 offset(m_display));
	m_visibility	 = (visibility)			el->get_property<int>( _visibility_,		true,	visibility_visible,		 offset(m_visibility));
	m_float			 = (element_float)		el->get_property<int>( _float_,			false,	float_none,				 offset(m_float));
	m_clear			 = (element_clear)		el->get_property<int>( _clear_,			false,	clear_none,				 offset(m_clear));
	m_appearance	 = (appearance)			el->get_property<int>( _appearance_,		false,	appearance_none,			 offset(m_appearance));
	m_box_sizing	 = (box_sizing)			el->get_property<int>( _box_sizing_,		false,	box_sizing_content_box,	 offset(m_box_sizing));
	m_overflow		 = (overflow)			el->get_property<int>( _overflow_,		false,	overflow_visible,		 offset(m_overflow));
	m_text_align	 = (text_align)			el->get_property<int>( _text_align_,		true,	text_align_left,			offset(m_text_align));
	m_vertical_align = (vertical_align)		el->get_property<int>( _vertical_align_,	false,	va_baseline,				 offset(m_vertical_align));
	m_text_transform = (text_transform)		el->get_property<int>( _text_transform_,	true,	text_transform_none,		 offset(m_text_transform));
	m_white_space	 = (white_space)		el->get_property<int>( _white_space_,		true,	white_space_normal,		 offset(m_white_space));
	m_caption_side	 = (caption_side)		el->get_property<int>( _caption_side_,	true,	caption_side_top,		 offset(m_caption_side));

	// https://www.w3.org/TR/CSS22/visuren.html#dis-pos-flo
	if (m_display == display_none)
	{
		// 1. If 'display' has the value 'none', then 'position' and 'float' do not apply. In this case, the element
		//    generates no box.
		m_float = float_none;
	} else
	{
		// 2. Otherwise, if 'position' has the value 'absolute' or 'fixed', the box is absolutely positioned,
		//    the computed value of 'float' is 'none', and display is set according to the table below.
		//    The position of the box will be determined by the 'top', 'right', 'bottom' and 'left' properties
		//    and the box's containing block.
		if (m_el_position == element_position_absolute || m_el_position == element_position_fixed)
		{
			m_float = float_none;

			if (m_display == display_inline_table)
			{
				m_display = display_table;
			} else if (m_display == display_inline ||
					   m_display == display_table_row_group ||
					   m_display == display_table_column ||
					   m_display == display_table_column_group ||
					   m_display == display_table_header_group ||
					   m_display == display_table_footer_group ||
					   m_display == display_table_row ||
					   m_display == display_table_cell ||
					   m_display == display_table_caption ||
					   m_display == display_inline_block)
			{
				m_display = display_block;
			}
		} else if (m_float != float_none)
		{
			// 3. Otherwise, if 'float' has a value other than 'none', the box is floated and 'display' is set
			//    according to the table below.
			if (m_display == display_inline_table)
			{
				m_display = display_table;
			} else if (m_display == display_inline ||
					   m_display == display_table_row_group ||
					   m_display == display_table_column ||
					   m_display == display_table_column_group ||
					   m_display == display_table_header_group ||
					   m_display == display_table_footer_group ||
					   m_display == display_table_row ||
					   m_display == display_table_cell ||
					   m_display == display_table_caption ||
					   m_display == display_inline_block)
			{
				m_display = display_block;
			}
		} else if(el->is_root())
		{
			// 4. Otherwise, if the element is the root element, 'display' is set according to the table below,
			//    except that it is undefined in CSS 2.2 whether a specified value of 'list-item' becomes a
			//    computed value of 'block' or 'list-item'.
			if (m_display == display_inline_table)
			{
				m_display = display_table;
			} else if (m_display == display_inline ||
				m_display == display_table_row_group ||
				m_display == display_table_column ||
				m_display == display_table_column_group ||
				m_display == display_table_header_group ||
				m_display == display_table_footer_group ||
				m_display == display_table_row ||
				m_display == display_table_cell ||
				m_display == display_table_caption ||
				m_display == display_inline_block ||
				m_display == display_list_item)
			{
				m_display = display_block;
			}
		} else if(el->is_replaced() && m_display == display_inline)
		{
			m_display = display_inline_block;
		}
	}
	// 5. Otherwise, the remaining 'display' property values apply as specified.

	compute_font(el, doc);
	pixel_t font_size = get_font_size();

	const css_length _auto = css_length::predef_value(0);
	const css_length none = _auto, normal = _auto;

	m_css_width      = el->get_property<css_length>(_width_,      false, _auto, offset(m_css_width));
	m_css_height     = el->get_property<css_length>(_height_,     false, _auto, offset(m_css_height));

	m_css_min_width  = el->get_property<css_length>(_min_width_,  false, _auto, offset(m_css_min_width));
	m_css_min_height = el->get_property<css_length>(_min_height_, false, _auto, offset(m_css_min_height));

	m_css_max_width  = el->get_property<css_length>(_max_width_,  false, none, offset(m_css_max_width));
	m_css_max_height = el->get_property<css_length>(_max_height_, false, none, offset(m_css_max_height));

	doc->cvt_units(m_css_width, m_font_metrics, 0);
	doc->cvt_units(m_css_height, m_font_metrics, 0);

	doc->cvt_units(m_css_min_width, m_font_metrics, 0);
	doc->cvt_units(m_css_min_height, m_font_metrics, 0);

	doc->cvt_units(m_css_max_width, m_font_metrics, 0);
	doc->cvt_units(m_css_max_height, m_font_metrics, 0);

	m_css_margins.left   = el->get_property<css_length>(_margin_left_,   false, 0, offset(m_css_margins.left));
	m_css_margins.right  = el->get_property<css_length>(_margin_right_,  false, 0, offset(m_css_margins.right));
	m_css_margins.top    = el->get_property<css_length>(_margin_top_,    false, 0, offset(m_css_margins.top));
	m_css_margins.bottom = el->get_property<css_length>(_margin_bottom_, false, 0, offset(m_css_margins.bottom));

	doc->cvt_units(m_css_margins.left,	 m_font_metrics, 0);
	doc->cvt_units(m_css_margins.right,	 m_font_metrics, 0);
	doc->cvt_units(m_css_margins.top,	 m_font_metrics, 0);
	doc->cvt_units(m_css_margins.bottom, m_font_metrics, 0);

	m_css_padding.left   = el->get_property<css_length>(_padding_left_,   false, 0, offset(m_css_padding.left));
	m_css_padding.right  = el->get_property<css_length>(_padding_right_,  false, 0, offset(m_css_padding.right));
	m_css_padding.top    = el->get_property<css_length>(_padding_top_,    false, 0, offset(m_css_padding.top));
	m_css_padding.bottom = el->get_property<css_length>(_padding_bottom_, false, 0, offset(m_css_padding.bottom));

	doc->cvt_units(m_css_padding.left,	 m_font_metrics, 0);
	doc->cvt_units(m_css_padding.right,	 m_font_metrics, 0);
	doc->cvt_units(m_css_padding.top,	 m_font_metrics, 0);
	doc->cvt_units(m_css_padding.bottom, m_font_metrics, 0);

	m_css_borders.left.color   = get_color_property(el, _border_left_color_,   false, m_color, offset(m_css_borders.left.color));
	m_css_borders.right.color  = get_color_property(el, _border_right_color_,  false, m_color, offset(m_css_borders.right.color));
	m_css_borders.top.color    = get_color_property(el, _border_top_color_,    false, m_color, offset(m_css_borders.top.color));
	m_css_borders.bottom.color = get_color_property(el, _border_bottom_color_, false, m_color, offset(m_css_borders.bottom.color));

	m_css_borders.left.style   = (border_style) el->get_property<int>(_border_left_style_,   false, border_style_none, offset(m_css_borders.left.style));
	m_css_borders.right.style  = (border_style) el->get_property<int>(_border_right_style_,  false, border_style_none, offset(m_css_borders.right.style));
	m_css_borders.top.style    = (border_style) el->get_property<int>(_border_top_style_,    false, border_style_none, offset(m_css_borders.top.style));
	m_css_borders.bottom.style = (border_style) el->get_property<int>(_border_bottom_style_, false, border_style_none, offset(m_css_borders.bottom.style));

	m_css_borders.left.width   = el->get_property<css_length>(_border_left_width_,   false, border_width_medium_value, offset(m_css_borders.left.width));
	m_css_borders.right.width  = el->get_property<css_length>(_border_right_width_,  false, border_width_medium_value, offset(m_css_borders.right.width));
	m_css_borders.top.width    = el->get_property<css_length>(_border_top_width_,    false, border_width_medium_value, offset(m_css_borders.top.width));
	m_css_borders.bottom.width = el->get_property<css_length>(_border_bottom_width_, false, border_width_medium_value, offset(m_css_borders.bottom.width));

	if (m_css_borders.left.style == border_style_none || m_css_borders.left.style == border_style_hidden)
		m_css_borders.left.width = 0;
	if (m_css_borders.right.style == border_style_none || m_css_borders.right.style == border_style_hidden)
		m_css_borders.right.width = 0;
	if (m_css_borders.top.style == border_style_none || m_css_borders.top.style == border_style_hidden)
		m_css_borders.top.width = 0;
	if (m_css_borders.bottom.style == border_style_none || m_css_borders.bottom.style == border_style_hidden)
		m_css_borders.bottom.width = 0;

	snap_border_width(m_css_borders.left.width,		doc);
	snap_border_width(m_css_borders.right.width,	doc);
	snap_border_width(m_css_borders.top.width,		doc);
	snap_border_width(m_css_borders.bottom.width,	doc);

	m_css_borders.radius.top_left_x = el->get_property<css_length>(_border_top_left_radius_x_, false, 0, offset(m_css_borders.radius.top_left_x));
	m_css_borders.radius.top_left_y = el->get_property<css_length>(_border_top_left_radius_y_, false, 0, offset(m_css_borders.radius.top_left_y));

	m_css_borders.radius.top_right_x = el->get_property<css_length>(_border_top_right_radius_x_, false, 0, offset(m_css_borders.radius.top_right_x));
	m_css_borders.radius.top_right_y = el->get_property<css_length>(_border_top_right_radius_y_, false, 0, offset(m_css_borders.radius.top_right_y));

	m_css_borders.radius.bottom_left_x = el->get_property<css_length>(_border_bottom_left_radius_x_, false, 0, offset(m_css_borders.radius.bottom_left_x));
	m_css_borders.radius.bottom_left_y = el->get_property<css_length>(_border_bottom_left_radius_y_, false, 0, offset(m_css_borders.radius.bottom_left_y));

	m_css_borders.radius.bottom_right_x = el->get_property<css_length>(_border_bottom_right_radius_x_, false, 0, offset(m_css_borders.radius.bottom_right_x));
	m_css_borders.radius.bottom_right_y = el->get_property<css_length>(_border_bottom_right_radius_y_, false, 0, offset(m_css_borders.radius.bottom_right_y));

	doc->cvt_units( m_css_borders.radius.top_left_x,			m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.top_left_y,			m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.top_right_x,			m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.top_right_y,			m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.bottom_left_x,		m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.bottom_left_y,		m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.bottom_right_x,		m_font_metrics, 0);
	doc->cvt_units( m_css_borders.radius.bottom_right_y,		m_font_metrics, 0);

	m_border_collapse = (border_collapse) el->get_property<int>(_border_collapse_, true, border_collapse_separate, offset(m_border_collapse));

	m_css_border_spacing_x = el->get_property<css_length>(__litehtml_border_spacing_x_, true, 0, offset(m_css_border_spacing_x));
	m_css_border_spacing_y = el->get_property<css_length>(__litehtml_border_spacing_y_, true, 0, offset(m_css_border_spacing_y));

	doc->cvt_units(m_css_border_spacing_x, m_font_metrics, 0);
	doc->cvt_units(m_css_border_spacing_y, m_font_metrics, 0);

	m_css_offsets.left	 = el->get_property<css_length>(_left_,	 false, _auto, offset(m_css_offsets.left));
	m_css_offsets.right  = el->get_property<css_length>(_right_, false, _auto, offset(m_css_offsets.right));
	m_css_offsets.top	 = el->get_property<css_length>(_top_,	 false, _auto, offset(m_css_offsets.top));
	m_css_offsets.bottom = el->get_property<css_length>(_bottom_,false, _auto, offset(m_css_offsets.bottom));

	doc->cvt_units(m_css_offsets.left,   m_font_metrics, 0);
	doc->cvt_units(m_css_offsets.right,  m_font_metrics, 0);
	doc->cvt_units(m_css_offsets.top,    m_font_metrics, 0);
	doc->cvt_units(m_css_offsets.bottom, m_font_metrics, 0);

	m_z_index = el->get_property<css_length>(_z_index_, false, _auto, offset(m_z_index));
	m_content = el->get_property<string>(_content_, false, "", offset(m_content));
	m_cursor = el->get_property<string>(_cursor_, true, "auto", offset(m_cursor));
	m_opacity = el->get_property<float>(_opacity_, false, 1.0f, offset(m_opacity));
	m_filter = el->get_property<string>(_filter_, false, "none", offset(m_filter));

	// CSS Transform
	m_transform_str = el->get_property<string>(_transform_, false, "none", offset(m_transform_str));
	if (!m_transform_str.empty() && m_transform_str != "none")
		m_transform_matrix = CSSTransform::parse(m_transform_str);
	else
		m_transform_matrix = TransformMatrix::identity();

	// Transform origin (default: 50% 50%)
	m_transform_origin_x = css_length(50, css_units_percentage);
	m_transform_origin_y = css_length(50, css_units_percentage);
	// TODO: Parse transform-origin string if provided

	// Parse box-shadow
	string box_shadow_str = el->get_property<string>(_box_shadow_, false, "", 0);
	m_box_shadows.clear();
	if (!box_shadow_str.empty() && box_shadow_str != "none")
	{
		// Simple parsing: split by comma for multiple shadows
		// Each shadow: [inset] offset-x offset-y [blur] [spread] [color]
		// For now, parse basic format: offset-x offset-y blur spread color
		size_t pos = 0;
		while (pos < box_shadow_str.size())
		{
			// Find next comma or end
			size_t comma = box_shadow_str.find(',', pos);
			if (comma == string::npos) comma = box_shadow_str.size();

			string shadow_part = box_shadow_str.substr(pos, comma - pos);

			// Trim whitespace
			size_t start = shadow_part.find_first_not_of(" \t");
			size_t end = shadow_part.find_last_not_of(" \t");
			if (start != string::npos && end != string::npos)
			{
				shadow_part = shadow_part.substr(start, end - start + 1);

				box_shadow shadow;
				bool inset = false;

				// Check for "inset" keyword
				if (shadow_part.find("inset") == 0)
				{
					inset = true;
					shadow_part = shadow_part.substr(5);
					start = shadow_part.find_first_not_of(" \t");
					if (start != string::npos)
						shadow_part = shadow_part.substr(start);
				}

				// Parse values: try to extract numbers and color
				// Simplified: expect "Xpx Ypx [blur] [spread] [color]"
				std::vector<float> numbers;
				web_color color(0, 0, 0, 128); // Default shadow color (semi-transparent black)

				std::istringstream iss(shadow_part);
				string token;
				while (iss >> token)
				{
					// Try to parse as number with units
					bool is_number = false;
					float val = 0;
					if (!token.empty())
					{
						char* endptr;
						val = strtof(token.c_str(), &endptr);
						if (endptr != token.c_str())
						{
							is_number = true;
							numbers.push_back(val);
						}
					}

					// If not a number, might be a color - handle basic formats
					if (!is_number && !token.empty())
					{
						// Handle hex colors (#rgb, #rrggbb, #rrggbbaa)
						if (token[0] == '#' && token.size() >= 4)
						{
							unsigned int r = 0, g = 0, b = 0, a = 255;
							if (token.size() == 4) // #rgb
							{
								sscanf(token.c_str(), "#%1x%1x%1x", &r, &g, &b);
								r *= 17; g *= 17; b *= 17;
							}
							else if (token.size() == 7) // #rrggbb
							{
								sscanf(token.c_str(), "#%2x%2x%2x", &r, &g, &b);
							}
							else if (token.size() == 9) // #rrggbbaa
							{
								sscanf(token.c_str(), "#%2x%2x%2x%2x", &r, &g, &b, &a);
							}
							color = web_color((byte)r, (byte)g, (byte)b, (byte)a);
						}
						// Handle basic named colors
						else if (token == "black") color = web_color(0, 0, 0, 255);
						else if (token == "white") color = web_color(255, 255, 255, 255);
						else if (token == "red") color = web_color(255, 0, 0, 255);
						else if (token == "green") color = web_color(0, 128, 0, 255);
						else if (token == "blue") color = web_color(0, 0, 255, 255);
						else if (token == "gray" || token == "grey") color = web_color(128, 128, 128, 255);
						else if (token == "transparent") color = web_color(0, 0, 0, 0);
						// Handle rgba() format - simplified
						else if (token.find("rgba(") == 0 || token.find("rgb(") == 0)
						{
							// For rgba, we'd need to handle multi-token parsing
							// Skip for now - use default color
						}
					}
				}

				// Assign parsed values
				if (numbers.size() >= 2)
				{
					shadow.offset_x = numbers[0];
					shadow.offset_y = numbers[1];
					if (numbers.size() >= 3) shadow.blur_radius = numbers[2];
					if (numbers.size() >= 4) shadow.spread_radius = numbers[3];
					shadow.color = color;
					shadow.inset = inset;
					m_box_shadows.push_back(shadow);
				}
			}

			pos = comma + 1;
		}
	}

	// Parse CSS Transitions
	// transition: <property> <duration> [<timing-function>] [<delay>]
	// Multiple transitions separated by commas
	string transition_str = el->get_property<string>(_transition_, false, "", 0);
	m_transitions.clear();
	if (!transition_str.empty() && transition_str != "none")
	{
		// Also check individual properties
		string prop_str = el->get_property<string>(_transition_property_, false, "", 0);
		string dur_str = el->get_property<string>(_transition_duration_, false, "", 0);
		string timing_str = el->get_property<string>(_transition_timing_function_, false, "", 0);
		string delay_str = el->get_property<string>(_transition_delay_, false, "", 0);

		// Parse shorthand transition string
		// Format: property duration timing-function delay, ...
		size_t pos = 0;
		while (pos < transition_str.size())
		{
			// Find next comma or end
			size_t comma = transition_str.find(',', pos);
			if (comma == string::npos) comma = transition_str.size();

			string part = transition_str.substr(pos, comma - pos);

			// Trim whitespace
			size_t start = part.find_first_not_of(" \t");
			size_t end = part.find_last_not_of(" \t");
			if (start != string::npos && end != string::npos)
			{
				part = part.substr(start, end - start + 1);

				transition_spec spec;

				// Parse tokens from part
				std::istringstream iss(part);
				string token;
				int token_idx = 0;
				while (iss >> token)
				{
					// Check for timing functions
					if (token == "ease") {
						spec.timing = transition_timing_ease;
					} else if (token == "linear") {
						spec.timing = transition_timing_linear;
					} else if (token == "ease-in") {
						spec.timing = transition_timing_ease_in;
					} else if (token == "ease-out") {
						spec.timing = transition_timing_ease_out;
					} else if (token == "ease-in-out") {
						spec.timing = transition_timing_ease_in_out;
					} else if (token == "step-start") {
						spec.timing = transition_timing_step_start;
					} else if (token == "step-end") {
						spec.timing = transition_timing_step_end;
					}
					// Check for time values (ends in 's' or 'ms')
					else if (token.size() > 1 && (token.back() == 's' || (token.size() > 2 && token.substr(token.size()-2) == "ms")))
					{
						float time_val = 0;
						bool is_ms = token.find("ms") != string::npos;
						char* endptr;
						time_val = strtof(token.c_str(), &endptr);
						if (!is_ms) time_val *= 1000.0f; // Convert seconds to ms

						// First time value is duration, second is delay
						if (spec.duration_ms == 0)
							spec.duration_ms = time_val;
						else
							spec.delay_ms = time_val;
					}
					// Property name (first non-keyword token)
					else if (token_idx == 0 || spec.property_name == "all")
					{
						spec.property_name = token;
					}
					token_idx++;
				}

				if (!spec.property_name.empty())
				{
					m_transitions.push_back(spec);
				}
			}

			pos = comma + 1;
		}
	}

	// Parse CSS Animations
	// animation: <name> <duration> [<timing-function>] [<delay>] [<iteration-count>] [<direction>] [<fill-mode>] [<play-state>]
	// Multiple animations separated by commas
	string animation_str = el->get_property<string>(_animation_, false, "", 0);
	m_animations.clear();
	if (!animation_str.empty() && animation_str != "none")
	{
		// Parse shorthand animation string
		size_t pos = 0;
		while (pos < animation_str.size())
		{
			// Find next comma or end
			size_t comma = animation_str.find(',', pos);
			if (comma == string::npos) comma = animation_str.size();

			string part = animation_str.substr(pos, comma - pos);

			// Trim whitespace
			size_t start = part.find_first_not_of(" \t");
			size_t end = part.find_last_not_of(" \t");
			if (start != string::npos && end != string::npos)
			{
				part = part.substr(start, end - start + 1);

				animation_spec spec;
				bool has_time = false;

				// Parse tokens from part
				std::istringstream iss(part);
				string token;
				while (iss >> token)
				{
					// Check for timing functions
					if (token == "ease") {
						spec.timing = transition_timing_ease;
					} else if (token == "linear") {
						spec.timing = transition_timing_linear;
					} else if (token == "ease-in") {
						spec.timing = transition_timing_ease_in;
					} else if (token == "ease-out") {
						spec.timing = transition_timing_ease_out;
					} else if (token == "ease-in-out") {
						spec.timing = transition_timing_ease_in_out;
					} else if (token == "step-start") {
						spec.timing = transition_timing_step_start;
					} else if (token == "step-end") {
						spec.timing = transition_timing_step_end;
					}
					// Check for time values (ends in 's' or 'ms')
					else if (token.size() > 1 && (token.back() == 's' || (token.size() > 2 && token.substr(token.size()-2) == "ms")))
					{
						float time_val = 0;
						bool is_ms = token.find("ms") != string::npos;
						char* endptr;
						time_val = strtof(token.c_str(), &endptr);
						if (!is_ms) time_val *= 1000.0f; // Convert seconds to ms

						// First time value is duration, second is delay
						if (!has_time) {
							spec.duration_ms = time_val;
							has_time = true;
						} else {
							spec.delay_ms = time_val;
						}
					}
					// Check for iteration count
					else if (token == "infinite") {
						spec.iteration_count = std::numeric_limits<float>::infinity();
					}
					// Check for direction
					else if (token == "normal") {
						spec.direction = animation_direction_normal;
					} else if (token == "reverse") {
						spec.direction = animation_direction_reverse;
					} else if (token == "alternate") {
						spec.direction = animation_direction_alternate;
					} else if (token == "alternate-reverse") {
						spec.direction = animation_direction_alternate_reverse;
					}
					// Check for fill-mode
					else if (token == "forwards") {
						spec.fill_mode = animation_fill_forwards;
					} else if (token == "backwards") {
						spec.fill_mode = animation_fill_backwards;
					} else if (token == "both") {
						spec.fill_mode = animation_fill_both;
					}
					// Check for play-state
					else if (token == "running") {
						spec.play_state = animation_play_running;
					} else if (token == "paused") {
						spec.play_state = animation_play_paused;
					}
					// Check if it's a number (iteration count)
					else if (!token.empty() && (isdigit(token[0]) || token[0] == '.'))
					{
						char* endptr;
						float val = strtof(token.c_str(), &endptr);
						if (*endptr == '\0') {
							spec.iteration_count = val;
						}
					}
					// Animation name (any unrecognized identifier)
					else if (spec.name.empty() && token != "none")
					{
						spec.name = token;
					}
				}

				if (!spec.name.empty())
				{
					m_animations.push_back(spec);
				}
			}

			pos = comma + 1;
		}
	}

	m_css_text_indent = el->get_property<css_length>(_text_indent_, true, 0, offset(m_css_text_indent));
	doc->cvt_units(m_css_text_indent, m_font_metrics, 0);

	// Letter spacing: normal (0) or <length>
	m_letter_spacing = el->get_property<css_length>(_letter_spacing_, true, css_length::predef_value(0), offset(m_letter_spacing));
	if (!m_letter_spacing.is_predefined())
		doc->cvt_units(m_letter_spacing, m_font_metrics, 0);

	// Word spacing: normal (0) or <length>
	m_word_spacing = el->get_property<css_length>(_word_spacing_, true, css_length::predef_value(0), offset(m_word_spacing));
	if (!m_word_spacing.is_predefined())
		doc->cvt_units(m_word_spacing, m_font_metrics, 0);

	// Text shadow parsing (not inherited - we parse the string ourselves)
	string text_shadow_str = el->get_property<string>(_text_shadow_, false, "", 0);
	m_text_shadows.clear();
	if (!text_shadow_str.empty() && text_shadow_str != "none")
	{
		// Parse text-shadow: offset-x offset-y [blur-radius] [color], ...
		size_t pos = 0;
		while (pos < text_shadow_str.size())
		{
			// Find next comma or end
			size_t comma = text_shadow_str.find(',', pos);
			if (comma == string::npos) comma = text_shadow_str.size();

			string shadow_part = text_shadow_str.substr(pos, comma - pos);

			// Trim whitespace
			size_t start = shadow_part.find_first_not_of(" \t");
			size_t end = shadow_part.find_last_not_of(" \t");
			if (start != string::npos && end != string::npos)
			{
				shadow_part = shadow_part.substr(start, end - start + 1);

				text_shadow shadow;
				std::vector<float> numbers;
				web_color color(0, 0, 0, 255); // Default shadow color (black)

				std::istringstream iss(shadow_part);
				string token;
				while (iss >> token)
				{
					// Try to parse as number with units
					bool is_number = false;
					float val = 0;
					if (!token.empty())
					{
						char* endptr;
						val = strtof(token.c_str(), &endptr);
						if (endptr != token.c_str())
						{
							is_number = true;
							numbers.push_back(val);
						}
					}

					// If not a number, might be a color
					if (!is_number && !token.empty())
					{
						if (token[0] == '#' && token.size() >= 4)
						{
							unsigned int r = 0, g = 0, b = 0, a = 255;
							if (token.size() == 4) // #rgb
							{
								sscanf(token.c_str(), "#%1x%1x%1x", &r, &g, &b);
								r *= 17; g *= 17; b *= 17;
							}
							else if (token.size() == 7) // #rrggbb
							{
								sscanf(token.c_str(), "#%2x%2x%2x", &r, &g, &b);
							}
							else if (token.size() == 9) // #rrggbbaa
							{
								sscanf(token.c_str(), "#%2x%2x%2x%2x", &r, &g, &b, &a);
							}
							color = web_color((byte)r, (byte)g, (byte)b, (byte)a);
						}
						else if (token == "black") color = web_color(0, 0, 0, 255);
						else if (token == "white") color = web_color(255, 255, 255, 255);
						else if (token == "red") color = web_color(255, 0, 0, 255);
						else if (token == "green") color = web_color(0, 128, 0, 255);
						else if (token == "blue") color = web_color(0, 0, 255, 255);
						else if (token == "gray" || token == "grey") color = web_color(128, 128, 128, 255);
						else if (token == "transparent") color = web_color(0, 0, 0, 0);
					}
				}

				// Assign parsed values (minimum 2 numbers for offset-x offset-y)
				if (numbers.size() >= 2)
				{
					shadow.offset_x = numbers[0];
					shadow.offset_y = numbers[1];
					if (numbers.size() >= 3) shadow.blur_radius = numbers[2];
					shadow.color = color;
					m_text_shadows.push_back(shadow);
				}
			}

			pos = comma + 1;
		}
	}

	m_line_height.css_value = el->get_property<css_length>(_line_height_, true, normal, offset(m_line_height.css_value));
	if(m_line_height.css_value.is_predefined())
	{
		m_line_height.computed_value = m_font_metrics.height;
	} else if(m_line_height.css_value.units() == css_units_none)
	{
		m_line_height.computed_value = (pixel_t) (m_line_height.css_value.val() * font_size);
	} else
	{
		m_line_height.computed_value = doc->to_pixels(m_line_height.css_value, m_font_metrics, m_font_metrics.font_size);
		m_line_height.css_value = (float) m_line_height.computed_value;
	}

	m_list_style_type     = (list_style_type)     el->get_property<int>(_list_style_type_,     true, list_style_type_disc,        offset(m_list_style_type));
	m_list_style_position = (list_style_position) el->get_property<int>(_list_style_position_, true, list_style_position_outside, offset(m_list_style_position));

	m_list_style_image = el->get_property<string>(_list_style_image_, true, "", offset(m_list_style_image));
	if (!m_list_style_image.empty())
	{
		m_list_style_image_baseurl = el->get_property<string>(_list_style_image_baseurl_, true, "", offset(m_list_style_image_baseurl));
		doc->container()->load_image(m_list_style_image.c_str(), m_list_style_image_baseurl.c_str(), true);
	}

	m_order = el->get_property<int>(_order_, false, 0, offset(m_order));

	compute_background(el, doc);
	compute_flex(el, doc);
	compute_grid(el, doc);
}

// used for all color properties except `color` (color:currentcolor is converted to color:inherit during parsing)
litehtml::web_color litehtml::css_properties::get_color_property(const html_tag* el, string_id name, bool inherited, web_color default_value, uint_ptr member_offset) const
{
	web_color color = el->get_property<web_color>(name, inherited, default_value, member_offset);
	if (color.is_current_color) color = m_color;
	return color;
}

static const litehtml::pixel_t font_size_table[8][7] =
{
		{ 9,    9,     9,     9,    11,    14,    18},
		{ 9,    9,     9,    10,    12,    15,    20},
		{ 9,    9,     9,    11,    13,    17,    22},
		{ 9,    9,    10,    12,    14,    18,    24},
		{ 9,    9,    10,    13,    16,    20,    26},
		{ 9,    9,    11,    14,    17,    21,    28},
		{ 9,   10,    12,    15,    17,    23,    30},
		{ 9,   10,    13,    16,    18,    24,    32}
};

void litehtml::css_properties::compute_font(const html_tag* el, const document::ptr& doc)
{
	// initialize font size
	css_length sz = el->get_property<css_length>(_font_size_, true, css_length::predef_value(font_size_medium), offset(m_font_size));

	pixel_t parent_sz = 0;
	pixel_t doc_font_size = doc->container()->get_default_font_size();
	element::ptr el_parent = el->parent();
	if (el_parent)
	{
		parent_sz = el_parent->css().get_font_size();
	} else
	{
		parent_sz = doc_font_size;
	}

	pixel_t font_size = parent_sz;

	if(sz.is_predefined())
	{
		int idx_in_table = round_f(doc_font_size - 9);
		if(idx_in_table >= 0 && idx_in_table <= 7)
		{
			if(sz.predef() >= font_size_xx_small && sz.predef() <= font_size_xx_large)
			{
				font_size = font_size_table[idx_in_table][sz.predef()];
			} else if(sz.predef() == font_size_smaller)
			{
				font_size = parent_sz / (pixel_t) 1.2;
			}  else if(sz.predef() == font_size_larger)
			{
				font_size = parent_sz * (pixel_t) 1.2;
			} else
			{
				font_size = parent_sz;
			}
		} else
		{
			switch(sz.predef())
			{
				case font_size_xx_small:
					font_size = doc_font_size * 3 / 5;
					break;
				case font_size_x_small:
					font_size = doc_font_size * 3 / 4;
					break;
				case font_size_small:
					font_size = doc_font_size * 8 / 9;
					break;
				case font_size_large:
					font_size = doc_font_size * 6 / 5;
					break;
				case font_size_x_large:
					font_size = doc_font_size * 3 / 2;
					break;
				case font_size_xx_large:
					font_size = doc_font_size * 2;
					break;
				case font_size_smaller:
					font_size = parent_sz / (pixel_t) 1.2;
					break;
				case font_size_larger:
					font_size = parent_sz * (pixel_t) 1.2;
					break;
				default:
					font_size = parent_sz;
					break;
			}
		}
	} else
	{
		if(sz.units() == css_units_percentage)
		{
			font_size = sz.calc_percent(parent_sz);
		} else
		{
			font_metrics fm;
			fm.x_height = fm.font_size = parent_sz;
			font_size = doc->to_pixels(sz, fm, 0);
		}
	}

	m_font_size = (float)font_size;

	// initialize font
	m_font_family		=              el->get_property<string>(    _font_family_,		true, doc->container()->get_default_font_name(),	offset(m_font_family));
	m_font_weight		=              el->get_property<css_length>(_font_weight_,		true, css_length::predef_value(font_weight_normal), offset(m_font_weight));
	m_font_style		= (font_style) el->get_property<int>(       _font_style_,		true, font_style_normal,							offset(m_font_style));
	bool propagate_decoration = !is_one_of(m_display, display_inline_block, display_inline_table, display_inline_flex) &&
								m_float == float_none && !is_one_of(m_el_position, element_position_absolute, element_position_fixed);

	m_text_decoration_line = el->get_property<int>(_text_decoration_line_, propagate_decoration, text_decoration_line_none, offset(m_text_decoration_line));

	// Merge parent text decoration with child text decoration
	if (propagate_decoration && el->parent())
	{
		m_text_decoration_line |= el->parent()->css().get_text_decoration_line();
	}

	if(m_text_decoration_line)
	{
		m_text_decoration_thickness = el->get_property<css_length>(_text_decoration_thickness_, propagate_decoration, css_length::predef_value(text_decoration_thickness_auto), offset(m_text_decoration_thickness));
		m_text_decoration_style = (text_decoration_style) el->get_property<int>(_text_decoration_style_, propagate_decoration, text_decoration_style_solid, offset(m_text_decoration_style));
		m_text_decoration_color = get_color_property(el, _text_decoration_color_, propagate_decoration, web_color::current_color, offset(m_text_decoration_color));
	} else
	{
		m_text_decoration_thickness = css_length::predef_value(text_decoration_thickness_auto);
		m_text_decoration_color = web_color::current_color;
	}

	// text-emphasis
	m_text_emphasis_style = el->get_property<string>(_text_emphasis_style_, true, "", offset(m_text_emphasis_style));
	m_text_emphasis_position = el->get_property<int>(_text_emphasis_position_, true, text_emphasis_position_over, offset(m_text_emphasis_position));
	m_text_emphasis_color = get_color_property(el, _text_emphasis_color_, true, web_color::current_color, offset(m_text_emphasis_color));

	if(el->parent())
	{
		if(m_text_emphasis_style.empty() || m_text_emphasis_style == "initial" || m_text_emphasis_style == "unset")
		{
			m_text_emphasis_style = el->parent()->css().get_text_emphasis_style();
		}
		if(m_text_emphasis_color == web_color::current_color)
		{
			m_text_emphasis_color = el->parent()->css().get_text_emphasis_color();
		}
		m_text_emphasis_position |= el->parent()->css().get_text_emphasis_position();
	}

	if(m_font_weight.is_predefined())
	{
		switch(m_font_weight.predef())
		{
			case font_weight_bold:
				m_font_weight = 700;
				break;
			case font_weight_bolder:
				{
					const int inherited = (int) el->parent()->css().m_font_weight.val();
					if(inherited < 400) m_font_weight = 400;
					else if(inherited >= 400 && inherited < 600) m_font_weight = 700;
					else m_font_weight = 900;
				}
				break;
			case font_weight_lighter:
				{
					const int inherited = (int) el->parent()->css().m_font_weight.val();
					if(inherited < 600) m_font_weight = 100;
					else if(inherited >= 600 && inherited < 800) m_font_weight = 400;
					else m_font_weight = 700;
				}
				break;
			default:
				m_font_weight = 400;
				break;
		}
	}

	font_description descr;
	descr.family 				= m_font_family;
	descr.size					= std::round(font_size);
	descr.style					= m_font_style;
	descr.weight				= (int) m_font_weight.val();
	descr.decoration_line		= m_text_decoration_line;
	descr.decoration_thickness	= m_text_decoration_thickness;
	descr.decoration_style		= m_text_decoration_style;
	descr.decoration_color		= m_text_decoration_color;
	descr.emphasis_style		= m_text_emphasis_style;
	descr.emphasis_color		= m_text_emphasis_color;
	descr.emphasis_position		= m_text_emphasis_position;
	// letter-spacing and word-spacing are set after compute() when we have all computed values
	descr.letter_spacing		= 0;
	descr.word_spacing			= 0;

	m_font = doc->get_font(descr, &m_font_metrics);
}

void litehtml::css_properties::compute_background(const html_tag* el, const document::ptr& doc)
{
	m_bg.m_color		= get_color_property(el, _background_color_, false, web_color::transparent, offset(m_bg.m_color));

	const css_size auto_auto(css_length::predef_value(background_size_auto), css_length::predef_value(background_size_auto));
	m_bg.m_position_x	= el->get_property<length_vector>(_background_position_x_, false, { css_length(0, css_units_percentage) }, offset(m_bg.m_position_x));
	m_bg.m_position_y	= el->get_property<length_vector>(_background_position_y_, false, { css_length(0, css_units_percentage) }, offset(m_bg.m_position_y));
	m_bg.m_size			= el->get_property<size_vector>  (_background_size_,       false, { auto_auto }, offset(m_bg.m_size));

	for (auto& x : m_bg.m_position_x) doc->cvt_units(x, m_font_metrics, 0);
	for (auto& y : m_bg.m_position_y) doc->cvt_units(y, m_font_metrics, 0);
	for (auto& size : m_bg.m_size)
	{
		doc->cvt_units(size.width,  m_font_metrics, 0);
		doc->cvt_units(size.height, m_font_metrics, 0);
	}

	m_bg.m_attachment = el->get_property<int_vector>(_background_attachment_, false, { background_attachment_scroll }, offset(m_bg.m_attachment));
	m_bg.m_repeat     = el->get_property<int_vector>(_background_repeat_,     false, { background_repeat_repeat },     offset(m_bg.m_repeat));
	m_bg.m_clip       = el->get_property<int_vector>(_background_clip_,       false, { background_box_border },        offset(m_bg.m_clip));
	m_bg.m_origin     = el->get_property<int_vector>(_background_origin_,     false, { background_box_padding },       offset(m_bg.m_origin));

	m_bg.m_image   = el->get_property<vector<image>>(_background_image_,  false, {{}}, offset(m_bg.m_image));
	m_bg.m_baseurl = el->get_property<string>(_background_image_baseurl_, false, "",   offset(m_bg.m_baseurl));

	for (auto& image : m_bg.m_image)
	{
		switch (image.type)
		{

			case image::type_none:
				break;
			case image::type_url:
				if (!image.url.empty())
				{
					doc->container()->load_image(image.url.c_str(), m_bg.m_baseurl.c_str(), true);
				}
				break;
			case image::type_gradient:
				for(auto& item : image.m_gradient.m_colors)
				{
					if (item.length)
						doc->cvt_units(*item.length, m_font_metrics, 0);
				}
				break;
		}
	}
}

void litehtml::css_properties::compute_flex(const html_tag* el, const document::ptr& doc)
{
	if (m_display == display_flex || m_display == display_inline_flex)
	{
		m_flex_direction = (flex_direction) el->get_property<int>(_flex_direction_, false, flex_direction_row, offset(m_flex_direction));
		m_flex_wrap = (flex_wrap) el->get_property<int>(_flex_wrap_, false, flex_wrap_nowrap, offset(m_flex_wrap));

		m_flex_justify_content = (flex_justify_content) el->get_property<int>(_justify_content_, false, flex_justify_content_flex_start, offset(m_flex_justify_content));
		m_flex_align_items = (flex_align_items) el->get_property<int>(_align_items_, false, flex_align_items_normal, offset(m_flex_align_items));
		m_flex_align_content = (flex_align_content) el->get_property<int>(_align_content_, false, flex_align_content_stretch, offset(m_flex_align_content));

		// Gap properties
		m_flex_row_gap = el->get_property<css_length>(_row_gap_, false, 0, offset(m_flex_row_gap));
		m_flex_column_gap = el->get_property<css_length>(_column_gap_, false, 0, offset(m_flex_column_gap));
		doc->cvt_units(m_flex_row_gap, m_font_metrics, 0);
		doc->cvt_units(m_flex_column_gap, m_font_metrics, 0);
	}
	m_flex_align_self = (flex_align_items) el->get_property<int>(_align_self_, false, flex_align_items_auto, offset(m_flex_align_self));
	auto parent = el->parent();
	if (parent && (parent->css().m_display == display_flex || parent->css().m_display == display_inline_flex))
	{
		m_flex_grow = el->get_property<float>(_flex_grow_, false, 0, offset(m_flex_grow));
		m_flex_shrink = el->get_property<float>(_flex_shrink_, false, 1, offset(m_flex_shrink));
		m_flex_basis = el->get_property<css_length>(_flex_basis_, false, css_length::predef_value(flex_basis_auto), offset(m_flex_basis));
		if(!m_flex_basis.is_predefined() && m_flex_basis.units() == css_units_none && m_flex_basis.val() != 0)
		{
			// flex-basis property must contain units
			m_flex_basis.predef(flex_basis_auto);
		}
		doc->cvt_units(m_flex_basis, m_font_metrics, 0);
		if(m_display == display_inline || m_display == display_inline_block)
		{
			m_display = display_block;
		} else if(m_display == display_inline_table)
		{
			m_display = display_table;
		} else if(m_display == display_inline_flex)
		{
			m_display = display_flex;
		}
	}
}

void litehtml::css_properties::compute_grid(const html_tag* el, const document::ptr& doc)
{
	if (m_display == display_grid || m_display == display_inline_grid)
	{
		// Grid container properties
		m_grid_template_columns = el->get_property<string>(_grid_template_columns_, false, "", offset(m_grid_template_columns));
		m_grid_template_rows = el->get_property<string>(_grid_template_rows_, false, "", offset(m_grid_template_rows));

		// Gap properties (reuse flex gap - they're the same CSS properties)
		m_flex_row_gap = el->get_property<css_length>(_row_gap_, false, 0, offset(m_flex_row_gap));
		m_flex_column_gap = el->get_property<css_length>(_column_gap_, false, 0, offset(m_flex_column_gap));
		doc->cvt_units(m_flex_row_gap, m_font_metrics, 0);
		doc->cvt_units(m_flex_column_gap, m_font_metrics, 0);

		// Grid alignment properties (container)
		m_justify_items = (flex_align_items) el->get_property<int>(_justify_items_, false, flex_align_items_stretch, offset(m_justify_items));
		m_flex_align_items = (flex_align_items) el->get_property<int>(_align_items_, false, flex_align_items_stretch, offset(m_flex_align_items));
	}

	// Grid item properties (apply to children of grid containers)
	auto parent = el->parent();
	if (parent && (parent->css().m_display == display_grid || parent->css().m_display == display_inline_grid))
	{
		m_grid_column_start = el->get_property<int>(_grid_column_start_, false, 0, offset(m_grid_column_start));
		m_grid_column_end = el->get_property<int>(_grid_column_end_, false, 0, offset(m_grid_column_end));
		m_grid_row_start = el->get_property<int>(_grid_row_start_, false, 0, offset(m_grid_row_start));
		m_grid_row_end = el->get_property<int>(_grid_row_end_, false, 0, offset(m_grid_row_end));

		// Grid item alignment
		m_justify_self = (flex_align_items) el->get_property<int>(_justify_self_, false, flex_align_items_auto, offset(m_justify_self));

		// Blockify grid items (similar to flex items)
		if(m_display == display_inline || m_display == display_inline_block)
		{
			m_display = display_block;
		} else if(m_display == display_inline_table)
		{
			m_display = display_table;
		} else if(m_display == display_inline_flex)
		{
			m_display = display_flex;
		} else if(m_display == display_inline_grid)
		{
			m_display = display_grid;
		}
	}
}

// https://www.w3.org/TR/css-values-4/#snap-a-length-as-a-border-width
void litehtml::css_properties::snap_border_width(css_length& width, const std::shared_ptr<document>& doc)
{
	if (width.is_predefined() || width.units() == css_units_percentage)
	{
		return;
	}

	pixel_t px = doc->to_pixels(width, m_font_metrics, 0);
	
	if (px > 0 && px < 1)
	{
		px = 1;
	} else
	{
		px = std::floor(px);
	}

	width.set_value(px, css_units_px);
}

std::vector<std::tuple<litehtml::string, litehtml::string>> litehtml::css_properties::dump_get_attrs()
{
	std::vector<std::tuple<string, string>> ret;

	ret.emplace_back("display", index_value(m_display, style_display_strings));
	ret.emplace_back("el_position", index_value(m_el_position, element_position_strings));
	ret.emplace_back("text_align", index_value(m_text_align, text_align_strings));
	ret.emplace_back("font_size", m_font_size.to_string());
	ret.emplace_back("overflow", index_value(m_overflow, overflow_strings));
	ret.emplace_back("white_space", index_value(m_white_space, white_space_strings));
	ret.emplace_back("visibility", index_value(m_visibility, visibility_strings));
	ret.emplace_back("appearance", index_value(m_appearance, appearance_strings));
	ret.emplace_back("box_sizing", index_value(m_box_sizing, box_sizing_strings));
	ret.emplace_back("z_index", m_z_index.to_string());
	ret.emplace_back("vertical_align", index_value(m_vertical_align, vertical_align_strings));
	ret.emplace_back("float", index_value(m_float, element_float_strings));
	ret.emplace_back("clear", index_value(m_clear, element_clear_strings));
	ret.emplace_back("margins", m_css_margins.to_string());
	ret.emplace_back("padding", m_css_padding.to_string());
	ret.emplace_back("borders", m_css_borders.to_string());
	ret.emplace_back("width", m_css_width.to_string());
	ret.emplace_back("height", m_css_height.to_string());
	ret.emplace_back("min_width", m_css_min_width.to_string());
	ret.emplace_back("min_height", m_css_min_width.to_string());
	ret.emplace_back("max_width", m_css_max_width.to_string());
	ret.emplace_back("max_height", m_css_max_width.to_string());
	ret.emplace_back("offsets", m_css_offsets.to_string());
	ret.emplace_back("text_indent", m_css_text_indent.to_string());
	ret.emplace_back("line_height", std::to_string(m_line_height.computed_value));
	ret.emplace_back("list_style_type", index_value(m_list_style_type, list_style_type_strings));
	ret.emplace_back("list_style_position", index_value(m_list_style_position, list_style_position_strings));
	ret.emplace_back("border_spacing_x", m_css_border_spacing_x.to_string());
	ret.emplace_back("border_spacing_y", m_css_border_spacing_y.to_string());

	return ret;
}
