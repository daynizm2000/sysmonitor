#include "../include/sysmonitor.h"
#include <memory.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

struct sysm_fds *sysm_cached_fds = NULL;
static struct sysm_internal_info sysm_internal_info;
int sysm_logger_fd = STDERR_FILENO;
unsigned int sysm_update_interval_sec = SYSM_UPDATE_INTERVAL_SEC;

static unsigned long sysm_uptime_sec_update(void)
{
        struct sysinfo info;

        if (sysinfo(&info))
                sysm_log_ret(0, SYSM_WARN "Failed to struct sysinfo init\n");

        return info.uptime;
}

static sysm_errno_t sysm_loadavg_update(double loadavg[3])
{
        if (getloadavg(loadavg, 3) < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_WARN "Failed to getloadavg\n");

        return SYSM_SUCCESS;
}

static sysm_errno_t sysm_fds_init(struct sysm_fds *fds)
{
        memset(fds, -1, sizeof(struct sysm_fds));

        fds->pfs_meminfo = open("/proc/meminfo", O_RDONLY);
        if (fds->pfs_meminfo < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to open /proc/meminfo\n");

        fds->pfs_stat = open("/proc/stat", O_RDONLY);
        if (fds->pfs_stat < 0)
                sysm_log_goto(fail, SYSM_ERR "Failed to open /proc/stat\n");

        fds->pfs_net_dev = open("/proc/net/dev", O_RDONLY);
        if (fds->pfs_net_dev < 0)
                sysm_log_goto(fail, SYSM_ERR "Failed to open /proc/net/dev\n");

        fds->sysfs_cpu0_scaling_cur_freq = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
                O_RDONLY);
        if (fds->sysfs_cpu0_scaling_cur_freq < 0)
                sysm_log_goto(fail, SYSM_ERR "Failed to open /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq\n");

        fds->sysfs_cpu0_cpuinfo_max_freq = open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
                O_RDONLY);
        if (fds->sysfs_cpu0_cpuinfo_max_freq < 0)
                sysm_log_goto(fail, SYSM_ERR "Failed to open /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq\n");

        fds->pfs_mounts = open("/proc/mounts", O_RDONLY);
        if (fds->pfs_mounts < 0)
                sysm_log_goto(fail, SYSM_ERR "Failed to open /proc/mounts\n");

        fds->pfs_diskstats = open("/proc/diskstats", O_RDONLY);
        if (fds->pfs_diskstats < 0)
                sysm_log_goto(fail, SYSM_ERR "Failed to open /proc/diskstats\n");

        sysm_cached_fds = fds;

        sysm_log_ret(SYSM_SUCCESS, SYSM_LOG "Success opening system info files\n");

fail:
        if (fds->pfs_meminfo >= 0)
                close(fds->pfs_meminfo);

        if (fds->pfs_stat >= 0)
                close(fds->pfs_stat);

        if (fds->pfs_net_dev >= 0)
                close(fds->pfs_net_dev);

        if (fds->sysfs_cpu0_scaling_cur_freq >= 0)
                close(fds->sysfs_cpu0_scaling_cur_freq);

        if (fds->sysfs_cpu0_cpuinfo_max_freq >= 0)
                close(fds->sysfs_cpu0_cpuinfo_max_freq);

        if (fds->pfs_mounts >= 0)
                close(fds->pfs_mounts);

        sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to opening system info files\n");
}

static void sysm_fds_destroy(struct sysm_fds *fds)
{
        if (fds->pfs_meminfo >= 0)
                close(fds->pfs_meminfo);

        if (fds->pfs_stat >= 0)
                close(fds->pfs_stat);

        if (fds->pfs_net_dev >= 0)
                close(fds->pfs_net_dev);

        if (fds->sysfs_cpu0_scaling_cur_freq >= 0)
                close(fds->sysfs_cpu0_scaling_cur_freq);

        if (fds->sysfs_cpu0_cpuinfo_max_freq >= 0)
                close(fds->sysfs_cpu0_cpuinfo_max_freq);

        if (fds->pfs_mounts >= 0)
                close(fds->pfs_mounts);

        if (fds->pfs_diskstats >= 0)
                close(fds->pfs_diskstats);

        sysm_cached_fds = NULL;
        memset(fds, 0, sizeof(struct sysm_fds));
}

sysm_errno_t sysm_init(struct sysmonitor *sysm)
{
        sysm_errno_t ret;

        if (!sysm)
                return SYSM_FAILURE;

        memset(sysm, 0, sizeof(struct sysmonitor));
        mlib_list_head_init(&sysm->proc_table);

        ret = sysm_fds_init(&sysm->fds);

        if (ret != SYSM_SUCCESS)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to open system files\n");

        ret = sysm_sys_info_init(&sysm->sys);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to update system info\n");

        ret = proc_info_table_init(&sysm->proc_table);

        if (ret != SYSM_SUCCESS)
                sysm_log_goto(err, SYSM_ERR "Failed to process info table init\n");
        
        ret = cpu_info_init(&sysm->cpu);

        if (ret != SYSM_SUCCESS)
                sysm_log_goto(err1, SYSM_ERR "Failed to cpu info struct init\n");

        disk_info_init(&sysm->disk);
        mem_info_init(&sysm->mem);
        network_info_init(&sysm->network);
        swap_info_init(&sysm->swap);

        sysm_log_ret(SYSM_SUCCESS, SYSM_LOG "Success system monitor struct init\n");
err1:
        proc_info_table_destroy(&sysm->proc_table);
err:
        sysm_fds_destroy(&sysm->fds);

        return ret;
}

sysm_errno_t sysm_update_first(struct sysmonitor *sysm)
{
        sysm_errno_t ret;

        if (!sysm)
                return SYSM_FAILURE;

        ret = mem_info_update(&sysm->mem);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to update memory info\n");

        ret = swap_info_update(&sysm->swap);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to update swap info\n");

        ret = cpu_info_update_first(&sysm->cpu);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to first part update CPU info\n");

        sysm_internal_info.memtotal_b = sysm->mem.total_gib * 1024 * 1024 * 1024;
        sysm_internal_info.cpu_ticks_delta = sysm->cpu.cpu_ticks_delta;

        ret = proc_info_table_update_first(&sysm->proc_table, &sysm_internal_info);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to first part update process info table\n");

        ret = disk_info_update_first(&sysm->disk);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to first part update disk info\n");

        ret = network_info_update_first(&sysm->network);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to first part update network info\n");

        return SYSM_SUCCESS;
}

sysm_errno_t sysm_update_last(struct sysmonitor *sysm)
{
        sysm_errno_t ret;

        if (!sysm)
                return SYSM_FAILURE;

        ret = cpu_info_update_last(&sysm->cpu);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to two part update CPU info\n");

        sysm_internal_info.cpu_ticks_delta = sysm->cpu.cpu_ticks_delta;

        ret = proc_info_table_update_last(&sysm->proc_table, &sysm_internal_info);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to two part update process info table\n");

        ret = disk_info_update_last(&sysm->disk);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to last part update disk info\n");

        ret = network_info_update_last(&sysm->network);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to last part update network info\n");

        sysm->uptime_sec = sysm_uptime_sec_update();

        if (!sysm->uptime_sec)
                sysm_log(SYSM_WARN "Failed to update uptime\n");

        ret = sysm_loadavg_update(sysm->load_avg);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to update loadavg\n");

        sysm_log(SYSM_LOG "Sysmonitor has updated\n");

        return SYSM_SUCCESS;
}

sysm_errno_t sysm_update(struct sysmonitor *sysm)
{
        sysm_errno_t ret;

        if (!sysm)
                return SYSM_FAILURE;

        ret = sysm_update_first(sysm);

        if (ret != SYSM_SUCCESS)
                return ret;

        sysm_sleep();

        sysm_update_last(sysm);

        return SYSM_SUCCESS;
}

void sysm_destroy(struct sysmonitor *sysm)
{
        sysm_fds_destroy(&sysm->fds);
        proc_info_table_destroy(&sysm->proc_table);
        cpu_info_destroy(&sysm->cpu);
        disk_info_destroy(&sysm->disk);

        sysm_log(SYSM_LOG "Sysmonitor has destroyed\n");
}
