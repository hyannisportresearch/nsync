/**
 * @file nsync_ubuntu.c
 * Ubuntu driver for nsync
 * 
 * @author Ben London
 * @date 8/11/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_ubuntu.h"
#include <time.h>

state_func_t ubuntu_state_funcs = {
    .check_OS = NULL,
    .get_config = &ubuntu_get_config,
    .get_unsynced = &ubuntu_get_unsynced,
    .check_exist = &ubuntu_check_persistent_files,
    .compare_config = &ubuntu_compare_configs,
    .backup = &ubuntu_backup_files,
    .overwrite = &ubuntu_overwrite_configs,
    .create_write = &ubuntu_create_and_write_to_file,
    .keep_existing = &ubuntu_keep_existing,
    .if_synced = &ubuntu_mark_synced,
    .done = &ubuntu_cleanup_and_free,
};
cmd_list_t ubuntu_cmd_list = {
    .get_OS = "cat /etc/lsb-release",
    .get_if_list = "ip link",
    .get_active_if_cfg = "ip addr show %s",
    .get_routes = "ip route",
};

/**
 * @brief Gets all information about the current persistent network configuration files
 * and information about the active network configuration and stores all the data in the
 * struct provided
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. If the function is successful, it should return
 * the NSYNC_GET_UNSYNCED state. If unsuccessful, will return the NSYNC_ERROR state.
 */
nsync_state_t ubuntu_get_config(net_sync_info_t *info)
{
    ubuntu_parse_func_t *parsers = (ubuntu_parse_func_t *)info->parsers;

    info->net_config = calloc(1, sizeof(ubuntu_net_cfg_t));
    MEM_CHECK(info->net_config, NSYNC_ERROR);
    
    /** Get Active Interfaces */
    UBUNTU_ACTIVE_IFS = parsers->ubuntu_parse_active_interfaces(UBUNTU_GET_IF_LIST, UBUNTU_GET_ACTIVE_IF_CFG);
    if (!UBUNTU_ACTIVE_IFS) return NSYNC_ERROR;

    /** Get Active Routes */
    UBUNTU_ACTIVE_ROUTES = parsers->ubuntu_parse_active_routes(UBUNTU_GET_ROUTES);
    if (!UBUNTU_ACTIVE_ROUTES) return NSYNC_ERROR;

    /** Map Active Routes to Active Interfaces */
    if(!parsers->ubuntu_map_routes(UBUNTU_ACTIVE_IFS, UBUNTU_ACTIVE_ROUTES)){
        return NSYNC_ERROR;
    }

    /** Get saved routes */
    char route_file[FILENAME_MAX];
    sprintf(route_file, "%s%s", CFG_FILE_LOC, ROUTE_FILE);
    UBUNTU_PERSIST_ROUTES = parsers->ubuntu_parse_persist_routes(route_file);
    if (!UBUNTU_PERSIST_ROUTES) return NSYNC_ERROR;

    /** Get saved Inferface Configs */
    char if_file[FILENAME_MAX];
    sprintf(if_file, "%s%s", CFG_FILE_LOC, CFG_FILE);
    UBUNTU_PERSIST_IFS = parsers->ubuntu_parse_persist_interfaces(if_file);
    if (!UBUNTU_PERSIST_IFS) return NSYNC_ERROR;

    /** Map Persistent Routes to Persistent Interfaces */
    if(!parsers->ubuntu_map_routes(UBUNTU_PERSIST_IFS, UBUNTU_PERSIST_ROUTES)){
        return NSYNC_ERROR;
    }


    /** Print some general info about what was parsed */
    if (info->verbose) {
        system("clear;");
        printf("##################################################################\n\n");
        printf("Found the following active interfaces:");
        for (int i = 0; i < UBUNTU_ACTIVE_IF_NUM; i++) {
            printf(" %s", UBUNTU_ACTIVE_IF_LIST_NAME(i));
        }
        printf("\n\n");

        for (int i = 0; i < UBUNTU_ACTIVE_IF_NUM; i++) {
            printf("Interface %s", UBUNTU_ACTIVE_IF_NAME(i));
            
            if(!UBUNTU_ACTIVE_IF_ROUTE_NUM(i)) {
                printf(" has no routes associated.\n\n");
                continue;
            }

            printf(" has the following route(s):\n");
            for (int j = 0; j < UBUNTU_ACTIVE_IF_ROUTE_NUM(i); j++) {
                printf("%s\n", UBUNTU_ACTIVE_IF_ROUTE(i, j));
            }
            printf("\n");
        }
        printf("##################################################################\n\n");
        
        printf("Found the following persistent interfaces:");
        for (int i = 0; i < UBUNTU_PERSIST_IF_NUM; i++) {
            printf(" %s", UBUNTU_PERSIST_IF_LIST_NAME(i));
        }
        printf("\n\n");

        for (int i = 0; i < UBUNTU_PERSIST_IF_NUM; i++) {
            printf("Interface %s", UBUNTU_PERSIST_IF_NAME(i));
            if(!UBUNTU_PERSIST_IF_ROUTE_NUM(i)) {
                printf(" has no routes associated.\n\n");
                continue;
            }
            printf(" has the following route(s):\n");
            for (int j = 0; j < UBUNTU_PERSIST_IF_ROUTE_NUM(i); j++) {
                printf("%s\n", UBUNTU_PERSIST_IF_ROUTE(i, j));
            }
            printf("\n");
        }
        printf("##################################################################\n\n");
    }

    return NSYNC_GET_UNSYNCED;
}


/**
 * @brief Sets and determines the next interface that needs to be synced
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. If there is still an interface that hasn't been 
 * synced then it returns NSYNC_CHECK_EXIST. If all interfaces have been synced it 
 * will return NSYNC_DONE.
 */
nsync_state_t ubuntu_get_unsynced(net_sync_info_t *info)
{
    ubuntu_net_cfg_t *net_config = (ubuntu_net_cfg_t *)info->net_config;

    /** Find next unsynced interface */
    int i;
    for (i = 0; i < net_config->active_ifs->num_if; i++) {
        if(!info->synced[i]) break;
    }

    /** If none are unsynced then we are done! */
    if (i == net_config->active_ifs->num_if) 
        return NSYNC_DONE;

    /** Save the next unsynced interface index */
    info->next_to_sync = i;

    if(info->verbose) {
        printf("Syncing %s ......\n",net_config->active_ifs->if_name_list[i]);
    }

    return NSYNC_CHECK_EXIST;
}

/**
 * @brief Checks for the existence of the persistent configuration file
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. If a persistent file exists, then 
 * it returns NSYNC_COMPARE_CONFIG. If there are not persistent files, then the
 * function returns NSYNC_CREATE_WRITE. If there is an error then the function
 * return NSYNC_ERROR.
 */
nsync_state_t ubuntu_check_persistent_files(net_sync_info_t *info)
{
    if (strcmp(CFG_FILE, info->route_file) != 0) {
        sprintf(err_msg, "the persistent configurations should all be stored together in Ubuntu 16.04");
        return NSYNC_ERROR;
    }


    char interface_file[FILENAME_MAX];
    sprintf(interface_file, "%s%s", CFG_FILE_LOC, CFG_FILE);
    // No need to check routes because they are stored in the same location
    
    if (file_exists(interface_file))
        return NSYNC_COMPARE_CONFIG;

    return NSYNC_CREATE_WRITE;
}


/**
 * @brief Creates the interface configuration file
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. Will return NSYNC_IF_SYNCED state if 
 * it is successful, but will return NSYNC_ERROR in the case of failure
 */
nsync_state_t ubuntu_create_and_write_to_file(net_sync_info_t *info)
{
    FILE * fp;
    
    int i = info->next_to_sync;

    /** Make sure there is an active configuration for this interface */
    if(UBUNTU_ACTIVE_INTERFACE(i) == NULL){
        return NSYNC_IF_SYNCED;
    }

    /** Build filepath */
    char cfg_file[FILENAME_MAX];
    sprintf(cfg_file, "%s%s.tmp", CFG_FILE_LOC, CFG_FILE);
    
    fp = fopen(cfg_file, "a");
    if (fp == NULL){
        sprintf(err_msg, "Could not open file for writing/appending: %s", cfg_file);
        return NSYNC_ERROR;
    }
    
    /** Print out details of this interface */
    if(UBUNTU_ACTIVE_IF_AUTO_OPT(i)) 
        fprintf(fp, "auto %s\n", UBUNTU_ACTIVE_IF_NAME(i));
    
    fprintf(fp, "iface %s inet %s\n", UBUNTU_ACTIVE_IF_NAME(i), UBUNTU_ACTIVE_IF_LINKTYPE(i));
    
    if (strcmp("static", UBUNTU_ACTIVE_IF_LINKTYPE(i)) == 0){
        fprintf(fp, "address %s\n", UBUNTU_ACTIVE_IF_ADDRESS(i));
        
        if (UBUNTU_ACTIVE_IF_NETMASK(i)) 
            fprintf(fp, "netmask %s\n", UBUNTU_ACTIVE_IF_NETMASK(i));

        if (UBUNTU_ACTIVE_IF_BROADCAST(i)) 
            fprintf(fp, "broadcast %s\n", UBUNTU_ACTIVE_IF_BROADCAST(i));

        if (UBUNTU_ACTIVE_IF_METRIC(i)) 
            fprintf(fp, "metric %s\n", UBUNTU_ACTIVE_IF_METRIC(i));

        if (UBUNTU_ACTIVE_IF_HWADDRESS(i)) 
            fprintf(fp, "hwaddress %s\n", UBUNTU_ACTIVE_IF_HWADDRESS(i));

        if (UBUNTU_ACTIVE_IF_GATEWAY(i)) 
            fprintf(fp, "gateway %s\n", UBUNTU_ACTIVE_IF_GATEWAY(i));

        if (UBUNTU_ACTIVE_IF_MTU(i)) 
            fprintf(fp, "mtu %s\n", UBUNTU_ACTIVE_IF_MTU(i));

        if (UBUNTU_ACTIVE_IF_SCOPE(i)) 
            fprintf(fp, "scope %s\n", UBUNTU_ACTIVE_IF_SCOPE(i));

        if (UBUNTU_ACTIVE_IF_UNMANAGED(i)) 
            fprintf(fp, "%s\n", UBUNTU_ACTIVE_IF_UNMANAGED(i));

        
        for (int j = 0; j < UBUNTU_ACTIVE_IF_ROUTE_NUM(i); j++) {
            fprintf(fp, "up %s\n", UBUNTU_ACTIVE_IF_ROUTE(i,j));
        }
    }
    fclose(fp);
    return NSYNC_IF_SYNCED;
}


/**
 * @brief Compares active and persistent configurations to determine if any changes
 * have been made since the persistent files were last edited
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. Will return NSYNC_KEEP_EXISTING if the
 * persistent interface config matches the active configuration. But will return 
 * NSYNC_BACKUP if they don't.
 */
nsync_state_t ubuntu_compare_configs(net_sync_info_t *info)
{
    /** compare basics first */
    if (UBUNTU_ACTIVE_IF_NUM != UBUNTU_PERSIST_IF_NUM){
        return NSYNC_BACKUP;
    }
    
    int i = info->next_to_sync;
    bool match = true;

    /** 
     * Check that values are either both null or both not null (i.e. matching)
     * If they aren't null then check that they have the same value.
     */

    if(!(!(UBUNTU_ACTIVE_IF_NAME(i)) != !(UBUNTU_PERSIST_IF_NAME(i)))) {
        if (UBUNTU_ACTIVE_IF_NAME(i) && strcmp(UBUNTU_PERSIST_IF_NAME(i), UBUNTU_ACTIVE_IF_NAME(i)) != 0) {
            match = false;
        }
    } 
    else 
        match = false;

    /** If the interface is loopback or dhcp, then the rest of the fields dont apply */
    if(!(!(UBUNTU_ACTIVE_IF_LINKTYPE(i)) != !(UBUNTU_PERSIST_IF_LINKTYPE(i)))) {
        if (UBUNTU_ACTIVE_IF_LINKTYPE(i) && strcmp(UBUNTU_PERSIST_IF_LINKTYPE(i), UBUNTU_ACTIVE_IF_LINKTYPE(i)) != 0) {
            match = false;
        } else if (strcmp("loopback", UBUNTU_ACTIVE_IF_LINKTYPE(i)) == 0 || strcmp("dhcp", UBUNTU_ACTIVE_IF_LINKTYPE(i)) == 0){
            return NSYNC_KEEP_EXISTING;
        }
    } 
    else match = false;

    if(!(!(UBUNTU_ACTIVE_IF_ADDRESS(i)) != !(UBUNTU_PERSIST_IF_ADDRESS(i)))) {
        if (UBUNTU_ACTIVE_IF_ADDRESS(i) && strcmp(UBUNTU_PERSIST_IF_ADDRESS(i), UBUNTU_ACTIVE_IF_ADDRESS(i)) != 0) {
            match = false;
        }
    } else match = false;

    if(!(!(UBUNTU_ACTIVE_IF_NETMASK(i)) != !(UBUNTU_PERSIST_IF_NETMASK(i)))) {
        if (UBUNTU_ACTIVE_IF_NETMASK(i) && strcmp(UBUNTU_PERSIST_IF_NETMASK(i), UBUNTU_ACTIVE_IF_NETMASK(i)) != 0) {
            match = false;
        }
    } else match = false;

    if(!(!(UBUNTU_ACTIVE_IF_BROADCAST(i)) != !(UBUNTU_PERSIST_IF_BROADCAST(i)))) {
        if (UBUNTU_ACTIVE_IF_BROADCAST(i) && strcmp(UBUNTU_PERSIST_IF_BROADCAST(i), UBUNTU_ACTIVE_IF_BROADCAST(i)) != 0) {
            match = false;
        }
    } else match = false;

    if(!(!(UBUNTU_ACTIVE_IF_HWADDRESS(i)) != !(UBUNTU_PERSIST_IF_HWADDRESS(i)))) {
        if (UBUNTU_ACTIVE_IF_HWADDRESS(i) && strcmp(UBUNTU_PERSIST_IF_HWADDRESS(i), UBUNTU_ACTIVE_IF_HWADDRESS(i)) != 0) {
            match = false;
        }
    } else match = false;

    if(!(!(UBUNTU_ACTIVE_IF_MTU(i)) != !(UBUNTU_PERSIST_IF_MTU(i)))) {
        if (UBUNTU_ACTIVE_IF_MTU(i) && strcmp(UBUNTU_PERSIST_IF_MTU(i), UBUNTU_ACTIVE_IF_MTU(i)) != 0) {
            match = false;
        }
    } else match = false;

    if(!(!(UBUNTU_ACTIVE_IF_SCOPE(i)) != !(UBUNTU_PERSIST_IF_SCOPE(i)))) {
        if (UBUNTU_ACTIVE_IF_SCOPE(i) && strcmp(UBUNTU_PERSIST_IF_SCOPE(i), UBUNTU_ACTIVE_IF_SCOPE(i)) != 0) {
            match = false;
        }  
    } else match = false;


    if(!(UBUNTU_ACTIVE_IF_AUTO_OPT(i) != UBUNTU_PERSIST_IF_AUTO_OPT(i))) {
        if (UBUNTU_ACTIVE_IF_AUTO_OPT(i) != UBUNTU_PERSIST_IF_AUTO_OPT(i)) {
            match = false;
        }
    } else match = false;
    
    
    /** Check routes */
    if (UBUNTU_ACTIVE_IF_ROUTE_NUM(i) != UBUNTU_PERSIST_IF_ROUTE_NUM(i)){
        return NSYNC_BACKUP;
    }

    for (int j = 0; j < UBUNTU_ACTIVE_IF_ROUTE_NUM(i); j++){
        if (strcmp(UBUNTU_ACTIVE_IF_ROUTE(i,j), UBUNTU_PERSIST_IF_ROUTE(i,j)))
            match= false;
    }

    if (match){ 
        return NSYNC_KEEP_EXISTING;
    }

    return NSYNC_BACKUP;
}


/** 
 * @brief Keeps existing persistent configurations for the
 * current interface by wriuting them to a temp file
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. Will return NSYNC_IF_SYNCED if the
 * writing was successful and NSYNC_ERROR is it fails to write to the temp file.
 */
nsync_state_t ubuntu_keep_existing(net_sync_info_t *info)
{
    char cfg_file[FILENAME_MAX];
    
    sprintf(cfg_file, "%s%s.tmp",CFG_FILE_LOC, CFG_FILE); 

    /** Open the file for appending */
    FILE *fp = fopen(cfg_file, "a");
    if (fp == NULL){
        sprintf(err_msg, "could not open file '%s' for appending", cfg_file);
        return NSYNC_ERROR;
    }

    /** Append the perisstent configuration of the interface to temp config file */
    int i = info->next_to_sync;

    if(UBUNTU_PERSIST_IF_AUTO_OPT(i)) 
        fprintf(fp, "auto %s\n", UBUNTU_PERSIST_IF_NAME(i));
    
    fprintf(fp, "iface %s inet %s\n", UBUNTU_PERSIST_IF_NAME(i), UBUNTU_PERSIST_IF_LINKTYPE(i));
    
    if (strcmp("static", UBUNTU_PERSIST_IF_LINKTYPE(i)) == 0){

        fprintf(fp, "address %s\n", UBUNTU_PERSIST_IF_ADDRESS(i));

        if (UBUNTU_PERSIST_IF_NETMASK(i)) 
            fprintf(fp, "netmask %s\n", UBUNTU_PERSIST_IF_NETMASK(i));

        if (UBUNTU_PERSIST_IF_BROADCAST(i)) 
            fprintf(fp, "broadcast %s\n", UBUNTU_PERSIST_IF_BROADCAST(i));

        if (UBUNTU_PERSIST_IF_METRIC(i)) 
            fprintf(fp, "metric %s\n", UBUNTU_PERSIST_IF_METRIC(i));

        if (UBUNTU_PERSIST_IF_HWADDRESS(i)) 
            fprintf(fp, "hwaddress %s\n", UBUNTU_PERSIST_IF_HWADDRESS(i));

        if (UBUNTU_PERSIST_IF_GATEWAY(i)) 
            fprintf(fp, "gateway %s\n", UBUNTU_PERSIST_IF_GATEWAY(i));

        if (UBUNTU_PERSIST_IF_MTU(i)) 
            fprintf(fp, "mtu %s\n", UBUNTU_PERSIST_IF_MTU(i));

        if (UBUNTU_PERSIST_IF_SCOPE(i)) 
            fprintf(fp, "scope %s\n", UBUNTU_PERSIST_IF_SCOPE(i));

        if (UBUNTU_PERSIST_IF_UNMANAGED(i)) 
            fprintf(fp, "%s", UBUNTU_PERSIST_IF_UNMANAGED(i));

        for (int j = 0; j < UBUNTU_PERSIST_IF_ROUTE_NUM(i); j++) {
            fprintf(fp, "up ip route add %s\n", UBUNTU_PERSIST_IF_ROUTE(i,j));
        }
    }
    fprintf(fp, "\n");

    fclose(fp);

    return NSYNC_IF_SYNCED;
}

/**
 * @brief Backs up the single interfaces file by making system level calls.
 * Because Ubuntu 16.04 keep all configs in a single file, it only needs to
 * be backed up once per run of the utility.
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @return the next state of the utility, which will always be NSYNC_OVERWRITE.
 */
nsync_state_t ubuntu_backup_files(net_sync_info_t *info)
{
    /** All details stored in a single file. Only needs to be backup up once */
    if (info->backup.complete)
        return NSYNC_OVERWRITE;


    time_t now = time(NULL);
    char timestamp[MAX_CMD_LEN];
    strftime(timestamp, 20, "%Y%m%d", localtime(&now));


    const char *location = info->backup.user_path;
    if (!info->backup.backup_set){
        location = info->backup.default_path;
    }

    char *dir_fmt = "nsync.%s";
    char dir[FILENAME_MAX];
    sprintf(dir, dir_fmt, timestamp);

    char *full_path_fmt = "%s%s";
    char full_path[FILENAME_MAX];
    sprintf(full_path, full_path_fmt, location, dir);

    int ver = 1;
    while(dir_check(full_path)){
        char *dir_num_fmt = "nsync.%s.%02i";
        sprintf(dir, dir_num_fmt, timestamp, ver);
        sprintf(full_path, full_path_fmt, location, dir);
        ver++;
    }

    if (!dir_check(full_path)){
        char mkdir_cmd[MAX_CMD_LEN];
        sprintf(mkdir_cmd, "mkdir %s", full_path);
        if (system(mkdir_cmd) == -1){
            sprintf(err_msg, "command `%s` failed", mkdir_cmd);
            return NSYNC_ERROR;
        }        
        info->backup.started = true;
    }

    char *backup_cmd_fmt = "cp %s %s";
    char backup_cmd[MAX_CMD_LEN];
    char backup_file[FILENAME_MAX];
    sprintf(backup_file, "%s%s", CFG_FILE_LOC, CFG_FILE);
    sprintf(backup_cmd, backup_cmd_fmt, backup_file, full_path);

    if (system(backup_cmd) == -1){
        sprintf(err_msg, "command `%s` failed", backup_cmd);
        return NSYNC_ERROR;
    }
    info->backup.complete = true;

    return NSYNC_OVERWRITE;
}


/**
 * @brief Marks the currently syncing interface as synced.
 * 
 * @param info pointer to the struct containing all info related to the nsync utility
 * @returns the next state of the utility -- NSYNC_GET_UNSYNCED
 */
nsync_state_t ubuntu_mark_synced(net_sync_info_t *info)
{
    if(info->verbose){
        printf("Done syncing interface\n\n");
    }
    info->synced[info->next_to_sync++] = true;
    return NSYNC_GET_UNSYNCED;
}


/**
 * @brief Overwrites any existing configuration by writing out new interface
 * and route configurations to a temp config file (that will later replace 
 * the existing config file)
 * 
 * @param info pointer to the struct containing all info related to the nsync utility
 * @returns the next state of the utility: NSYNC_IF_SYNCED. If there is an error
 * found during the execution of this function then it returns NSYNC_ERROR.
 */
nsync_state_t ubuntu_overwrite_configs(net_sync_info_t *info)
{
    char cfg_file[FILENAME_MAX];
    
    sprintf(cfg_file, "%s%s.tmp",CFG_FILE_LOC, CFG_FILE); 

    FILE *fp = fopen(cfg_file, "a");
    if (fp == NULL){
        sprintf(err_msg, "could not open file '%s' for appending", cfg_file);
        return NSYNC_ERROR;
    }

    int i = info->next_to_sync;

    if(UBUNTU_ACTIVE_IF_AUTO_OPT(i)) 
        fprintf(fp, "auto %s\n", UBUNTU_ACTIVE_IF_NAME(i));

    fprintf(fp, "iface %s inet %s\n", UBUNTU_ACTIVE_IF_NAME(i), UBUNTU_ACTIVE_IF_LINKTYPE(i));
    
    if (strcmp("static", UBUNTU_ACTIVE_IF_LINKTYPE(i)) == 0){
        fprintf(fp, "address %s\n", UBUNTU_ACTIVE_IF_ADDRESS(i));

        if (UBUNTU_ACTIVE_IF_NETMASK(i)) 
            fprintf(fp, "netmask %s\n", UBUNTU_ACTIVE_IF_NETMASK(i));

        if (UBUNTU_ACTIVE_IF_BROADCAST(i)) 
            fprintf(fp, "broadcast %s\n", UBUNTU_ACTIVE_IF_BROADCAST(i));

        if (UBUNTU_ACTIVE_IF_METRIC(i)) 
            fprintf(fp, "metric %s\n", UBUNTU_ACTIVE_IF_METRIC(i));

        if (UBUNTU_ACTIVE_IF_HWADDRESS(i)) 
            fprintf(fp, "hwaddress %s\n", UBUNTU_ACTIVE_IF_HWADDRESS(i));

        if (UBUNTU_ACTIVE_IF_GATEWAY(i)) 
            fprintf(fp, "gateway %s\n", UBUNTU_ACTIVE_IF_GATEWAY(i));

        if (UBUNTU_ACTIVE_IF_MTU(i)) 
            fprintf(fp, "mtu %s\n", UBUNTU_ACTIVE_IF_MTU(i));

        if (UBUNTU_ACTIVE_IF_SCOPE(i)) 
            fprintf(fp, "scope %s\n", UBUNTU_ACTIVE_IF_SCOPE(i));

        if (UBUNTU_ACTIVE_IF_UNMANAGED(i)) 
            fprintf(fp, "%s", UBUNTU_ACTIVE_IF_UNMANAGED(i));

        for (int j = 0; j < UBUNTU_ACTIVE_IF_ROUTE_NUM(i); j++) {
            fprintf(fp, "up ip route add %s\n", UBUNTU_ACTIVE_IF_ROUTE(i,j));
        }
    }
    fprintf(fp, "\n");

    fclose(fp);

    return NSYNC_IF_SYNCED;
}


/**
 * @brief Frees all heap allocated data in the provided net_sync_info struct. Additionaly, 
 * replaces the persistent config file with the temporary one that has been built up over the
 * execution of the utility.
 * 
 * @param info pointer to the struct containing all information relating to the nsync utility
 * @returns the final state of the utility -- NSYNC_SUCCESS
 */
nsync_state_t ubuntu_cleanup_and_free(net_sync_info_t *info)
{
    if (info->verbose) {
        printf("##################################################################\n\n");
        printf("Syncing complete!\n\n");
    }

    /** Overwrite/replace the cfg file with the tmp one that has the most updated details */
    char cmd[FILENAME_MAX];
    sprintf(cmd, "mv %s%s.tmp %s%s", CFG_FILE_LOC, CFG_FILE, CFG_FILE_LOC, CFG_FILE);
    if (system(cmd) == -1){
        sprintf(err_msg, "command `%s` failed", cmd);
        return NSYNC_ERROR;
    } 

    /** Free sys info */
    free(info->sys.os_str);

    for (int i = 0; i < UBUNTU_ACTIVE_ROUTES_NUM; i++){
        free(UBUNTU_ACTIVE_ROUTE(i));
    }
    for (int i = 0; i < UBUNTU_PERSIST_ROUTE_NUM; i++){
        free(UBUNTU_PERSIST_ROUTE(i));
    }

    /** Free network config stuff */
    for (int i = 0; i < UBUNTU_ACTIVE_IF_NUM; i++) {
        free(UBUNTU_ACTIVE_IF_LIST_NAME(i));

        free_interface(UBUNTU_ACTIVE_INTERFACE(i));
        free(UBUNTU_ACTIVE_INTERFACE(i));
    }
    for (int i = 0; i < UBUNTU_PERSIST_IF_NUM; i++) {
        free(UBUNTU_PERSIST_IF_LIST_NAME(i));

        free_interface(UBUNTU_PERSIST_INTERFACE(i));
        free(UBUNTU_PERSIST_INTERFACE(i));
    }

    free(UBUNTU_ACTIVE_IFS);
    free(UBUNTU_PERSIST_IFS);
    free(UBUNTU_ACTIVE_ROUTES);
    free(UBUNTU_PERSIST_ROUTES);
    free(UBUNTU_NET_CONFIG);
    
    return NSYNC_SUCCESS;
}
