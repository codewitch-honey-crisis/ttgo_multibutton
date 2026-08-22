#pragma once
#include "panel.h"
#ifdef LCD_BUS
#include "gfx.h"
#include "uix.hpp"
extern uix::display devkit_display;
#endif


void devkit_init(void);
void devkit_update(void);