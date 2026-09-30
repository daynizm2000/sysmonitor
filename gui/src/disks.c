#include "../include/disks.h"
#include "../include/utils.h"

G_DEFINE_TYPE(GuiDiskPart, gui_disk_part, G_TYPE_OBJECT)

enum {
        GUI_DISK_TABLE_NAME_COL,
        GUI_DISK_TABLE_MPOINT_COL,
        GUI_DISK_TABLE_FS_COL,
        GUI_DISK_TABLE_USED_COL,
        GUI_DISK_TABLE_TOTAL_COL,
        GUI_DISK_TABLE_USAGE_COL
};

static void gui_disk_part_init(GuiDiskPart *self)
{
        (void)self;
}

static void gui_disk_part_class_init(GuiDiskPartClass *klass)
{
        (void)klass;
}

GuiDiskPart *gui_disk_part_new(const struct disk_part *info)
{
        GuiDiskPart *part = g_object_new(GUI_TYPE_DISK_PART, NULL);

        part->info = info;
        
        return part;
}

static void on_cell_setup(GtkSignalListItemFactory *f, GtkListItem *item, gpointer arg)
{
        (void)f;
        (void)arg;

        GtkWidget *label = gtk_label_new(NULL);
        gtk_list_item_set_child(item, label);
}

static void on_cell_bind(GtkSignalListItemFactory *f, GtkListItem *item, gpointer arg)
{
        (void)f;

        GuiDiskPart *part = GUI_DISK_PART(gtk_list_item_get_item(item));
        GtkWidget *label = gtk_list_item_get_child(item);
        int col_idx = GPOINTER_TO_INT(arg);
        char buf[64];
        const char *res = buf;

        switch (col_idx) {
                case GUI_DISK_TABLE_NAME_COL:
                        res = part->info->disk_name;
                        break;
                case GUI_DISK_TABLE_MPOINT_COL:
                        res = part->info->mount_point;
                        break;
                case GUI_DISK_TABLE_FS_COL:
                        res = part->info->fs;
                        break;
                case GUI_DISK_TABLE_USED_COL:
                        snprintf(buf, sizeof(buf), "%.1lf GiB", B_TO_GIB(part->info->used_b));
                        break;
                case GUI_DISK_TABLE_TOTAL_COL:
                        snprintf(buf, sizeof(buf), "%.1lf GiB", B_TO_GIB(part->info->total_b));
                        break;
                case GUI_DISK_TABLE_USAGE_COL:
                        snprintf(buf, sizeof(buf), "%.1lf%%", part->info->usage_pct);
                        break;
        }

        gtk_label_set_text(GTK_LABEL(label), res);
}

void gui_disk_table_init(struct gui_disk_table *table, const char *title)
{
        GtkSingleSelection *sel;

        table->frame = gtk_frame_new(NULL);

        if (title) {
                GtkWidget *label = gtk_label_new(NULL);
                gtk_label_set_markup(GTK_LABEL(label), title);
                gtk_frame_set_label_widget(GTK_FRAME(table->frame), label);
        }

        table->scrolled_win = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(table->scrolled_win, TRUE);

        table->store = g_list_store_new(GUI_TYPE_DISK_PART);
        sel = g_object_ref_sink(gtk_single_selection_new(G_LIST_MODEL(table->store)));
        gtk_single_selection_set_autoselect(sel, FALSE);

        table->columnview = gtk_column_view_new(GTK_SELECTION_MODEL(sel));
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(table->scrolled_win), table->columnview);

        gui_columnview_add_column(table->columnview, "Disk Name", on_cell_setup, NULL, on_cell_bind,
                        GUINT_TO_POINTER(GUI_DISK_TABLE_NAME_COL));

        gui_columnview_add_column(table->columnview, "Mount Point", on_cell_setup, NULL, on_cell_bind,
                        GUINT_TO_POINTER(GUI_DISK_TABLE_MPOINT_COL));

        gui_columnview_add_column(table->columnview, "Filesystem", on_cell_setup, NULL, on_cell_bind,
                        GUINT_TO_POINTER(GUI_DISK_TABLE_FS_COL));

        gui_columnview_add_column(table->columnview, "Used", on_cell_setup, NULL, on_cell_bind,
                        GUINT_TO_POINTER(GUI_DISK_TABLE_USED_COL));

        gui_columnview_add_column(table->columnview, "Total", on_cell_setup, NULL, on_cell_bind,
                        GUINT_TO_POINTER(GUI_DISK_TABLE_TOTAL_COL));

        gui_columnview_add_column(table->columnview, "Usage", on_cell_setup, NULL, on_cell_bind,
                        GUINT_TO_POINTER(GUI_DISK_TABLE_USAGE_COL));

        gui_columnview_set_expand_col(table->columnview, GUI_DISK_TABLE_MPOINT_COL);

        gtk_frame_set_child(GTK_FRAME(table->frame), table->scrolled_win);
}

void gui_disk_table_update(struct gui_disk_table *table, mlib_list_head_t *parts)
{
        struct disk_part *iter;

        g_list_store_remove_all(table->store);

        mlib_list_for_each_entry(iter, parts, list) {
                GuiDiskPart *part = gui_disk_part_new(iter);

                g_list_store_append(table->store, part);
                g_object_unref(part);
        } 
}