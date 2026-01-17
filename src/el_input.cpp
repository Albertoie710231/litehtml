#include "html.h"
#include "el_input.h"
#include "render_item.h"
#include "render_image.h"  // For render_item_image (used for replaced elements)
#include "document_container.h"

namespace litehtml
{

el_input::el_input(const document::ptr& doc) : html_tag(doc)
{
	m_type = form_control_input_text;
	m_checked = false;
	m_disabled = false;
	m_readonly = false;
	m_css.set_display(display_inline_block);
}

bool el_input::is_replaced() const
{
	return true;
}

form_control_type el_input::parse_input_type(const char* type_str)
{
	if (!type_str || !*type_str)
		return form_control_input_text;

	string type = lowcase(type_str);

	if (type == "text")         return form_control_input_text;
	if (type == "password")     return form_control_input_password;
	if (type == "checkbox")     return form_control_input_checkbox;
	if (type == "radio")        return form_control_input_radio;
	if (type == "submit")       return form_control_input_submit;
	if (type == "reset")        return form_control_input_reset;
	if (type == "button")       return form_control_input_button;
	if (type == "hidden")       return form_control_input_hidden;
	if (type == "file")         return form_control_input_file;
	if (type == "number")       return form_control_input_number;
	if (type == "range")        return form_control_input_range;
	if (type == "color")        return form_control_input_color;
	if (type == "date")         return form_control_input_date;

	// Default to text for unknown types
	return form_control_input_text;
}

void el_input::parse_attributes()
{
	m_type = parse_input_type(get_attr("type", "text"));
	m_value = get_attr("value", "");
	m_placeholder = get_attr("placeholder", "");
	m_checked = get_attr("checked") != nullptr;
	m_disabled = get_attr("disabled") != nullptr;
	m_readonly = get_attr("readonly") != nullptr;

	// Handle size attribute for text inputs
	const char* size_attr = get_attr("size");
	if (size_attr)
	{
		int size = atoi(size_attr);
		if (size > 0)
		{
			// Approximate width based on character count
			// This is a rough estimate - real browsers use font metrics
			string width_str = std::to_string(size * 8) + "px";
			m_style.add_property(_width_, width_str);
		}
	}

	// Handle width/height attributes
	const char* w_attr = get_attr("width");
	if (w_attr)
		map_to_dimension_property(_width_, w_attr);

	const char* h_attr = get_attr("height");
	if (h_attr)
		map_to_dimension_property(_height_, h_attr);
}

void el_input::compute_styles(bool recursive, bool use_cache)
{
	html_tag::compute_styles(recursive, use_cache);

	// Hidden inputs should not be displayed
	if (m_type == form_control_input_hidden)
	{
		m_css.set_display(display_none);
	}
}

void el_input::get_content_size(size& sz, pixel_t /*max_width*/)
{
	const auto& c = css();
	const auto& padding = c.get_padding();
	pixel_t padLeft = static_cast<pixel_t>(padding.left.val());
	pixel_t padRight = static_cast<pixel_t>(padding.right.val());
	pixel_t padTop = static_cast<pixel_t>(padding.top.val());
	pixel_t padBottom = static_cast<pixel_t>(padding.bottom.val());

	// For button types, measure the text
	if (m_type == form_control_input_submit ||
	    m_type == form_control_input_reset ||
	    m_type == form_control_input_button)
	{
		string text = m_value;
		if (text.empty()) {
			if (m_type == form_control_input_submit) text = "Submit";
			else if (m_type == form_control_input_reset) text = "Reset";
			else text = "Button";
		}

		auto doc = get_document();
		auto container = doc->container();
		uint_ptr font = c.get_font();

		if (font) {
			sz.width = container->text_width(text.c_str(), font);
		} else {
			sz.width = static_cast<pixel_t>(text.length() * 8);
		}

		// Add CSS padding
		sz.width += padLeft + padRight;
		sz.height = c.get_font_metrics().height + padTop + padBottom;
		if (sz.height < 24) sz.height = 24;
	}
	else
	{
		get_document()->container()->get_form_control_size(m_type, sz);
	}
}

void el_input::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri)
{
	// Don't call html_tag::draw - form controls are replaced elements
	// that handle all their own drawing via draw_form_control

	position pos = ri->pos();
	pos.x += x;
	pos.y += y;
	pos.round();

	if (pos.does_intersect(clip))
	{
		form_control_state state;
		state.focused = false; // TODO: track focus state
		state.hovered = false; // TODO: track hover state
		state.checked = m_checked;
		state.disabled = m_disabled;
		state.readonly = m_readonly;
		state.value = m_value;
		state.placeholder = m_placeholder;

		// Extract CSS properties from computed styles
		const auto& c = css();
		state.text_color = c.get_color();
		state.background_color = c.get_bg().m_color;

		// Use left border as the reference for border properties
		const auto& borders = c.get_borders();
		state.border_color = borders.left.color;
		state.border_width = static_cast<pixel_t>(borders.left.width.val());

		// Get padding values
		const auto& padding = c.get_padding();
		state.padding_left = static_cast<pixel_t>(padding.left.val());
		state.padding_right = static_cast<pixel_t>(padding.right.val());
		state.padding_top = static_cast<pixel_t>(padding.top.val());
		state.padding_bottom = static_cast<pixel_t>(padding.bottom.val());

		// Get font properties
		state.font_size = c.get_font_size();
		state.font = c.get_font();
		state.line_height = c.line_height().computed_value;
		if (state.line_height <= 0) {
			state.line_height = c.get_font_metrics().height;
		}

		// Placeholder color: use text color with reduced alpha
		state.placeholder_color = state.text_color;
		state.placeholder_color.alpha = 128;  // 50% opacity

		get_document()->container()->draw_form_control(hdc, m_type, pos, state);
	}
}

void el_input::set_value(const string& val)
{
	m_value = val;
	get_document()->container()->on_form_control_change(shared_from_this());
}

void el_input::set_checked(bool checked)
{
	m_checked = checked;
	get_document()->container()->on_form_control_change(shared_from_this());
}

string el_input::dump_get_name()
{
	return "input type=\"" + string(get_attr("type", "text")) + "\"";
}

std::shared_ptr<render_item> el_input::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	// Use render_item_image for proper replaced element sizing
	auto ret = std::make_shared<render_item_image>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}

} // namespace litehtml
