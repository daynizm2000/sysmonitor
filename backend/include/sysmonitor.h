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

#define KB_TO_GIB(val) ((val) / (1024.0 * 1024))
#define B_TO_MIB(val) ((val) / (1024.0 * 1024))
#define B_TO_GIB(val) ((val) / (1024.0 * 1024 * 1024))

#define ARRAY_SIZE(a) (sizeof((a)) / sizeof(*(a)))
#define STRLEN_LIT(str) (sizeof(str "") - 1)

typedef enum {
        SYSM_STATE_FIRST_UPDATE,
        SYSM_STATE_LAST_UPDATE
} sysm_update_status_t;

#define SYSM_STATE_IS_FIRST_UPDATE(state) ((state) == SYSM_STATE_FIRST_UPDATE)
#define SYSM_STATE_IS_LAST_UPDATE(state) ((state) == SYSM_STATE_LAST_UPDATE)

#define SYSM_UPDATE_STATUS_WARN(type, member) \
        sysm_log(SYSM_WARN SYSM_BUG "Getting (" #type ")." #member " at first update state\n")

#define SYSM_UPDATE_STATUS_WARN_ON(state, type, member) \
        ((SYSM_STATE_IS_FIRST_UPDATE(state)) ? SYSM_UPDATE_STATUS_WARN(type, member) : 0)

static inline void sysm_update_status_update(sysm_update_status_t *st)
{
        (*st)++;
}

extern unsigned int sysm_update_interval_sec;

typedef enum {
        SYSM_SUCCESS, // only 0
        SYSM_FAILURE
} sysm_errno_t;

typedef enum {
        // pfs - proc_fs

        SYSM_FDS_PFS_STAT,
        SYSM_FDS_PFS_NET_DEV,
        SYSM_FDS_PFS_MEMINFO,
        SYSM_FDS_PFS_MOUNTS,
        SYSM_FDS_PFS_DISKSTATS,
        SYSM_FDS_SYSFS_CPU0_SCALING_CUR_FREQ,
        SYSM_FDS_SYSFS_CPU0_CPUINFO_MAX_FREQ,
        SYSM_FDS_NR
} sysm_nfd_t;

struct sysm_fds {
        sysm_nfd_t fds[SYSM_FDS_NR];
};

extern struct sysm_fds *sysm_cached_fds;

static inline int sysm_fds_get_fd(sysm_nfd_t n, struct sysm_fds *fds)
{
        if (!fds)
                sysm_log_ret(-1, SYSM_ERR SYSM_BUG "Invalid argument! struct sysm_fds *fds IS NULL in function int sysm_fds_get_fd()\n");

        if (n >= SYSM_FDS_NR)
                sysm_log_ret(-1, SYSM_ERR SYSM_BUG "Buffer overflow attempt when receiving a descriptor with struct sysm_fds\n");
        
        lseek(fds->fds[n], 0, SEEK_SET);
        return fds->fds[n];
}

static inline int sysm_cached_fds_get_fd(sysm_nfd_t n)
{
        return sysm_fds_get_fd(n, sysm_cached_fds);
}

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

        sysm_update_status_t state;
};

static inline double cpu_core_info_usage_pct(struct cpu_core_info *c)
{
        return c->usage_pct;
}

struct cpu_info {
        double usage_pct;

        struct cpu_time_metrics __ticks;

        unsigned long long cpu_ticks_delta;

        double current_ghz;
        double max_ghz;

        struct cpu_core_info *cores;
        unsigned int core_count;

        sysm_update_status_t state;
};

static inline double cpu_info_total_usage_pct(struct cpu_info *c)
{
        SYSM_UPDATE_STATUS_WARN_ON(c->state, struct cpu_info, usage_pct);

        return c->usage_pct;
}

static inline double cpu_info_current_ghz(struct cpu_info *c)
{
        return c->current_ghz;
}

static inline double cpu_info_max_ghz(struct cpu_info *c)
{
        return c->max_ghz;
}

static inline struct cpu_core_info *cpu_info_cores(struct cpu_info *c)
{
        return c->cores;
}

static inline unsigned int cpu_info_core_count(struct cpu_info *c)
{
        return c->core_count;
}

struct disk_part {
        char disk_name[SYSM_LINUX_NAME_MAXLEN + 1];
        char mount_point[SYSM_LINUX_FPATH_MAXLEN + 1];
        char fs[SYSM_LINUX_FS_NAME_MAXLEN + 1];

        unsigned long long used_b;
        unsigned long long total_b;

        double usage_pct;

        mlib_list_head_t list;

        sysm_update_status_t state;
};

static inline char *disk_part_disk_name(struct disk_part *p)
{
        return p->disk_name;
}

static inline char *disk_part_mount_point(struct disk_part *p)
{
        return p->mount_point;
}

static inline char *disk_part_fs(struct disk_part *p)
{
        return p->fs;
}

static inline unsigned long long disk_part_used_b(struct disk_part *p)
{
        return p->used_b;
}

static inline unsigned long long disk_part_total_b(struct disk_part *p)
{
        return p->total_b;
}

static inline double disk_part_usage_pct(struct disk_part *p)
{
        SYSM_UPDATE_STATUS_WARN_ON(p->state, struct disk_part, usage_pct);

        return p->usage_pct;
}

struct disk_info {
        double read_speed_b_sec;
        double write_speed_b_sec;

        mlib_list_head_t parts;

        sysm_update_status_t state;
};

static inline double disk_info_read_speed_b_sec(struct disk_info *d)
{
        SYSM_UPDATE_STATUS_WARN_ON(d->state, struct disk_info, read_speed_b_sec);

        return d->read_speed_b_sec;
}

static inline double disk_info_write_speed_b_sec(struct disk_info *d)
{
        SYSM_UPDATE_STATUS_WARN_ON(d->state, struct disk_info, write_speed_b_sec);

        return d->write_speed_b_sec;
}

struct swap_info {
        double total_gib;
        double used_gib;
        double usage_pct;
};

static inline double swap_info_total_gib(struct swap_info *s)
{
        return s->total_gib;
}

static inline double swap_info_used_gib(struct swap_info *s)
{
        return s->used_gib;
}

static inline double swap_info_usage_pct(struct swap_info *s)
{
        return s->usage_pct;
}

struct network_info {
        double read_speed_b_sec;
        double write_speed_b_sec;

        sysm_update_status_t state;
};

static inline double network_info_read_speed_b_sec(struct network_info *n)
{
        SYSM_UPDATE_STATUS_WARN_ON(n->state, struct network_info, read_speed_b_sec);

        return n->read_speed_b_sec;
}

static inline double network_info_write_speed_b_sec(struct network_info *n)
{
        SYSM_UPDATE_STATUS_WARN_ON(n->state, struct network_info, write_speed_b_sec);

        return n->write_speed_b_sec;
}

struct mem_info {
        double total_gib;
        double used_gib;
        double usage_pct;
};

static inline double mem_info_total_gib(struct mem_info *m)
{
        return m->total_gib;
}

static inline double mem_info_used_gib(struct mem_info *m)
{
        return m->used_gib;
}

static inline double mem_info_usage_pct(struct mem_info *m)
{
        return m->usage_pct;
}

struct sysm_internal_info {
        unsigned long memtotal_b;
        unsigned long long cpu_ticks_delta;
};

struct proc_table {
        mlib_list_head_t list;
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
        sysm_update_status_t state;
};

static inline pid_t proc_info_pid(struct proc_info *p)
{
        return p->pid;
}

static inline char *proc_info_name(struct proc_info *p)
{
        return p->name;
}

static inline double proc_info_cpu_usage_pct(struct proc_info *p)
{
        SYSM_UPDATE_STATUS_WARN_ON(p->state, struct proc_info, cpu_usage_pct);

        return p->cpu_usage_pct;
}

static inline double proc_info_mem_usage_pct(struct proc_info *p)
{
        SYSM_UPDATE_STATUS_WARN_ON(p->state, struct proc_info, mem_usage_pct);

        return p->mem_usage_pct;
}

static inline unsigned long proc_info_rss_b(struct proc_info *p)
{
        return p->rss_b;
}

static inline unsigned int proc_info_threads(struct proc_info *p)
{
        return p->threads;
}

#define proc_info_entry(l) mlib_list_entry((l), struct proc_info, list)

static inline struct proc_info *proc_table_next_elem(struct proc_table *t, struct proc_info *p)
{
        if (mlib_list_is_last(&p->list, &t->list))
                return NULL;

        return mlib_list_next_entry(p, list);
}

static inline struct proc_info *proc_table_prev_elem(struct proc_table *t, struct proc_info *p)
{
        if (mlib_list_is_first(&p->list, &t->list))
                return NULL;

        return mlib_list_prev_entry(p, list);
}

#define proc_table_for_each(p, t) \
        mlib_list_for_each_entry(p, &(t)->list, list)

struct motherboard_info {
        char vendor[SYSM_MOTHERBOARD_VENDOR_MAXLEN + 1];
        char name[SYSM_MOTHERBOARD_NAME_MAXLEN + 1];
        char bios_ver[SYSM_MOTHERBOARD_BIOS_VER_MAXLEN + 1];
};

static inline char *motherboard_info_vendor(struct motherboard_info *m)
{
        return m->vendor;
}

static inline char *motherboard_info_name(struct motherboard_info *m)
{
        return m->name;
}

static inline char *motherboard_info_bios_ver(struct motherboard_info *m)
{
        return m->bios_ver;
}

struct sys_info {
        char distr[SYSM_LINUX_DISTR_NAME_MAXLEN + 1];
        char kernel_ver[SYSM_LINUX_VERSION_FMT_MAXLEN + 1];
        char arch[SYSM_ARCH_NAME_MAXLEN + 1];
        char cpu_name[SYSM_CPU_NAME_MAXLEN + 1];

        struct motherboard_info motherboard;
};

static inline char *sys_info_distr(struct sys_info *s)
{
        return s->distr;
}

static inline char *sys_info_kernel_ver(struct sys_info *s)
{
        return s->kernel_ver;
}

static inline char *sys_info_arch(struct sys_info *s)
{
        return s->arch;
}

static inline char *sys_info_cpu_name(struct sys_info *s)
{
        return s->cpu_name;
}

static inline struct motherboard_info *sys_info_motherboard(struct sys_info *s)
{
        return &s->motherboard;
}

struct sysmonitor {
        unsigned long uptime_sec;

        double load_avg[3];

        struct cpu_info cpu;
        struct disk_info disk;
        struct swap_info swap;
        struct network_info network;
        struct mem_info mem;
        struct sys_info sys;
        
        struct proc_table proc_table;

        struct sysm_fds fds;
};

static inline unsigned long sysm_uptime_sec(struct sysmonitor *s)
{        
        return s->uptime_sec;
}

static inline double *sysm_load_avg(struct sysmonitor *s)
{
        return s->load_avg;
}

static inline struct cpu_info *sysm_cpu(struct sysmonitor *s)
{
        return &s->cpu;
}

static inline struct disk_info *sysm_disk(struct sysmonitor *s)
{
        return &s->disk;
}

static inline struct swap_info *sysm_swap(struct sysmonitor *s)
{
        return &s->swap;
}

static inline struct network_info *sysm_network(struct sysmonitor *s)
{
        return &s->network;
}

static inline struct mem_info *sysm_meminfo(struct sysmonitor *s)
{
        return &s->mem;
}

static inline struct sys_info *sysm_sys(struct sysmonitor *s)
{
        return &s->sys;
}

static inline struct proc_table *sysm_proc_table(struct sysmonitor *s)
{
        return &s->proc_table;
}

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

sysm_errno_t sys_info_init(struct sys_info *sys_info);

// proc_info.c

sysm_errno_t proc_info_update_first(struct proc_info *procinfo,
        struct sysm_internal_info *sys_info, pid_t pid);
sysm_errno_t proc_info_update_last(struct proc_info *procinfo,
        struct sysm_internal_info *sys_info);

sysm_errno_t proc_info_table_update_first(struct proc_table *table, struct sysm_internal_info *sys_info);
sysm_errno_t proc_info_table_update_last(struct proc_table *table, struct sysm_internal_info *sys_info);

void proc_info_init(struct proc_info *procinfo);
sysm_errno_t proc_info_update(struct proc_info *procinfo, struct sysm_internal_info *sys_info, pid_t pid);
void proc_info_destroy(struct proc_info *procinfo);

sysm_errno_t proc_info_table_init(struct proc_table *table);
sysm_errno_t proc_info_table_update(struct proc_table *table, struct sysm_internal_info *sys_info);
void proc_info_table_destroy(struct proc_table *table);

// sysmonitor.c

sysm_errno_t sysm_init(struct sysmonitor *sysm);
sysm_errno_t sysm_update_first(struct sysmonitor *sysm);
sysm_errno_t sysm_update_last(struct sysmonitor *sysm);
sysm_errno_t sysm_update(struct sysmonitor *sysm);
void sysm_destroy(struct sysmonitor *sysm);

// utils.c

ssize_t sysm_read_file(int fd, char *buffer, size_t bufsize);
ssize_t sysm_read_file_from_path(const char *fpath, char *buffer, size_t bufsize);

unsigned long long sysm_meminfo_parse(const char *key, const char *filedata, size_t size);
sysm_errno_t sysm_meminfo_file_parse_lines(const char **keys, unsigned long long *retvals, unsigned int count);
unsigned long long sysm_meminfo_file_parse(const char *key);

static inline void sysm_sleep(void)
{
        sleep(sysm_update_interval_sec);
}