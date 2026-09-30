#include "../include/sysmonitor.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define PFS_STAT_USER_POS    1
#define PFS_STAT_NICE_POS    2
#define PFS_STAT_SYSTEM_POS  3
#define PFS_STAT_IDLE_POS    4
#define PFS_STAT_IOWAIT_POS  5
#define PFS_STAT_IRQ_POS     6
#define PFS_STAT_SOFTIRQ_POS 7
#define PFS_STAT_STEAL_POS   8

#define KHZ_TO_GHZ(val) ((val) / 1000000.0)

static void pfs_stat_parse_line(const char *line, unsigned int *sort_words,
        unsigned long long *retvals, unsigned int count)
{
        unsigned int word = 1;
        unsigned int idx = 0;

        for (const char *l = line; *l != '\n' && idx < count; word++) {
                if (word == sort_words[idx]) {
                        retvals[idx] = atoll(l);
                        idx++;
                }

                while (!isspace(*l))
                        l++;

                while (*l != '\n' && isspace(*l))
                        l++;
        }
}

static const char *pfs_stat_find_line(const char *buffer,
        size_t size, const char *name)
{
        const char *res;
        unsigned int nlen = strlen(name);

        res = memmem(buffer, size, name, nlen);

        if (!res)
                return NULL;

        res += nlen;

        while (!isspace(*res))
                res++;

        if (*res == '\n')
                return res;

        while (*res != '\n' && isspace(*res))
                res++;

        return res;
}

static inline void __cpu_info_set_ticks(struct cpu_time_metrics *ticks, const char *filedata, size_t size)
{
        unsigned int words[] = {
                PFS_STAT_USER_POS,
                PFS_STAT_NICE_POS,
                PFS_STAT_SYSTEM_POS,
                PFS_STAT_IDLE_POS,
                PFS_STAT_IOWAIT_POS,
                PFS_STAT_IRQ_POS,
                PFS_STAT_SOFTIRQ_POS,
                PFS_STAT_STEAL_POS
        };
        unsigned long long retvals[ARRAY_SIZE(words)];
        const char *line;

        line = pfs_stat_find_line(filedata, size, "cpu");

        if (!line)
                sysm_log_ret(, SYSM_WARN "Failed to parse /proc/stat, line=cpu info\n");

        pfs_stat_parse_line(line, words, retvals, sizeof(words) / sizeof(*words));

        ticks->user = retvals[0];
        ticks->nice = retvals[1];
        ticks->system = retvals[2];
        ticks->idle = retvals[3];
        ticks->iowait = retvals[4];
        ticks->irq = retvals[5];
        ticks->softirq = retvals[6];
        ticks->steal = retvals[7];
}

static sysm_errno_t __cpu_info_set_usage_ticks(struct cpu_info *cpuinfo)
{
        char fdata[4096];
        ssize_t fsize;

        if (sysm_cached_fds)
                fsize = sysm_read_file(sysm_cached_fds_get_fd(SYSM_FDS_PFS_STAT), fdata, sizeof(fdata));
        else
                fsize = sysm_read_file_from_path("/proc/stat", fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to read file /proc/stat\n");

        __cpu_info_set_ticks(&cpuinfo->__ticks, fdata, fsize);

        for (unsigned int i = 0; i < cpuinfo->core_count; i++) {
                char line_name[64];
                struct cpu_time_metrics *ticks;
                const char *line;

                snprintf(line_name, sizeof(line_name), "cpu%u", i);
                ticks = &cpuinfo->cores[i].__ticks;
                line = pfs_stat_find_line(fdata, fsize, line_name);

                if (!line) {
                        sysm_log(SYSM_WARN "Failed to parse /proc/stat, line=%s info\n", line_name);
                }
                else {
                        unsigned int words[] = {
                                PFS_STAT_USER_POS,
                                PFS_STAT_SYSTEM_POS,
                                PFS_STAT_IDLE_POS
                        };
                        unsigned long long retvals[ARRAY_SIZE(words)];

                        pfs_stat_parse_line(line, words, retvals, ARRAY_SIZE(words));

                        ticks->user = retvals[0];
                        ticks->system = retvals[1];
                        ticks->idle = retvals[2];
                }
        }

        return SYSM_SUCCESS;
}

static inline unsigned long long calculate_sum_ticks(struct cpu_time_metrics *ticks)
{
        return ticks->idle + ticks->iowait + ticks->irq + 
                        ticks->nice + ticks->softirq +
                        ticks->steal + ticks->system + ticks->user;
}

static inline sysm_errno_t cpu_info_set_usage_first(struct cpu_info *cpuinfo)
{
        return __cpu_info_set_usage_ticks(cpuinfo);
}

static double calculate_cpu_usage(struct cpu_time_metrics *old_ticks,
        struct cpu_time_metrics *ticks)
{
        unsigned long long diff_idle;
        unsigned long long diff_total;
        double pct;

        diff_idle = ticks->idle - old_ticks->idle;
        diff_total = diff_idle + (ticks->system - old_ticks->system) + (ticks->user - old_ticks->user);

        if (diff_total > 0) {
                pct = (1 - ((double)diff_idle / diff_total)) * 100;

                if (pct < 0)
                        pct = 0;
                if (pct > 100.0)
                        pct = 100.0;
        }
        else {
                pct = 0;
        }

        return pct;
}

static inline sysm_errno_t cpu_info_set_usage_last(struct cpu_info *cpuinfo)
{
        sysm_errno_t ret;
        struct cpu_time_metrics cpuinfo_ticks;
        struct cpu_time_metrics *cpu_core_ticks;

        ret = SYSM_SUCCESS;
        cpu_core_ticks = NULL;

        if (cpuinfo->core_count) {
                cpu_core_ticks = malloc((cpuinfo->core_count + 1) * sizeof(struct cpu_time_metrics));
                
                if (!cpu_core_ticks)
                        sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to memory allocation from libc allocator\n");
        }

        cpuinfo_ticks = cpuinfo->__ticks;

        for (unsigned int i = 0; i < cpuinfo->core_count; i++)
                cpu_core_ticks[i] = cpuinfo->cores[i].__ticks;

        ret = __cpu_info_set_usage_ticks(cpuinfo);

        if (ret)
                goto cleanup;

        cpuinfo->usage_pct = calculate_cpu_usage(&cpuinfo_ticks, &cpuinfo->__ticks);

        for (unsigned int i = 0; i < cpuinfo->core_count; i++)
                cpuinfo->cores[i].usage_pct = calculate_cpu_usage(&cpu_core_ticks[i],
                                                        &cpuinfo->cores[i].__ticks);

cleanup:
        if (cpu_core_ticks)
                free(cpu_core_ticks);

        return ret;
}

sysm_errno_t cpu_info_update_first(struct cpu_info *cpuinfo)
{
        sysm_errno_t ret;

        if (!cpuinfo)
                return SYSM_FAILURE;

        ret = cpu_info_set_usage_first(cpuinfo);

        if (ret)
                return ret;

        cpuinfo->cpu_ticks_delta = calculate_sum_ticks(&cpuinfo->__ticks);

        sysm_update_status_update(&cpuinfo->state);

        return ret;
}

static unsigned int read_cpu_khz(int fd)
{
        char buffer[128];
        ssize_t buflen;

        buflen = sysm_read_file(fd, buffer, sizeof(buffer));

        if (buflen < 0)
                return 0;

        return atoi(buffer);
}

static unsigned int read_cpu_khz_from_path(const char *path)
{
        char buffer[128];
        ssize_t bufsize;

        bufsize = sysm_read_file_from_path(path, buffer, sizeof(buffer));

        if (bufsize < 0)
                sysm_log_ret(0, SYSM_ERR "Failed to read: %s\n", path);

        return atoi(buffer);
}

static void cpu_info_set_ghz(struct cpu_info *cpuinfo)
{
        unsigned int cur_khz;
        unsigned int max_khz;

        if (sysm_cached_fds) {
                cur_khz = read_cpu_khz(sysm_cached_fds_get_fd(SYSM_FDS_SYSFS_CPU0_SCALING_CUR_FREQ));
                max_khz = read_cpu_khz(sysm_cached_fds_get_fd(SYSM_FDS_SYSFS_CPU0_CPUINFO_MAX_FREQ));
        }
        else {
                cur_khz = read_cpu_khz_from_path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
                max_khz = read_cpu_khz_from_path("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
        }

        if (cur_khz)
                cpuinfo->current_ghz = KHZ_TO_GHZ(cur_khz);
        if (max_khz)
                cpuinfo->max_ghz = KHZ_TO_GHZ(max_khz);
}

sysm_errno_t cpu_info_update_last(struct cpu_info *cpuinfo)
{
        sysm_errno_t ret;
        unsigned long long sum_ticks;

        if (!cpuinfo)
                return SYSM_FAILURE;

        ret = cpu_info_set_usage_last(cpuinfo);

        cpu_info_set_ghz(cpuinfo);

        sum_ticks = calculate_sum_ticks(&cpuinfo->__ticks);

        if (sum_ticks > cpuinfo->cpu_ticks_delta)
                cpuinfo->cpu_ticks_delta = sum_ticks - cpuinfo->cpu_ticks_delta;
        else
                cpuinfo->cpu_ticks_delta = 0;

        sysm_update_status_update(&cpuinfo->state);

        return ret;
}

sysm_errno_t cpu_info_init(struct cpu_info *cpuinfo)
{
        if (!cpuinfo)
                return SYSM_FAILURE;

        memset(cpuinfo, 0, sizeof(struct cpu_info));
        cpuinfo->state = SYSM_STATE_FIRST_UPDATE;
        
        cpuinfo->core_count = sysconf(_SC_NPROCESSORS_ONLN);
        
        if (!cpuinfo->core_count)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to retrieve the number of CPU cores\n");

        cpuinfo->cores = calloc(cpuinfo->core_count, sizeof(struct cpu_core_info));

        if (!cpuinfo->cores)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to memory allocation from libc allocator\n");

        return SYSM_SUCCESS;
}

sysm_errno_t cpu_info_update(struct cpu_info *cpuinfo)
{
        sysm_errno_t ret;

        if (!cpuinfo)
                return SYSM_FAILURE;

        ret = cpu_info_update_first(cpuinfo);

        if (ret)
                return ret;

        sysm_sleep();

        return cpu_info_update_last(cpuinfo);
}

void cpu_info_destroy(struct cpu_info *cpuinfo)
{
        if (!cpuinfo)
                return;

        if (cpuinfo->cores)
                free(cpuinfo->cores);

        memset(cpuinfo, 0, sizeof(struct cpu_info));
}