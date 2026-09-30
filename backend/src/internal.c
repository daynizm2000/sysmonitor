#include "../include/sysmonitor.h"
#include <fcntl.h>
#include <unistd.h>

static unsigned long long get_sys_cpu_ticks(void)
{
        char fdata[4096];
        ssize_t fsize;
        unsigned long long u = 0, ni = 0, s = 0, id = 0, io = 0, irq = 0, sirq = 0, steal = 0;

        if (sysm_cached_fds)
                fsize = sysm_read_file(sysm_cached_fds_get_fd(SYSM_FDS_PFS_STAT), fdata, sizeof(fdata));
        else
                fsize = sysm_read_file_from_path("/proc/stat", fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(0, SYSM_ERR "Failed to read file /proc/stat\n");
        
        if (sscanf(fdata, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", 
                   &u, &ni, &s, &id, &io, &irq, &sirq, &steal) >= 4) {
                
                return u + ni + s + id + io + irq + sirq + steal;
        }

        return 0;
}

sysm_errno_t sysm_internal_update_first(struct sysm_internal_info *internal_info)
{
        if (!internal_info)
                return SYSM_FAILURE;

        internal_info->memtotal_b = sysm_meminfo_file_parse("MemTotal") * 1024;
        internal_info->cpu_ticks_delta = get_sys_cpu_ticks();

        return SYSM_SUCCESS;
}

sysm_errno_t sysm_internal_update_last(struct sysm_internal_info *internal_info)
{
        unsigned long long cpu_ticks;

        if (!internal_info)
                return SYSM_FAILURE;

        cpu_ticks = get_sys_cpu_ticks();

        if (cpu_ticks > internal_info->cpu_ticks_delta)
                internal_info->cpu_ticks_delta = cpu_ticks - internal_info->cpu_ticks_delta;
        else
                internal_info->cpu_ticks_delta = 0;

        return SYSM_SUCCESS;
}