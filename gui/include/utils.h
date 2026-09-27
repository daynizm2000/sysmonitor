#pragma once

#include "gui.h"

void gui_page_base_init(struct sysm_page *page, const char *id,
        const char *name, const char *title);

struct sysm_graph_line *gui_graph_line_create_init(GdkRGBA *color);
void gui_graph_line_free(struct sysm_graph_line *line);

void gui_graph_init(struct sysm_graph *graph, struct sysm_graph_line *line,
                void (*draw)(GtkDrawingArea*, cairo_t*, int, int, gpointer),
                void *arg, double maxval);

void gui_graph_update(struct sysm_graph *graph, const double *vals, int count);

void gui_columnview_add_column(GtkWidget *columnview, const char *title,
                void (*setup)(GtkSignalListItemFactory *f, GtkListItem *item, gpointer arg),
                void *arg,
                void (*bind)(GtkSignalListItemFactory *p, GtkListItem *item, gpointer arg),
                void *arg1);

void gui_columnview_set_expand_col(GtkWidget *columnview, int index);

static inline void gui_widget_set_margins(GtkWidget *widget,
        int left, int right, int top, int bottom)
{
        gtk_widget_set_margin_bottom(widget, bottom);
        gtk_widget_set_margin_end(widget, right);
        gtk_widget_set_margin_start(widget, left);
        gtk_widget_set_margin_top(widget, top);
}
