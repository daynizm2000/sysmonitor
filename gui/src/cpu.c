#include "../include/cpu.h"
#include "../include/utils.h"
#include "gtk/gtk.h"

void gui_cpu_core_init(struct sysm_cpu_box *core, const char *title, bool is_markup)
{
        core->frame = gtk_frame_new(NULL);
        core->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        if (title) {
                GtkWidget *label = gtk_label_new(NULL);

                if (is_markup)
                        gtk_label_set_markup(GTK_LABEL(label), title);
                else
                        gtk_label_set_text(GTK_LABEL(label), title);

                gtk_box_append(GTK_BOX(core->box), label);
                gui_widget_set_margins(label, 10, 10, 10, 0);
        }

        core->usage_label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(core->usage_label), "<b>0%</b>");
        gui_widget_set_margins(core->usage_label, 10, 10, 0, 10);

        gtk_box_append(GTK_BOX(core->box), core->usage_label);
        gtk_frame_set_child(GTK_FRAME(core->frame), core->box);

        mlib_list_head_init(&core->list);
}

void gui_cpu_main_core_init(struct sysm_cpu_box *cpu, const char *title, bool is_markup)
{
        gui_cpu_core_init(cpu, title, is_markup);

        cpu->usage_ghz_label = gtk_label_new("0 / 0 GHz");
        gtk_box_append(GTK_BOX(cpu->box), cpu->usage_ghz_label);
        gui_widget_set_margins(cpu->usage_label, 10, 10, 0, 0);
        gui_widget_set_margins(cpu->usage_ghz_label, 10, 10, 0, 10);
}

struct sysm_cpu_box *gui_cpu_core_create_init(const char *title, bool is_markup)
{
        struct sysm_cpu_box *core = malloc(sizeof(struct sysm_cpu_box));

        if (!core)
                sysm_log_ret(NULL, SYSM_ERR "Failed to memory allocation in libc malloc\n");

        gui_cpu_core_init(core, title, is_markup);

        return core;
}

void gui_cpu_core_free(struct sysm_cpu_box *core)
{
        free(core);
}

void gui_cpu_core_update(struct sysm_cpu_box *core, const struct cpu_core_info *info)
{
        char buf[64];

        snprintf(buf, sizeof(buf), "<b>%.1lf%%</b>", info->usage_pct);
        gtk_label_set_markup(GTK_LABEL(core->usage_label), buf);
}

void gui_cpu_main_core_update(struct sysm_cpu_box *cpu, const struct cpu_info *info)
{
        char buf[64];

        snprintf(buf, sizeof(buf), "<b>%.1lf%%</b>", info->total_usage_pct);
        gtk_label_set_markup(GTK_LABEL(cpu->usage_label), buf);

        snprintf(buf, sizeof(buf), "%.2lf / %.2lf GHz", info->current_ghz, info->max_ghz);
        gtk_label_set_text(GTK_LABEL(cpu->usage_ghz_label), buf);
}

void gui_cpu_page_init(struct sysm_page_cpu *page, struct sysm_app *app)
{
        struct cpu_info *info = &app->backend.sysmon.cpu;

        gui_page_base_init(&page->base, "cpu", "CPU", "<b>CPU</b>");

        mlib_list_head_init(&page->cores);

        gui_cpu_main_core_init(&page->all, "CPU", false);
        gtk_widget_set_margin_end(page->all.frame, 10);
        gtk_box_append(GTK_BOX(page->base.box), page->all.frame);

        for (unsigned int i = 0; i < info->core_count; i++) {
                char title[32];
                struct sysm_cpu_box *core;

                snprintf(title, sizeof(title), "CPU%u", i);

                core = gui_cpu_core_create_init(title, false);

                if (!core) {
                        struct sysm_cpu_box *tmp;
                        unsigned int j = 0;

                        mlib_list_for_each_entry_safe(core, tmp, &page->cores, list) {
                                if (j >= i)
                                        break;

                                gui_cpu_core_free(core);

                                j++;
                        }

                        return;
                }

                gtk_widget_set_margin_end(core->frame, 10);

                mlib_list_add_tail(&core->list, &page->cores);
                gtk_box_append(GTK_BOX(page->base.box), core->frame);
        }
}

void gui_cpu_page_update(struct sysm_page_cpu *page, struct sysm_app *app)
{
        struct cpu_info *info = &app->backend.sysmon.cpu;
        struct sysm_cpu_box *iter;
        struct sysm_cpu_box *tmp;
        unsigned int i = 0;

        gui_cpu_main_core_update(&page->all, info);

        mlib_list_for_each_entry_safe(iter, tmp, &page->cores, list) {
                if (i >= info->core_count) {
                        gtk_box_remove(GTK_BOX(page->base.box), iter->frame);
                        mlib_list_del(&iter->list);
                        gui_cpu_core_free(iter);
                        
                        continue;
                }

                gui_cpu_core_update(iter, &info->cores[i]);

                i++;
        }

        for ( ; i < info->core_count; i++) {
                char title[32];
                struct sysm_cpu_box *core;

                snprintf(title, sizeof(title), "CPU%u", i);

                core = gui_cpu_core_create_init(title, false);

                if (!core)
                        continue;

                mlib_list_add_tail(&core->list, &page->cores);
                gtk_widget_set_margin_end(core->frame, 10);
                gtk_box_append(GTK_BOX(page->base.box), core->frame);
        }
}

void gui_cpu_page_destroy(struct sysm_page_cpu *page)
{
        struct sysm_cpu_box *iter;
        struct sysm_cpu_box *tmp;

        mlib_list_for_each_entry_safe(iter, tmp, &page->cores, list) {
                mlib_list_del(&iter->list);
                gui_cpu_core_free(iter);
        }
}