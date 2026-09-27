#pragma once

#include "gui.h"

SysmDiskPart *sysm_disk_part_new(const struct disk_partition *info);

void sysm_disk_table_init(struct sysm_disk_table *table, const char *title);
void sysm_disk_table_update(struct sysm_disk_table *table, mlib_list_head_t *parts);