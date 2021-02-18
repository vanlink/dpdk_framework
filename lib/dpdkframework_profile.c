#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rte_common.h>

#include "dkfw_profile.h"

void dkfw_profile_init(DKFW_PROFILE *profile, int item_cnt, int single_cnt)
{
    memset(profile, 0, sizeof(DKFW_PROFILE));
    profile->item_cnt = (item_cnt ? item_cnt : MAX_PROFILE_ITEM_NUM);
    profile->single_cnt = single_cnt;
}

cJSON *dkfw_profile_to_json(DKFW_PROFILE *profile)
{
    int i;
    char buff[64];
    cJSON *root = cJSON_CreateObject();
    cJSON *json_array;

    cJSON_AddItemToObject(root, "item_cnt", cJSON_CreateNumber(profile->item_cnt));

    sprintf(buff, "%lu", profile->loops_cnt);
    cJSON_AddItemToObject(root, "loops_cnt", cJSON_CreateString(buff));

    sprintf(buff, "%lu", profile->all_time);
    cJSON_AddItemToObject(root, "all_time", cJSON_CreateString(buff));

    json_array = cJSON_CreateArray();
    for(i=0;i<profile->item_cnt;i++){
        sprintf(buff, "%lu", profile->items_time[i]);
        cJSON_AddItemToArray(json_array, cJSON_CreateString(buff));
    }
    cJSON_AddItemToObject(root, "items_time", json_array);

    json_array = cJSON_CreateArray();
    for(i=0;i<profile->item_cnt;i++){
        sprintf(buff, "%lu", profile->items_time_cnt[i]);
        cJSON_AddItemToArray(json_array, cJSON_CreateString(buff));
    }
    cJSON_AddItemToObject(root, "items_time_cnt", json_array);

    json_array = cJSON_CreateArray();
    for(i=0;i<profile->single_cnt;i++){
        sprintf(buff, "%lu", profile->single_time[i]);
        cJSON_AddItemToArray(json_array, cJSON_CreateString(buff));
    }
    cJSON_AddItemToObject(root, "singles_time", json_array);

    json_array = cJSON_CreateArray();
    for(i=0;i<profile->single_cnt;i++){
        sprintf(buff, "%lu", profile->single_time_cnt[i]);
        cJSON_AddItemToArray(json_array, cJSON_CreateString(buff));
    }
    cJSON_AddItemToObject(root, "singles_time_cnt", json_array);

    return root;
}

