#ifndef LH_EL_SVG_H
#define LH_EL_SVG_H

#include "html_tag.h"
#include "document.h"
#include "types.h"
#include <unordered_map>
#include <mutex>

namespace litehtml
{
	class el_svg : public html_tag
	{
		string	m_svg_id;  // Unique identifier for this SVG content
		bool	m_loaded = false;

		// Static registry for looking up SVG elements by ID
		static std::unordered_map<string, el_svg*> s_registry;
		static std::mutex s_registryMutex;

	public:
		el_svg(const document::ptr& doc);
		~el_svg();

		bool	is_replaced() const override;
		void	parse_attributes() override;
		void	compute_styles(bool recursive = true, bool use_cache = true) override;
		void	draw(uint_ptr hdc, pixel_t x, pixel_t y, const position *clip, const std::shared_ptr<render_item> &ri) override;
		void	get_content_size(size& sz, pixel_t max_width) override;
		string	dump_get_name() override;

		std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

		// Get the unique SVG identifier
		const string& get_svg_id() const { return m_svg_id; }

		// Static method to find an SVG element by ID
		static el_svg* find_by_id(const string& svg_id);

		// Clear the registry (call when navigating to new page)
		static void clear_registry();
	};
}

#endif  // LH_EL_SVG_H
