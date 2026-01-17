#include "html.h"
#include "el_textarea.h"
#include "render_item.h"
#include "render_image.h"
#include "document_container.h"

namespace litehtml
{

el_textarea::el_textarea(const document::ptr& doc) : html_tag(doc)
{
	m_disabled = false;
	m_readonly = false;
	m_rows = 2;
	m_cols = 20;
	m_css.set_display(display_inline_block);
}

bool el_textarea::is_replaced() const
{
	return true;
}

void el_textarea::parse_attributes()
{
	m_placeholder = get_attr("placeholder", "");
	m_disabled = get_attr("disabled") != nullptr;
	m_readonly = get_attr("readonly") != nullptr;

	// Get initial value from text content
	string text;
	get_text(text);
	m_value = text;

	// Handle rows and cols attributes
	const char* rows_attr = get_attr("rows");
	if (rows_attr)
		m_rows = std::max(1, atoi(rows_attr));

	const char* cols_attr = get_attr("cols");
	if (cols_attr)
		m_cols = std::max(1, atoi(cols_attr));

	// Note: actual sizing is now handled in get_content_size() using CSS properties
	// The master_css provides default width/height, and rows/cols override via get_content_size
}

void el_textarea::compute_styles(bool recursive, bool use_cache)
{
	html_tag::compute_styles(recursive, use_cache);
}

void el_textarea::get_content_size(size& sz, pixel_t /*max_width*/)
{
	get_document()->container()->get_form_control_size(form_control_textarea, sz);

	const auto& c = css();

	// Use font metrics for character/row sizing
	auto container = get_document()->container();
	uint_ptr font = c.get_font();
	pixel_t charWidth = 8;  // Fallback
	pixel_t lineHeight = c.line_height().computed_value;
	if (lineHeight <= 0) {
		lineHeight = c.get_font_metrics().height;
	}
	if (lineHeight <= 0) {
		lineHeight = 16;  // Fallback
	}

	// Use font metrics to estimate character width if possible
	if (font) {
		charWidth = container->text_width("M", font);
		if (charWidth <= 0) charWidth = 8;
	}

	// Override with rows/cols if specified
	if (m_cols > 0)
		sz.width = m_cols * charWidth;
	if (m_rows > 0)
		sz.height = m_rows * lineHeight;
}

void el_textarea::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri)
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
		state.focused = false;
		state.hovered = false;
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

		get_document()->container()->draw_form_control(hdc, form_control_textarea, pos, state);
	}
}

void el_textarea::set_value(const string& val)
{
	m_value = val;
	get_document()->container()->on_form_control_change(shared_from_this());
}

string el_textarea::dump_get_name()
{
	return "textarea";
}

std::shared_ptr<render_item> el_textarea::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	auto ret = std::make_shared<render_item_image>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}

} // namespace litehtml
