#include "../include/overview.h"
#include "../include/cpu.h"
#include "../include/procs.h"
#include "../include/sysinfo.h"
#include "../include/utils.h"
#include "gtk/gtk.h"
#include <wchar.h>

#define GUI_APP_NAME "com.sysmonitor.app"
#define GUI_TOOLTIP "SysMonitor"

#define GUI_WINDOW_WIDTH_DEFAULT_PX 1000
#define GUI_WINDOW_HEIGHT_DEFAULT_PX 1000

static inline void gui_running_update(struct sysm_app *app)
{
        char buf[64];
        time_t diff = time(NULL) - app->gui.running.start_time;

        snprintf(buf, sizeof(buf), "Running for: %02ld:%02ld:%02ld",
                        diff / 3600, (diff % 3600) / 60, diff % 60);
        
        gtk_label_set_text(GTK_LABEL(app->gui.running.label), buf);
}

static void gui_update(struct sysm_app *app)
{
        gui_running_update(app);
        gui_proc_store_update(&app->gui.proc_store, &app->backend.sysmon.proc_table.list);
        gui_overview_page_update(&app->gui.overview, app);
        gui_cpu_page_update(&app->gui.cpu, app);
        // gui_procs_page_update(&app->gui.procs, app);
        // gui_sysinfo_page_update(&app->gui.sysinfo);
}

static gboolean gui_update_tick(gpointer arg)
{
        struct sysm_app *app = arg;

        sysm_update_last(&app->backend.sysmon);
        gui_update(app);
        sysm_update_first(&app->backend.sysmon);

        return G_SOURCE_CONTINUE;
}

static void gui_window_destroy(GtkWidget *widget, gpointer arg)
{
        struct sysm_app *app = arg;

        (void)widget;

        if (app->gui.timer_id > 0) {
                g_source_remove(app->gui.timer_id);
                app->gui.timer_id = 0;
        }
}

static GtkWidget *gui_window_create_init(struct app_gui *app)
{
        GtkWidget *window = gtk_application_window_new(app->app);

        gtk_window_set_title(GTK_WINDOW(window), GUI_TOOLTIP);
        gtk_window_set_default_size(GTK_WINDOW(window),
                GUI_WINDOW_WIDTH_DEFAULT_PX, GUI_WINDOW_HEIGHT_DEFAULT_PX); 

        return window;
}

static GtkWidget *gui_navigation_widget_create_init(void)
{
        GtkWidget *sidebar;

        sidebar = gtk_stack_sidebar_new();

        gtk_widget_set_vexpand(sidebar, TRUE);
        
        return sidebar;
}

static void gui_content_widgets_add_init(struct sysm_app *app)
{
        gui_overview_page_init(&app->gui.overview, app);
        gui_cpu_page_init(&app->gui.cpu, app);
        gui_procs_page_init(&app->gui.procs, app);
        gui_sysinfo_page_init(&app->gui.sysinfo, app);
}

static GtkWidget *gui_content_stack_create_init(struct app_gui *app)
{
        GtkWidget *stack;

        stack = gtk_stack_new();

        gtk_stack_add_titled(GTK_STACK(stack), app->overview.base.box, app->overview.base.id, app->overview.base.name);
        gtk_stack_add_titled(GTK_STACK(stack), app->cpu.base.box, app->cpu.base.id, app->cpu.base.name);
        gtk_stack_add_titled(GTK_STACK(stack), app->procs.base.box, app->procs.base.id, app->procs.base.name);
        gtk_stack_add_titled(GTK_STACK(stack), app->sysinfo.base.box, app->sysinfo.base.id, app->sysinfo.base.name);

        gtk_widget_set_hexpand(stack, TRUE);
        gtk_widget_set_vexpand(stack, TRUE);

        return stack;
}

static inline void gui_running_init_add(struct app_gui *app, GtkBox *parent)
{
        app->running.label = gtk_label_new("Running for: 00:00:00");
        gui_widget_set_margins(app->running.label, 10, 10, 0, 10);
        gtk_box_append(parent, app->running.label);
}

static inline void gui_refresh_rate_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer arg)
{
        (void)pspec;

        struct sysm_app *app = arg;

        const char *sel = gtk_string_list_get_string(
                        GTK_STRING_LIST(gtk_drop_down_get_model(dropdown)),
                        gtk_drop_down_get_selected(dropdown)
                        );

        sysm_update_interval_sec = atoi(sel);

        if (app->gui.timer_id > 0)
                g_source_remove(app->gui.timer_id);

        app->gui.timer_id = g_timeout_add_seconds(sysm_update_interval_sec, gui_update_tick, app);
}

static inline void gui_refresh_rate_init_add(struct app_gui *app, GtkBox *parent)
{
        const char *times[] = {"1s", "2s", "3s", "5s", "10s", "15s", "20s", NULL};

        app->refresh_rate.box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        
        app->refresh_rate.label = gtk_label_new("Refresh Rate: ");
        gtk_widget_set_halign(app->refresh_rate.label, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(app->refresh_rate.box), app->refresh_rate.label);

        app->refresh_rate.dropdown = gtk_drop_down_new_from_strings(times);

        gtk_widget_set_halign(app->refresh_rate.dropdown, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(app->refresh_rate.box), app->refresh_rate.dropdown);

        gui_widget_set_margins(app->refresh_rate.box, 10, 10, 0, 10);
        gtk_box_append(parent, app->refresh_rate.box);

        g_signal_connect(app->refresh_rate.dropdown, "notify::selected",
                G_CALLBACK(gui_refresh_rate_changed), app);
}

static void gui_widgets_add_init(struct sysm_app *app, GtkWidget *window)
{
        GtkWidget *box;
        GtkWidget *navbox;
        GtkWidget *contbox;
        GtkWidget *stack;
        GtkWidget *sidebar;
        struct app_gui *gui = &app->gui;

        box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_window_set_child(GTK_WINDOW(window), box);

        navbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        contbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        gui_content_widgets_add_init(app);

        sidebar = gui_navigation_widget_create_init();
        gtk_box_append(GTK_BOX(navbox), sidebar);

        gui_refresh_rate_init_add(gui, GTK_BOX(navbox));
        gui_running_init_add(gui, GTK_BOX(navbox));

        stack = gui_content_stack_create_init(gui);
        gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sidebar), GTK_STACK(stack));
        gtk_box_append(GTK_BOX(contbox), stack);

        gtk_widget_set_hexpand(contbox, TRUE);
        gtk_widget_set_vexpand(contbox, TRUE);

        gtk_widget_set_vexpand(navbox, TRUE);

        gtk_box_append(GTK_BOX(box), navbox);
        gtk_box_append(GTK_BOX(box), contbox);
}

static void gui_timer_updater_init(struct sysm_app *app)
{
        sysm_update_first(&app->backend.sysmon);

        app->gui.timer_id = g_timeout_add_seconds(sysm_update_interval_sec,
                gui_update_tick, app);
}

static void gui_activate_window(GtkApplication *gapp, gpointer arg)
{
        struct sysm_app *app = arg;
        GtkWidget *window;

        (void)gapp;

        window = gui_window_create_init(&app->gui);
        gui_widgets_add_init(app, window);
        gui_timer_updater_init(app);

        g_signal_connect(window, "destroy", G_CALLBACK(gui_window_destroy), app);

        gtk_window_present(GTK_WINDOW(window));
}

static sysm_errno_t sysm_gui_init(struct app_gui *app)
{
        app->running.start_time = time(NULL);

        gui_proc_store_init(&app->proc_store);

        app->app = gtk_application_new(GUI_APP_NAME,
                G_APPLICATION_DEFAULT_FLAGS);

        g_signal_connect(app->app, "activate",
                G_CALLBACK(gui_activate_window), app);

        return SYSM_SUCCESS;
}

sysm_errno_t sysm_app_init(struct sysm_app *app)
{
        sysm_errno_t ret;
        
        if (!app)
                return SYSM_FAILURE;

        ret = sysm_init(&app->backend.sysmon);

        if (ret)
                sysm_log_ret(ret, SYSM_ERR "Failed to sysmonitor backend struct initialization\n");

        ret = sysm_gui_init(&app->gui);

        if (ret)
                sysm_log_ret(ret, SYSM_ERR "Failed to GUI interface initialization\n");

        return SYSM_SUCCESS;
}

int sysm_app_run(struct sysm_app *app)
{
        if (!app)
                return -1;

        return g_application_run(G_APPLICATION(app->gui.app), 0, NULL);
}

void sysm_app_destroy(struct sysm_app *app)
{
        gui_proc_store_destroy(&app->gui.proc_store);
        gui_overview_page_destroy(&app->gui.overview);
        gui_cpu_page_destroy(&app->gui.cpu);
        g_object_unref(app->gui.app);
}