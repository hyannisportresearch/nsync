/**
 * @file nsync_driver.c
 * Main driver for the nsync (Network Configuration Sync) utility
 * 
 * @author Ben London
 * @date 7/27/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_driver.h"
#include <stdio.h>

int main(int argc, char *argv[])
{

    /** Initial setup */
    net_sync_info_t *nsync_info = calloc(1, sizeof(net_sync_info_t));
    if(!nsync_info){
        fprintf(stderr, "\n%sError: memory could not be allocated%s\n", KRED, KNRM);
        return -1;
    }

    nsync_info->CURR_STATE = NSYNC_START;
    nsync_info->sys.os = INVALID_OS;
    nsync_info->arping_wait = true;

    sprintf(err_msg, "unknown error");

    // Parse CMD Line args
    for (int i = 1; i < argc; i++) {
    
        if (strcmp(argv[i],"-v")==0) {
            nsync_info->verbose = true;
        }
        else if (strcmp(argv[i],"-a") == 0){
            nsync_info->arping_wait = false;
        }
        else if (strcmp(argv[i],"-b")==0) {
            if (i+1 <= argc && argv[i+1] && argv[i+1][0] != '-') {
                if (dir_check(argv[++i])) {
                    if (!(argv[i][strlen(argv[i])-1] == '/')) {
                        char edited_backup[FILENAME_MAX];
                        sprintf(edited_backup,"%s/", argv[i]);
                        nsync_info->backup.user_path = edited_backup;
                    } else nsync_info->backup.user_path = argv[i];
                    nsync_info->backup.backup_set = true;
                }
                else {
                    fprintf(stderr, "nsync: %s does not exist or is not a directory\n", argv[i]);
                    return 1;    
                }
            }
            else {
                fprintf(stderr, "nsync: -b flag must be followed by the </path/to/backup/>\n");
                return 1;
            }
        }
        else if (strcmp(argv[i],"-h") == 0){
            printf("\nUsage: %s [-h] [-v] [-b </path/to/backup/>] \n\n"
                        "\t-h -- prints this usage\n"
	                    "\t-v -- runs nsync in verbose mode\n"
                        "\t-a -- toggles the arping wait value off in config files (not useful for all OS)\n"
	                    "\t-b -- sets backup location to the <path/to/backup> that follows\n\n", argv[0]);
            return 0;
        }
        else {
            fprintf(stderr, "nsync: unknown argument: %s\n", argv[i]);
            return -1;
        }
    }
    
    int ret_val = driver(nsync_info);
    if (ret_val == 0) free(nsync_info);
    return ret_val;
}


/**
 * @brief main driver function for the utility -- implemented as a state machine
 * 
 * @param nsync_info pointer to the struct containing all of the utility's data and information
 * about the current execution
 * 
 * @returns integer code representing succes/failure of the utility: 1 = error, 0 = success
 */
int driver(net_sync_info_t *nsync_info)
{
    while(nsync_info->CURR_STATE != NSYNC_SUCCESS){

        switch (nsync_info->CURR_STATE)
        {
            case NSYNC_START:
                nsync_info->CURR_STATE = NSYNC_CHECK_OS;
                break;

            case NSYNC_CHECK_OS:
                nsync_info->CURR_STATE = check_OS(nsync_info);
                break;

            case NSYNC_GET_CONFIG:
                nsync_info->CURR_STATE = nsync_info->state_func->get_config(nsync_info);
                break;

            case NSYNC_GET_UNSYNCED:
                nsync_info->CURR_STATE = nsync_info->state_func->get_unsynced(nsync_info);
                break;

            case NSYNC_CHECK_EXIST:
                nsync_info->CURR_STATE = nsync_info->state_func->check_exist(nsync_info);
                break;
            
            case NSYNC_COMPARE_CONFIG:
                nsync_info->CURR_STATE = nsync_info->state_func->compare_config(nsync_info);
                break;
            
            case NSYNC_CREATE_WRITE:
                nsync_info->CURR_STATE = nsync_info->state_func->create_write(nsync_info);
                break;

            case NSYNC_KEEP_EXISTING:
                nsync_info->CURR_STATE = nsync_info->state_func->keep_existing(nsync_info);
                break;

            case NSYNC_BACKUP:
                nsync_info->CURR_STATE = nsync_info->state_func->backup(nsync_info);
                break;
            case NSYNC_OVERWRITE:
                nsync_info->CURR_STATE = nsync_info->state_func->overwrite(nsync_info);
                break;

            case NSYNC_IF_SYNCED:
                nsync_info->CURR_STATE = nsync_info->state_func->if_synced(nsync_info);
                break;

            case NSYNC_DONE:
                nsync_info->CURR_STATE = nsync_info->state_func->done(nsync_info);
                return 0;

            case NSYNC_ERROR:
                fprintf(stderr, "\n%sError: %s%s\n", KRED, err_msg, KNRM);
                goto error;
                break;

            default:
                fprintf(stderr, "Unknown State: %i\n", nsync_info->CURR_STATE);
                goto error;
        }
    }
    return (0);

    error:
        return (-1);
}


/**
 * @brief Determines if the OS is supported by the utility by iterating through a list 
 * of supported os's, and if so, set the corresponding values in the info struct
 * @param info A struct containing all of the info related to the network configuration
 * @param os a string containing the operating system
 * @returns whether or not the os is supported
 */
bool supported_os(net_sync_info_t *info, char *os)
{
    int i;
    for(i = 0; i < NUM_OS; i++){
        if (strncmp(os, os_name[i], MAX_OS_LEN) == 0){
            info->sys.os = i;
            info->sys.os_str = os;
            return true;
        }
    }
    return false;
}

/**
 * @brief Determines the OS of the system and checks that it is supported
 * @param info A struct containing all of the info related to the network configuration
 * @returns the next state of the utility
 */
nsync_state_t check_OS(net_sync_info_t *info)
{
    if(info->verbose) system("clear;");
    
    char *os = calloc(MAX_OS_LEN, sizeof(char));
    MEM_CHECK(os, NSYNC_ERROR);

    /** Check for Centos */
    if (file_exists("/etc/centos-release")){
        char cmd[MAX_CMD_LEN];
        sprintf(cmd, "cat %s", "/etc/centos-release");
        FILE *fp = popen(cmd, "r");
        if (fp == NULL){
            sprintf(err_msg, "Could not run command: %s", cmd);
            return NSYNC_ERROR;
        }
        
        char os_release[MAX_OS_LEN];
        if (fgets(os_release, MAX_OS_LEN, fp) != NULL){
            char *dot_delim = ".";
            get_field_delim(os_release, os_release, 1, MAX_OS_LEN, dot_delim);

            char *space_delim = " ";
            get_field_delim(os, os_release, 1, MAX_OS_LEN-2, space_delim);
            os[strlen(os)] = '_';
            os[strlen(os)] = os_release[strlen(os_release)-1];
        }
        pclose(fp);

    }
    /** Check for Ubuntu */
    else if (file_exists("/etc/lsb-release")){  
        char cmd[MAX_CMD_LEN];
        sprintf(cmd, "cat %s", "/etc/lsb-release");
        FILE *fp = popen(cmd, "r");
        if (fp == NULL){
            sprintf(err_msg, "Could not run command: %s", cmd);
            return NSYNC_ERROR;
        }
        char distr_id[MAX_OS_LEN];
        if (fgets(distr_id, MAX_OS_LEN, fp) != NULL){
            get_field_delim(distr_id, distr_id, 2, MAX_OS_LEN, "=");
            char release_ver[MAX_OS_LEN];
            if (fgets(release_ver, MAX_OS_LEN, fp) != NULL){
                get_field_delim(release_ver, release_ver, 2, MAX_OS_LEN, "=");

                memcpy(os, distr_id, strlen(distr_id));
                os[strlen(os)-1] = '_';
                memcpy(&os[strlen(distr_id)], release_ver, strlen(release_ver)-1);
            }
        }
        else {
            sprintf(err_msg, "Could not parse lsb-release file");
            return NSYNC_ERROR;
        }
        pclose(fp);
    }
    else {
        sprintf(err_msg, "unsupported OS");
        return NSYNC_ERROR;
    }


    /** Check that the distro/version is supported (not just parse-able) */
    if (!supported_os(info, os)){
        sprintf(err_msg, "operating system %s is not supported", os);
        return NSYNC_ERROR;
    }

    /** If the OS is valid then set the commands, files, etc. for that OS */
    info->cmd_list = os_cmd[info->sys.os];
    info->parsers = os_parse[info->sys.os];
    info->cfg_file_loc = cfg_locations[info->sys.os];
    info->cfg_file = ifcfg_if_file_fmt[info->sys.os];
    info->route_file = route_if_file_fmt[info->sys.os];
    info->state_func = os_state_funcs[info->sys.os];

    /** Ensure that there is indeed access to the directory. */
    int ret = access(info->cfg_file_loc, W_OK);
    if (ret == -1){
        char *err_msg_fmt = "access error with directory: %s -- %s\n";
        sprintf(err_msg, err_msg_fmt, info->cfg_file_loc, strerror(errno));
        return NSYNC_ERROR;
    }
    
    /** If the backup location was not manually set, then use the default location */
    if(!info->backup.backup_set) info->backup.default_path = cfg_locations[info->sys.os];
    else {
        int backup_dir_access = access(info->backup.user_path, W_OK);
        if (backup_dir_access == -1){
            char *err_msg_fmt = "access error with backup directory: %s -- %s\n";
            printf( err_msg_fmt, info->backup.user_path, strerror(errno));
            printf("%sUsing default backup path: %s%s", KYEL, cfg_locations[info->sys.os], KNRM);
            info->backup.backup_set = false;
            info->backup.default_path = cfg_locations[info->sys.os];
        }
    }
    
    return NSYNC_GET_CONFIG;
}
