#pragma once

#include "gui.h"

void gui_cpu_core_init(struct gui_cpu_card *core, const char *title, bool is_markup);
void gui_cpu_main_core_init(struct gui_cpu_card *cpu, const char *title, bool is_markup);
struct gui_cpu_card *gui_cpu_core_create_init(const char *title, bool is_markup);
void gui_cpu_core_free(struct gui_cpu_card *core);
void gui_cpu_core_update(struct gui_cpu_card *core, const struct cpu_core_info *info);
void gui_cpu_main_core_update(struct gui_cpu_card *cpu, const struct cpu_info *info);

void gui_cpu_page_update(struct gui_page_cpu *page, struct sysm_app *app);
void gui_cpu_page_init(struct gui_page_cpu *page, struct sysm_app *app);
void gui_cpu_page_destroy(struct gui_page_cpu *page);