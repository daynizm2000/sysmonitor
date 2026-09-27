#pragma once

#include <gtk/gtk.h>


#define SYSM_TYPE_PROC_LINE (sysm_proc_line_get_type())
G_DECLARE_FINAL_TYPE(SysmProcLine, sysm_proc_line, SYSM, PROC_LINE, GObject)

struct _SysmProcLine {
        GObject parent;

        pid_t pid;

        const struct proc_info *info;
};

struct sysm_proc_table {
        GtkWidget *frame;
        GtkWidget *scrolled_win;
        GtkWidget *columnview;
        GListStore *store;
};

struct sysm_proc_store {
        GListStore *lstore;
        GtkSingleSelection *sel;
        GHashTable *pid_to_idx;
        GArray *stale;
};

#define SYSM_TYPE_DISK_PART (sysm_disk_part_get_type())
G_DECLARE_FINAL_TYPE(SysmDiskPart, sysm_disk_part, SYSM, DISK_PART, GObject)

struct _SysmDiskPart {
        GObject parent;
        const struct disk_partition *info;
};

struct sysm_disk_table {
        GtkWidget *frame;
        GtkWidget *scrolled_win;
        GtkWidget *columnview;
        GListStore *store;
};
