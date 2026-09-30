#pragma once

#include <gtk/gtk.h>
#include "../../libs/mlib-memory/libs/mlib/list/list/list.h"

#define GUI_TYPE_DISK_PART (gui_disk_part_get_type())
G_DECLARE_FINAL_TYPE(GuiDiskPart, gui_disk_part, GUI, DISK_PART, GObject)

struct _GuiDiskPart {
        GObject parent;
        const struct disk_part *info;
};

struct gui_disk_table {
        GtkWidget *frame;
        GtkWidget *scrolled_win;
        GtkWidget *columnview;
        GListStore *store;
};

GuiDiskPart *gui_disk_part_new(const struct disk_part *info);

void gui_disk_table_init(struct gui_disk_table *table, const char *title);
void gui_disk_table_update(struct gui_disk_table *table, mlib_list_head_t *parts);