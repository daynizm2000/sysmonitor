#include "../include/utils.h"

void gui_page_base_init(struct gui_page *page, const char *id,
        const char *name, const char *title)
{
        GtkWidget *label;

        page->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        page->id = id;
        page->name = name;

        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), title);
        gtk_box_append(GTK_BOX(page->box), label);
}

struct gui_graph_line *gui_graph_line_create_init(GdkRGBA *color)
{
        struct gui_graph_line *line = malloc(sizeof(struct gui_graph_line));

        if (!line)
                sysm_log_ret(NULL, SYSM_ERR "failed to memory allocation in libc malloc\n");

        if (color) {
                line->color = *color;
                line->__color = true;
        }
        else
                line->__color = false;

        mlib_list_head_init(&line->list);

        return line;
}

void gui_graph_line_free(struct gui_graph_line *line)
{
        free(line);
}

void gui_graph_init(struct gui_graph *graph, struct gui_graph_line *line,
                void (*draw)(GtkDrawingArea*, cairo_t*, int, int, gpointer),
                void *arg, double maxval)
{
        graph->passed_sec = 0;
        graph->maxval = maxval;
        graph->area = gtk_drawing_area_new();

        gtk_widget_set_hexpand(graph->area, TRUE);
        gtk_widget_set_vexpand(graph->area, TRUE);
        
        gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(graph->area), draw, arg, NULL);

        mlib_list_head_init(&graph->lines);

        if (line) {
                graph->lines.next = &line->list;
                graph->lines.prev = line->list.prev;
                line->list.prev->next = &graph->lines;
                line->list.prev = &graph->lines;
        }
}

static inline void graph_line_history_update(struct gui_graph_line *line, double val, int sec)
{
        unsigned int step = sysm_update_interval_sec;

        if (sec >= GUI_GRAPH_SECS) {
                if (step >= GUI_GRAPH_SECS) {
                        for (int i = 0; i < GUI_GRAPH_SECS; i++)
                                line->history[i] = val;

                        return;
                }

                memmove(line->history, &line->history[step], (GUI_GRAPH_SECS - step) * sizeof(double));

                for (int i = GUI_GRAPH_SECS - step; i < GUI_GRAPH_SECS; i++)
                        line->history[i] = val;
        }
        else {
                int start = sec - step + 1;

                if (start < 0)
                        start = 0;

                for (int i = start; i <= sec && i < GUI_GRAPH_SECS; i++)
                        line->history[i] = val;
        }
}

void gui_graph_update(struct gui_graph *graph, const double *vals, int count)
{
        struct gui_graph_line *iter;
        int i = 0;

        graph->passed_sec += sysm_update_interval_sec;

        if (graph->passed_sec > GUI_GRAPH_SECS)
                graph->passed_sec = GUI_GRAPH_SECS;

        mlib_list_for_each_entry(iter, &graph->lines, list) {
                double val;

                if (i >= count)
                        break;

                val = vals[i++];
                
                graph_line_history_update(iter, val, graph->passed_sec);
        }

        gtk_widget_queue_draw(GTK_WIDGET(graph->area));
}

void gui_columnview_add_column(GtkWidget *columnview, const char *title,
                void (*setup)(GtkSignalListItemFactory *f, GtkListItem *item, gpointer arg),
                void *arg,
                void (*bind)(GtkSignalListItemFactory *p, GtkListItem *item, gpointer arg),
                void *arg1)
{
        GtkListItemFactory *fac = gtk_signal_list_item_factory_new();
        GtkColumnViewColumn *col = gtk_column_view_column_new(title, fac);

        g_signal_connect(fac, "setup", G_CALLBACK(setup), arg);
        g_signal_connect(fac, "bind", G_CALLBACK(bind), arg1);
        gtk_column_view_append_column(GTK_COLUMN_VIEW(columnview), col);

        g_object_unref(col);
}

void gui_columnview_set_expand_col(GtkWidget *columnview, int index)
{
        GListModel *cols = G_LIST_MODEL(gtk_column_view_get_columns(GTK_COLUMN_VIEW(columnview)));
        GtkColumnViewColumn *col = g_list_model_get_item(cols, index);

        gtk_column_view_column_set_expand(col, TRUE);

        g_object_unref(col);
}
