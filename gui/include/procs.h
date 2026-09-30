#pragma once

#include <gtk/gtk.h>
#include "../../backend/include/sysmonitor.h"
#include "../../libs/mlib-memory/libs/mlib/list/list/list.h"

struct gui_page_procs;
struct sysm_app;

#define GUI_TYPE_PROC_LINE (gui_proc_line_get_type())
G_DECLARE_FINAL_TYPE(GuiProcLine, gui_proc_line, GUI, PROC_LINE, GObject)

struct _GuiProcLine {
        GObject parent;

        pid_t pid;

        const struct proc_info *info;
};

struct gui_proc_table {
        GtkWidget *frame;
        GtkWidget *scrolled_win;
        GtkWidget *columnview;
        GListStore *store;
};

struct gui_proc_store {
        GListStore *lstore;
        GtkSingleSelection *sel;
        GHashTable *pid_to_idx;
        GArray *stale;
};

static inline pid_t gui_proc_line_pid(GuiProcLine *line)
{
        return line->pid;
}

static inline const struct proc_info *gui_proc_line_info(GuiProcLine *line)
{
        return line->info;
}

static inline void gui_proc_line_update(GuiProcLine *line, const struct proc_info *info)
{
        line->info = info;
        line->pid = proc_info_pid((struct proc_info*)info);
}

GuiProcLine *gui_proc_line_new(const struct proc_info *info);

void gui_proc_table_init(struct gui_proc_table *table, struct gui_proc_store *store, const char *title);

//void gui_procs_page_update(struct gui_page_procs *page, struct gui_app *app);
void gui_procs_page_init(struct gui_page_procs *page, struct sysm_app *app);

void gui_proc_store_init(struct gui_proc_store *store);
void gui_proc_store_update(struct gui_proc_store *store, mlib_list_head_t *procs);
void gui_proc_store_destroy(struct gui_proc_store *store);