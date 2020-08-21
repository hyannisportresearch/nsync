/**
 * @file nsync_centos.c
 * Centos driver for nsync
 * 
 * @author Ben London
 * @date 7/27/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_centos.h"
#include <time.h>

state_func_t centos_state_funcs = {
    .check_OS = NULL,
    .get_config = &centos_get_config,
    .get_unsynced = &centos_get_unsynced,
    .check_exist = &centos_check_persistent_files,
    .compare_config = &centos_compare_configs,
    .backup = &centos_backup_files,
    .overwrite = &centos_overwrite_configs,
    .create_write = &centos_create_and_write_to_file,
    .keep_existing = &centos_keep_existing,
    .if_synced = &centos_mark_synced,
    .done = &centos_cleanup_and_free,
};

cmd_list_t centos_cmd_list = { 
    .get_OS = "cat /etc/centos-release",
    .get_if_list = "ip link",
    .get_active_if_cfg = "ip addr show %s",
    .get_routes = "ip route",
};

/**
 * @brief Gets all information about the current persistent network configuration files
 * and information about the active network configuration and stores all the data in the
 * struct provided. Upon completion, returns the next state of the utility
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. If the function is successful, it should return
 * the NSYNC_GET_UNSYNCED state. If unsuccessful, will return the NSYNC_ERROR state.
 */
nsync_state_t centos_get_config(net_sync_info_t *info)
{
    
    centos_parse_func_t parsers = *(centos_parse_func_t *)info->parsers;
    
    info->net_config = calloc(1, sizeof(centos_net_cfg_t));

    /** Get all the interfaces */
    if_list_parsed_t *if_parsed = parsers.parse_if_list(CENTOS_GET_IF_LIST); 
    if(!if_parsed) return NSYNC_ERROR;

    CENTOS_IF_LIST = if_parsed->if_list;
    CENTOS_NUM_IF = if_parsed->num_if;

    /** Get all the routes */
    routes_parsed_t *routes_parsed = parsers.parse_routes(CENTOS_GET_ACTIVE_ROUTES);
    if (!routes_parsed) return NSYNC_ERROR;

    CENTOS_ACTIVE_ROUTES = routes_parsed->route_list;
    CENTOS_ACTIVE_NUM_ROUTES = routes_parsed->num_route;

    /** map routes to their interfaces */
    map_routes_if_t mapped = parsers.map_routes_to_if(routes_parsed, if_parsed);

    CENTOS_MAPPED = mapped;


    /** Get all stored configs */
    char cfg_filepath_fmt[FILENAME_MAX];
    char cfg_filepath[FILENAME_MAX];
    int i;
    for(i = 0; i < CENTOS_NUM_IF; i++){ 
        sprintf(cfg_filepath_fmt, "%s%s", CFG_FILE_LOC, CFG_FILE);
        sprintf(cfg_filepath, cfg_filepath_fmt, CENTOS_IF_LIST_I(i));
        CENTOS_STORED_IFCFG(i)  = parsers.parse_ifcfg(cfg_filepath);
        if(CENTOS_STORED_IFCFG(i) == fatal_err_ptr) return NSYNC_ERROR;
    }

    /** Get all stored persistent routes */
    char rt_filepath_fmt[FILENAME_MAX];
    char rt_filepath[FILENAME_MAX];
    for(i = 0; i < CENTOS_NUM_IF; i++){ 
        sprintf(rt_filepath_fmt, "%s%s", CFG_FILE_LOC, ROUTE_FILE);
        sprintf(rt_filepath, rt_filepath_fmt, CENTOS_IF_LIST_I(i));
        CENTOS_PERSIST_ROUTES(i) = parsers.parse_persist_routes(rt_filepath);
        if (CENTOS_PERSIST_ROUTES(i) == fatal_err_ptr) return NSYNC_ERROR;
    }

    /** Get active network config */
    char get_link_fields_cmd[MAX_CMD_LEN];
    for(i = 0; i < CENTOS_NUM_IF; i++){ 
        sprintf(get_link_fields_cmd, CENTOS_GET_ACTIVE_CFG, CENTOS_IF_LIST_I(i));
        CENTOS_ACTIVE_CFG(i) = parsers.parse_ip_show(get_link_fields_cmd);
        if(CENTOS_ACTIVE_CFG(i) == fatal_err_ptr) return NSYNC_ERROR;
    }


    /** Print some general info about what was parsed */
    if (info->verbose) {
        printf("##################################################################\n\n");

        printf("Found the following active interfaces:");
        for (int i = 0; i < CENTOS_NUM_IF; i++) {
            printf(" %s", CENTOS_IF_LIST_I(i));
        }
        printf("\n\n");

        for (int i = 0; i < CENTOS_NUM_IF; i++) {
            printf("Interface %s",  CENTOS_IF_LIST_I(i));
            
            if(!CENTOS_MAPPED_I(i) || CENTOS_MAPPED_I_ROUTE_NUM(i) == 0)
            {
                printf(" has no routes associated.\n\n");
                continue;
            }

            printf(" has the following route(s):\n");
            for (int j = 0; j < CENTOS_MAPPED_I_ROUTE_NUM(i); j++) {
                printf("%s\n", CENTOS_MAPPED_I_ROUTE_J(i,j));
            }
            printf("\n");
        }

        printf("##################################################################\n\n");

        printf("Found the following persistent interfaces:");
        for (int i = 0; i < CENTOS_NUM_IF; i++) {
            if (CENTOS_STORED_IFCFG(i)) 
                printf(" %s", CENTOS_STORED_IFCFG_DEVICE(i));
        }
        printf("\n\n");

        for (int i = 0; i < CENTOS_NUM_IF; i++) {
            if (CENTOS_STORED_IFCFG(i)){
                printf("Interface %s", CENTOS_IF_LIST_I(i));

                if(!CENTOS_PERSIST_ROUTES(i) || CENTOS_PERSIST_ROUTES_NUM_ROUTE(i) == 0) {
                    printf(" has no routes associated.\n\n");
                    continue;
                }

                printf(" has the following route(s):\n");
                for (int j = 0; j < CENTOS_PERSIST_ROUTES_NUM_ROUTE(i); j++) {
                        printf("%s\n", CENTOS_PERSIST_ROUTES_ROUTE(i,j));
                }    
                printf("\n");
            }
        }
        printf("##################################################################\n\n");
    }

    /** Free the memory */
    free(if_parsed);
    free(routes_parsed);

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
nsync_state_t centos_get_unsynced(net_sync_info_t *info)
{
    /** Find next unsynced interface */
    int i;
    for (i = 0; i < CENTOS_NUM_IF; i++) {
        if(!info->synced[i]) break;
    }

    /** If none are unsynced then we are done! */
    if (i == CENTOS_NUM_IF) 
        return NSYNC_DONE;

    /** Save the next unsynced interface index */
    if(info->verbose) {
        printf("Syncing %s ......",CENTOS_IF_LIST_I(i));
    }

    info->next_to_sync = i;
    return NSYNC_CHECK_EXIST;
}


/**
 * @brief Checks for the existence of persistent route and ifcfg files
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. If a persistent files exists, then 
 * it returns NSYNC_COMPARE_CONFIG. If there are not persistent files, then the
 * function returns NSYNC_CREATE_WRITE.
 */
nsync_state_t centos_check_persistent_files(net_sync_info_t *info)
{
    /** Check for interface */
    char *interface = CENTOS_IF_LIST_I(info->next_to_sync);
    if (interface == NULL) {
        sprintf(err_msg, "interface is (null)");
        return NSYNC_ERROR;
    }

    /** Check for the route and ifcfg file */
    char route_file[FILENAME_MAX];
    char cfg_file[FILENAME_MAX];
    
    char route_file_fmt[FILENAME_MAX];
    char cfg_file_fmt[FILENAME_MAX];

    sprintf(cfg_file_fmt, "%s%s", CFG_FILE_LOC, CFG_FILE);
    sprintf(cfg_file, cfg_file_fmt, interface);

    sprintf(route_file_fmt, "%s%s", CFG_FILE_LOC, ROUTE_FILE);
    sprintf(route_file, route_file_fmt, interface);
    
    /** If a persistent file exists then compare configurations */
    if (file_exists(cfg_file))
        return NSYNC_COMPARE_CONFIG;

    return NSYNC_CREATE_WRITE;
}

/**
 * @brief Creates configuration files for both routes and ifcfg and populates
 * them wil the appropriate configuration values and commands.
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. Will return NSYNC_IF_SYNCED state if 
 * it is successful, but will 
 */
nsync_state_t centos_create_and_write_to_file(net_sync_info_t *info)
{
    FILE * fp;
    
    int i = info->next_to_sync;

    /** Make sure there is an active configuration for this interface */
    if(CENTOS_ACTIVE_CFG(i) == NULL){
        /** If there is no active configuration for this index, 
         * then we can consider it "synced" */
        return NSYNC_IF_SYNCED;
    }

    char if_cfg_file_fmt[FILENAME_MAX];
    char if_cfg_file[FILENAME_MAX];
    sprintf(if_cfg_file_fmt, "%s%s",CFG_FILE_LOC, CFG_FILE); 
    sprintf(if_cfg_file, if_cfg_file_fmt, CENTOS_IF_LIST_I(i));
    fp = fopen(if_cfg_file, "w");
    if (fp == NULL){
        sprintf(err_msg, "Could not open file for writing: %s", if_cfg_file);
        return NSYNC_ERROR;
    }

    /** By default we assume an ethernet interface */
    if (CENTOS_ACTIVE_CFG_LINK(i))
        fprintf(fp, "TYPE=Ethernet \n#%s\n", CENTOS_ACTIVE_CFG_LINK(i));

    /** All interfaces must have a device (and name for for ease of use) */
    fprintf(fp, "DEVICE=%s\nNAME=%s\n",CENTOS_IF_LIST_I(i), CENTOS_IF_LIST_I(i));

    /** 
     * If we have an active ipv4 then we should bring up interface on boot 
     * otherwise, don't bring up on boot and asyume dynamic because we have
     * no ip address to manually assign.
     */
    if (CENTOS_ACTIVE_CFG_INET(i)) {
        fprintf(fp, "ONBOOT=yes\n");

        /** Check if interface is dynamic */
        if(CENTOS_ACTIVE_CFG_DYNAMIC(i))
            fprintf(fp, "BOOTPROTO=dhcp\nIPADDR=%s\n", CENTOS_ACTIVE_CFG_INET(i));
        else 
            fprintf(fp, "BOOTPROTO=none\nIPADDR=%s\n", CENTOS_ACTIVE_CFG_INET(i));
    }
    else 
        fprintf(fp, "ONBOOT=no\nBOOTPROTO=dhcp\n");


    /** Check for these few fields and if they have a value then add to the config */
    if (CENTOS_ACTIVE_CFG_INET_MASK(i)) 
        fprintf(fp, "NETMASK=%s\n", CENTOS_ACTIVE_CFG_INET_MASK(i));

    if (CENTOS_ACTIVE_CFG_MTU(i)) 
        fprintf(fp, "MTU=%s\n", CENTOS_ACTIVE_CFG_MTU(i));

    /** If there is an ipv6 address then add ipv6 activation to the config */
    if (CENTOS_ACTIVE_CFG_INET6(i)) 
        fprintf(fp, "IPV6INIT=yes\nIPV6AUTOCONF=yes");
    else 
        fprintf(fp, "IPV6INIT=no\n");

    /** Added by default, toggled off by -a flag */
    if (info->arping_wait) fprintf(fp, "ARPING_WAIT=8\n");

    fclose(fp);

    /** Create route files if there are active routes */
    if (CENTOS_MAPPED_I_ROUTE_NUM(i) != 0){
        
        /** Create filepath */
        char route_file_fmt[FILENAME_MAX];
        char route_file[FILENAME_MAX];
        sprintf(route_file_fmt, "%s%s", CFG_FILE_LOC, ROUTE_FILE); 
        sprintf(route_file, route_file_fmt, CENTOS_IF_LIST_I(i));

        fp = fopen(route_file, "w");
        if (fp == NULL){
            sprintf(err_msg, "Could not open file for writing: %s", route_file);
            return NSYNC_ERROR;
        }
        /** Iterate through the routes */
        for (int j = 0; j < CENTOS_MAPPED_I_ROUTE_NUM(i); j++) {
            fprintf(fp,"%s", CENTOS_MAPPED_I_ROUTE_J(i,j));
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
    return NSYNC_IF_SYNCED;
}


/**
 * @brief Compares active and persistent configurations to determine if any changes
 * have been made since the persistent files were last edited. If changes are found
 * the value of the field in the persistent config struct is overwritten.
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. Will return NSYNC_KEEP_EXISTING if the
 * persistent files match the active configuration. But will return NSYNC_BACKUP
 * if they don't.
 */
nsync_state_t centos_compare_configs(net_sync_info_t *info)
{
    bool match = true;
    int to_sync = info->next_to_sync;

    /** Compare number of routes */
    if (!CENTOS_PERSIST_ROUTES(to_sync)){
        if (CENTOS_MAPPED_I_ROUTE_NUM(to_sync) > 0)
            return NSYNC_BACKUP;
        else goto compare_ifcfg;
    }
    

    int persist_rt_cnt = CENTOS_PERSIST_ROUTES_NUM_ROUTE(to_sync);
    int active_rt_cnt = CENTOS_MAPPED_I_ROUTE_NUM(to_sync);
    if (persist_rt_cnt != active_rt_cnt)
        match = false;

    /** Compare all the routes */
    for (int i = 0; i < active_rt_cnt; i++) {
        if (!CENTOS_PERSIST_ROUTES_ROUTE(to_sync, i)){ 
            CENTOS_PERSIST_ROUTES_ROUTE(to_sync, i) = calloc(MAX_OUTPUT_LEN, sizeof(char));
            CENTOS_PERSIST_ROUTES_NUM_ROUTE(to_sync)++;
        }
        if (strcmp(CENTOS_PERSIST_ROUTES_ROUTE(to_sync, i), CENTOS_ACTIVE_ROUTES_I(i)) != 0){
            match = false;
            memcpy(CENTOS_PERSIST_ROUTES_ROUTE(to_sync, i), CENTOS_ACTIVE_ROUTES_I(i), MAX_OUTPUT_LEN);
        
        }
    }

compare_ifcfg:

    /** Compare the existence of interfaces */
    if (!CENTOS_STORED_IFCFG(to_sync)){
        if (CENTOS_ACTIVE_CFG(to_sync))
            return NSYNC_BACKUP;
        else return NSYNC_KEEP_EXISTING;
    }

    /** Compare each of the fields of the configurations */
    ifcfg_fields_t *stored = CENTOS_STORED_IFCFG(to_sync);
    ip_show_fields_t *active = CENTOS_ACTIVE_CFG(to_sync);

   
    if(active->name != NULL){
        if (!stored->device){ 
            stored->device = calloc(MAX_OPT_LEN, sizeof(char));
            MEM_CHECK(stored->device, NSYNC_ERROR);
        }
        if(strcmp(active->name, stored->device) != 0){
            match = false;
            memcpy(stored->device, active->name, MAX_OPT_LEN);
        }
    }

    // MTU may be left out as 1500 is default
    if(active->mtu != NULL){
        if (!stored->mtu){ 
            stored->mtu = calloc(MAX_OPT_LEN, sizeof(char));
            MEM_CHECK(stored->mtu, NSYNC_ERROR);
        }
        if(strcmp(active->mtu, stored->mtu) != 0 && strcmp(active->mtu, "1500") != 0){
            match = false;
            memcpy(stored->mtu, active->mtu, MAX_OPT_LEN);
        }
    }

    if(active->inet != NULL && !active->dynamic){
        if (!stored->ipaddr){ 
            stored->ipaddr = calloc(20, sizeof(char));
            MEM_CHECK(stored->ipaddr, NSYNC_ERROR);
        }
        if(strcmp(active->inet, stored->ipaddr) != 0){
            match = false;
            memcpy(stored->ipaddr, active->inet, 20);
        }
    }

    if(active->inet_mask != NULL && !active->dynamic){
        if (!stored->netmask){ 
            stored->netmask = calloc(20, sizeof(char));
            MEM_CHECK(stored->netmask, NSYNC_ERROR);
        }
        if(strcmp(active->inet_mask, stored->netmask) != 0){
            match = false;
            memcpy(stored->netmask, active->inet_mask, 20);
        }
    }

    if (info->arping_wait && !stored->arping_wait){
        printf("match");
        match = false;
        stored->arping_wait = calloc(MAX_OPT_LEN, sizeof(char));
        MEM_CHECK(stored->arping_wait, NSYNC_ERROR);
        memcpy(stored->arping_wait, "8", 1);
    }

    if (match) {
        return NSYNC_KEEP_EXISTING;
    }
    return NSYNC_BACKUP;
}


/** 
 * @brief Keeps existing persistent configurations for the
 * current interface. For CentOS_X this is simply a continue
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @returns the next state of the utility. Will always return NSYNC_IF_SYNCED
 */
nsync_state_t centos_keep_existing(net_sync_info_t *info)
{
    (void) info;
    return NSYNC_IF_SYNCED;
}


/**
 * @brief Backs up existing route and ifcfg files for the interface
 * currently being synced. Does so by making system level calls.
 * 
 * @param info A struct containing all of the info related to the nsync utility
 * @return the next state of the utility, which will always be NSYNC_OVERWRITE.
 */
nsync_state_t centos_backup_files(net_sync_info_t *info)
{    
    /** Get the current datestamp */
    time_t now = time(NULL);
    char timestamp[MAX_CMD_LEN];
    strftime(timestamp, 20, "%Y%m%d", localtime(&now));


    /** Determine backup location */
    const char *location = info->backup.user_path;
    if (!info->backup.backup_set){
        location = info->backup.default_path;
    }

    char *interface = CENTOS_IF_LIST_I(info->next_to_sync);

    /** Format backup folder */
    char *dir_fmt = "nsync.%s";
    char dir[FILENAME_MAX];
    sprintf(dir, dir_fmt, timestamp);

    char *full_path_fmt = "%s%s";
    char full_path[FILENAME_MAX];
    sprintf(full_path, full_path_fmt, location, dir);

    /** Check for existing backups of the same name and iterate version #s */
    int ver = 1;
    while(!info->backup.started && dir_check(full_path)){
        char *dir_num_fmt = "nsync.%s.%02i";
        sprintf(dir, dir_num_fmt, timestamp, ver);
        sprintf(full_path, full_path_fmt, location, dir);
        ver++;
    }

    /** 
     * If the backup has just started (i.e. this is the first interface being synced)
     * then create the backup directory 
     * */
    if (!dir_check(full_path) && !info->backup.started){
        char mkdir_cmd[MAX_CMD_LEN];
        sprintf(mkdir_cmd, "mkdir %s", full_path);
        if (system(mkdir_cmd) == -1){
            sprintf(err_msg, "command `%s` failed", mkdir_cmd);
            return NSYNC_ERROR;
        }
        info->backup.started = true;
    }

    /** Build system level backup commands */
    char *backup_cmd_fmt = "cp %s %s";
    char backup_cmd[MAX_CMD_LEN];

    char backup_file_fmt[FILENAME_MAX];
    sprintf(backup_file_fmt, "%s%s", CFG_FILE_LOC, CFG_FILE);
    char backup_file[FILENAME_MAX];

    /** Backup ifcfg-<interface> file */
    sprintf(backup_file, backup_file_fmt, interface);
    if (file_exists(backup_file)){
        sprintf(backup_cmd, backup_cmd_fmt, backup_file, full_path);
        if (system(backup_cmd) == -1){
            sprintf(err_msg, "command `%s` failed", backup_cmd);
            return NSYNC_ERROR;
        }
    }

    /** Backup route-<interface> file */
    sprintf(backup_file_fmt, "%s%s", CFG_FILE_LOC, ROUTE_FILE);
    sprintf(backup_file, backup_file_fmt, interface);
    if (file_exists(backup_file)){
        sprintf(backup_cmd, backup_cmd_fmt, backup_file, full_path);
        if (system(backup_cmd) == -1){
            sprintf(err_msg, "command `%s` failed", backup_cmd);
            return NSYNC_ERROR;
        }
    }

    return NSYNC_OVERWRITE;
}


/**
 * @brief Marks the currently syncing interface as synced.
 * 
 * @param info pointer to the struct containing all info related to the nsync utility
 * @returns the next state of the utility -- NSYNC_GET_UNSYNCED
 */
nsync_state_t centos_mark_synced(net_sync_info_t *info)
{
    if(info->verbose){
        printf(" done\n\n");
    }
    /** Mark current interface as synced */
    info->synced[info->next_to_sync] = true;
    return NSYNC_GET_UNSYNCED;
}


/**
 * @brief Overwrites any existing configuration by writing out new ifcfg and
 * route files for the curent interface based ont he active configuration
 * 
 * @param info pointer to the struct containing all info related to the nsync utility
 * @returns the next state of the utility: NSYNC_IF_SYNCED. If there is an error
 * found during the execution of this function then it returns NSYNC_ERROR.
 */
nsync_state_t centos_overwrite_configs(net_sync_info_t *info)
{
    int i = info->next_to_sync;

    ifcfg_fields_t *stored_cfg = CENTOS_STORED_IFCFG(i);
    routes_parsed_t *active_routes = CENTOS_MAPPED_I(i);

    if (active_routes->num_route > 0) {
        if (stored_cfg->onboot != NULL)
            memcpy(stored_cfg->onboot, "yes", 3);
        else {
            stored_cfg->onboot = calloc(MAX_OPT_LEN, sizeof(char));
            MEM_CHECK(stored_cfg->onboot, NSYNC_ERROR);
            memcpy(stored_cfg->onboot, "yes", 3);
        }
    }

    char if_cfg_file_fmt[FILENAME_MAX];
    char if_cfg_file[FILENAME_MAX];
    
    sprintf(if_cfg_file_fmt, "%s%s.tmp",CFG_FILE_LOC, CFG_FILE); 
    sprintf(if_cfg_file, if_cfg_file_fmt, CENTOS_IF_LIST_I(i));
    FILE *fp = fopen(if_cfg_file, "w");
    if (fp == NULL){
        sprintf(err_msg, "could not open file '%s' for writing", if_cfg_file);
        return NSYNC_ERROR;
    }


    char cfg_filepath_fmt[FILENAME_MAX];
    char cfg_filepath[FILENAME_MAX];
    sprintf(cfg_filepath_fmt, "%s%s",CFG_FILE_LOC, CFG_FILE);
    sprintf(cfg_filepath, cfg_filepath_fmt, CENTOS_IF_LIST_I(i));

    /** Open the file for reading */
    FILE *fp_old = fopen(cfg_filepath, "r");
    if (!fp_old) {
        sprintf(err_msg, "file %s could not be read", cfg_filepath);
        return NSYNC_ERROR;
    }

    char line[CFG_LINE_LEN];
    
    bool added_arp_wait = false;

    /** Iterate through the lines of the file and parse out the field of the configuration */
    while (fgets(line, CFG_LINE_LEN, fp_old)) {
        char opt_str[MAX_OPT_LEN];
        char opt_val[MAX_VAL_LEN];

        /** Check for comments */
        if (line[0] == '#'){
            sprintf(opt_str, "#");
            safe_strncpy(opt_val, &line[1], MAX_VAL_LEN);
        } else {
            get_field_delim(opt_str, line, 1, MAX_OPT_LEN, "=");
            get_field_delim(opt_val, line, 2, MAX_VAL_LEN, "=");
        }
        /** Remove quotes around opt_val */
        trim(opt_val,"\"\n");

        /** If we find the appropriate field then check that it has a value and print it */
        ifcfg_opt_t opt = opt_str_to_enum(opt_str);
        switch(opt){
            case COMMENT:
                fprintf(fp, "#%s\n", opt_val);    
                break;

            case TYPE:
                if (stored_cfg->type)               
                    fprintf(fp, "TYPE=%s\n",stored_cfg->type);
                break;

            case DEVICE:
                if (stored_cfg->device)              
                    fprintf(fp, "DEVICE=%s\n",stored_cfg->device);
                break;

            case ONBOOT:
                if (stored_cfg->onboot)              
                    fprintf(fp, "ONBOOT=%s\n",stored_cfg->onboot);
                break;

            case BOOTPROTO:
                if (stored_cfg->bootproto)           
                    fprintf(fp, "BOOTPROTO=%s\n",stored_cfg->bootproto);
                break;

            case IPADDR:
                if (stored_cfg->ipaddr)              
                    fprintf(fp, "IPADDR=%s\n",stored_cfg->ipaddr);
                break;

            case GATEWAY:
                if (stored_cfg->gateway)             
                    fprintf(fp, "GATEWAY=%s\n",stored_cfg->gateway);
                break;

            case NETMASK:
                if (stored_cfg->netmask)             
                    fprintf(fp, "NETMASK=%s\n",stored_cfg->netmask);
                break;

            case DNS1:
                if (stored_cfg->dns[0])              
                    fprintf(fp, "DNS1=%s\n",stored_cfg->dns[0]);
                break;

            case DNS2:
                if (stored_cfg->dns[1])              
                    fprintf(fp, "DNS2=%s\n",stored_cfg->dns[1]);
                break;

            case IPV4_FAILURE_FATAL:
                if (stored_cfg->ipv4_failure_fatal)  
                    fprintf(fp, "IPV4_FAILURE_FATAL=%s\n",stored_cfg->ipv4_failure_fatal);
                break;

            case IPV6ADDR:
                if (stored_cfg->ipv6addr)            
                    fprintf(fp, "IPV6ADDR=%s\n",stored_cfg->ipv6addr);
                break;

            case IPV6INIT:
                if (stored_cfg->ipv6init)            
                    fprintf(fp, "IPV6INIT=%s\n",stored_cfg->ipv6init);
                break;

            case NM_CONTROLLED:
                if (stored_cfg->nm_controlled)       
                    fprintf(fp, "NM_CONTROLLED=%s\n",stored_cfg->nm_controlled);
                break;

            case USERCTL:
                if (stored_cfg->userctl)             
                    fprintf(fp, "USERCTL=%s\n",stored_cfg->userctl);
                break;

            case DEFROUTE:
                if (stored_cfg->defroute)            
                    fprintf(fp, "DEFROUTE=%s\n",stored_cfg->defroute);
                break;

            case VLAN:
                if (stored_cfg->vlan)                
                    fprintf(fp, "VLAN=%s\n",stored_cfg->vlan);
                break;

            case MTU:
                if (stored_cfg->mtu && strlen(stored_cfg->mtu) && strcmp(stored_cfg->mtu, "1500"))                 
                    fprintf(fp, "MTU=%s\n",stored_cfg->mtu);
                break;
            case HWADDR:
                if (stored_cfg->hwaddr)              
                fprintf(fp, "HWADDR=%s\n",stored_cfg->hwaddr);
                break;

            case UUID:
                if (stored_cfg->uuid)                
                fprintf(fp, "UUID=%s\n",stored_cfg->uuid);
                break;

            case NETWORK:
                if (stored_cfg->network)             
                fprintf(fp, "NETWORK=%s\n",stored_cfg->network);
                break;

            case BROADCAST:
                if (stored_cfg->broadcast)           
                fprintf(fp, "BROADCAST=%s\n",stored_cfg->broadcast);
                break;

            case NAME:
                if (stored_cfg->name)                
                fprintf(fp, "NAME=%s\n",stored_cfg->name);
                break;

            case IPV6_AUTOCONF:
                if (stored_cfg->ipv6_autoconf)       
                fprintf(fp, "IPV6_AUTOCONF=%s\n",stored_cfg->ipv6_autoconf);
                break;

            case PROXY_METHOD:
                if (stored_cfg->proxy_method)        
                fprintf(fp, "PROXY_METHOD=%s\n",stored_cfg->proxy_method);
                break;

            case BROWSER_ONLY:
                if (stored_cfg->browser_only)        
                fprintf(fp, "BROWSER_ONLY=%s\n",stored_cfg->browser_only);
                break;

            case ARPING_WAIT:
                if (stored_cfg->arping_wait)         
                fprintf(fp, "ARPING_WAIT=%s\n", stored_cfg->arping_wait);
                added_arp_wait = true;
                break;

            case UNKNOWN_OPT:
                fprintf(fp, "%s", line);
                break;

            default:
                sprintf(err_msg, "invalid opt: %s", opt_str);
                return NSYNC_ERROR;
        }
    }
    if (info->arping_wait && !added_arp_wait && stored_cfg->arping_wait){
        fprintf(fp, "ARPING_WAIT=%s\n", stored_cfg->arping_wait);
    }
    
    fclose(fp_old);
    fclose(fp);

    /** Overwrite existing with the tmp file */
    char cpy_cmd[MAX_CMD_LEN];
    sprintf(cpy_cmd, "mv %s %s",if_cfg_file, cfg_filepath);
    if (system(cpy_cmd) == -1){
            sprintf(err_msg, "command `%s` failed", cpy_cmd);
            return NSYNC_ERROR;
        }

    /** Write routes */
    char route_file_fmt[FILENAME_MAX];
    char route_file[FILENAME_MAX];
    sprintf(route_file_fmt, "%s%s",CFG_FILE_LOC,info->route_file); 
    sprintf(route_file, route_file_fmt, CENTOS_IF_LIST_I(i));
    fp = fopen(route_file, "w");
    if (fp == NULL){
        sprintf(err_msg, "could not open file '%s' for writing", route_file);
        return NSYNC_ERROR;
    }

    if (active_routes->num_route == 0) {
        fclose(fp);
        return NSYNC_IF_SYNCED;
    }

    /** 
     * For each of the active routes, if there is a matching persistent
     * route add any whitespace and comments associated with it before
     * writing the route. Otherwise just write the route.
     */
    for (int j = 0; j < active_routes->num_route; j++) {
        
        if (!CENTOS_PERSIST_ROUTES(info->next_to_sync)){
            fprintf(fp, "%s\n", CENTOS_ACTIVE_ROUTES_I(j));
            continue;
        }
        
        char *gaps = CENTOS_PERSIST_ROUTES_GAP(info->next_to_sync, j);
        if (gaps) 
            fprintf(fp, "%s", gaps);

        char *comment = CENTOS_PERSIST_ROUTES_COMMENT(info->next_to_sync, j);
        if (comment) 
            fprintf(fp, "%s", comment);

        fprintf(fp, "%s\n", CENTOS_ACTIVE_ROUTES_I(j));
    }
        
    fclose(fp);

    return NSYNC_IF_SYNCED;
}

/**
 * @brief Frees all heap allocated data in the provided net_sync_info struct.
 * 
 * @param info pointer to the struct containing all information relating to the nsync utility
 * @returns the final state of the utility -- NSYNC_SUCCESS
 */
nsync_state_t centos_cleanup_and_free(net_sync_info_t *info)
{
    if (info->verbose) {
        printf("##################################################################\n\n");
        printf("Syncing complete!\n\n");
    }

    /** Free sys info */
    free(info->sys.os_str);

    /** Free network config stuff */
    for (int i = 0; i < MAX_NUM_IF; i++){
        free(CENTOS_IF_LIST_I(i));
        free_ifcfg_fields(CENTOS_STORED_IFCFG(i));
        free(CENTOS_STORED_IFCFG(i));
        free_ip_show_fields(CENTOS_ACTIVE_CFG(i));
        free(CENTOS_ACTIVE_CFG(i));
    }
    free(CENTOS_IF_LIST);

    for (int i = 0; i < MAX_ROUTES; i++){
        free(CENTOS_ACTIVE_ROUTES_I(i));
        CENTOS_ACTIVE_ROUTES_I(i) = NULL;
    }
    free(CENTOS_ACTIVE_ROUTES);

    for (int i = 0; i < CENTOS_NUM_IF; i++) {
        
        free(CENTOS_MAPPED_I_ROUTES(i));
        free(CENTOS_MAPPED_I(i));
        free_route_config(CENTOS_PERSIST_ROUTES(i));
        free(CENTOS_PERSIST_ROUTES(i));
    }
    free(CENTOS_MAPPED);
    free(CENTOS_NET_CFG);
    
    return NSYNC_SUCCESS;
}
