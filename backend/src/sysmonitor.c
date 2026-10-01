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

const char *fpaths[SYSM_FDS_NR] = {
        "/proc/stat", "/proc/net/dev",
        "/proc/meminfo",
        "/proc/mounts", "/proc/diskstats",
        "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq",
        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
};

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
        memset(fds->fds, -1, ARRAY_SIZE(fds->fds));

        for (sysm_nfd_t i = 0; i < SYSM_FDS_NR; i++) {
                fds->fds[i] = open(fpaths[i], O_RDONLY);

                if (fds->fds[i] < 0)
                        sysm_log_goto(fail, SYSM_WARN "Failed to open: %s\n", fpaths[i]);
        }

        sysm_log_ret(SYSM_SUCCESS, SYSM_LOG "Success opening system info files\n");
fail:
        for (sysm_nfd_t i = 0; i < SYSM_FDS_NR; i++) {
                if (fds->fds[i] < 0)
                        break;

                close(fds->fds[i]);
        }

        sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to opening system info files\n");
}

static void sysm_fds_destroy(struct sysm_fds *fds)
{
        for (sysm_nfd_t i = 0; i < SYSM_FDS_NR; i++) {
                if (fds->fds[i] >= 0) {
                        close(fds->fds[i]);
                        fds->fds[i] = -1;
                }
        }
}

sysm_errno_t sysm_init(struct sysmonitor *sysm)
{
        sysm_errno_t ret;

        if (!sysm)
                return SYSM_FAILURE;

        memset(sysm, 0, sizeof(struct sysmonitor));
        mlib_list_head_init(&sysm->proc_table.list);

        ret = sysm_fds_init(&sysm->fds);

        if (ret)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to open system files\n");

        sysm_cached_fds = &sysm->fds;

        ret = sys_info_init(&sysm->sys);

        if (ret)
                sysm_log(SYSM_WARN "Failed to update system info\n");

        ret = proc_info_table_init(&sysm->proc_table);

        if (ret)
                sysm_log_goto(err, SYSM_ERR "Failed to process info table init\n");
        
        ret = cpu_info_init(&sysm->cpu);

        if (ret)
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

        if (ret)
                sysm_log(SYSM_WARN "Failed to update memory info\n");

        ret = swap_info_update(&sysm->swap);

        if (ret)
                sysm_log(SYSM_WARN "Failed to update swap info\n");

        ret = cpu_info_update_first(&sysm->cpu);

        if (ret)
                sysm_log(SYSM_WARN "Failed to first part update CPU info\n");

        sysm_internal_info.memtotal_b = sysm->mem.total_gib * 1024 * 1024 * 1024;
        sysm_internal_info.cpu_ticks_delta = sysm->cpu.cpu_ticks_delta;

        ret = proc_info_table_update_first(&sysm->proc_table, &sysm_internal_info);

        if (ret)
                sysm_log(SYSM_WARN "Failed to first part update process info table\n");

        ret = disk_info_update_first(&sysm->disk);

        if (ret)
                sysm_log(SYSM_WARN "Failed to first part update disk info\n");

        ret = network_info_update_first(&sysm->network);

        if (ret)
                sysm_log(SYSM_WARN "Failed to first part update network info\n");

        return SYSM_SUCCESS;
}

sysm_errno_t sysm_update_last(struct sysmonitor *sysm)
{
        sysm_errno_t ret;

        if (!sysm)
                return SYSM_FAILURE;

        ret = cpu_info_update_last(&sysm->cpu);

        if (ret)
                sysm_log(SYSM_WARN "Failed to two part update CPU info\n");

        sysm_internal_info.cpu_ticks_delta = sysm->cpu.cpu_ticks_delta;

        ret = proc_info_table_update_last(&sysm->proc_table, &sysm_internal_info);

        if (ret)
                sysm_log(SYSM_WARN "Failed to two part update process info table\n");

        ret = disk_info_update_last(&sysm->disk);

        if (ret)
                sysm_log(SYSM_WARN "Failed to last part update disk info\n");

        ret = network_info_update_last(&sysm->network);

        if (ret)
                sysm_log(SYSM_WARN "Failed to last part update network info\n");

        sysm->uptime_sec = sysm_uptime_sec_update();

        if (!sysm->uptime_sec)
                sysm_log(SYSM_WARN "Failed to update uptime\n");

        ret = sysm_loadavg_update(sysm->load_avg);

        if (ret)
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

        if (ret)
                return ret;

        sysm_sleep();

        sysm_update_last(sysm);

        return SYSM_SUCCESS;
}

void sysm_destroy(struct sysmonitor *sysm)
{
        sysm_fds_destroy(&sysm->fds);
        sysm_cached_fds = NULL;

        proc_info_table_destroy(&sysm->proc_table);
        cpu_info_destroy(&sysm->cpu);
        disk_info_destroy(&sysm->disk);

        sysm_log(SYSM_LOG "Sysmonitor has destroyed\n");
}
