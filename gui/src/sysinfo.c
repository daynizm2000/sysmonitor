#include "../include/sysinfo.h"
#include "../include/utils.h"
#include "gtk/gtk.h"

void gui_sysinfo_page_update(struct gui_page_sysinfo *page)
{
        (void)page;
}

static inline GtkWidget *sys_info_block_frame_create_init(const char *markup)
{
        GtkWidget *label = gtk_label_new(NULL);
        GtkWidget *frame = gtk_frame_new(NULL);

        gtk_label_set_markup(GTK_LABEL(label), markup);
        gtk_frame_set_label_widget(GTK_FRAME(frame), label);

        gtk_widget_set_margin_end(frame, 10);

        return frame;
}

static inline void sys_info_system_init(struct gui_page_sysinfo *page,
        struct sys_info *info)
{
        char buf[512];

        snprintf(buf, sizeof(buf),
                "Distribution: %s\n"
                "Kernel: %s\n"
                "Arch: %s\n"
                "CPU: %s\n",
                info->distr, info->kernel_ver,
                info->arch, info->cpu_name);

        page->system.info_label = gtk_label_new(buf);
        gtk_widget_set_halign(page->system.info_label, GTK_ALIGN_START);
}

static inline void sys_info_motherboard_init(struct gui_page_sysinfo *page,
        struct motherboard_info *info)
{
        char buf[512];

        snprintf(buf, sizeof(buf),
                "Vendor: %s\n"
                "Model: %s\n"
                "BIOS Version: %s\n",
                info->vendor, info->name,
                info->bios_ver);

        page->motherboard.info_label = gtk_label_new(buf);
        gtk_widget_set_halign(page->motherboard.info_label, GTK_ALIGN_START);
}

static inline void sys_info_about_init(struct gui_page_sysinfo *page)
{
        char buf[256];

        page->about_label = gtk_label_new(NULL);

        snprintf(buf, sizeof(buf),
                "<span alpha=\"55%%\">"
                "Author:  %s  |  "
                "<a href=\"%s\">Github</a>  |  "
                "<a href=\"%s\">Repository</a>"
                "</span>",
                SYSM_AUTHOR, SYSM_AUTHOR_GITHUB,
                SYSM_REPOSITORY);

        gtk_label_set_markup(GTK_LABEL(page->about_label), buf);
        gtk_widget_set_vexpand(GTK_WIDGET(page->about_label), TRUE);
        gtk_widget_set_valign(GTK_WIDGET(page->about_label), GTK_ALIGN_END);
        gtk_widget_set_halign(page->about_label, GTK_ALIGN_CENTER);
}

void gui_sysinfo_page_init(struct gui_page_sysinfo *page, struct sysm_app *app)
{
        gui_page_base_init(&page->base, "system_info", "System Info", "<b>System Info</b>");

        page->system.frame = sys_info_block_frame_create_init("<b>OS &amp; Hardware</b>");
        page->motherboard.frame = sys_info_block_frame_create_init("<b>Motherboard</b>");

        sys_info_system_init(page, &app->backend.sysmon.sys);
        sys_info_motherboard_init(page, &app->backend.sysmon.sys.motherboard);
        sys_info_about_init(page);

        gtk_frame_set_child(GTK_FRAME(page->system.frame), page->system.info_label);
        gtk_frame_set_child(GTK_FRAME(page->motherboard.frame), page->motherboard.info_label);
        
        gtk_box_append(GTK_BOX(page->base.box), page->system.frame);
        gtk_box_append(GTK_BOX(page->base.box), page->motherboard.frame);
        gtk_box_append(GTK_BOX(page->base.box), page->about_label);
}