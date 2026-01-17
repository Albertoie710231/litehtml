#ifndef LH_EL_BUTTON_H
#define LH_EL_BUTTON_H

#include "html_tag.h"
#include "document.h"

namespace litehtml
{
	class el_button : public html_tag
	{
		string m_type; // submit, reset, button
		bool m_disabled;

	public:
		el_button(const document::ptr& doc);

		bool is_replaced() const override;
		void parse_attributes() override;
		void compute_styles(bool recursive = true, bool use_cache = true) override;
		void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri) override;
		void get_content_size(size& sz, pixel_t max_width) override;
		string dump_get_name() override;

		std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

		// Form control specific methods
		const string& get_button_type() const { return m_type; }
		bool is_disabled() const { return m_disabled; }
		string get_value() const;
	};
}

#endif  // LH_EL_BUTTON_H
