#include "html.h"
#include "el_button.h"
#include "render_item.h"
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
	get_document()->container()->get_form_control_size(form_control_button, sz);
}

void el_button::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri)
{
	html_tag::draw(hdc, x, y, clip, ri);

	position pos = ri->pos();
	pos.x += x;
	pos.y += y;
	pos.round();

	if (pos.does_intersect(clip))
	{
		form_control_state state;
		state.focused = false;
		state.hovered = false;
		state.disabled = m_disabled;
		state.value = get_value();

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
	auto ret = std::make_shared<render_item>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}

} // namespace litehtml
