#ifndef LH_SELECTOR_FILTER_H
#define LH_SELECTOR_FILTER_H

#include "string_id.h"
#include <vector>
#include <array>

namespace litehtml
{

// Counting bloom filter for fast-rejection of descendant selectors
// Based on WebKit's SelectorFilter and Servo's StyleBloom
class selector_filter
{
public:
	// Salt values to separate different identifier types (from WebKit)
	static constexpr unsigned TagNameSalt = 13;
	static constexpr unsigned IdSalt = 17;
	static constexpr unsigned ClassSalt = 19;

	// Bloom filter size (must be power of 2)
	// 256 bytes = 2048 bits, ~1% false positive rate with ~20 ancestors
	static constexpr size_t FilterSize = 256;

	selector_filter() : m_filter{}, m_stack_depth(0) {}

	// Push an element's identifiers onto the filter (call when entering an element)
	void push_element(string_id tag, string_id id, const std::vector<string_id>& classes)
	{
		element_hashes hashes;

		// Hash tag name
		if (tag != empty_id && tag != star_id)
		{
			unsigned h = hash_with_salt(tag, TagNameSalt);
			add_hash(h);
			hashes.push_back(h);
		}

		// Hash ID
		if (id != empty_id)
		{
			unsigned h = hash_with_salt(id, IdSalt);
			add_hash(h);
			hashes.push_back(h);
		}

		// Hash classes
		for (const auto& cls : classes)
		{
			unsigned h = hash_with_salt(cls, ClassSalt);
			add_hash(h);
			hashes.push_back(h);
		}

		m_hash_stack.push_back(std::move(hashes));
		m_stack_depth++;
	}

	// Pop element from filter (call when leaving an element)
	void pop_element()
	{
		if (m_stack_depth == 0 || m_hash_stack.empty()) return;

		const auto& hashes = m_hash_stack.back();
		for (unsigned h : hashes)
		{
			remove_hash(h);
		}
		m_hash_stack.pop_back();
		m_stack_depth--;
	}

	// Check if a selector's required identifiers might be in the ancestor chain
	// Returns true if ancestors MIGHT match, false if they definitely DON'T
	bool might_have_ancestor_with_tag(string_id tag) const
	{
		if (tag == empty_id || tag == star_id) return true;
		return might_contain(hash_with_salt(tag, TagNameSalt));
	}

	bool might_have_ancestor_with_id(string_id id) const
	{
		if (id == empty_id) return true;
		return might_contain(hash_with_salt(id, IdSalt));
	}

	bool might_have_ancestor_with_class(string_id cls) const
	{
		if (cls == empty_id) return true;
		return might_contain(hash_with_salt(cls, ClassSalt));
	}

	// Combined check for a css_element_selector's tag and first class/id
	// Returns false if we can fast-reject this selector
	bool might_match_ancestor(string_id tag, const std::vector<string_id>& classes, string_id id) const
	{
		// Check tag
		if (tag != empty_id && tag != star_id)
		{
			if (!might_contain(hash_with_salt(tag, TagNameSalt)))
				return false;
		}

		// Check id
		if (id != empty_id)
		{
			if (!might_contain(hash_with_salt(id, IdSalt)))
				return false;
		}

		// Check classes (only first one for speed, covers most cases)
		if (!classes.empty())
		{
			if (!might_contain(hash_with_salt(classes[0], ClassSalt)))
				return false;
		}

		return true;
	}

	size_t depth() const { return m_stack_depth; }
	void clear()
	{
		m_filter.fill(0);
		m_hash_stack.clear();
		m_stack_depth = 0;
	}

private:
	using element_hashes = std::vector<unsigned>;

	// Counting bloom filter - each byte is a counter (max 255)
	std::array<uint8_t, FilterSize> m_filter;

	// Stack of hashes for each element (for popping)
	std::vector<element_hashes> m_hash_stack;

	size_t m_stack_depth;

	static unsigned hash_with_salt(string_id id, unsigned salt)
	{
		// Simple hash combining string_id with salt
		return static_cast<unsigned>(id) * salt;
	}

	unsigned filter_index(unsigned hash) const
	{
		// Two hash functions for better distribution
		return hash % FilterSize;
	}

	unsigned filter_index2(unsigned hash) const
	{
		return (hash / FilterSize) % FilterSize;
	}

	void add_hash(unsigned hash)
	{
		unsigned idx1 = filter_index(hash);
		unsigned idx2 = filter_index2(hash);

		if (m_filter[idx1] < 255) m_filter[idx1]++;
		if (m_filter[idx2] < 255) m_filter[idx2]++;
	}

	void remove_hash(unsigned hash)
	{
		unsigned idx1 = filter_index(hash);
		unsigned idx2 = filter_index2(hash);

		if (m_filter[idx1] > 0) m_filter[idx1]--;
		if (m_filter[idx2] > 0) m_filter[idx2]--;
	}

	bool might_contain(unsigned hash) const
	{
		// Both positions must be non-zero for a possible match
		return m_filter[filter_index(hash)] > 0 &&
		       m_filter[filter_index2(hash)] > 0;
	}
};

} // namespace litehtml

#endif // LH_SELECTOR_FILTER_H
