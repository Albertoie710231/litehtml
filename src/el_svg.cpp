#include "el_svg.h"
#include "render_image.h"
#include "document_container.h"
#include <functional>
#include <sstream>

// Static members
std::unordered_map<litehtml::string, litehtml::el_svg*> litehtml::el_svg::s_registry;
std::mutex litehtml::el_svg::s_registryMutex;

litehtml::el_svg::el_svg(const document::ptr& doc) : html_tag(doc)
{
	m_css.set_display(display_inline_block);
}

litehtml::el_svg::~el_svg()
{
	// Unregister from static registry
	if (!m_svg_id.empty()) {
		std::lock_guard<std::mutex> lock(s_registryMutex);
		s_registry.erase(m_svg_id);
	}
}

void litehtml::el_svg::get_content_size(size& sz, pixel_t /*max_width*/)
{
	// First, try CSS computed dimensions (this handles CSS width/height properly)
	const auto& computed = css();
	css_length css_w = computed.get_width();
	css_length css_h = computed.get_height();

	if (!css_w.is_predefined() && css_w.units() == css_units_px && css_w.val() > 0) {
		sz.width = static_cast<int>(css_w.val());
	}
	if (!css_h.is_predefined() && css_h.units() == css_units_px && css_h.val() > 0) {
		sz.height = static_cast<int>(css_h.val());
	}

	// If CSS didn't provide dimensions, try HTML attributes
	if (sz.width == 0 || sz.height == 0) {
		const char* w = get_attr("width");
		const char* h = get_attr("height");

		if (w && sz.width == 0) sz.width = atoi(w);
		if (h && sz.height == 0) sz.height = atoi(h);
	}

	// If still no dimensions, try container (for external SVG images)
	if ((sz.width == 0 || sz.height == 0) && !m_svg_id.empty()) {
		size container_sz;
		get_document()->container()->get_image_size(m_svg_id.c_str(), nullptr, container_sz);
		if (sz.width == 0 && container_sz.width > 0) sz.width = container_sz.width;
		if (sz.height == 0 && container_sz.height > 0) sz.height = container_sz.height;
	}

	// Default to a small icon size (24x24) rather than 100x100
	// This is more appropriate for inline SVG icons
	if (sz.width == 0) sz.width = 24;
	if (sz.height == 0) sz.height = 24;
}

bool litehtml::el_svg::is_replaced() const
{
	return true;
}

void litehtml::el_svg::parse_attributes()
{
	// Nothing special to parse - SVG content comes from children
}

void litehtml::el_svg::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri)
{
	html_tag::draw(hdc, x, y, clip, ri);
	position pos = ri->pos();
	pos.x += x;
	pos.y += y;
	pos.round();

	// Draw SVG as image
	if(pos.does_intersect(clip) && !m_svg_id.empty())
	{
		if (pos.width > 0 && pos.height > 0)
		{
			background_layer layer;
			layer.clip_box = pos;
			layer.origin_box = pos;
			layer.border_box = pos;
			layer.border_box += ri->get_paddings();
			layer.border_box += ri->get_borders();
			layer.repeat = background_repeat_no_repeat;
			layer.border_radius = css().get_borders().radius.calc_percents(layer.border_box.width, layer.border_box.height);
			get_document()->container()->draw_image(hdc, layer, m_svg_id, {});
		}
	}
}

void litehtml::el_svg::compute_styles(bool recursive)
{
	html_tag::compute_styles(recursive);

	if (!m_loaded)
	{
		// Generate a unique ID for this SVG based on pointer
		m_svg_id = "svg://inline/" + std::to_string(reinterpret_cast<uintptr_t>(this));

		// Register in static registry so DocumentContainer can find us
		{
			std::lock_guard<std::mutex> lock(s_registryMutex);
			s_registry[m_svg_id] = this;
		}

		m_loaded = true;
	}
}

litehtml::string litehtml::el_svg::dump_get_name()
{
	return "svg id=\"" + m_svg_id + "\"";
}

std::shared_ptr<litehtml::render_item> litehtml::el_svg::create_render_item(const std::shared_ptr<render_item>& parent_ri)
{
	auto ret = std::make_shared<render_item_image>(shared_from_this());
	ret->parent(parent_ri);
	return ret;
}

litehtml::el_svg* litehtml::el_svg::find_by_id(const string& svg_id)
{
	std::lock_guard<std::mutex> lock(s_registryMutex);
	auto it = s_registry.find(svg_id);
	if (it != s_registry.end()) {
		return it->second;
	}
	return nullptr;
}

void litehtml::el_svg::clear_registry()
{
	std::lock_guard<std::mutex> lock(s_registryMutex);
	s_registry.clear();
}
