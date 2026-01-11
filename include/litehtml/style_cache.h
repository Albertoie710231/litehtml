#ifndef LH_STYLE_CACHE_H
#define LH_STYLE_CACHE_H

#include "string_id.h"
#include "css_properties.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace litehtml
{

// Style sharing cache - stores computed styles for reuse between similar elements
// Based on WebKit's style sharing cache and Quantum CSS's style sharing
class style_cache
{
public:
	// Maximum cache size to prevent unbounded memory growth
	static constexpr size_t MaxCacheSize = 4096;

	struct cache_key
	{
		string_id tag;
		size_t classes_hash;             // Hash of sorted classes
		size_t style_hash;               // Hash of matched CSS rules (m_style)
		size_t parent_style_hash;        // Hash of parent's computed style

		bool operator==(const cache_key& other) const
		{
			return tag == other.tag &&
			       style_hash == other.style_hash &&
			       parent_style_hash == other.parent_style_hash &&
			       classes_hash == other.classes_hash;
		}
	};

	struct cache_key_hash
	{
		size_t operator()(const cache_key& key) const
		{
			size_t h = std::hash<int>{}(static_cast<int>(key.tag));
			h ^= key.style_hash + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= key.parent_style_hash + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= key.classes_hash + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	// Compute hash for a vector of classes
	static size_t hash_classes(const std::vector<string_id>& classes)
	{
		// Sort a copy for consistent hashing
		std::vector<string_id> sorted = classes;
		std::sort(sorted.begin(), sorted.end());

		size_t h = 0;
		for (auto cls : sorted)
		{
			h ^= std::hash<int>{}(static_cast<int>(cls)) + 0x9e3779b9 + (h << 6) + (h >> 2);
		}
		return h;
	}

	// Try to find cached style for element
	// Returns nullptr if not found
	const css_properties* find(string_id tag, const std::vector<string_id>& classes,
	                           size_t style_hash, size_t parent_style_hash) const
	{
		cache_key key;
		key.tag = tag;
		key.classes_hash = hash_classes(classes);
		key.style_hash = style_hash;
		key.parent_style_hash = parent_style_hash;

		auto it = m_cache.find(key);
		if (it != m_cache.end())
		{
			m_hits++;
			return &it->second;
		}
		m_misses++;
		return nullptr;
	}

	// Store computed style in cache
	void store(string_id tag, const std::vector<string_id>& classes,
	           size_t style_hash, size_t parent_style_hash,
	           const css_properties& computed_style)
	{
		// Evict if cache is full (simple LRU would be better but this is simpler)
		if (m_cache.size() >= MaxCacheSize)
		{
			// Clear half the cache (crude but effective)
			auto it = m_cache.begin();
			for (size_t i = 0; i < MaxCacheSize / 2 && it != m_cache.end(); ++i)
			{
				it = m_cache.erase(it);
			}
		}

		cache_key key;
		key.tag = tag;
		key.classes_hash = hash_classes(classes);
		key.style_hash = style_hash;
		key.parent_style_hash = parent_style_hash;

		m_cache[key] = computed_style;
	}

	void clear()
	{
		m_cache.clear();
		m_hits = 0;
		m_misses = 0;
	}

	size_t size() const { return m_cache.size(); }
	size_t hits() const { return m_hits; }
	size_t misses() const { return m_misses; }
	float hit_rate() const { return m_hits + m_misses > 0 ? float(m_hits) / (m_hits + m_misses) : 0; }

private:
	mutable std::unordered_map<cache_key, css_properties, cache_key_hash> m_cache;
	mutable size_t m_hits = 0;
	mutable size_t m_misses = 0;
};

} // namespace litehtml

#endif // LH_STYLE_CACHE_H
