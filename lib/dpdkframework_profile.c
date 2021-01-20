#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_common.h>

#include "dkfw_profile.h"

void dkfw_profile_init(DKFW_PROFILE *profile, int item_cnt)
{
    memset(profile, 0, sizeof(DKFW_PROFILE));
    profile->item_cnt = (item_cnt ? item_cnt : MAX_PROFILE_ITEM_NUM);
}

