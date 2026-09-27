#include "../include/sysmonitor.h"
#include <string.h>

void mem_info_init(struct mem_info *meminfo)
{
        if (!meminfo)
                return;

        memset(meminfo, 0, sizeof(struct mem_info));
}

sysm_errno_t mem_info_update(struct mem_info *meminfo)
{
        unsigned long long memtotal;
        unsigned long long memavail;
        const char *keys[2] = {"MemTotal", "MemAvailable"};
        unsigned long long rets[2] = {0};

        if (!meminfo)
                return SYSM_FAILURE;

        sysm_meminfo_file_parse_lines(keys, rets, 2);
        memtotal = rets[0];
        memavail = rets[1];

        if (!memtotal || !memavail)
                return SYSM_FAILURE;

        meminfo->total_gib = SYSM_KB_TO_GIB(memtotal);
        meminfo->used_gib = SYSM_KB_TO_GIB(memtotal - memavail);
        meminfo->usage_pct = (double)(memtotal - memavail) / memtotal * 100;

        return SYSM_SUCCESS;
}