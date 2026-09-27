#pragma once

#include "gui.h"

void gui_cpu_core_init(struct sysm_cpu_box *core, const char *title, bool is_markup);
void gui_cpu_main_core_init(struct sysm_cpu_box *cpu, const char *title, bool is_markup);
struct sysm_cpu_box *gui_cpu_core_create_init(const char *title, bool is_markup);
void gui_cpu_core_free(struct sysm_cpu_box *core);
void gui_cpu_core_update(struct sysm_cpu_box *core, const struct cpu_core_info *info);
void gui_cpu_main_core_update(struct sysm_cpu_box *cpu, const struct cpu_info *info);

void gui_cpu_page_update(struct sysm_page_cpu *page, struct sysm_app *app);
void gui_cpu_page_init(struct sysm_page_cpu *page, struct sysm_app *app);
void gui_cpu_page_destroy(struct sysm_page_cpu *page);