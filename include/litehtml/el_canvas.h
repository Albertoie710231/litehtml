#ifndef LH_EL_CANVAS_H
#define LH_EL_CANVAS_H

#include "html_tag.h"
#include "document.h"
#include "types.h"

namespace litehtml
{

class el_canvas : public html_tag
{
public:
    el_canvas(const document::ptr& doc);

    bool    is_replaced() const override;
    void    parse_attributes() override;
    void    get_content_size(size& sz, pixel_t max_width) override;
    string  dump_get_name() override;
};

}

#endif  // LH_EL_CANVAS_H
