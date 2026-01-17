#ifndef LH_EL_SELECT_H
#define LH_EL_SELECT_H

#include "html_tag.h"
#include "document.h"

namespace litehtml
{
	class el_select : public html_tag
	{
		bool m_disabled;
		bool m_multiple;
		int m_size;
		int m_selected_index;

	public:
		el_select(const document::ptr& doc);

		bool is_replaced() const override;
		void parse_attributes() override;
		void compute_styles(bool recursive = true, bool use_cache = true) override;
		void draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip, const std::shared_ptr<render_item>& ri) override;
		void get_content_size(size& sz, pixel_t max_width) override;
		string dump_get_name() override;

		std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& parent_ri) override;

		// Form control specific methods
		int get_selected_index() const { return m_selected_index; }
		void set_selected_index(int index);
		bool is_disabled() const { return m_disabled; }
		string get_value() const;
	};
}

#endif  // LH_EL_SELECT_H
