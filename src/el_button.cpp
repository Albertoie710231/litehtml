#include "html.h"
#include "el_button.h"
#include "render_item.h"
#include "render_image.h"
#include "document_container.h"

namespace litehtml
{

el_button::el_button(const document::ptr& doc) : html_tag(doc)
{
	m_type = "submit"; // Default type
	m_disabled = false;
	m_css.set_display(display_inline_block);
}

bool el_button::is_replaced() const
{
	return true;
}

void el_button::parse_attributes()
{
	m_type = get_attr("type", "submit");
	m_disabled = get_attr("disabled") != nullptr;
}

void el_button::compute_styles(bool recursive, bool use_cache)
{
	html_tag::compute_styles(recursive, use_cache);
}

void el_button::get_content_size(size& sz, pixel_t /*max_width*/)
{
	// Get button text
	string text = get_value();
	if (text.empty()) {
		text = "Button";
	}

	// Measure text width using the element's font
	auto doc = get_document();
	auto container = doc->container();

	const auto& c = css();
	const auto& padding = c.get_padding();
	pixel_t padLeft = static_cast<pixel_t>(padding.left.val());
	pixel_t padRight = static_cast<pixel_t>(padding.right.val());
	pixel_t padTop = static_cast<pixel_t>(padding.top.val());
	pixel_t padBottom = static_cast<pixel_t>(padding.bottom.val());

	// Get font metrics
	uint_ptr font = c.get_font();
	if (font) {
		sz.width = container->text_width(text.c_str(), font);
	} else {
		// Fallback: estimate based on character count
		sz.width = static_cast<pixel_t>(text.length() * 8);
	}

	// Add CSS padding
	sz.width += padLeft + padRight;

	// Height based on font size + CSS padding
	sz.height = c.get_font_metrics().height + padTop + padBottom;
	if (sz.height < 24) sz.height = 24;
}

void el_button::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri)
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

		get_document()->container()->draw_form_control(hdc, form_control_button, pos, state);
	}
}

string el_button::get_value() const
{
	const char* val = get_attr("value");
	if (val)
		return val;
	// Use text content as label
	string text;
	const_cast<el_button*>(this)->get_text(text);
	return text;
}

string el_button::dump_get_name()
{
	return "button type=\"" + m_type + "\"";
}

std::shared_ptr<render_item> el_button::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	auto ret = std::make_shared<render_item_image>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}

} // namespace litehtml
