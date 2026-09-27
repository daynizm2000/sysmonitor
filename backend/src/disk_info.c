#include "../include/sysmonitor.h"
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <stdlib.h>

#define PFS_MOUNTS_NAME_POS 0
#define PFS_MOUNTS_MOUNT_POS 1
#define PFS_MOUNTS_FS_POS 2
#define PFS_MOUNTS_LAST_POS PFS_MOUNTS_FS_POS

#define PFS_DISKSTATS_SEC_READ_POS 5
#define PFS_DISKSTATS_SEC_WRITE_POS 9

#define LINUX_SECTOR_SIZE 512

mlib_mem_allocator_t disk_part_cache;

sysm_errno_t disk_info_init(struct disk_info *diskinfo)
{
        if (!diskinfo)
                return SYSM_FAILURE;

        memset(diskinfo, 0, sizeof(struct disk_info));
        mlib_list_head_init(&diskinfo->parts);

        if (mlib_mem_allocator_cache_init(&disk_part_cache,
                NULL, sizeof(struct disk_partition), 0, 0) < 0)

                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to create cache allocator for disk_partition struct\n");

        return SYSM_SUCCESS;
}

static inline void pfs_mount_parse_word_for_disk_part(const char *base_word,
        unsigned int word, struct disk_partition *part)
{
        unsigned int wordlen = 0;

        while (base_word[wordlen] && !isspace(base_word[wordlen]))
                wordlen++;

        switch (word) {
                case PFS_MOUNTS_NAME_POS: {
                        base_word += sizeof("/dev/") - 1;
                        wordlen -= sizeof("/dev/") - 1;

                        if (wordlen > SYSM_LINUX_NAME_MAXLEN)
                                wordlen = SYSM_LINUX_NAME_MAXLEN;

                        strncpy(part->disk_name, base_word, wordlen);
                        break;
                }
                case PFS_MOUNTS_MOUNT_POS: {
                        if (wordlen > SYSM_LINUX_FPATH_MAXLEN)
                                wordlen = SYSM_LINUX_FPATH_MAXLEN;

                        strncpy(part->mount_point, base_word, wordlen);
                        break;
                }
                case PFS_MOUNTS_FS_POS: {
                        if (wordlen > SYSM_LINUX_FS_NAME_MAXLEN)
                                wordlen = SYSM_LINUX_FS_NAME_MAXLEN;

                        strncpy(part->fs, base_word, wordlen);
                        break;
                }
        }
}

static void pfs_mounts_parse_for_disk_part(const char *base_line,
        struct disk_partition *part)
{
        unsigned int word = 0;

        for (const char *l = base_line; *l != '\n'; word++) {
                if (word > PFS_MOUNTS_LAST_POS)
                        break;

                pfs_mount_parse_word_for_disk_part(l, word, part);

                while (*l && !isspace(*l))
                        l++;

                if (*l == '\n')
                        break;

                while (*l != '\n' && isspace(*l))
                        l++;
        }
}

static void statvfs_parse_for_disk_part(struct disk_partition *part)
{
        struct statvfs stvfs;

        if (statvfs(part->mount_point, &stvfs) < 0)
                sysm_log_ret(, SYSM_WARN "Failed to check VFS staticstic for %s chapter\n", part->mount_point);

        part->total_b = stvfs.f_blocks * stvfs.f_frsize;
        part->used_b = (stvfs.f_blocks - stvfs.f_bfree) * stvfs.f_frsize;

        if (stvfs.f_blocks > 0)
                part->usage_pct = ((stvfs.f_blocks - stvfs.f_bfree) * 100.0) / stvfs.f_blocks;
        else
                part->usage_pct = 0.0;
}

static inline sysm_errno_t disk_parts_update(mlib_list_head_t *head)
{
        char fdata[8192];
        ssize_t fsize;
        mlib_list_head_t *curr;
        const char *pattern;

        if (sysm_cached_fds)
                fsize = sysm_read_file(sysm_fds_get_fd(sysm_cached_fds->pfs_mounts), fdata, sizeof(fdata));
        else
                fsize = sysm_read_file_from_path("/proc/mounts", fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to read file /proc/mounts\n");

        curr = mlib_list_next(head);

        if (strncmp(fdata, "/dev/", sizeof("/dev/") - 1) == 0)
                pattern = fdata;
        else
                pattern = strstr(fdata, "\n/dev/");
                
        while (pattern) {
                struct disk_partition *part;

                if (*pattern == '\n')
                        pattern++;

                if (mlib_list_is_head(curr, head)) {
                        part = disk_part_cache.alloc(&disk_part_cache);

                        if (!part)
                                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to memory allocation in disk_partition cache allocator\n");

                        memset(part, 0, sizeof(struct disk_partition));
                        mlib_list_add_tail(&part->list, head);
                        curr = &part->list;
                }
                else {
                        part = mlib_list_entry(curr, struct disk_partition, list);
                }

                pfs_mounts_parse_for_disk_part(pattern, part);
                statvfs_parse_for_disk_part(part);

                pattern = strstr(pattern + sizeof("/dev/") - 1, "\n/dev/");
                curr = mlib_list_next(curr);
        }

        while (!mlib_list_is_head(curr, head)) {
                struct disk_partition *p = mlib_list_entry(curr, struct disk_partition, list);

                curr = curr->next;

                mlib_list_del(&p->list);
                disk_part_cache.free(&disk_part_cache, p);
        }

        return SYSM_SUCCESS;
}

static inline struct disk_partition *find_main_disk_part(mlib_list_head_t *head)
{
        struct disk_partition *iter;

        mlib_list_for_each_entry(iter, head, list)
                if (strcmp(iter->mount_point, "/") == 0)
                        return iter;

        return NULL;
}

static char *pfs_diskstats_find_line(const char *fdata, size_t fsize,
        const char *diskname)
{
        char buff[SYSM_LINUX_NAME_MAXLEN + 3];
        unsigned int len;
        char *ret;

        len = snprintf(buff, sizeof(buff), " %s ", diskname);
        ret = memmem(fdata, fsize, buff, len);

        if (!ret)
                return NULL;

        while (ret != fdata && *ret != '\n')
                ret--;

        if (ret == fdata)
                return ret;

        while (isspace(*ret))
                ret++;

        return ret;
}

static void pfs_diskstats_parse_line(const char *base_line,
        unsigned int *sort_words, unsigned long long *retvals, unsigned int count)
{
        unsigned int word = 0;
        unsigned int idx = 0;

        while (*base_line != '\n' && idx < count) {
                if (word == sort_words[idx]) {
                        retvals[idx] = atoll(base_line);
                        idx++;
                }

                while (!isspace(*base_line))
                        base_line++;

                if (*base_line == '\n')
                        break;

                while (*base_line != '\n' && isspace(*base_line))
                        base_line++;

                word++;
        }
}

static sysm_errno_t pfs_diskstats_parse_file(const char *diskname, unsigned int *sort_words,
        unsigned long long *retvals, unsigned int count)
{
        char fdata[4096];
        const char *line;
        ssize_t fsize;

        if (sysm_cached_fds)
                fsize = sysm_read_file(sysm_fds_get_fd(sysm_cached_fds->pfs_diskstats), fdata, sizeof(fdata));
        else
                fsize = sysm_read_file_from_path("/proc/diskstats", fdata, sizeof(fdata));

        if (fsize < 0)
                sysm_log_ret(SYSM_FAILURE, SYSM_ERR "Failed to read file /proc/diskstats\n");

        line = pfs_diskstats_find_line(fdata, fsize, diskname);

        if (!line)
                sysm_log_ret(SYSM_FAILURE, SYSM_WARN "The disk information line was not found in /proc/diskstats\n");

        pfs_diskstats_parse_line(line, sort_words, retvals, count);

        return SYSM_SUCCESS;
}

static sysm_errno_t disk_info_update_speed_time(struct disk_info *diskinfo)
{
        sysm_errno_t ret;
        unsigned int words[] = {
                PFS_DISKSTATS_SEC_READ_POS,
                PFS_DISKSTATS_SEC_WRITE_POS
        };
        unsigned long long retvals[sizeof(words) / sizeof(*words)] = {0};
        const struct disk_partition *mpart;

        mpart = find_main_disk_part(&diskinfo->parts);

        if (!mpart) {
                sysm_log(SYSM_WARN "Main disk partition not found\n");
        }
        else {
                ret = pfs_diskstats_parse_file(mpart->disk_name, words, retvals, sizeof(words) / sizeof(*words));

                if (ret != SYSM_SUCCESS)
                        return ret;
        }
        
        diskinfo->read_speed_b_sec = retvals[0];
        diskinfo->write_speed_b_sec = retvals[1];

        return SYSM_SUCCESS;
}

sysm_errno_t disk_info_update_first(struct disk_info *diskinfo)
{
        if (!diskinfo)
                return SYSM_FAILURE;

        disk_parts_update(&diskinfo->parts);

        return disk_info_update_speed_time(diskinfo);
}

sysm_errno_t disk_info_update_last(struct disk_info *diskinfo)
{
        unsigned long long old_read_speed_b_sec;
        unsigned long long old_write_speed_b_sec;
        sysm_errno_t ret;

        if (!diskinfo)
                return SYSM_FAILURE;

        old_read_speed_b_sec = diskinfo->read_speed_b_sec;
        old_write_speed_b_sec = diskinfo->write_speed_b_sec;

        ret = disk_info_update_speed_time(diskinfo);

        if (ret != SYSM_SUCCESS)
                sysm_log(SYSM_WARN "Failed to get speed time for disk\n");

        diskinfo->read_speed_b_sec = (diskinfo->read_speed_b_sec >= old_read_speed_b_sec)
                        ? diskinfo->read_speed_b_sec - old_read_speed_b_sec : 0;
                
        diskinfo->write_speed_b_sec = (diskinfo->write_speed_b_sec >= old_write_speed_b_sec)
                        ? diskinfo->write_speed_b_sec - old_write_speed_b_sec : 0;

        diskinfo->read_speed_b_sec *= LINUX_SECTOR_SIZE / (double)SYSM_UPDATE_INTERVAL_SEC;
        diskinfo->write_speed_b_sec *= LINUX_SECTOR_SIZE / (double)SYSM_UPDATE_INTERVAL_SEC;

        return SYSM_SUCCESS;
}

sysm_errno_t disk_info_update(struct disk_info *diskinfo)
{
        sysm_errno_t ret;

        if (!diskinfo)
                return SYSM_FAILURE;

        ret = disk_info_update_first(diskinfo);

        if (ret != SYSM_SUCCESS)
                return ret;

        sysm_sleep();

        return disk_info_update_last(diskinfo);
}

void disk_info_destroy(struct disk_info *diskinfo)
{
        if (!diskinfo)
                return;

        if (!mlib_list_empty(&diskinfo->parts)) {
                struct disk_partition *iter;
                struct disk_partition *tmp;

                mlib_list_for_each_entry_safe(iter, tmp, &diskinfo->parts, list) {
                        mlib_list_del(&iter->list);
                        disk_part_cache.free(&disk_part_cache, iter);
                }
        }

        mlib_mem_allocator_destroy(&disk_part_cache);

        memset(diskinfo, 0, sizeof(struct disk_info));
}
