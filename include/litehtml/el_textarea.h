#ifndef LH_EL_TEXTAREA_H
#define LH_EL_TEXTAREA_H

#include "html_tag.h"
#include "document.h"
#include "document_container.h"

namespace litehtml
{
	class el_textarea : public html_tag
	{
		string m_value;
		string m_placeholder;
		bool m_disabled;
		bool m_readonly;
		int m_rows;
		int m_cols;

	public:
		el_textarea(const document::ptr& doc);

		bool is_replaced() const override;
		void parse_attributes() override;
		void compute_styles(bool recursive = true, bool use_cache = true) override;
		void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri) override;
		void get_content_size(size& sz, pixel_t max_width) override;
		string dump_get_name() override;

		std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

		// Form control specific methods
		const string& get_value() const { return m_value; }
		void set_value(const string& val);
		bool is_disabled() const { return m_disabled; }
	};
}

#endif  // LH_EL_TEXTAREA_H
