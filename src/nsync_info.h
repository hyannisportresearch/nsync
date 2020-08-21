/**
 * @file nsync_info.h
 * Stores the primary structs, data, and enums used throughout the utilty
 * @author Ben London
 * @date 7/30/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_utils.h"

#ifndef NSYNC_INFO_H
#define NSYNC_INFO_H

#define MAX_CMD_LEN FILENAME_MAX*2

/** MACRO CONSTANTS */
#define MAX_NUM_IF 255
#define MAX_ROUTES 100
#define MAX_IF_NAME 100

#define MAX_NUM_IP 100
#define MAX_NUM_GW 100
#define MAX_NUM_NM 100

extern char err_msg[ERR_LEN];

typedef struct net_sync_info net_sync_info_t;
/**********************************************************************/
/*                          NSYNC STATES                              */
/**********************************************************************/
typedef enum
{
    NSYNC_START = 0,
    NSYNC_CHECK_OS,
    NSYNC_GET_CONFIG,
    NSYNC_GET_UNSYNCED,
    NSYNC_CHECK_EXIST,
    NSYNC_COMPARE_CONFIG,
    NSYNC_BACKUP,
    NSYNC_OVERWRITE,
    NSYNC_CREATE_WRITE,
    NSYNC_KEEP_EXISTING,
    NSYNC_IF_SYNCED,
    NSYNC_ERROR,
    NSYNC_DONE,
    NSYNC_SUCCESS,
} nsync_state_t;



/** State Functions */
typedef struct state_func {
    nsync_state_t (*check_OS)(net_sync_info_t *);
    nsync_state_t (*get_config)(net_sync_info_t *);
    nsync_state_t (*get_unsynced)(net_sync_info_t *);
    nsync_state_t (*check_exist)(net_sync_info_t *);
    nsync_state_t (*compare_config)(net_sync_info_t *);
    nsync_state_t (*backup)(net_sync_info_t *);
    nsync_state_t (*overwrite)(net_sync_info_t *);
    nsync_state_t (*create_write)(net_sync_info_t *);
    nsync_state_t (*keep_existing)(net_sync_info_t *);
    nsync_state_t (*if_synced)(net_sync_info_t *);
    nsync_state_t (*done)(net_sync_info_t *);
}state_func_t;

/**********************************************************************/
/*                               OS                                   */
/**********************************************************************/

#define MAX_OS_LEN 100
typedef enum {
    CENTOS_6 = 0,
    CENTOS_7,
    CENTOS_8,
    UBUNTU_1604,
    NUM_OS,
    INVALID_OS,
} os_enum_t;

/**********************************************************************/
/*                             COMMANDS                               */
/**********************************************************************/
typedef struct command_list {
        char get_OS[MAX_CMD_LEN];
        char get_if_list[MAX_CMD_LEN];
        char get_active_if_cfg[MAX_CMD_LEN];
        char get_routes[MAX_CMD_LEN];
} cmd_list_t;

/**********************************************************************/
/*                            MAIN STRUCT                             */
/**********************************************************************/
struct net_sync_info {
    nsync_state_t CURR_STATE;

    struct sys_info {
        char *os_str;
        nsync_state_t os;
    } sys;

    cmd_list_t *cmd_list;

    struct backup_data {
        bool backup_set;
        const char *default_path;
        char *user_path;
        bool started;
        bool complete;
    } backup;

    bool verbose;

    void *net_config;

    bool synced[MAX_NUM_IF];
    int next_to_sync;

    void *parsers;
    state_func_t *state_func;

    const char *cfg_file_loc;
    const char *cfg_file;
    const char *route_file;

    bool arping_wait;
};

#define CFG_FILE info->cfg_file
#define CFG_FILE_LOC info->cfg_file_loc
#define ROUTE_FILE info->route_file

#endif