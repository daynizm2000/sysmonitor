#pragma once

#include "../../libs/mlib-memory/include/memory.h"
#include "log.h"
#include <sys/types.h>
#include <unistd.h>

#define SYSM_VERSION "v0.0.1"
#define SYSM_AUTHOR "daynizm2000"
#define SYSM_AUTHOR_GITHUB "https://github.com/daynizm2000"
#define SYSM_REPOSITORY "https://github.com/daynizm2000/sysmonitor"

#define SYSM_UPDATE_INTERVAL_SEC 1
#define SYSM_LINUX_FPATH_MAXLEN 255
#define SYSM_LINUX_NAME_MAXLEN 255
#define SYSM_LINUX_FS_NAME_MAXLEN 32
#define SYSM_LINUX_COMM_MAXLEN 15
#define SYSM_LINUX_DISTR_NAME_MAXLEN 255
#define SYSM_LINUX_VERSION_FMT_MAXLEN 64
#define SYSM_ARCH_NAME_MAXLEN 64
#define SYSM_MOTHERBOARD_VENDOR_MAXLEN 48
#define SYSM_MOTHERBOARD_NAME_MAXLEN 48
#define SYSM_MOTHERBOARD_BIOS_VER_MAXLEN 32
#define SYSM_CPU_NAME_MAXLEN 48

#define SYSM_KB_TO_GIB(val) ((val) / (1024.0 * 1024))
#define SYSM_B_TO_MIB(val) ((val) / (1024.0 * 1024))
#define SYSM_B_TO_GIB(val) ((val) / (1024.0 * 1024 * 1024))

extern unsigned int sysm_update_interval_sec;

typedef enum {
        SYSM_SUCCESS,
        SYSM_FAILURE
} sysm_errno_t;

struct cpu_time_metrics {
        unsigned long long user;
        unsigned long long system;
        unsigned long long idle;
        unsigned long long nice;
        unsigned long long iowait;
        unsigned long long irq;
        unsigned long long softirq;
        unsigned long long steal;
};

struct cpu_core_info {
        double usage_pct;

        struct cpu_time_metrics __ticks;
};

struct cpu_info {
        double total_usage_pct;

        struct cpu_time_metrics __ticks;

        unsigned long long cpu_ticks_delta;

        double current_ghz;
        double max_ghz;

        struct cpu_core_info *cores;
        unsigned int core_count;
};

struct disk_partition {
        char disk_name[SYSM_LINUX_NAME_MAXLEN + 1];
        char mount_point[SYSM_LINUX_FPATH_MAXLEN + 1];
        char fs[SYSM_LINUX_FS_NAME_MAXLEN + 1];

        unsigned long long used_b;
        unsigned long long total_b;

        double usage_pct;

        mlib_list_head_t list;
};

struct disk_info {
        double read_speed_b_sec;
        double write_speed_b_sec;

        mlib_list_head_t parts;
};

struct swap_info {
        double total_gib;
        double used_gib;
        double usage_pct;
};

struct network_info {
        double read_speed_b_sec;
        double write_speed_b_sec;
};

struct mem_info {
        double total_gib;
        double used_gib;
        double usage_pct;
};

struct sysm_internal_info {
        unsigned long memtotal_b;
        unsigned long long cpu_ticks_delta;
};

struct proc_info {
        pid_t pid;
        char name[SYSM_LINUX_COMM_MAXLEN + 1]; // comm

        double cpu_usage_pct;

        unsigned long __utime;
        unsigned long __stime;

        double mem_usage_pct;
        unsigned long rss_b;

        unsigned int threads;

        mlib_list_head_t list;
};

struct motherboard_info {
        char vendor[SYSM_MOTHERBOARD_VENDOR_MAXLEN + 1];
        char name[SYSM_MOTHERBOARD_NAME_MAXLEN + 1];
        char bios_ver[SYSM_MOTHERBOARD_BIOS_VER_MAXLEN + 1];
};

struct sys_info {
        char distr[SYSM_LINUX_DISTR_NAME_MAXLEN + 1];
        char kernel_ver[SYSM_LINUX_VERSION_FMT_MAXLEN + 1];
        char arch[SYSM_ARCH_NAME_MAXLEN + 1];
        char cpu_name[SYSM_CPU_NAME_MAXLEN + 1];

        struct motherboard_info motherboard;
};

struct sysm_fds {
        // pfs - proc_fs

        int pfs_meminfo;
        int pfs_stat;
        int pfs_net_dev;
        int pfs_mounts;

        int pfs_diskstats;

        int sysfs_cpu0_scaling_cur_freq;
        int sysfs_cpu0_cpuinfo_max_freq;
};

extern struct sysm_fds *sysm_cached_fds;

struct sysmonitor {
        unsigned long uptime_sec;

        double load_avg[3];

        struct cpu_info cpu;
        struct disk_info disk;
        struct swap_info swap;
        struct network_info network;
        struct mem_info mem;
        struct sys_info sys;
        
        mlib_list_head_t proc_table;

        struct sysm_fds fds;
};

// cpu_info.c

sysm_errno_t cpu_info_update_first(struct cpu_info *cpuinfo);
sysm_errno_t cpu_info_update_last(struct cpu_info *cpuinfo);

sysm_errno_t cpu_info_init(struct cpu_info *cpuinfo);
sysm_errno_t cpu_info_update(struct cpu_info *cpuinfo);
void cpu_info_destroy(struct cpu_info *cpuinfo);

// disk_info.c

sysm_errno_t disk_info_update_first(struct disk_info *diskinfo);
sysm_errno_t disk_info_update_last(struct disk_info *diskinfo);

sysm_errno_t disk_info_init(struct disk_info *diskinfo);
sysm_errno_t disk_info_update(struct disk_info *diskinfo);
void disk_info_destroy(struct disk_info *diskinfo);

// swap_info.c

void swap_info_init(struct swap_info *swapinfo);
sysm_errno_t swap_info_update(struct swap_info *swapinfo);

// network_info.c

sysm_errno_t network_info_update_first(struct network_info *netwinfo);
sysm_errno_t network_info_update_last(struct network_info *netwinfo);

void network_info_init(struct network_info *netwinfo);
sysm_errno_t network_info_update(struct network_info *netwinfo);

// mem_info.c

void mem_info_init(struct mem_info *meminfo);
sysm_errno_t mem_info_update(struct mem_info *meminfo);

// internal.c

sysm_errno_t sysm_internal_update_first(struct sysm_internal_info *sys_info);
sysm_errno_t sysm_internal_update_last(struct sysm_internal_info *sys_info);

// sys_info.c

sysm_errno_t sysm_sys_info_init(struct sys_info *sys_info);

// proc_info.c

sysm_errno_t proc_info_update_first(struct proc_info *procinfo,
        struct sysm_internal_info *sys_info, pid_t pid);
sysm_errno_t proc_info_update_last(struct proc_info *procinfo,
        struct sysm_internal_info *sys_info);

sysm_errno_t proc_info_table_update_first(mlib_list_head_t *head, struct sysm_internal_info *sys_info);
sysm_errno_t proc_info_table_update_last(mlib_list_head_t *head, struct sysm_internal_info *sys_info);

void proc_info_init(struct proc_info *procinfo);
sysm_errno_t proc_info_update(struct proc_info *procinfo, struct sysm_internal_info *sys_info, pid_t pid);
void proc_info_destroy(struct proc_info *procinfo);

sysm_errno_t proc_info_table_init(mlib_list_head_t *head);
sysm_errno_t proc_info_table_update(mlib_list_head_t *head, struct sysm_internal_info *sys_info);
void proc_info_table_destroy(mlib_list_head_t *head);

// sysmonitor.c

sysm_errno_t sysm_init(struct sysmonitor *sysm);
sysm_errno_t sysm_update_first(struct sysmonitor *sysm);
sysm_errno_t sysm_update_last(struct sysmonitor *sysm);
sysm_errno_t sysm_update(struct sysmonitor *sysm);
void sysm_destroy(struct sysmonitor *sysm);

// utils.c

int sysm_fds_get_fd(int fd);
ssize_t sysm_read_file(int fd, char *buffer, size_t bufsize);
ssize_t sysm_read_file_from_path(const char *fpath, char *buffer, size_t bufsize);

unsigned long long sysm_meminfo_parse(const char *key, const char *filedata, size_t size);
sysm_errno_t sysm_meminfo_file_parse_lines(const char **keys, unsigned long long *retvals, unsigned int count);
unsigned long long sysm_meminfo_file_parse(const char *key);

static inline void sysm_sleep(void)
{
        sleep(sysm_update_interval_sec);
}
