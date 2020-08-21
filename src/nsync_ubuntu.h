/**
 * @file nsync_ubuntu.h
 * @author Ben London
 * @date 8/11/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_ubuntu_parse.h"

#ifndef NSYNC_UBUNTU_H
#define NSYNC_UBUNTU_H


/**********************************************************************/
/*                            NET CONFIGS                             */
/**********************************************************************/
typedef struct ubuntu_network_config {
    route_list_t *persist_routes;
    route_list_t *active_routes;

    if_data_t *persist_ifs;
    if_data_t *active_ifs;

}ubuntu_net_cfg_t;


extern cmd_list_t ubuntu_cmd_list;

/**********************************************************************/
/*                           CMD MACROS                               */
/**********************************************************************/
#define UBUNTU_GET_OS               info->cmd_list->get_os
#define UBUNTU_GET_IF_LIST          info->cmd_list->get_if_list
#define UBUNTU_GET_ACTIVE_IF_CFG    info->cmd_list->get_active_if_cfg
#define UBUNTU_GET_ROUTES           info->cmd_list->get_routes

/**********************************************************************/
/*                         ACCESS MACROS                              */
/**********************************************************************/
#define UBUNTU_NET_CONFIG              ((ubuntu_net_cfg_t *)info->net_config)

#define UBUNTU_PERSIST_ROUTES          UBUNTU_NET_CONFIG->persist_routes
#define UBUNTU_PERSIST_ROUTE_NUM       UBUNTU_NET_CONFIG->persist_routes->num_routes
#define UBUNTU_PERSIST_ROUTE(i)        UBUNTU_NET_CONFIG->persist_routes->routes[i]

#define UBUNTU_ACTIVE_ROUTES           UBUNTU_NET_CONFIG->active_routes
#define UBUNTU_ACTIVE_ROUTES_NUM       UBUNTU_NET_CONFIG->active_routes->num_routes
#define UBUNTU_ACTIVE_ROUTE(i)         UBUNTU_NET_CONFIG->active_routes->routes[i]

#define UBUNTU_PERSIST_IFS             UBUNTU_NET_CONFIG->persist_ifs
#define UBUNTU_PERSIST_IF_NAME_LIST    UBUNTU_NET_CONFIG->persist_ifs->if_name_list
#define UBUNTU_PERSIST_IF_LIST_NAME(i) UBUNTU_NET_CONFIG->persist_ifs->if_name_list[i]
#define UBUNTU_PERSIST_IF_NUM          UBUNTU_NET_CONFIG->persist_ifs->num_if
#define UBUNTU_PERSIST_INTERFACES      UBUNTU_NET_CONFIG->persist_ifs->interfaces
#define UBUNTU_PERSIST_INTERFACE(i)    UBUNTU_NET_CONFIG->persist_ifs->interfaces[i]
#define UBUNTU_PERSIST_IF_NAME(i)      IF_I_NAME(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_LINKTYPE(i)  IF_I_LINKTYPE(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_ADDRESS(i)   IF_I_ADDRESS(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_NETMASK(i)   IF_I_NETMASK(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_BROADCAST(i) IF_I_BROADCAST(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_METRIC(i)    IF_I_METRIC(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_HWADDRESS(i) IF_I_HWADDRESS(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_GATEWAY(i)   IF_I_GATEWAY(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_MTU(i)       IF_I_MTU(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_SCOPE(i)     IF_I_SCOPE(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_UNMANAGED(i) IF_I_UNMANAGED(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_AUTO_OPT(i)  IF_I_AUTO_OPT(UBUNTU_PERSIST_INTERFACES, i)

#define UBUNTU_PERSIST_IF_ROUTES(i)    IF_I_ROUTES(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_ROUTE_NUM(i) IF_I_ROUTE_NUM(UBUNTU_PERSIST_INTERFACES, i)
#define UBUNTU_PERSIST_IF_ROUTE(i,j)   IF_I_ROUTE_J(UBUNTU_PERSIST_INTERFACES, i, j)

#define UBUNTU_ACTIVE_IFS              UBUNTU_NET_CONFIG->active_ifs
#define UBUNTU_ACTIVE_IF_NAME_LIST     UBUNTU_NET_CONFIG->active_ifs->if_name_list
#define UBUNTU_ACTIVE_IF_LIST_NAME(i)  UBUNTU_NET_CONFIG->active_ifs->if_name_list[i]
#define UBUNTU_ACTIVE_IF_NUM           UBUNTU_NET_CONFIG->active_ifs->num_if
#define UBUNTU_ACTIVE_INTERFACES       UBUNTU_NET_CONFIG->active_ifs->interfaces
#define UBUNTU_ACTIVE_INTERFACE(i)     UBUNTU_NET_CONFIG->active_ifs->interfaces[i]
#define UBUNTU_ACTIVE_IF_NAME(i)       IF_I_NAME(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_LINKTYPE(i)   IF_I_LINKTYPE(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_ADDRESS(i)    IF_I_ADDRESS(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_NETMASK(i)    IF_I_NETMASK(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_BROADCAST(i)  IF_I_BROADCAST(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_METRIC(i)     IF_I_METRIC(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_HWADDRESS(i)  IF_I_HWADDRESS(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_GATEWAY(i)    IF_I_GATEWAY(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_MTU(i)        IF_I_MTU(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_SCOPE(i)      IF_I_SCOPE(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_UNMANAGED(i)  IF_I_UNMANAGED(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_AUTO_OPT(i)   IF_I_AUTO_OPT(UBUNTU_ACTIVE_INTERFACES, i)

#define UBUNTU_ACTIVE_IF_ROUTES(i)     IF_I_ROUTES(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_ROUTE_NUM(i)  IF_I_ROUTE_NUM(UBUNTU_ACTIVE_INTERFACES, i)
#define UBUNTU_ACTIVE_IF_ROUTE(i,j)    IF_I_ROUTE_J(UBUNTU_ACTIVE_INTERFACES, i, j)

/**********************************************************************/
/*                              Ubuntu                                */
/**********************************************************************/
extern state_func_t ubuntu_state_funcs;

nsync_state_t ubuntu_get_config(net_sync_info_t *info);

nsync_state_t ubuntu_get_unsynced(net_sync_info_t *info);

nsync_state_t ubuntu_check_persistent_files(net_sync_info_t *info);

nsync_state_t ubuntu_create_and_write_to_file(net_sync_info_t *info);

nsync_state_t ubuntu_compare_configs(net_sync_info_t *info);

nsync_state_t ubuntu_keep_existing(net_sync_info_t *info);

nsync_state_t ubuntu_backup_files(net_sync_info_t *info);

nsync_state_t ubuntu_mark_synced(net_sync_info_t *info);

nsync_state_t ubuntu_overwrite_configs(net_sync_info_t *info);

nsync_state_t ubuntu_cleanup_and_free(net_sync_info_t *info);

#endif