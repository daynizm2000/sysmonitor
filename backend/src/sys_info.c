#include "../include/sysmonitor.h"
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

static inline void sys_info_set_distr(struct sys_info *sys_info)
{
        char buffer[4096];
        char *start;
        bool in_quotes = false;
        unsigned int len = 0;
        ssize_t bufsize;

        bufsize = sysm_read_file_from_path("/etc/os-release", buffer, sizeof(buffer));

        if (bufsize < 0)
                sysm_log_ret(, SYSM_ERR "Failed to read: /etc/os-release\n");

        start = memmem(buffer, bufsize, "PRETTY_NAME=", sizeof("PRETTY_NAME=") - 1);

        if (!start)
                goto parse_err;

        start += sizeof("PRETTY_NAME=") - 1;

        if (*start == '"')
                in_quotes = true;

        start++;

        while (&start[len] < &buffer[bufsize] && start[len] != '\n') {
                if (in_quotes && start[len] == '"')
                        break;

                len++;
        }

        if (len > SYSM_LINUX_DISTR_NAME_MAXLEN)
                len = SYSM_LINUX_DISTR_NAME_MAXLEN;

        strncpy(sys_info->distr, start, len);

        return;
parse_err:
        sysm_log(SYSM_WARN "Failed to parse linux-distribution name in /etc/os-release\n");
}

static inline void sys_info_set_kernel_ver(struct sys_info *sys_info, const struct utsname *uts)
{
        strcpy(sys_info->kernel_ver, uts->release);
}

static inline void sys_info_set_arch(struct sys_info *sys_info, const struct utsname *uts)
{
        strcpy(sys_info->arch, uts->machine);
}

static inline void sys_info_set_cpu_name(struct sys_info *sys_info)
{
        char buffer[8192];
        ssize_t bufsize;
        unsigned int len = 0;
        char *start;

        bufsize = sysm_read_file_from_path("/proc/cpuinfo", buffer, sizeof(buffer));

        if (bufsize < 0)
                sysm_log_ret(, SYSM_ERR "Failed to open /proc/cpuinfo\n");

        start = memmem(buffer, bufsize, "model name", sizeof("model name") - 1);

        if (!start)
                return;

        start += sizeof("model name") - 1;

        while (*start != ':') {
                start++;

                if (start >= &buffer[bufsize])
                        goto parse_err;
        }

        start++;
        
        while (isspace(*start)) {
                start++;

                if (start >= &buffer[bufsize])
                        goto parse_err;
        }

        while (&start[len] < &buffer[bufsize] && start[len] != '\n')
                len++;

        if (len > SYSM_CPU_NAME_MAXLEN)
                len = SYSM_CPU_NAME_MAXLEN;

        strncpy(sys_info->cpu_name, start, len);
        
        return;
parse_err:
        sysm_log(SYSM_WARN "Failed to parse CPU name in /proc/cpuinfo\n");
}

static inline void sysm_read_file_for_str(const char *path, char *buffer, size_t len)
{
        ssize_t res = sysm_read_file_from_path(path, buffer, len);

        if (res < 0)
                sysm_log_ret(, SYSM_ERR "Failed to read %s\n", path);

        buffer[res] = '\0';
}

static inline void str_del_last_n(char *str)
{
        char *n = strrchr(str, '\n');
        
        if (n)
                *n = '\0';
}

static inline void motherboard_info_init(struct motherboard_info *mb_info)
{
        sysm_read_file_for_str("/sys/class/dmi/id/board_vendor",
                mb_info->vendor, SYSM_MOTHERBOARD_VENDOR_MAXLEN);

        sysm_read_file_for_str("/sys/class/dmi/id/board_name",
                mb_info->name, SYSM_MOTHERBOARD_NAME_MAXLEN);

        sysm_read_file_for_str("/sys/class/dmi/id/bios_version",
                mb_info->bios_ver, SYSM_MOTHERBOARD_BIOS_VER_MAXLEN);

        str_del_last_n(mb_info->vendor);
        str_del_last_n(mb_info->name);
        str_del_last_n(mb_info->bios_ver);
}

sysm_errno_t sysm_sys_info_init(struct sys_info *sys_info)
{
        struct utsname uts;

        memset(sys_info, 0, sizeof(struct sys_info));

        if (!uname(&uts)) {
                sys_info_set_kernel_ver(sys_info, &uts);
                sys_info_set_arch(sys_info, &uts);
        }
        else
                sysm_log(SYSM_WARN "Failed to uname(), errno=%s\n", strerror(errno));

        sys_info_set_distr(sys_info);
        sys_info_set_cpu_name(sys_info);
        motherboard_info_init(&sys_info->motherboard);

        return SYSM_SUCCESS;
}