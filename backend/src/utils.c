#include "../include/sysmonitor.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

ssize_t sysm_read_file(int fd, char *buffer, size_t bufsize)
{
        ssize_t read_bytes;
        size_t total_bytes;

        if (fd < 0 || !buffer || !bufsize)
                return -1;

        total_bytes = 0;

        while (total_bytes < bufsize) {
                read_bytes = read(fd, buffer + total_bytes, bufsize - total_bytes);

                if (!read_bytes)
                        break;

                if (read_bytes < 0)
                        return -1;

                total_bytes += read_bytes;
        }

        return total_bytes;
}

ssize_t sysm_read_file_from_path(const char *fpath, char *buffer, size_t bufsize)
{
        ssize_t ret;
        int fd = open(fpath, O_RDONLY);

        if (fd < 0)
                return -1;

        ret = sysm_read_file(fd, buffer, bufsize);

        close(fd);

        return ret;
}

unsigned long long sysm_meminfo_parse(const char *key, const char *filedata, size_t size)
{
        const char *line;
        size_t keylen;

        if (!key)
                return 0;

        keylen = strlen(key);
        line = memmem(filedata, size, key, keylen);

        if (!line)
                sysm_log_ret(0, SYSM_WARN "Not found value by key=%s from /proc/meminfo\n", key);

        line += keylen;

        while ((size_t)(line - filedata) < size && !isdigit(*line))
                line++;

        return atoll(line);
}

sysm_errno_t sysm_meminfo_file_parse_lines(const char **keys, unsigned long long *retvals, unsigned int count)
{
        char fdata[8192];
        ssize_t fsize;

        if (!keys || !retvals || !count)
                return SYSM_FAILURE;

        if (sysm_cached_fds)
                fsize = sysm_read_file(sysm_cached_fds_get_fd(SYSM_FDS_PFS_MEMINFO), fdata, sizeof(fdata));
        else
                fsize = sysm_read_file_from_path("/proc/meminfo", fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to read file /proc/meminfo\n");

        for (unsigned int i = 0; i < count; i++)
                retvals[i] = sysm_meminfo_parse(keys[i], fdata, fsize);

        return SYSM_SUCCESS;
}

unsigned long long sysm_meminfo_file_parse(const char *key)
{
        unsigned long long ret;

        if (!key)
                return 0;

        ret = 0;

        sysm_meminfo_file_parse_lines(&key, &ret, 1);

        return ret;
}