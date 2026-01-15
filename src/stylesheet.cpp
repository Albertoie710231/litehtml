#include "html.h"
#include "stylesheet.h"
#include "css_parser.h"
#include "document.h"
#include "document_container.h"

namespace litehtml
{

// https://www.w3.org/TR/css-syntax-3/#parse-a-css-stylesheet
template<class Input> // Input == string or css_token_vector
void css::parse_css_stylesheet(const Input& input, string baseurl, document::ptr doc, media_query_list_list::ptr media, bool top_level)
{
	if (doc && media)
		doc->add_media_list(media);

	// To parse a CSS stylesheet, first parse a stylesheet.
	auto rules = css_parser::parse_stylesheet(input, top_level);
	bool import_allowed = top_level;

	// Interpret all of the resulting top-level qualified rules as style rules, defined below.
	// If any style rule is invalid, or any at-rule is not recognized or is invalid according
	// to its grammar or context, it's a parse error. Discard that rule.
	for (auto rule : rules)
	{
		if (rule->type == raw_rule::qualified)
		{
			if (parse_style_rule(rule, baseurl, doc, media))
				import_allowed = false;
			continue;
		}

		// Otherwise: at-rule
		switch (_id(lowcase(rule->name)))
		{
		case _charset_: // ignored  https://www.w3.org/TR/css-syntax-3/#charset-rule
			break;

		case _import_:
			if (import_allowed)
				parse_import_rule(rule, baseurl, doc, media);
			else
				css_parse_error("incorrect placement of @import rule");
			break;

		// https://www.w3.org/TR/css-conditional-3/#at-media
		// @media <media-query-list> { <stylesheet> }
		case _media_:
		{
			if (rule->block.type != CURLY_BLOCK) break;
			auto new_media = media;
			auto mq_list = parse_media_query_list(rule->prelude, doc);
			// An empty media query list evaluates to true.  https://drafts.csswg.org/mediaqueries-5/#example-6f06ee45
			if (!mq_list.empty())
			{
				new_media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
				new_media->add(mq_list);
			}
			parse_css_stylesheet(rule->block.value, baseurl, doc, new_media, false);
			import_allowed = false;
			break;
		}

		// https://www.w3.org/TR/css-animations-1/#keyframes
		// @keyframes <keyframes-name> { <rule-list> }
		case _keyframes_:
		{
			if (rule->block.type != CURLY_BLOCK) break;
			parse_keyframes_rule(rule, doc);
			import_allowed = false;
			break;
		}

		default:
			css_parse_error("unrecognized rule @" + rule->name);
		}
	}
}

// https://drafts.csswg.org/css-cascade-5/#at-import
// `layer` and `supports` are not supported
// @import [ <url> | <string> ] <media-query-list>?
void css::parse_import_rule(raw_rule::ptr rule, string baseurl, document::ptr doc, media_query_list_list::ptr media)
{
	auto tokens = rule->prelude;
	int index = 0;
	skip_whitespace(tokens, index);
	auto tok = at(tokens, index);
	string url;
	auto parse_string = [](const css_token& tok, string& str)
	{
		if (tok.type != STRING) return false;
		str = tok.str;
		return true;
	};
	bool ok = parse_url(tok, url) || parse_string(tok, url);
	if (!ok) {
		css_parse_error("invalid @import rule");
		return;
	}
	document_container* container = doc->container();
	string css_text;
	string css_baseurl = baseurl;
	container->import_css(css_text, url, css_baseurl);

	auto new_media = media;
	tokens = slice(tokens, index + 1);
	auto mq_list = parse_media_query_list(tokens, doc);
	if (!mq_list.empty())
	{
		new_media = make_shared<media_query_list_list>(media ? *media : media_query_list_list());
		new_media->add(mq_list);
	}

	parse_css_stylesheet(css_text, css_baseurl, doc, new_media, true);
}

// https://www.w3.org/TR/css-syntax-3/#style-rules
bool css::parse_style_rule(raw_rule::ptr rule, string baseurl, document::ptr doc, media_query_list_list::ptr media)
{
	// The prelude of the qualified rule is parsed as a <selector-list>. If this returns failure, the entire style rule is invalid.
	auto list = parse_selector_list(rule->prelude, strict_mode, doc->mode());
	if (list.empty())
	{
		css_parse_error("invalid selector");
		return false;
	}

	style::ptr style = make_shared<litehtml::style>(); // style block
	// The content of the qualified rule's block is parsed as a style block's contents.
	style->add(rule->block.value, baseurl, doc->container());

	for (auto sel : list)
	{
		sel->m_style = style;
		sel->m_media_query = media;
		sel->calc_specificity();
		add_selector(sel);
	}
	return true;
}

void css::sort_selectors()
{
	std::sort(m_selectors.begin(), m_selectors.end(),
		 [](const css_selector::ptr& v1, const css_selector::ptr& v2)
		 {
			 return (*v1) < (*v2);
		 }
	);
	// Build index for fast selector lookup
	build_index();
}

void css::index_selector(const css_selector::ptr& selector)
{
	const auto& right = selector->m_right;

	// Index by tag (unless it's universal *)
	if (right.m_tag != star_id)
	{
		m_tag_index[right.m_tag].push_back(selector);
		return; // Tag is the primary key, don't add to other indexes
	}

	// For * selectors, check if there's a class or id in attrs
	for (const auto& attr : right.m_attrs)
	{
		if (attr.type == select_class)
		{
			m_class_index[attr.name].push_back(selector);
			return; // Indexed by first class
		}
		else if (attr.type == select_id)
		{
			m_id_index[attr.name].push_back(selector);
			return; // Indexed by id
		}
	}

	// Pure universal selector (*) or attribute selector without tag/class/id
	m_universal_selectors.push_back(selector);
}

void css::build_index()
{
	if (m_index_built) return;

	// Reserve approximate sizes to avoid reallocations
	m_tag_index.reserve(m_selectors.size() / 4);
	m_class_index.reserve(m_selectors.size() / 2);
	m_id_index.reserve(m_selectors.size() / 8);
	m_universal_selectors.reserve(m_selectors.size() / 20);

	for (const auto& selector : m_selectors)
	{
		index_selector(selector);
	}

	m_index_built = true;
}

void css::get_potentially_matching_selectors(
	string_id tag,
	const std::vector<string_id>& classes,
	string_id id,
	css_selector::vector& out_selectors) const
{
	if (!m_index_built)
	{
		// Fallback to all selectors if index not built
		out_selectors = m_selectors;
		return;
	}

	out_selectors.clear();

	// Add selectors that match by tag
	auto tag_it = m_tag_index.find(tag);
	if (tag_it != m_tag_index.end())
	{
		out_selectors.insert(out_selectors.end(), tag_it->second.begin(), tag_it->second.end());
	}

	// Add selectors that match by any class
	for (const auto& cls : classes)
	{
		auto class_it = m_class_index.find(cls);
		if (class_it != m_class_index.end())
		{
			out_selectors.insert(out_selectors.end(), class_it->second.begin(), class_it->second.end());
		}
	}

	// Add selectors that match by id
	if (id != empty_id)
	{
		auto id_it = m_id_index.find(id);
		if (id_it != m_id_index.end())
		{
			out_selectors.insert(out_selectors.end(), id_it->second.begin(), id_it->second.end());
		}
	}

	// Add universal selectors (they always need to be checked)
	out_selectors.insert(out_selectors.end(), m_universal_selectors.begin(), m_universal_selectors.end());

	// Sort by order to maintain cascade order
	std::sort(out_selectors.begin(), out_selectors.end(),
		[](const css_selector::ptr& a, const css_selector::ptr& b) {
			return *a < *b;
		});
}

// https://www.w3.org/TR/css-animations-1/#keyframes
// Parse @keyframes <name> { <keyframe-block-list> }
void css::parse_keyframes_rule(raw_rule::ptr rule, document::ptr doc)
{
	// Get keyframes name from prelude
	auto tokens = rule->prelude;
	int index = 0;
	skip_whitespace(tokens, index);

	if (index >= (int)tokens.size())
	{
		css_parse_error("@keyframes missing name");
		return;
	}

	// Name can be an ident or string
	string name;
	const auto& tok = tokens[index];
	if (tok.type == IDENT)
	{
		name = tok.ident();
	}
	else if (tok.type == STRING)
	{
		name = tok.str;
	}
	else
	{
		css_parse_error("@keyframes invalid name");
		return;
	}

	if (name.empty() || name == "none")
	{
		css_parse_error("@keyframes name cannot be 'none'");
		return;
	}

	keyframes_rule kf_rule;
	kf_rule.name = name;

	// Parse the block content as a list of keyframe blocks
	// Each keyframe block is: <keyframe-selector># { <declaration-list> }
	// <keyframe-selector> = from | to | <percentage>
	// Keyframes blocks are not like regular CSS rules - they don't have selectors
	// Instead we parse them as qualified rules where the prelude is the selector
	auto block_rules = css_parser::parse_stylesheet(rule->block.value, false);

	for (const auto& block : block_rules)
	{
		if (block->type != raw_rule::qualified) continue;

		// Parse keyframe selectors (can have multiple: 0%, 50% { ... })
		std::vector<float> offsets;
		auto sel_tokens = block->prelude;
		int sel_idx = 0;

		while (sel_idx < (int)sel_tokens.size())
		{
			skip_whitespace(sel_tokens, sel_idx);
			if (sel_idx >= (int)sel_tokens.size()) break;

			const auto& sel_tok = sel_tokens[sel_idx];

			if (sel_tok.type == IDENT)
			{
				string id = sel_tok.ident();
				if (id == "from")
				{
					offsets.push_back(0.0f);
				}
				else if (id == "to")
				{
					offsets.push_back(1.0f);
				}
				sel_idx++;
			}
			else if (sel_tok.type == PERCENTAGE)
			{
				offsets.push_back(sel_tok.n.number / 100.0f);
				sel_idx++;
			}
			else if (sel_tok.type == COMMA)
			{
				sel_idx++;
			}
			else
			{
				sel_idx++;  // Skip unknown tokens
			}
		}

		if (offsets.empty()) continue;

		// Parse declarations from the block
		if (block->block.type != CURLY_BLOCK) continue;

		std::map<string, string> properties;

		// Parse declarations
		auto decl_tokens = block->block.value;
		int decl_idx = 0;

		while (decl_idx < (int)decl_tokens.size())
		{
			skip_whitespace(decl_tokens, decl_idx);
			if (decl_idx >= (int)decl_tokens.size()) break;

			// Expect property name
			if (decl_tokens[decl_idx].type != IDENT)
			{
				decl_idx++;
				continue;
			}

			string prop_name = decl_tokens[decl_idx].ident();
			decl_idx++;
			skip_whitespace(decl_tokens, decl_idx);

			// Expect colon
			if (decl_idx >= (int)decl_tokens.size() || decl_tokens[decl_idx].ch != ':')
			{
				continue;
			}
			decl_idx++;
			skip_whitespace(decl_tokens, decl_idx);

			// Collect value until semicolon or end
			string value;
			while (decl_idx < (int)decl_tokens.size() && decl_tokens[decl_idx].ch != ';')
			{
				const auto& val_tok = decl_tokens[decl_idx];
				if (!value.empty() && val_tok.type != COMMA) value += " ";

				if (val_tok.type == IDENT)
				{
					value += val_tok.ident();
				}
				else if (val_tok.type == STRING)
				{
					value += "\"" + val_tok.str + "\"";
				}
				else if (val_tok.type == NUMBER || val_tok.type == PERCENTAGE || val_tok.type == DIMENSION)
				{
					value += std::to_string(val_tok.n.number);
					if (val_tok.type == PERCENTAGE) value += "%";
					else if (val_tok.type == DIMENSION) value += val_tok.unit;
				}
				else if (val_tok.type == HASH)
				{
					value += "#" + val_tok.name;
				}
				else if (val_tok.type == COMMA)
				{
					value += ",";
				}
				else if (val_tok.type == CV_FUNCTION)
				{
					value += val_tok.name + "(";
					for (size_t i = 0; i < val_tok.value.size(); i++)
					{
						const auto& v = val_tok.value[i];
						if (i > 0 && v.type != COMMA) value += " ";
						if (v.type == IDENT) value += v.ident();
						else if (v.type == NUMBER || v.type == PERCENTAGE || v.type == DIMENSION)
						{
							value += std::to_string(v.n.number);
							if (v.type == PERCENTAGE) value += "%";
							else if (v.type == DIMENSION) value += v.unit;
						}
						else if (v.type == COMMA) value += ",";
					}
					value += ")";
				}
				decl_idx++;
			}

			if (decl_idx < (int)decl_tokens.size() && decl_tokens[decl_idx].ch == ';')
			{
				decl_idx++;
			}

			if (!prop_name.empty() && !value.empty())
			{
				properties[prop_name] = value;
			}
		}

		// Create keyframe entries for each offset
		for (float offset : offsets)
		{
			keyframe kf;
			kf.offset = offset;
			kf.properties = properties;
			kf_rule.keyframes.push_back(kf);
		}
	}

	// Sort keyframes by offset
	std::sort(kf_rule.keyframes.begin(), kf_rule.keyframes.end(),
		[](const keyframe& a, const keyframe& b) {
			return a.offset < b.offset;
		});

	// Add to document
	doc->add_keyframes(kf_rule);
}

} // namespace litehtml
