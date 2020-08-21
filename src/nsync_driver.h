/**
 * @file nsync_driver.h
 * @author Ben London
 * @date 7/27/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_info.h"
#include "nsync_centos.h"
#include "nsync_ubuntu.h"
#include "nsync_utils.h"

extern char err_msg[ERR_LEN];



/**********************************************************************/
/*                               OS                                   */
/**********************************************************************/
const char os_name[NUM_OS + 2][MAX_OS_LEN] =
{
    [CENTOS_6]      =   "CentOS_6",
    [CENTOS_7]      =   "CentOS_7",
    [CENTOS_8]      =   "CentOS_8",
    [UBUNTU_1604]   =   "Ubuntu_16.04",
    [NUM_OS]        =   "Invalid",
    [INVALID_OS]    =   "Invalid",
};



/**********************************************************************/
/*                           PARSING                                  */
/**********************************************************************/

void *os_parse[NUM_OS] = 
{
    [CENTOS_6]      =   &centos_parsers,

    [CENTOS_7]      =   &centos_parsers,

    [CENTOS_8]      =   &centos_parsers,

    [UBUNTU_1604]   =   &ubuntu_parsers,
};

/**********************************************************************/
/*                            STATES                                  */
/**********************************************************************/

state_func_t *os_state_funcs[NUM_OS] = 
{
    [CENTOS_6]      =   &centos_state_funcs,
    [CENTOS_7]      =   &centos_state_funcs,
    [CENTOS_8]      =   &centos_state_funcs,
    [UBUNTU_1604]   =   &ubuntu_state_funcs,
};


/**********************************************************************/
/*                             COMMANDS                               */
/**********************************************************************/
cmd_list_t *os_cmd[NUM_OS] = {
    [CENTOS_6]      =   &centos_cmd_list,
    [CENTOS_7]      =   &centos_cmd_list,
    [CENTOS_8]      =   &centos_cmd_list,
    [UBUNTU_1604]   =   &ubuntu_cmd_list,
};


/**********************************************************************/
/*                              FILES                                 */
/**********************************************************************/

/** Mappings of OS's to the location of their configuration files */
const char *cfg_locations[NUM_OS] = 
{
    [CENTOS_6]      =   "/etc/sysconfig/network-scripts/",
    [CENTOS_7]      =   "/etc/sysconfig/network-scripts/",
    [CENTOS_8]      =   "/etc/sysconfig/network-scripts/",
    [UBUNTU_1604]   =   "/etc/network/"
};

/** Mappings of OS's to their format for peristent route files */
const char *route_if_file_fmt[NUM_OS] =
{
    [CENTOS_6]      =   "route-%s",
    [CENTOS_7]      =   "route-%s",
    [CENTOS_8]      =   "route-%s",    
    [UBUNTU_1604]   =   "interfaces",
};

/** Mappings of OS's to the format of their persistent config file names */
const char *ifcfg_if_file_fmt[NUM_OS] =
{
    [CENTOS_6]      =   "ifcfg-%s",
    [CENTOS_7]      =   "ifcfg-%s",
    [CENTOS_8]      =   "ifcfg-%s",    
    [UBUNTU_1604]   =   "interfaces",
};


/**
 * @brief main driver function for the utility -- implemented as a state machine
 * 
 * @param nsync_info pointer to the struct containing all of the utility's data and information
 * about the current execution
 * 
 * @returns integer code representing succes/failure of the utility: 1 = error, 0 = success
 */
int driver(net_sync_info_t *nsync_info);

/**
 * @brief Determines the OS of the system and checks that it is supported
 * @param info A struct containing all of the info related to the network configuration
 * @returns the next state of the utility
 */
nsync_state_t check_OS(net_sync_info_t *info);

/**
 * @brief Determines if the OS is supported by the utility by iterating through a list 
 * of supported os's, and if so, set the corresponding values in the info struct
 * @param info A struct containing all of the info related to the network configuration
 * @param os a string containing the operating system
 * @returns whether or not the os is supported
 */
bool supported_os(net_sync_info_t *info, char *os);

