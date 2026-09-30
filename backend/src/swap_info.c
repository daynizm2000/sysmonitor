#include "../include/sysmonitor.h"
#include <string.h>

void swap_info_init(struct swap_info *swapinfo)
{
        if (!swapinfo)
                return;

        memset(swapinfo, 0, sizeof(struct swap_info));
}

sysm_errno_t swap_info_update(struct swap_info *swapinfo)
{
        unsigned long long swaptotal;
        unsigned long long swapfree;
        const char *keys[] = {"SwapTotal", "SwapFree"};
        unsigned long long rets[ARRAY_SIZE(keys)] = {0};

        if (!swapinfo)
                return SYSM_FAILURE;

        sysm_meminfo_file_parse_lines(keys, rets, ARRAY_SIZE(keys));

        swaptotal = rets[0];
        swapfree = rets[1];

        if (!swaptotal || !swapfree) {
                swapinfo->total_gib = 0;
                swapinfo->usage_pct = 0;
                swapinfo->used_gib = 0;
        
                return SYSM_SUCCESS;
        }

        swapinfo->total_gib = KB_TO_GIB(swaptotal);
        swapinfo->used_gib = KB_TO_GIB(swaptotal - swapfree);
        swapinfo->usage_pct = (double)(swaptotal - swapfree) / swaptotal * 100;

        return SYSM_SUCCESS;
}