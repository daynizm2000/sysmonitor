#pragma once

#include "gui.h"

void gui_overview_page_update(struct gui_page_overview *page, struct sysm_app *app);
void gui_overview_page_init(struct gui_page_overview *page, struct sysm_app *app);
void gui_overview_page_destroy(struct gui_page_overview *page);
