#pragma once

#include "gui.h"

void gui_overview_page_update(struct sysm_page_overview *page, struct sysm_app *app);
void gui_overview_page_init(struct sysm_page_overview *page, struct sysm_app *app);
void gui_overview_page_destroy(struct sysm_page_overview *page);
