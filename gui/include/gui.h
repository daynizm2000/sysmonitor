#pragma once

#include "../../backend/include/sysmonitor.h"
#include "types.h"
#include <gtk/gtk.h>

#define SYSM_GRAPH_SECS 60

struct sysm_page {
        GtkWidget *box;
        
        const char *id; // literal
        const char *name; // literal
};

struct sysm_graph_line {
        double history[SYSM_GRAPH_SECS];
        GdkRGBA color;
        
        mlib_list_head_t list;

        bool __color;
};

struct sysm_graph {
        GtkWidget *area;
        mlib_list_head_t lines;
        int passed_sec;

        double maxval;
};

struct sysm_cpu_box {
        GtkWidget *frame;
        GtkWidget *box;

        GtkWidget *usage_label;
        GtkWidget *usage_ghz_label;

        mlib_list_head_t list;
};

struct sysm_page_overview {
        struct sysm_page base;

        GtkWidget *uptime_label;

        struct {
                GtkWidget *frame;
                GtkWidget *box;

                struct sysm_cpu_box cpu;

                struct {
                        GtkWidget *frame;
                        GtkWidget *box;

                        GtkWidget *usage_pct_label;
                        GtkWidget *usage_gib_label;
                } mem;

                struct {
                        GtkWidget *frame;
                        GtkWidget *box;

                        GtkWidget *usage_pct_label;
                        GtkWidget *usage_gib_label;
                } swap;

                struct {
                        GtkWidget *frame;
                        GtkWidget *box;

                        GtkWidget *read_speed_label;
                        GtkWidget *write_speed_label;
                } network;

                struct {
                        GtkWidget *frame;
                        GtkWidget *box;

                        GtkWidget *read_speed_label;
                        GtkWidget *write_speed_label;
                } disk;
        } summary;

        struct {
                GtkWidget *frame;
                GtkWidget *grid;

                struct {
                        GtkWidget *frame;
                        struct sysm_graph graph;
                } cpu;

                struct {
                        GtkWidget *frame;
                        struct sysm_graph graph;
                } mem;

                struct {
                        GtkWidget *frame;
                        struct sysm_graph graph;
                } swap;

                struct {
                        GtkWidget *frame;
                        struct sysm_graph graph;
                } network;

                struct {
                        GtkWidget *frame;
                        struct sysm_graph graph;
                } disk;

                struct {
                        GtkWidget *frame;
                        struct sysm_graph graph;
                } loadavg;
        } graphs;

        struct {
                GtkWidget *box;

                struct sysm_proc_table procs;
                struct sysm_disk_table disks;
        } tables;
};

struct sysm_page_cpu {
        struct sysm_page base;

        struct sysm_cpu_box all;
        mlib_list_head_t cores;
};

struct sysm_page_procs {
        struct sysm_page base;
        struct sysm_proc_table procs;
};

struct sysm_page_sysinfo {
        struct sysm_page base;
        
        struct {
                GtkWidget *frame;
                GtkWidget *info_label;
        } system;

        struct {
                GtkWidget *frame;
                GtkWidget *info_label;
        } motherboard;

        GtkWidget *about_label;
};

struct sysm_app {
        struct {
                GtkApplication *app;

                struct sysm_proc_store proc_store;

                struct sysm_page_overview overview;
                struct sysm_page_cpu cpu;
                struct sysm_page_procs procs;
                struct sysm_page_sysinfo sysinfo;

                guint timer_id;
                
                struct {
                        time_t start_time;
                        GtkWidget *label;
                } running_for;

                struct {
                        GtkWidget *box;
                        GtkWidget *label;
                        GtkWidget *dropdown;
                } refresh_rate;
        } gui;

        struct {
                struct sysmonitor sysmon;
        } backend;
};

sysm_errno_t sysm_app_init(struct sysm_app *app);
int sysm_app_run(struct sysm_app *app);
void sysm_app_destroy(struct sysm_app *app);
