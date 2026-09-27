#include "../include/sysmonitor.h"
#include <ctype.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#define PROC_PID_STAT_NAME_LINE 2
#define PROC_PID_STAT_UTIME_LINE 14
#define PROC_PID_STAT_STIME_LINE 15
#define PROC_PID_STAT_THREADS_LINE 20
#define PROC_PID_STAT_LAST_LINE PROC_PID_STAT_THREADS_LINE
#define PROC_PID_STAT_COUNT 4

#define PROC_PID_STATM_RSS_LINE 2
#define PROC_PID_STATM_LAST_LINE PROC_PID_STATM_RSS_LINE

static mlib_mem_allocator_t proc_info_cache;
static struct sysm_internal_info proc_internal_info;
static struct sysm_internal_info proc_table_internal_info;

static inline void proc_info_clear(struct proc_info *procinfo)
{
        memset(procinfo->name, 0, sizeof(procinfo->name));
        procinfo->cpu_usage_pct = 0;
        procinfo->mem_usage_pct = 0;
        procinfo->pid = -1;
        procinfo->rss_b = 0;
        procinfo->threads = 0;
}

void proc_info_init(struct proc_info *procinfo)
{
        proc_info_clear(procinfo);
}

static inline void pfs_pid_stat_word_parse(struct proc_info *procinfo,
        const char *base_word, unsigned int word)
{
        const char *val;
        unsigned int val_len;

        val = base_word;
        val_len = 0;

        if (*val == '(') {
                val++;

                while (val[val_len] != ')')
                        val_len++;
        }
        else {
                while (!isspace(val[val_len]))
                        val_len++;
        }

        switch (word) {
                case PROC_PID_STAT_NAME_LINE: {
                        if (val_len > SYSM_LINUX_COMM_MAXLEN)
                                val_len = SYSM_LINUX_COMM_MAXLEN;

                        strncpy(procinfo->name, val, val_len);
                        break;
                }
                case PROC_PID_STAT_UTIME_LINE: {
                        procinfo->__utime = atol(val);
                        break;       
                }
                case PROC_PID_STAT_STIME_LINE: {
                        procinfo->__stime = atol(val);
                        break;
                }
                case PROC_PID_STAT_THREADS_LINE: {
                        procinfo->threads = atoi(val);
                        break;
                }
        }
}

static sysm_errno_t pfs_pid_stat_parse_words(struct proc_info *procinfo,
        unsigned int *sort_words, unsigned int count)
{
        char fdata[4096];
        ssize_t fsize;
        char fpath[64];
        unsigned int word;
        bool in_quotes;

        snprintf(fpath, sizeof(fpath), "/proc/%d/stat", procinfo->pid);

        fsize = sysm_read_file_from_path(fpath, fdata, sizeof(fdata));

        if (fsize < 0)
               sysm_log_ret(SYSM_FAILURE, SYSM_WARN "Failed to read file %s\n", fpath);

        word = 1;
        in_quotes = false;

        for (ssize_t i = 0, j = 0; i < fsize && word <= PROC_PID_STAT_LAST_LINE && j < count; word++) {
                if (word == sort_words[j]) {
                        pfs_pid_stat_word_parse(procinfo, &fdata[i], word);
                        j++;
                }

                if (in_quotes) {
                        while (i < fsize && fdata[i] != ')')
                                i++;

                        i++;
                        in_quotes = false;
                }
                else {
                        while (i < fsize && !isspace(fdata[i]))
                                i++;
                }

                while (i < fsize && isspace(fdata[i]))
                        i++;

                if (i < fsize && fdata[i] == '(')
                        in_quotes = true;
        }

        return SYSM_SUCCESS;
}

static inline sysm_errno_t pfs_pid_stat_parse(struct proc_info *procinfo)
{
        unsigned int words[PROC_PID_STAT_COUNT] = {
                PROC_PID_STAT_NAME_LINE, PROC_PID_STAT_UTIME_LINE,
                PROC_PID_STAT_STIME_LINE, PROC_PID_STAT_THREADS_LINE};

        return pfs_pid_stat_parse_words(procinfo, words, PROC_PID_STAT_COUNT);
}

static inline void pfs_pid_statm_word_parse(struct proc_info *procinfo,
        const char *base_word, unsigned int word)
{
        const char *val;
        unsigned int val_len;

        val = base_word;
        val_len = 0;

        while (!isspace(val[val_len]))
                val_len++;

        switch (word) {
                case PROC_PID_STATM_RSS_LINE: {
                        procinfo->rss_b = atol(val) * sysconf(_SC_PAGESIZE);
                        break;
                }
        }
}

static sysm_errno_t pfs_pid_statm_parse(struct proc_info *procinfo)
{
        char fpath[64];
        char fdata[4096];
        ssize_t fsize;
        unsigned int word;

        snprintf(fpath, sizeof(fpath), "/proc/%d/statm", procinfo->pid);

        fsize = sysm_read_file_from_path(fpath, fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to read file %s\n", fpath);

        word = 1;

        for (ssize_t i = 0; i < fsize && word <= PROC_PID_STATM_LAST_LINE; ) {
                pfs_pid_statm_word_parse(procinfo, &fdata[i], word);

                while (i < fsize && !isspace(fdata[i]))
                        i++;

                while (i < fsize && isspace(fdata[i]))
                        i++;
                
                word++;
        }

        return SYSM_SUCCESS;
}

sysm_errno_t proc_info_update_first(struct proc_info *procinfo,
        struct sysm_internal_info *internal_info, pid_t pid)
{
        if (!procinfo || pid < 0)
                return SYSM_FAILURE;

        procinfo->pid = pid;

        if (!internal_info) {
                internal_info = &proc_internal_info;
                sysm_internal_update_first(internal_info);
        }
        
        pfs_pid_stat_parse(procinfo);
        pfs_pid_statm_parse(procinfo);

        if (procinfo->rss_b && internal_info->memtotal_b)
                procinfo->mem_usage_pct = (double)procinfo->rss_b / (double)internal_info->memtotal_b * 100;

        return SYSM_SUCCESS;
}

static inline void proc_info_cpu_usage_parse(struct proc_info *procinfo, struct sysm_internal_info *internal_info)
{
       unsigned int pfs_stat_words[] = {
                PROC_PID_STAT_STIME_LINE,
                PROC_PID_STAT_UTIME_LINE
        };
        unsigned long old_stime;
        unsigned long old_utime;
        unsigned long long old_total_ticks;
        unsigned long long total_ticks;

        old_stime = procinfo->__stime;
        old_utime = procinfo->__utime;

        pfs_pid_stat_parse_words(procinfo, pfs_stat_words,
                sizeof(pfs_stat_words) / sizeof(*pfs_stat_words));

        old_total_ticks = old_stime + old_utime;
        total_ticks = procinfo->__stime + procinfo->__utime;

        if (total_ticks >= old_total_ticks && internal_info->cpu_ticks_delta > 0) {
                procinfo->cpu_usage_pct =
                        (double)(total_ticks - old_total_ticks)
                        / (double)internal_info->cpu_ticks_delta * 100 * sysconf(_SC_NPROCESSORS_ONLN);
        }
        else {
                procinfo->cpu_usage_pct = 0;
        }
}

sysm_errno_t proc_info_update_last(struct proc_info *procinfo, struct sysm_internal_info *internal_info)
{
        if (!procinfo)
                return SYSM_FAILURE;

        if (!internal_info) {
                internal_info = &proc_internal_info;
                sysm_internal_update_last(internal_info);
        }
        
        proc_info_cpu_usage_parse(procinfo, internal_info);

        return SYSM_SUCCESS;
}

sysm_errno_t proc_info_update(struct proc_info *procinfo, struct sysm_internal_info *internal_info, pid_t pid)
{
        sysm_errno_t ret;

        ret = proc_info_update_first(procinfo, internal_info, pid);

        if (ret != SYSM_SUCCESS)
                return ret;

        sysm_sleep();

        return proc_info_update_last(procinfo, internal_info);
}

void proc_info_destroy(struct proc_info *procinfo)
{
        proc_info_clear(procinfo);
}

sysm_errno_t proc_info_table_init(mlib_list_head_t *head)
{
        int ret;

        if (!head)
                return SYSM_FAILURE;

        ret = mlib_mem_allocator_cache_init(&proc_info_cache,
                NULL, sizeof(struct proc_info), 0, 0);

        if (ret)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to init cache allocator for proc_info struct\n");

        mlib_list_head_init(head);

        sysm_log_ret(SYSM_SUCCESS, SYSM_LOG "Success process info table init\n");
}

static inline bool strisdigits(const char *str)
{
        for (const char *ptr = str; *ptr; ptr++)
                if (!isdigit(*ptr))
                        return false;

        return true;
}

sysm_errno_t proc_info_table_update_first(mlib_list_head_t *head, struct sysm_internal_info *internal_info)
{
        DIR *dir;
        struct dirent *entry;
        mlib_list_head_t *curr;

        if (!head)
                return SYSM_FAILURE;

        dir = opendir("/proc");

        if (!dir)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to opendir /proc/\n");

        if (!internal_info) {
                internal_info = &proc_table_internal_info;
                sysm_internal_update_first(internal_info);
        }

        curr = mlib_list_next(head);
        
        while ((entry = readdir(dir))) {
                struct proc_info *pinf;

                if (strcmp(entry->d_name, ".") == 0
                        || strcmp(entry->d_name, "..") == 0
                        || !strisdigits(entry->d_name))
                                continue;

                if (mlib_list_is_head(curr, head)) {
                        pinf = proc_info_cache.alloc(&proc_info_cache);

                        if (!curr) {
                                sysm_log(SYSM_ERR "Failed to allocate proc_info obj in cache allocator\n");
                                continue;
                        }

                        proc_info_init(pinf);
                        mlib_list_add_tail(&pinf->list, head);
                        curr = &pinf->list;
                }
                else {
                        pinf = mlib_list_entry(curr, struct proc_info, list);
                        proc_info_clear(pinf);
                }

                proc_info_update_first(pinf, internal_info, atoi(entry->d_name));
                curr = mlib_list_next(curr);
        }

        closedir(dir);

        while (!mlib_list_is_head(curr, head)) {
                struct proc_info *p = mlib_list_entry(curr, struct proc_info, list);

                curr = mlib_list_next(curr);

                proc_info_destroy(p);
                mlib_list_del(&p->list);
                proc_info_cache.free(&proc_info_cache, p);
        }

        return SYSM_SUCCESS;
}

sysm_errno_t proc_info_table_update_last(mlib_list_head_t *head, struct sysm_internal_info *internal_info)
{
        struct proc_info *iter;

        if (!head)
                return SYSM_FAILURE;

        if (!internal_info) {
                internal_info = &proc_table_internal_info;
                sysm_internal_update_last(internal_info);
        }
        
        mlib_list_for_each_entry(iter, head, list) {
                pid_t pid = iter->pid;

                if (proc_info_update_last(iter, internal_info) != SYSM_SUCCESS)
                        sysm_log(SYSM_WARN
                                "Failed to two part update process pid=%d, in process table\n",
                                pid);
        }

        return SYSM_SUCCESS;
}

sysm_errno_t proc_info_table_update(mlib_list_head_t *head, struct sysm_internal_info *internal_info)
{
        sysm_errno_t ret;

        ret = proc_info_table_update_first(head, internal_info);

        if (ret != SYSM_SUCCESS)
                return ret;

        sysm_sleep();

        return proc_info_table_update_last(head, internal_info);
}

void proc_info_table_destroy(mlib_list_head_t *head)
{
        struct proc_info *iter;
        struct proc_info *tmp;
        
        mlib_list_for_each_entry_safe(iter, tmp, head, list) {
                proc_info_destroy(iter);
                mlib_list_del(&iter->list);
                proc_info_cache.free(&proc_info_cache, iter);
        }

        mlib_mem_allocator_destroy(&proc_info_cache);

        sysm_log(SYSM_LOG "Process info table has destroyed\n");
}
