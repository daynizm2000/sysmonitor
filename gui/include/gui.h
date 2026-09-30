#pragma once

#include "../../backend/include/sysmonitor.h"
#include "procs.h"
#include "disks.h"
#include <gtk/gtk.h>

#define GUI_GRAPH_SECS 60

struct gui_page {
        GtkWidget *box;
        
        const char *id;
        const char *name;
};

struct gui_graph_line {
        double history[GUI_GRAPH_SECS];
        GdkRGBA color;
        
        mlib_list_head_t list;

        bool __color;
};

struct gui_graph {
        GtkWidget *area;
        mlib_list_head_t lines;
        int passed_sec;

        double maxval;
};

struct gui_cpu_card {
        GtkWidget *frame;
        GtkWidget *box;

        GtkWidget *usage_label;
        GtkWidget *usage_ghz_label;

        mlib_list_head_t list;
};

struct gui_summary_mem_card {
        GtkWidget *frame;
        GtkWidget *box;

        GtkWidget *usage_pct_label;
        GtkWidget *usage_gib_label;
};

struct gui_summary_swap_card {
        GtkWidget *frame;
        GtkWidget *box;

        GtkWidget *usage_pct_label;
        GtkWidget *usage_gib_label;
};

struct gui_summary_network_card {
        GtkWidget *frame;
        GtkWidget *box;

        GtkWidget *read_speed_label;
        GtkWidget *write_speed_label;
};

struct gui_summary_disk_card {
        GtkWidget *frame;
        GtkWidget *box;

        GtkWidget *read_speed_label;
        GtkWidget *write_speed_label;
};

struct gui_summary_section {
        GtkWidget *frame;
        GtkWidget *box;

        struct gui_cpu_card cpu;
        struct gui_summary_mem_card mem;
        struct gui_summary_swap_card swap;
        struct gui_summary_network_card network;
        struct gui_summary_disk_card disk;
};

struct gui_graphs_card {
        GtkWidget *frame;
        struct gui_graph graph;
};

struct gui_graphs_section {
        GtkWidget *frame;
        GtkWidget *grid;

        struct gui_graphs_card cpu;
        struct gui_graphs_card mem;
        struct gui_graphs_card swap;
        struct gui_graphs_card network;
        struct gui_graphs_card disk;
        struct gui_graphs_card loadavg;
};

struct gui_tables_section {
        GtkWidget *box;

        struct gui_proc_table procs;
        struct gui_disk_table disks;
};

struct gui_page_overview {
        struct gui_page base;

        GtkWidget *uptime_label;

        struct gui_summary_section summary;
        struct gui_graphs_section graphs;
        struct gui_tables_section tables;
};

struct gui_page_cpu {
        struct gui_page base;

        struct gui_cpu_card total;
        mlib_list_head_t cores;
};

struct gui_page_procs {
        struct gui_page base;
        struct gui_proc_table procs;
};

struct gui_sysinfo_card {
        GtkWidget *frame;
        GtkWidget *info_label;
};

struct gui_page_sysinfo {
        struct gui_page base;
        
        struct gui_sysinfo_card system;
        struct gui_sysinfo_card motherboard;

        GtkWidget *about_label;
};

struct gui_app_running_card {
        time_t start_time;
        GtkWidget *label;
};

struct gui_app_refresh_rate_card {
        GtkWidget *box;
        GtkWidget *label;
        GtkWidget *dropdown;
};

struct app_gui {
        GtkApplication *app;

        struct gui_proc_store proc_store;

        struct gui_page_overview overview;
        struct gui_page_cpu cpu;
        struct gui_page_procs procs;
        struct gui_page_sysinfo sysinfo;

        guint timer_id;
        
        struct gui_app_running_card running;
        struct gui_app_refresh_rate_card refresh_rate;
};

struct app_backend {
        struct sysmonitor sysmon;
};

struct sysm_app {
        struct app_gui gui;
        struct app_backend backend;
};

sysm_errno_t sysm_app_init(struct sysm_app *app);
int sysm_app_run(struct sysm_app *app);
void sysm_app_destroy(struct sysm_app *app);