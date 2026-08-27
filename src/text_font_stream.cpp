#include "gfx.hpp"
// truetype font embedded in a header:
// Downloaded from:
// https://github.com/edx/edx-fonts/blob/master/open-sans/fonts/Regular/OpenSans-Regular.ttf
// Converted with:
// https://codewitch-honey-crisis.github.io/gfx_web/header/index.html

// here's the actual implementation of the font
#define OPENSANS_REGULAR_IMPLEMENTATION
#include "OpenSans_Regular.hpp"
#undef OPENSANS_REGULAR_IMPLEMENTATION
using namespace gfx;
const_buffer_stream& text_font_stream = OpenSans_Regular;