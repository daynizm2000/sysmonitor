#pragma once

#include "gui.h"

SysmProcLine *sysm_proc_line_new(const struct proc_info *info);

void sysm_proc_table_init(struct sysm_proc_table *table, struct sysm_proc_store *store, const char *title);

//void gui_procs_page_update(struct sysm_page_procs *page, struct sysm_app *app);
void gui_procs_page_init(struct sysm_page_procs *page, struct sysm_app *app);

void sysm_proc_store_init(struct sysm_proc_store *store);
void sysm_proc_store_update(struct sysm_proc_store *store, mlib_list_head_t *procs);
void sysm_proc_store_destroy(struct sysm_proc_store *store);