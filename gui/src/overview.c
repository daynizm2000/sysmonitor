#include "../include/gui.h"
#include "../include/utils.h"
#include "../include/procs.h"
#include "../include/disks.h"
#include "../include/cpu.h"
#include "gtk/gtk.h"

#define OVERVIEW_GRAPH_PADDING_LEFT 60
#define OVERVIEW_GRAPH_PADDING_BOTTOM 35
#define OVERVIEW_GRAPH_PADDING_RIGHT 15
#define OVERVIEW_GRAPH_PADDING_TOP 15
#define OVERVIEW_GRAPH_LEFT_VALS 3
#define OVERVIEW_GRAPH_GRID_VLINES 13
#define OVERVIEW_GRAPH_GRID_HLINES OVERVIEW_GRAPH_LEFT_VALS
#define OVERVIEW_GRAPH_TIME_FRAME "60 seconds"

#define HEX_COLOR_RED "#f38ba8"
#define HEX_COLOR_GREEN "#a6e3a1"
#define HEX_COLOR_BLUE "#1e66f5"

#define NETWORK_GRAPH_READLINE_HEX_COLOR HEX_COLOR_GREEN
#define NETWORK_GRAPH_WRITELINE_HEX_COLOR HEX_COLOR_RED

#define DISK_GRAPH_READLINE_HEX_COLOR HEX_COLOR_GREEN
#define DISK_GRAPH_WRITELINE_HEX_COLOR HEX_COLOR_RED

#define LOADAVG_GRAPH_1MIN_LINE_HEX_COLOR HEX_COLOR_GREEN
#define LOADAVG_GRAPH_5MIN_LINE_HEX_COLOR HEX_COLOR_BLUE
#define LOADAVG_GRAPH_15MIN_LINE_HEX_COLOR HEX_COLOR_RED

static struct sysm_graph_line *graph_line_create_init(GtkWidget *box, GdkRGBA *color,
                const char *markup)
{
        GtkWidget *label;
        struct sysm_graph_line *line = gui_graph_line_create_init(color);

        if (!line)
                return NULL;

        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), markup);

        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_widget_set_valign(label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(label, 10);

        gtk_box_append(GTK_BOX(box), label);

        return line;
}

static inline void uptime_init_add(struct sysm_page_overview *page)
{
        page->uptime_label = gtk_label_new("Uptime: 0 days, 0 hours, 0 mins, 0 secs");

        gtk_widget_set_halign(page->uptime_label, GTK_ALIGN_END);
        gtk_widget_set_margin_end(page->uptime_label, 10);

        gtk_box_append(GTK_BOX(page->base.box), page->uptime_label);
}

static inline void summary_cpu_init_add(struct sysm_page_overview *page)
{
        gui_cpu_main_core_init(&page->summary.cpu, "CPU Usage", false);

        gtk_widget_set_hexpand(page->summary.cpu.frame, TRUE);
        gui_widget_set_margins(page->summary.cpu.frame, 10, 10, 10, 10);

        gtk_box_append(GTK_BOX(page->summary.box), page->summary.cpu.frame);
}

static inline void summary_mem_init_add(struct sysm_page_overview *page)
{
        GtkWidget *title;

        page->summary.mem.frame = gtk_frame_new(NULL);
        page->summary.mem.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gtk_widget_set_hexpand(page->summary.mem.frame, TRUE);
        gui_widget_set_margins(page->summary.mem.frame, 10, 10, 10, 10);

        title = gtk_label_new("Memory Usage");
        gui_widget_set_margins(title, 10, 10, 10, 0);

        page->summary.mem.usage_pct_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(page->summary.mem.usage_pct_label), "<b>0%</b>");
        gui_widget_set_margins(page->summary.mem.usage_pct_label, 10, 10, 0, 0);

        page->summary.mem.usage_gib_label = gtk_label_new("0 / 0 GiB");
        gui_widget_set_margins(page->summary.mem.usage_gib_label, 10, 10, 0, 10);

        gtk_frame_set_child(GTK_FRAME(page->summary.mem.frame), page->summary.mem.box);
        gtk_box_append(GTK_BOX(page->summary.mem.box), title);
        gtk_box_append(GTK_BOX(page->summary.mem.box), page->summary.mem.usage_pct_label);
        gtk_box_append(GTK_BOX(page->summary.mem.box), page->summary.mem.usage_gib_label);

        gtk_box_append(GTK_BOX(page->summary.box), page->summary.mem.frame);
}

static inline void summary_swap_init_add(struct sysm_page_overview *page)
{
        GtkWidget *title;

        page->summary.swap.frame = gtk_frame_new(NULL);
        page->summary.swap.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gtk_widget_set_hexpand(page->summary.swap.frame, TRUE);
        gui_widget_set_margins(page->summary.swap.frame, 10, 10, 10, 10);

        title = gtk_label_new("Swap Usage");
        gui_widget_set_margins(title, 10, 10, 10, 0);

        page->summary.swap.usage_pct_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(page->summary.swap.usage_pct_label), "<b>0%</b>");
        gui_widget_set_margins(page->summary.swap.usage_pct_label, 10, 10, 0, 0);

        page->summary.swap.usage_gib_label = gtk_label_new("0 / 0 GiB");
        gui_widget_set_margins(page->summary.swap.usage_gib_label, 10, 10, 0, 10);

        gtk_frame_set_child(GTK_FRAME(page->summary.swap.frame), page->summary.swap.box);
        gtk_box_append(GTK_BOX(page->summary.swap.box), title);
        gtk_box_append(GTK_BOX(page->summary.swap.box), page->summary.swap.usage_pct_label);
        gtk_box_append(GTK_BOX(page->summary.swap.box), page->summary.swap.usage_gib_label);

        gtk_box_append(GTK_BOX(page->summary.box), page->summary.swap.frame);
}

static inline void summary_network_init_add(struct sysm_page_overview *page)
{
        GtkWidget *title;

        page->summary.network.frame = gtk_frame_new(NULL);
        page->summary.network.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gtk_widget_set_hexpand(page->summary.network.frame, TRUE);
        gui_widget_set_margins(page->summary.network.frame, 10, 10, 10, 10);

        title = gtk_label_new("Network (Total)");
        gui_widget_set_margins(title, 10, 10, 10, 0);

        page->summary.network.read_speed_label = gtk_label_new(" 0 MiB/s");
        gui_widget_set_margins(page->summary.network.read_speed_label, 10, 10, 0, 0);

        page->summary.network.write_speed_label = gtk_label_new(" 0 MiB/s");
        gui_widget_set_margins(page->summary.network.write_speed_label, 10, 10, 0, 10);

        gtk_frame_set_child(GTK_FRAME(page->summary.network.frame), page->summary.network.box);
        gtk_box_append(GTK_BOX(page->summary.network.box), title);
        gtk_box_append(GTK_BOX(page->summary.network.box), page->summary.network.read_speed_label);
        gtk_box_append(GTK_BOX(page->summary.network.box), page->summary.network.write_speed_label);

        gtk_box_append(GTK_BOX(page->summary.box), page->summary.network.frame);
}

static inline void summary_disk_init_add(struct sysm_page_overview *page)
{
        GtkWidget *title;

        page->summary.disk.frame = gtk_frame_new(NULL);
        page->summary.disk.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gtk_widget_set_hexpand(page->summary.disk.frame, TRUE);
        gui_widget_set_margins(page->summary.disk.frame, 10, 10, 10, 10);

        title = gtk_label_new("Disk (Total)");
        gui_widget_set_margins(title, 10, 10, 10, 0);

        page->summary.disk.read_speed_label = gtk_label_new("Read: 0 MiB/s");
        gui_widget_set_margins(page->summary.disk.read_speed_label, 10, 10, 0, 0);

        page->summary.disk.write_speed_label = gtk_label_new("Write: 0 MiB/s");
        gui_widget_set_margins(page->summary.disk.write_speed_label, 10, 10, 0, 10);

        gtk_frame_set_child(GTK_FRAME(page->summary.disk.frame), page->summary.disk.box);
        gtk_box_append(GTK_BOX(page->summary.disk.box), title);
        gtk_box_append(GTK_BOX(page->summary.disk.box), page->summary.disk.read_speed_label);
        gtk_box_append(GTK_BOX(page->summary.disk.box), page->summary.disk.write_speed_label);

        gtk_box_append(GTK_BOX(page->summary.box), page->summary.disk.frame);
}

static inline void summary_init_add(struct sysm_page_overview *page)
{
        GtkWidget *title;

        page->summary.frame = gtk_frame_new(NULL);
        page->summary.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        gui_widget_set_margins(page->summary.frame, 10, 10, 10, 10);

        title = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(title), "<b>Summary</b>");

        gtk_frame_set_label_widget(GTK_FRAME(page->summary.frame), title);
        gtk_widget_set_hexpand(page->summary.frame, TRUE);

        summary_cpu_init_add(page);
        summary_mem_init_add(page);
        summary_swap_init_add(page);
        summary_network_init_add(page);
        summary_disk_init_add(page);

        gtk_frame_set_child(GTK_FRAME(page->summary.frame), page->summary.box);
        gtk_box_append(GTK_BOX(page->base.box), page->summary.frame);
}

static void overview_graph_paint_grid(cairo_t *cr,
                unsigned int real_width, unsigned int real_height,
                const GdkRGBA *color)
{
        cairo_set_line_width(cr, 1);
        cairo_set_source_rgba(cr, color->red, color->green, color->blue, 0.12);

        for (int i = 0; i < OVERVIEW_GRAPH_GRID_HLINES; i++) {
                double ratio = (double)i / (OVERVIEW_GRAPH_GRID_HLINES - 1);
                double y = OVERVIEW_GRAPH_PADDING_TOP + (real_height * ratio);

                cairo_move_to(cr, OVERVIEW_GRAPH_PADDING_LEFT, y);
                cairo_line_to(cr, OVERVIEW_GRAPH_PADDING_LEFT + real_width, y);
        }

        for (int i = 0; i < OVERVIEW_GRAPH_GRID_VLINES; i++) {
                double x = OVERVIEW_GRAPH_PADDING_LEFT + real_width / (OVERVIEW_GRAPH_GRID_VLINES - 1.0) * i;

                cairo_move_to(cr, x, OVERVIEW_GRAPH_PADDING_TOP);
                cairo_line_to(cr, x, OVERVIEW_GRAPH_PADDING_TOP + real_height);
        }

        cairo_stroke(cr);
}

static void overview_graph_paint_time_frame(cairo_t *cr,
                int real_width, int height)
{
        cairo_text_extents_t extents;

        cairo_text_extents(cr, OVERVIEW_GRAPH_TIME_FRAME, &extents);
        cairo_move_to(cr,
                        OVERVIEW_GRAPH_PADDING_LEFT + (real_width / 2.0 - extents.width / 2),
                        height - 10);
        cairo_show_text(cr, OVERVIEW_GRAPH_TIME_FRAME);
}

static void graph_draw(GtkDrawingArea *area, cairo_t *cr,
                const char *vals[OVERVIEW_GRAPH_LEFT_VALS],
                int width, int height, struct sysm_graph *graph)
{
        GdkRGBA color;
        double vals_y_pos[OVERVIEW_GRAPH_LEFT_VALS] = {0, 0.5, 1};
        cairo_text_extents_t extents;
        int real_width;
        int real_height;
        struct sysm_graph_line *line;

        real_width = width - OVERVIEW_GRAPH_PADDING_LEFT - OVERVIEW_GRAPH_PADDING_RIGHT;
        real_height = height - OVERVIEW_GRAPH_PADDING_TOP - OVERVIEW_GRAPH_PADDING_BOTTOM;

        if (real_width <= 0 || real_height <= 0)
                return;

        gtk_widget_get_color(GTK_WIDGET(area), &color);
        cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);

        for (int i = 0; i < OVERVIEW_GRAPH_LEFT_VALS; i++) {
                cairo_text_extents(cr, vals[i], &extents);
                cairo_move_to(cr, OVERVIEW_GRAPH_PADDING_LEFT - extents.width - 10.0,
                                OVERVIEW_GRAPH_PADDING_TOP + (real_height * vals_y_pos[i] + extents.height / 2));
                cairo_show_text(cr, vals[i]);
        }

        overview_graph_paint_time_frame(cr, real_width, height);
        overview_graph_paint_grid(cr, real_width, real_height, &color);

        cairo_set_line_width(cr, 2);

        mlib_list_for_each_entry(line, &graph->lines, list) {
                if (line->__color)
                        cairo_set_source_rgba(cr, line->color.red, line->color.green, line->color.blue, 1.0);
                else
                        cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1.0);
        
                for (int i = 0; i < graph->passed_sec; i++) {
                        double val = line->history[i] / graph->maxval;
                        double x;
                        double y;

                        if (val < 0.0)
                                val = 0.0;
                        else if (val > 1.0)
                                val = 1.0; 

                        x = OVERVIEW_GRAPH_PADDING_LEFT + ((double)i * real_width / (SYSM_GRAPH_SECS - 1));
                        y = OVERVIEW_GRAPH_PADDING_TOP + (real_height * (1.0 - val));

                        if (!i)
                                cairo_move_to(cr, x, y);
                        else
                                cairo_line_to(cr, x, y);
                }

                cairo_stroke(cr);
        }
}

static void pct_draw(GtkDrawingArea *area, cairo_t *cr,
                        int width, int height, gpointer graph)
{
        const char *vals[] = {"100%", "50%", "0%"};
        graph_draw(area, cr, vals, width, height, graph);
}

static inline void graphs_cpu_init_add(struct sysm_page_overview *page)
{
        struct sysm_graph_line *line;
        
        line = gui_graph_line_create_init(NULL);

        if (!line)
                return;

        gui_graph_init(&page->graphs.cpu.graph, line, pct_draw, &page->graphs.cpu.graph, 100);

        page->graphs.cpu.frame = gtk_frame_new("CPU Usage (per core)");
        gui_widget_set_margins(page->graphs.cpu.frame, 10, 0, 10, 0);

        gtk_frame_set_child(GTK_FRAME(page->graphs.cpu.frame), page->graphs.cpu.graph.area);
        gtk_grid_attach(GTK_GRID(page->graphs.grid), page->graphs.cpu.frame, 0, 0, 1, 1);
}

static inline void graphs_mem_init_add(struct sysm_page_overview *page)
{
        struct sysm_graph_line *line;

        line = gui_graph_line_create_init(NULL);

        if (!line)
                return;

        gui_graph_init(&page->graphs.mem.graph, line, pct_draw, &page->graphs.mem.graph, 100);
        
        page->graphs.mem.frame = gtk_frame_new("Memory Usage");
        gui_widget_set_margins(page->graphs.mem.frame, 0, 0, 10, 0);

        gtk_frame_set_child(GTK_FRAME(page->graphs.mem.frame), page->graphs.mem.graph.area);
        gtk_grid_attach(GTK_GRID(page->graphs.grid), page->graphs.mem.frame, 1, 0, 1, 1);
}

static inline void graphs_swap_init_add(struct sysm_page_overview *page)
{
        struct sysm_graph_line *line;

        line = gui_graph_line_create_init(NULL);

        if (!line)
                return;

        gui_graph_init(&page->graphs.swap.graph, line, pct_draw, &page->graphs.swap.graph, 100);

        page->graphs.swap.frame = gtk_frame_new("Swap Usage");
        gui_widget_set_margins(page->graphs.swap.frame, 0, 10, 10, 0);

        gtk_frame_set_child(GTK_FRAME(page->graphs.swap.frame), page->graphs.swap.graph.area);
        gtk_grid_attach(GTK_GRID(page->graphs.grid), page->graphs.swap.frame, 2, 0, 1, 1);
}

static void network_draw(GtkDrawingArea *area, cairo_t *cr,
                        int width, int height, gpointer graph)
{
        const char *vals[] = {"10 MiB/s", "5 MiB/s", "0 MiB/s"};
        graph_draw(area, cr, vals, width, height, graph);
}

static inline void graphs_network_init_add(struct sysm_page_overview *page)
{
        struct sysm_graph_line *readline;
        struct sysm_graph_line *writeline;
        GdkRGBA readline_color;
        GdkRGBA writeline_color;
        GtkWidget *box;

        gdk_rgba_parse(&readline_color, NETWORK_GRAPH_READLINE_HEX_COLOR);
        gdk_rgba_parse(&writeline_color, NETWORK_GRAPH_WRITELINE_HEX_COLOR);

        page->graphs.network.frame = gtk_frame_new("Network I/O");
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        gui_widget_set_margins(page->graphs.network.frame, 10, 0, 0, 10);

        readline = graph_line_create_init(box, &readline_color,
                        "<span foreground='" NETWORK_GRAPH_READLINE_HEX_COLOR "'>■</span> receive");

        if (!readline)
                return;

        writeline = graph_line_create_init(box, &writeline_color,
                        "<span foreground='" NETWORK_GRAPH_WRITELINE_HEX_COLOR "'>■</span> send");

        if (!writeline) {
                gui_graph_line_free(readline);
                return;
        }

        mlib_list_add_tail(&writeline->list, &readline->list);
        gui_graph_init(&page->graphs.network.graph, readline, network_draw, &page->graphs.network.graph, 10);

        gtk_box_append(GTK_BOX(box), page->graphs.network.graph.area);
        gtk_frame_set_child(GTK_FRAME(page->graphs.network.frame), box);
        gtk_grid_attach(GTK_GRID(page->graphs.grid), page->graphs.network.frame, 0, 1, 1, 1);
}

static void disk_draw(GtkDrawingArea *area, cairo_t *cr,
                        int width, int height, gpointer graph)
{
        const char *vals[] = {"100 MiB/s", "50 MiB/s", "0 MiB/s"};
        graph_draw(area, cr, vals, width, height, graph);
}

static inline void graphs_disk_init_add(struct sysm_page_overview *page)
{
        struct sysm_graph_line *readline;
        struct sysm_graph_line *writeline;
        GdkRGBA readline_color;
        GdkRGBA writeline_color;
        GtkWidget *box;

        gdk_rgba_parse(&readline_color, DISK_GRAPH_READLINE_HEX_COLOR);
        gdk_rgba_parse(&writeline_color, DISK_GRAPH_WRITELINE_HEX_COLOR);

        page->graphs.disk.frame = gtk_frame_new("Disk I/O");
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        gui_widget_set_margins(page->graphs.disk.frame, 0, 0, 0, 10);

        readline = graph_line_create_init(box, &readline_color,
                        "<span foreground='" DISK_GRAPH_READLINE_HEX_COLOR "'>■</span> read");

        if (!readline)
                return;

        writeline = graph_line_create_init(box, &writeline_color,
                        "<span foreground='" DISK_GRAPH_WRITELINE_HEX_COLOR "'>■</span> write");

        if (!writeline) {
                gui_graph_line_free(readline);
                return;
        }

        mlib_list_add_tail(&writeline->list, &readline->list);
        gui_graph_init(&page->graphs.disk.graph, readline, disk_draw, &page->graphs.disk.graph, 100);

        gtk_box_append(GTK_BOX(box), page->graphs.disk.graph.area);
        gtk_frame_set_child(GTK_FRAME(page->graphs.disk.frame), box);
        gtk_grid_attach(GTK_GRID(page->graphs.grid), page->graphs.disk.frame, 1, 1, 1, 1);
}

static void loadavg_draw(GtkDrawingArea *area, cairo_t *cr,
                        int width, int height, gpointer graph)
{
        const char *vals[] = {"2.0", "1.0", "0.0"};
        graph_draw(area, cr, vals, width, height, graph);
}

static inline void graphs_loadavg_init_add(struct sysm_page_overview *page)
{
        struct sysm_graph_line *min1_line = NULL;
        struct sysm_graph_line *min5_line = NULL;
        struct sysm_graph_line *min15_line;
        GdkRGBA min1_color;
        GdkRGBA min5_color;
        GdkRGBA min15_color;
        GtkWidget *box;

        gdk_rgba_parse(&min1_color, LOADAVG_GRAPH_1MIN_LINE_HEX_COLOR);
        gdk_rgba_parse(&min5_color, LOADAVG_GRAPH_5MIN_LINE_HEX_COLOR);
        gdk_rgba_parse(&min15_color, LOADAVG_GRAPH_15MIN_LINE_HEX_COLOR);

        page->graphs.loadavg.frame = gtk_frame_new("Load Average");
        box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        gui_widget_set_margins(page->graphs.loadavg.frame, 0, 10, 0, 10);

        min1_line = graph_line_create_init(box, &min1_color,
                        "<span foreground='" LOADAVG_GRAPH_1MIN_LINE_HEX_COLOR "'>■</span> 1 minute");

        if (!min1_line)
                return;

        min5_line = graph_line_create_init(box, &min5_color,
                        "<span foreground='" LOADAVG_GRAPH_5MIN_LINE_HEX_COLOR "'>■</span> 5 minute");

        if (!min5_line)
                goto fail;

        min15_line = graph_line_create_init(box, &min15_color,
                        "<span foreground='" LOADAVG_GRAPH_15MIN_LINE_HEX_COLOR "'>■</span> 15 minute");

        if (!min15_line)
                goto fail;

        mlib_list_add_tail(&min5_line->list, &min1_line->list);
        mlib_list_add_tail(&min15_line->list, &min1_line->list);

        gui_graph_init(&page->graphs.loadavg.graph, min1_line, loadavg_draw, &page->graphs.loadavg.graph, 2);

        gtk_box_append(GTK_BOX(box), page->graphs.loadavg.graph.area);
        gtk_frame_set_child(GTK_FRAME(page->graphs.loadavg.frame), box);
        gtk_grid_attach(GTK_GRID(page->graphs.grid), page->graphs.loadavg.frame, 2, 1, 1, 1);

        return;
fail:
        if (min1_line)
                gui_graph_line_free(min1_line);
        if (min5_line)
                gui_graph_line_free(min5_line);
}

static inline void graphs_init_add(struct sysm_page_overview *page)
{
        GtkWidget *title;

        page->graphs.frame = gtk_frame_new(NULL);
        page->graphs.grid = gtk_grid_new();

        title = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(title), "<b>Graphs</b>");
        gtk_frame_set_label_widget(GTK_FRAME(page->graphs.frame), title);
        gtk_widget_set_hexpand(page->graphs.frame, TRUE);

        gui_widget_set_margins(page->graphs.frame, 10, 10, 10, 10);
        gtk_grid_set_row_spacing(GTK_GRID(page->graphs.grid), 10);
        gtk_grid_set_column_spacing(GTK_GRID(page->graphs.grid), 10);
        gtk_grid_set_row_homogeneous(GTK_GRID(page->graphs.grid), TRUE);
        gtk_grid_set_column_homogeneous(GTK_GRID(page->graphs.grid), TRUE);

        graphs_cpu_init_add(page);
        graphs_mem_init_add(page);
        graphs_swap_init_add(page);
        graphs_network_init_add(page);
        graphs_disk_init_add(page);
        graphs_loadavg_init_add(page);

        gtk_frame_set_child(GTK_FRAME(page->graphs.frame), page->graphs.grid);
        gtk_box_append(GTK_BOX(page->base.box), page->graphs.frame);
}

static inline void procs_init_add(struct sysm_page_overview *page, struct sysm_proc_store *proc_store)
{
        sysm_proc_table_init(&page->tables.procs, proc_store, "<b>Top Processes</b>");

        gtk_widget_set_hexpand(page->tables.procs.frame, TRUE);
        gtk_widget_set_vexpand(page->tables.procs.frame, TRUE);

        gtk_widget_set_hexpand(page->tables.procs.scrolled_win, TRUE);
        gtk_widget_set_vexpand(page->tables.procs.scrolled_win, TRUE);

        gtk_widget_set_hexpand(page->tables.procs.columnview, TRUE);
        gtk_widget_set_vexpand(page->tables.procs.columnview, TRUE);

        gui_widget_set_margins(page->tables.procs.frame, 10, 10, 0, 10);

        gtk_box_append(GTK_BOX(page->tables.box), page->tables.procs.frame);
}

static inline void disks_init_add(struct sysm_page_overview *page)
{
        sysm_disk_table_init(&page->tables.disks, "<b>Disk Usage</b>");

        gtk_widget_set_hexpand(page->tables.disks.frame, TRUE);
        gtk_widget_set_vexpand(page->tables.disks.frame, TRUE);

        gtk_widget_set_hexpand(page->tables.disks.scrolled_win, TRUE);
        gtk_widget_set_vexpand(page->tables.disks.scrolled_win, TRUE);

        gtk_widget_set_hexpand(page->tables.disks.columnview, TRUE);
        gtk_widget_set_vexpand(page->tables.disks.columnview, TRUE);

        gui_widget_set_margins(page->tables.disks.frame, 0, 10, 0, 10);

        gtk_box_append(GTK_BOX(page->tables.box), page->tables.disks.frame);
}

static inline void tables_init_add(struct sysm_page_overview *page, struct sysm_app *app)
{
        page->tables.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_widget_set_hexpand(page->tables.box, TRUE);
        gtk_widget_set_vexpand(page->tables.box, TRUE);

        procs_init_add(page, &app->gui.proc_store);
        disks_init_add(page);

        gtk_box_append(GTK_BOX(page->base.box), page->tables.box);
}

void gui_overview_page_init(struct sysm_page_overview *page, struct sysm_app *app)
{
        gui_page_base_init(&page->base, "overview", "Overview", "<b>Overview</b>");

        uptime_init_add(page);
        summary_init_add(page);
        graphs_init_add(page);
        tables_init_add(page, app);
}

static void graph_lines_destroy(mlib_list_head_t *lines)
{
        struct sysm_graph_line *iter;
        struct sysm_graph_line *tmp;

        mlib_list_for_each_entry_safe(iter, tmp, lines, list) {
                mlib_list_del(&iter->list);
                gui_graph_line_free(iter);
        }
}

void gui_overview_page_destroy(struct sysm_page_overview *page)
{
        graph_lines_destroy(&page->graphs.cpu.graph.lines);
        graph_lines_destroy(&page->graphs.mem.graph.lines);
        graph_lines_destroy(&page->graphs.disk.graph.lines);
        graph_lines_destroy(&page->graphs.network.graph.lines);
        graph_lines_destroy(&page->graphs.swap.graph.lines);
        graph_lines_destroy(&page->graphs.loadavg.graph.lines);
}

static inline void uptime_update(struct sysm_page_overview *page,
        unsigned long uptime_sec)
{
        char buf[64];
        unsigned long days = uptime_sec / 86400;
        unsigned long hours = (uptime_sec % 86400) / 3600;
        unsigned long mins = (uptime_sec % 3600) / 60;
        unsigned long secs = uptime_sec % 60;

        snprintf(buf, sizeof(buf), "Uptime: %lu days, %lu hours, %lu mins, %lu secs",
                days, hours, mins, secs);

        gtk_label_set_text(GTK_LABEL(page->uptime_label), buf);
}

static inline void summary_cpu_update(struct sysm_page_overview *page, const struct cpu_info *info)
{
        gui_cpu_main_core_update(&page->summary.cpu, info);
}

static inline void summary_mem_update(struct sysm_page_overview *page, const struct mem_info *info)
{
        char buf[128];

        snprintf(buf, sizeof(buf), "<b>%.1lf%%</b>", info->usage_pct);
        gtk_label_set_markup(GTK_LABEL(page->summary.mem.usage_pct_label), buf);

        snprintf(buf, sizeof(buf), "%.1lf / %.1lf GiB", info->used_gib, info->total_gib);
        gtk_label_set_text(GTK_LABEL(page->summary.mem.usage_gib_label), buf);
}

static inline void summary_swap_update(struct sysm_page_overview *page, const struct swap_info *info)
{
        char buf[128];

        snprintf(buf, sizeof(buf), "<b>%.1lf%%</b>", info->usage_pct);
        gtk_label_set_markup(GTK_LABEL(page->summary.swap.usage_pct_label), buf);

        snprintf(buf, sizeof(buf), "%.1lf / %.1lf GiB", info->used_gib, info->total_gib);
        gtk_label_set_text(GTK_LABEL(page->summary.swap.usage_gib_label), buf);
}

static inline void summary_network_update(struct sysm_page_overview *page, const struct network_info *info)
{
        char buf[128];

        snprintf(buf, sizeof(buf), " %.1lf MiB/s", SYSM_B_TO_MIB(info->read_speed_b_sec));
        gtk_label_set_text(GTK_LABEL(page->summary.network.read_speed_label), buf);

        snprintf(buf, sizeof(buf), " %.1lf MiB/s", SYSM_B_TO_MIB(info->write_speed_b_sec));
        gtk_label_set_text(GTK_LABEL(page->summary.network.write_speed_label), buf);
}

static inline void summary_disk_update(struct sysm_page_overview *page, const struct disk_info *info)
{
        char buf[128];

        snprintf(buf, sizeof(buf), "Read: %.1lf MiB/s", SYSM_B_TO_MIB(info->read_speed_b_sec));
        gtk_label_set_text(GTK_LABEL(page->summary.disk.read_speed_label), buf);

        snprintf(buf, sizeof(buf), "Write: %.1lf MiB/s", SYSM_B_TO_MIB(info->write_speed_b_sec));
        gtk_label_set_text(GTK_LABEL(page->summary.disk.write_speed_label), buf);
}

static inline void summary_update(struct sysm_page_overview *page, const struct sysmonitor *sysmon)
{
        summary_cpu_update(page, &sysmon->cpu);
        summary_mem_update(page, &sysmon->mem);
        summary_swap_update(page, &sysmon->swap);
        summary_network_update(page, &sysmon->network);
        summary_disk_update(page, &sysmon->disk);
}

static inline void graphs_cpu_update(struct sysm_page_overview *page, const struct cpu_info *info)
{
        gui_graph_update(&page->graphs.cpu.graph, &info->total_usage_pct, 1);
}

static inline void graphs_mem_update(struct sysm_page_overview *page, const struct mem_info *info)
{
        gui_graph_update(&page->graphs.mem.graph, &info->usage_pct, 1);
}

static inline void graphs_swap_update(struct sysm_page_overview *page, const struct swap_info *info)
{
        gui_graph_update(&page->graphs.swap.graph, &info->usage_pct, 1);
}

static inline void graphs_network_update(struct sysm_page_overview *page, const struct network_info *info)
{
        double vals[] = {
                SYSM_B_TO_MIB(info->read_speed_b_sec),
                SYSM_B_TO_MIB(info->write_speed_b_sec)
        };

        gui_graph_update(&page->graphs.network.graph, vals, sizeof(vals) / sizeof(*vals));
}

static inline void graphs_disk_update(struct sysm_page_overview *page, const struct disk_info *info)
{
        double vals[] = {
                SYSM_B_TO_MIB(info->read_speed_b_sec),
                SYSM_B_TO_MIB(info->write_speed_b_sec)
        };

        gui_graph_update(&page->graphs.disk.graph, vals, sizeof(vals) / sizeof(*vals));
}

static inline void graphs_loadavg_update(struct sysm_page_overview *page, const double loadavg[3])
{
        gui_graph_update(&page->graphs.loadavg.graph, loadavg, 3);
}

static inline void graphs_update(struct sysm_page_overview *page, const struct sysmonitor *sysmon)
{
        graphs_cpu_update(page, &sysmon->cpu);
        graphs_mem_update(page, &sysmon->mem);
        graphs_swap_update(page, &sysmon->swap);
        graphs_network_update(page, &sysmon->network);
        graphs_disk_update(page, &sysmon->disk);
        graphs_loadavg_update(page, sysmon->load_avg);
}

static inline void disks_update(struct sysm_page_overview *page, mlib_list_head_t *parts)
{
        sysm_disk_table_update(&page->tables.disks, parts);
}

static inline void tables_update(struct sysm_page_overview *page, struct sysm_app *app)
{
        disks_update(page, &app->backend.sysmon.disk.parts);
}

void gui_overview_page_update(struct sysm_page_overview *page, struct sysm_app *app)
{
        uptime_update(page, app->backend.sysmon.uptime_sec);
        summary_update(page, &app->backend.sysmon);
        graphs_update(page, &app->backend.sysmon);
        tables_update(page, app);
}
