#include "../include/procs.h"
#include "../include/utils.h"
#include "gtk/gtk.h"
#include "gtk/gtksingleselection.h"

G_DEFINE_TYPE(SysmProcLine, sysm_proc_line, G_TYPE_OBJECT)

enum {
        SYSM_PROC_TABLE_PID_COL,
        SYSM_PROC_TABLE_NAME_COL,
        SYSM_PROC_TABLE_CPU_COL,
        SYSM_PROC_TABLE_MEM_COL,
        SYSM_PROC_TABLE_RSS_COL,
        SYSM_PROC_TABLE_THREADS_COL
};

static void sysm_proc_line_init(SysmProcLine *self)
{
        (void)self;
}

static void sysm_proc_line_class_init(SysmProcLineClass *klass)
{
        (void)klass;
}

SysmProcLine *sysm_proc_line_new(const struct proc_info *info)
{
        SysmProcLine *l = g_object_new(SYSM_TYPE_PROC_LINE, NULL);

        l->info = info;
        l->pid = info->pid;

        return l;
}

static int proc_line_compare_cpu_desc(gconstpointer a, gconstpointer b, gpointer arg)
{
        (void)arg;

        const SysmProcLine *la = SYSM_PROC_LINE((gpointer)a);
        const SysmProcLine *lb = SYSM_PROC_LINE((gpointer)b);
 
        if (la->info->cpu_usage_pct > lb->info->cpu_usage_pct)
                return -1;
        if (la->info->cpu_usage_pct < lb->info->cpu_usage_pct)
                return 1;

        return 0;
}

static int guint_cmp_desc(gconstpointer a, gconstpointer b)
{
        guint ua = *(const guint *)a;
        guint ub = *(const guint *)b;

        return (ua < ub) - (ua > ub);
}

void sysm_proc_store_init(struct sysm_proc_store *store)
{
        GtkSorter *sorter;
        GtkSortListModel *sort_model;

        store->lstore = g_list_store_new(SYSM_TYPE_PROC_LINE);
        store->pid_to_idx = g_hash_table_new(g_direct_hash, g_direct_equal);
        store->stale = g_array_new(FALSE, FALSE, sizeof(guint));

        sorter = GTK_SORTER(gtk_custom_sorter_new(proc_line_compare_cpu_desc, NULL, NULL));
        sort_model = gtk_sort_list_model_new(G_LIST_MODEL(store->lstore), sorter);
        store->sel = g_object_ref_sink(gtk_single_selection_new(G_LIST_MODEL(sort_model)));
        gtk_single_selection_set_autoselect(store->sel, FALSE);
}

void sysm_proc_store_destroy(struct sysm_proc_store *store)
{
        g_hash_table_destroy(store->pid_to_idx);
        g_array_free(store->stale, TRUE);
}

void sysm_proc_store_update(struct sysm_proc_store *store, mlib_list_head_t *procs)
{
        struct proc_info *iter;
        guint n;
        gboolean any_updated = FALSE;

        n = g_list_model_get_n_items(G_LIST_MODEL(store->lstore));

        for (guint i = 0; i < n; i++) {
                SysmProcLine *line = g_list_model_get_item(G_LIST_MODEL(store->lstore), i);
                g_hash_table_insert(store->pid_to_idx, GUINT_TO_POINTER(line->pid), GUINT_TO_POINTER(i));
                g_object_unref(line);
        }

        mlib_list_for_each_entry(iter, procs, list) {
                gpointer idx_ptr;

                if (g_hash_table_lookup_extended(store->pid_to_idx, GUINT_TO_POINTER(iter->pid), NULL, &idx_ptr)) {
                        guint idx = GPOINTER_TO_UINT(idx_ptr);
                        SysmProcLine *line = g_list_model_get_item(G_LIST_MODEL(store->lstore), idx);

                        line->info = iter;
                        line->pid = iter->pid;

                        g_object_unref(line);
                        g_hash_table_remove(store->pid_to_idx, GUINT_TO_POINTER(iter->pid));
                        any_updated = TRUE;
                } else {
                        SysmProcLine *line = sysm_proc_line_new(iter);

                        g_list_store_append(store->lstore, line);
                        g_object_unref(line);
                }
        }

        if (any_updated) {
                guint current_n = g_list_model_get_n_items(G_LIST_MODEL(store->lstore));
                g_list_model_items_changed(G_LIST_MODEL(store->lstore), 0, current_n, current_n);
        }

        if (g_hash_table_size(store->pid_to_idx) > 0) {
                GHashTableIter hiter;
                gpointer key, val;

                g_hash_table_iter_init(&hiter, store->pid_to_idx);

                while (g_hash_table_iter_next(&hiter, &key, &val)) {
                        guint idx = GPOINTER_TO_UINT(val);
                        g_array_append_val(store->stale, idx);
                }

                g_array_sort(store->stale, guint_cmp_desc);

                for (guint i = 0; i < store->stale->len; i++)
                        g_list_store_remove(store->lstore, g_array_index(store->stale, guint, i));

                g_array_set_size(store->stale, 0);
                g_hash_table_remove_all(store->pid_to_idx);
        }
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

        SysmProcLine *line = SYSM_PROC_LINE(gtk_list_item_get_item(item));
        GtkWidget *label = gtk_list_item_get_child(item);
        int col_idx = GPOINTER_TO_INT(arg);
        char buf[64];
        const char *res = buf;

        switch (col_idx) {
                case SYSM_PROC_TABLE_PID_COL:
                        snprintf(buf, sizeof(buf), "%u", line->info->pid);
                        break;
                case SYSM_PROC_TABLE_NAME_COL:
                        res = line->info->name;
                        break;
                case SYSM_PROC_TABLE_CPU_COL:
                        snprintf(buf, sizeof(buf), "%.1lf", line->info->cpu_usage_pct);
                        break;
                case SYSM_PROC_TABLE_MEM_COL:
                        snprintf(buf, sizeof(buf), "%.1lf", line->info->mem_usage_pct);
                        break;
                case SYSM_PROC_TABLE_RSS_COL:
                        snprintf(buf, sizeof(buf), "%.1lf MiB", SYSM_B_TO_MIB(line->info->rss_b));
                        break;
                case SYSM_PROC_TABLE_THREADS_COL:
                        snprintf(buf, sizeof(buf), "%u", line->info->threads);
                        break;
        }

        gtk_label_set_text(GTK_LABEL(label), res);
}

void sysm_proc_table_init(struct sysm_proc_table *table, struct sysm_proc_store *store, const char *title)
{
        table->frame = gtk_frame_new(NULL);

        if (title) {
                GtkWidget *label = gtk_label_new(NULL);

                gtk_label_set_markup(GTK_LABEL(label), title);
                gtk_frame_set_label_widget(GTK_FRAME(table->frame), label);
        }

        table->scrolled_win = gtk_scrolled_window_new();
        gtk_widget_set_vexpand(table->scrolled_win, TRUE);

        table->store = store->lstore;

        table->columnview = gtk_column_view_new(GTK_SELECTION_MODEL(store->sel));
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(table->scrolled_win), table->columnview);

        gui_columnview_add_column(table->columnview, "PID", on_cell_setup, NULL, on_cell_bind,
                        GINT_TO_POINTER(SYSM_PROC_TABLE_PID_COL));

        gui_columnview_add_column(table->columnview, "Name", on_cell_setup, NULL, on_cell_bind,
                        GINT_TO_POINTER(SYSM_PROC_TABLE_NAME_COL));

        gui_columnview_add_column(table->columnview, "CPU%", on_cell_setup, NULL, on_cell_bind,
                        GINT_TO_POINTER(SYSM_PROC_TABLE_CPU_COL));

        gui_columnview_add_column(table->columnview, "MEM%", on_cell_setup, NULL, on_cell_bind,
                        GINT_TO_POINTER(SYSM_PROC_TABLE_MEM_COL));

        gui_columnview_add_column(table->columnview, "RSS", on_cell_setup, NULL, on_cell_bind,
                        GINT_TO_POINTER(SYSM_PROC_TABLE_RSS_COL));

        gui_columnview_add_column(table->columnview, "Threads", on_cell_setup, NULL, on_cell_bind,
                        GINT_TO_POINTER(SYSM_PROC_TABLE_THREADS_COL));

        gui_columnview_set_expand_col(table->columnview, SYSM_PROC_TABLE_NAME_COL);

        gtk_frame_set_child(GTK_FRAME(table->frame), table->scrolled_win);
        gtk_widget_set_margin_end(table->frame, 10);
        gtk_widget_set_margin_bottom(table->frame, 10);
}

void gui_procs_page_init(struct sysm_page_procs *page, struct sysm_app *app)
{
        gui_page_base_init(&page->base, "processes", "Processes", "<b>Processes</b>");
        sysm_proc_table_init(&page->procs, &app->gui.proc_store, NULL);
        gtk_box_append(GTK_BOX(page->base.box), page->procs.frame);
}
