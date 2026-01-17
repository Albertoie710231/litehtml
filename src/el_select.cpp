#include "html.h"
#include "el_select.h"
#include "render_item.h"
#include "render_image.h"
#include "document_container.h"

namespace litehtml
{

el_select::el_select(const document::ptr& doc) : html_tag(doc)
{
	m_disabled = false;
	m_multiple = false;
	m_size = 1;
	m_selected_index = -1;
	m_css.set_display(display_inline_block);
}

bool el_select::is_replaced() const
{
	return true;
}

void el_select::parse_attributes()
{
	m_disabled = get_attr("disabled") != nullptr;
	m_multiple = get_attr("multiple") != nullptr;

	const char* size_attr = get_attr("size");
	if (size_attr)
		m_size = std::max(1, atoi(size_attr));
	else if (m_multiple)
		m_size = 4; // Default size for multiple

	// Find the first selected option
	int idx = 0;
	for (const auto& child : m_children)
	{
		if (child && lowcase(child->get_tagName()) == "option")
		{
			if (child->get_attr("selected") != nullptr)
			{
				m_selected_index = idx;
				break;
			}
		}
		idx++;
	}

	// If no option is selected, select the first one
	if (m_selected_index < 0 && !m_multiple)
	{
		idx = 0;
		for (const auto& child : m_children)
		{
			if (child && lowcase(child->get_tagName()) == "option")
			{
				m_selected_index = idx;
				break;
			}
			idx++;
		}
	}
}

void el_select::compute_styles(bool recursive, bool use_cache)
{
	html_tag::compute_styles(recursive, use_cache);
}

void el_select::get_content_size(size& sz, pixel_t /*max_width*/)
{
	get_document()->container()->get_form_control_size(form_control_select, sz);

	const auto& c = css();

	// Use CSS line height for row sizing
	pixel_t lineHeight = c.line_height().computed_value;
	if (lineHeight <= 0) {
		lineHeight = c.get_font_metrics().height;
	}
	if (lineHeight <= 0) {
		lineHeight = 18;  // Fallback
	}

	// Adjust height for multiple/size
	if (m_size > 1)
	{
		sz.height = m_size * lineHeight;
	}
}

void el_select::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri)
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
		// Track focus/hover via pseudo-classes
		state.focused = has_pseudo_class(_focus_);
		state.hovered = has_pseudo_class(_hover_);
		state.disabled = m_disabled;
		state.selected_index = m_selected_index;
		state.value = get_value();

		// Extract CSS properties from computed styles
		const auto& c = css();
		state.text_color = c.get_color();
		state.background_color = c.get_bg().m_color;
		state.accent_color = c.get_accent_color();

		// Check appearance property - if none, don't use native rendering
		state.use_native_appearance = (c.get_appearance() != appearance_none);

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

		// Dropdown arrow color - default to text color
		state.arrow_color = state.text_color;
		state.arrow_size = c.get_font_size() / 2;  // Arrow size based on font

		get_document()->container()->draw_form_control(hdc, form_control_select, pos, state);
	}
}

string el_select::get_value() const
{
	if (m_selected_index >= 0)
	{
		int idx = 0;
		for (const auto& child : element::children())
		{
			if (idx == m_selected_index && child)
			{
				const char* val = child->get_attr("value");
				if (val)
					return val;
				// If no value attribute, use text content
				string text;
				child->get_text(text);
				return text;
			}
			idx++;
		}
	}
	return "";
}

void el_select::set_selected_index(int index)
{
	m_selected_index = index;
	get_document()->container()->on_form_control_change(shared_from_this());
}

string el_select::dump_get_name()
{
	return "select";
}

std::shared_ptr<render_item> el_select::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	auto ret = std::make_shared<render_item_image>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}

} // namespace litehtml
