#include "el_canvas.h"

litehtml::el_canvas::el_canvas(const document::ptr& doc) : html_tag(doc)
{
    // Canvas is inline-block by default, like img
    m_css.set_display(display_inline_block);
}

void litehtml::el_canvas::get_content_size(size& sz, pixel_t /*max_width*/)
{
    // Default canvas size per HTML spec: 300x150
    sz.width = 300;
    sz.height = 150;

    // Use HTML width/height attributes if specified
    const char* w_attr = get_attr("width");
    const char* h_attr = get_attr("height");

    if (w_attr)
    {
        sz.width = atoi(w_attr);
    }
    if (h_attr)
    {
        sz.height = atoi(h_attr);
    }
}

bool litehtml::el_canvas::is_replaced() const
{
    return true;
}

void litehtml::el_canvas::parse_attributes()
{
    // Map width/height attributes to CSS properties
    // https://html.spec.whatwg.org/multipage/canvas.html#attr-canvas-width
    const char* str = get_attr("width");
    if (str)
        map_to_dimension_property(_width_, str);

    str = get_attr("height");
    if (str)
        map_to_dimension_property(_height_, str);
}

litehtml::string litehtml::el_canvas::dump_get_name()
{
    return "canvas";
}
