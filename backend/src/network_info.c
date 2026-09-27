#include "../include/sysmonitor.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#define PFS_NET_DEV_READ_BYTES_POS 1
#define PFS_NET_DEV_WRITE_BYTES_POS 9

static inline void pfs_net_dev_parse_line(const char *line,
        unsigned int *sort_words, unsigned long long *retvals,
        unsigned int count)
{
        for (unsigned int word = 0, idx = 0; *line != '\n' && idx < count; ) {
                if (sort_words[idx] == word) {
                        retvals[idx] = atoll(line);
                        idx++;
                }

                while (!isspace(*line))
                        line++;

                if (*line == '\n')
                        break;

                while (*line != '\n' && isspace(*line))
                        line++;

                word++;
        }
}

static sysm_errno_t network_info_update_bytes(struct network_info *netwinfo)
{
        char fdata[4096];
        ssize_t fsize;
        unsigned int words[] = {
                PFS_NET_DEV_READ_BYTES_POS,
                PFS_NET_DEV_WRITE_BYTES_POS
        };
        unsigned long long retvals[sizeof(words) / sizeof(*words)] = {0};
        const char *pattern;

        if (sysm_cached_fds)
                fsize = sysm_read_file(sysm_fds_get_fd(sysm_cached_fds->pfs_net_dev), fdata, sizeof(fdata));
        else
                fsize = sysm_read_file_from_path("/proc/net/dev", fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to read file /proc/net/dev\n");

        netwinfo->read_speed_b_sec = 0;
        netwinfo->write_speed_b_sec = 0;

        pattern = strstr(fdata, ":");

        while (pattern) {
                const char *line = pattern;

                while (!isspace(*line))
                        line--;

                line++;

                pfs_net_dev_parse_line(line, words, retvals, sizeof(words) / sizeof(*words));

                netwinfo->read_speed_b_sec += retvals[0];
                netwinfo->write_speed_b_sec += retvals[1];

                pattern = strstr(pattern + 1, ":");
        }

        return SYSM_SUCCESS;
}

sysm_errno_t network_info_update_first(struct network_info *netwinfo)
{
        if (!netwinfo)
                return SYSM_FAILURE;

        return network_info_update_bytes(netwinfo);
}

sysm_errno_t network_info_update_last(struct network_info *netwinfo)
{
        double old_read_bytes;
        double old_write_bytes;
        sysm_errno_t ret;

        if (!netwinfo)
                return SYSM_FAILURE;

        old_read_bytes= netwinfo->read_speed_b_sec;
        old_write_bytes = netwinfo->write_speed_b_sec;

        ret = network_info_update_bytes(netwinfo);

        if (ret != SYSM_SUCCESS) {
                netwinfo->read_speed_b_sec = 0;
                netwinfo->write_speed_b_sec = 0;

                return ret;
        }

        netwinfo->read_speed_b_sec = (netwinfo->read_speed_b_sec >= old_read_bytes)
                ? (netwinfo->read_speed_b_sec - old_read_bytes) / (double)SYSM_UPDATE_INTERVAL_SEC : 0;

        netwinfo->write_speed_b_sec = (netwinfo->write_speed_b_sec >= old_write_bytes)
                ? (netwinfo->write_speed_b_sec - old_write_bytes) / (double)SYSM_UPDATE_INTERVAL_SEC : 0;

        return SYSM_SUCCESS;
}

void network_info_init(struct network_info *netwinfo)
{
        if (!netwinfo)
                return;

        memset(netwinfo, 0, sizeof(struct network_info));
}

sysm_errno_t network_info_update(struct network_info *netwinfo)
{
        sysm_errno_t ret;

        if (!netwinfo)
                return SYSM_FAILURE;

        ret = network_info_update_first(netwinfo);

        if (ret != SYSM_SUCCESS)
                return ret;

        sysm_sleep();

        return network_info_update_last(netwinfo);
}