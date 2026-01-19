#ifndef LH_EL_COMMENT_H
#define LH_EL_COMMENT_H

#include "element.h"

namespace litehtml
{
	class el_comment : public element
	{
		string	m_text;
	public:
		explicit el_comment(const std::shared_ptr<document>& doc);

		bool is_comment() const override;
		void get_text(string& text) const override;
		void set_data(const char* data) override;
		std::shared_ptr<render_item> create_render_item(const std::shared_ptr<render_item>& /*parent_ri*/) override
		{
			// Comments are not rendered
			return nullptr;
		}

		// Node interface overrides (per WHATWG DOM spec)
		NodeType			nodeType() const override { return COMMENT_NODE; }
		string				nodeName() const override { return "#comment"; }
		string				nodeValue() const override { return m_text; }
		void				set_nodeValue(const string& val) override;

		// Comment node specific methods
		const string&		data() const { return m_text; }
		size_t				length() const { return m_text.length(); }
	};
}

#endif  // LH_EL_COMMENT_H
