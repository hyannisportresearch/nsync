/**
 * @file nsync_centos.h 
 * @author Ben London
 * @date 7/27/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_centos_parse.h"

#ifndef NSYNC_CENTOS_H
#define NSYNC_CENTOS_H



extern state_func_t centos_state_funcs;

extern cmd_list_t centos_cmd_list;
/**********************************************************************/
/*                           CMD MACROS                               */
/**********************************************************************/
#define CENTOS_GET_OS               info->cmd_list->get_OS
#define CENTOS_GET_IF_LIST          info->cmd_list->get_if_list
#define CENTOS_GET_ACTIVE_CFG       info->cmd_list->get_active_if_cfg
#define CENTOS_GET_ACTIVE_ROUTES    info->cmd_list->get_routes

/**********************************************************************/
/*                            NET CONFIGS                             */
/**********************************************************************/
typedef struct centos_network_config {
    char **if_list;
    int num_if;
    
    centos_route_t *routes;
    int num_routes;

    map_routes_if_t mapped_routes_by_if;

    ifcfg_fields_t *stored_configs[MAX_NUM_IF];

    ip_show_fields_t *active_configs[MAX_NUM_IF];

    rt_cfg_t *persist_rts[MAX_NUM_IF];

}centos_net_cfg_t;


/**********************************************************************/
/*                         ACCESS MACROS                              */
/**********************************************************************/
#define CENTOS_NET_CFG                              ((centos_net_cfg_t *)info->net_config)

#define CENTOS_IF_LIST                              CENTOS_NET_CFG->if_list
#define CENTOS_IF_LIST_I(i)                         CENTOS_NET_CFG->if_list[i]
#define CENTOS_NUM_IF                               CENTOS_NET_CFG->num_if

#define CENTOS_ACTIVE_ROUTES                        CENTOS_NET_CFG->routes
#define CENTOS_ACTIVE_ROUTES_I(i)                   CENTOS_NET_CFG->routes[i]
#define CENTOS_ACTIVE_NUM_ROUTES                    CENTOS_NET_CFG->num_routes

#define CENTOS_MAPPED                               CENTOS_NET_CFG->mapped_routes_by_if
#define CENTOS_MAPPED_I(i)                          CENTOS_NET_CFG->mapped_routes_by_if[i]
#define CENTOS_MAPPED_I_ROUTES(i)                   CENTOS_NET_CFG->mapped_routes_by_if[i]->route_list
#define CENTOS_MAPPED_I_ROUTE_J(i,j)                CENTOS_NET_CFG->mapped_routes_by_if[i]->route_list[j]
#define CENTOS_MAPPED_I_ROUTE_NUM(i)                CENTOS_NET_CFG->mapped_routes_by_if[i]->num_route

#define CENTOS_STORED_IFCFG(i)                      CENTOS_NET_CFG->stored_configs[i]
#define CENTOS_STORED_IFCFG_COMMENT(i)              CENTOS_NET_CFG->stored_configs[i]->comment
#define CENTOS_STORED_IFCFG_TYPE(i)                 CENTOS_NET_CFG->stored_configs[i]->type
#define CENTOS_STORED_IFCFG_DEVICE(i)               CENTOS_NET_CFG->stored_configs[i]->device
#define CENTOS_STORED_IFCFG_ONBOOT(i)               CENTOS_NET_CFG->stored_configs[i]->onboot
#define CENTOS_STORED_IFCFG_BOOTPROTO(i)            CENTOS_NET_CFG->stored_configs[i]->bootproto
#define CENTOS_STORED_IFCFG_IPADDR(i)               CENTOS_NET_CFG->stored_configs[i]->ipaddr
#define CENTOS_STORED_IFCFG_GATEWAY(i)              CENTOS_NET_CFG->stored_configs[i]->gateway
#define CENTOS_STORED_IFCFG_NETMASK(i)              CENTOS_NET_CFG->stored_configs[i]->netmask
#define CENTOS_STORED_IFCFG_DNS1(i)                 CENTOS_NET_CFG->stored_configs[i]->dns[0]
#define CENTOS_STORED_IFCFG_DNS2(i)                 CENTOS_NET_CFG->stored_configs[i]->dns[1]
#define CENTOS_STORED_IFCFG_IPV4_FAILURE_FATAL(i)   CENTOS_NET_CFG->stored_configs[i]->ipv4_failure_fatal
#define CENTOS_STORED_IFCFG_IPV6ADDR(i)             CENTOS_NET_CFG->stored_configs[i]->ipv6addr
#define CENTOS_STORED_IFCFG_IPV6INIT(i)             CENTOS_NET_CFG->stored_configs[i]->ipv6init
#define CENTOS_STORED_IFCFG_NM_CONTROLLED(i)        CENTOS_NET_CFG->stored_configs[i]->nm_controlled
#define CENTOS_STORED_IFCFG_USERCTL(i)              CENTOS_NET_CFG->stored_configs[i]->userctl
#define CENTOS_STORED_IFCFG_DEFROUTE(i)             CENTOS_NET_CFG->stored_configs[i]->defroute
#define CENTOS_STORED_IFCFG_VLAN(i)                 CENTOS_NET_CFG->stored_configs[i]->vlan
#define CENTOS_STORED_IFCFG_MTU(i)                  CENTOS_NET_CFG->stored_configs[i]->mtu
#define CENTOS_STORED_IFCFG_HWADDR(i)               CENTOS_NET_CFG->stored_configs[i]->hwaddr
#define CENTOS_STORED_IFCFG_UUID(i)                 CENTOS_NET_CFG->stored_configs[i]->uuid
#define CENTOS_STORED_IFCFG_NETWORK(i)              CENTOS_NET_CFG->stored_configs[i]->network
#define CENTOS_STORED_IFCFG_BROADCAST(i)            CENTOS_NET_CFG->stored_configs[i]->broadcast
#define CENTOS_STORED_IFCFG_NAME(i)                 CENTOS_NET_CFG->stored_configs[i]->name
#define CENTOS_STORED_IFCFG_IPV6_AUTOCONF(i)        CENTOS_NET_CFG->stored_configs[i]->ipv6_autoconf
#define CENTOS_STORED_IFCFG_PROXY_METHOD(i)         CENTOS_NET_CFG->stored_configs[i]->proxy_method
#define CENTOS_STORED_IFCFG_BROWSER_ONLY(i)         CENTOS_NET_CFG->stored_configs[i]->browser_only
#define CENTOS_STORED_IFCFG_ARPING_WAIT(i)          CENTOS_NET_CFG->stored_configs[i]->arping_wait
#define CENTOS_STORED_IFCFG_UNKNOWN_OPT(i)          CENTOS_NET_CFG->stored_configs[i]->unknown

#define CENTOS_ACTIVE_CFG(i)                        CENTOS_NET_CFG->active_configs[i]
#define CENTOS_ACTIVE_CFG_NAME(i)                   CENTOS_NET_CFG->active_configs[i]->name
#define CENTOS_ACTIVE_CFG_MTU(i)                    CENTOS_NET_CFG->active_configs[i]->mtu
#define CENTOS_ACTIVE_CFG_LINK(i)                   CENTOS_NET_CFG->active_configs[i]->link
#define CENTOS_ACTIVE_CFG_INET(i)                   CENTOS_NET_CFG->active_configs[i]->inet
#define CENTOS_ACTIVE_CFG_INET_MASK(i)              CENTOS_NET_CFG->active_configs[i]->inet_mask
#define CENTOS_ACTIVE_CFG_INET6(i)                  CENTOS_NET_CFG->active_configs[i]->inet6
#define CENTOS_ACTIVE_CFG_INET6_MASK(i)             CENTOS_NET_CFG->active_configs[i]->inet6_mask
#define CENTOS_ACTIVE_CFG_DYNAMIC(i)                CENTOS_NET_CFG->active_configs[i]->dynamic

#define CENTOS_PERSIST_ROUTES(i)                    CENTOS_NET_CFG->persist_rts[i]
#define CENTOS_PERSIST_ROUTES_NUM_ROUTE(i)          CENTOS_NET_CFG->persist_rts[i]->num_routes
#define CENTOS_PERSIST_ROUTES_ROUTE(i,j)            CENTOS_NET_CFG->persist_rts[i]->routes[j]
#define CENTOS_PERSIST_ROUTES_COMMENT(i,j)          CENTOS_NET_CFG->persist_rts[i]->comments[j]
#define CENTOS_PERSIST_ROUTES_GAP(i,j)              CENTOS_NET_CFG->persist_rts[i]->gaps[j]

/**********************************************************************/
/*                           CENTOS 6, 7, 8                           */
/**********************************************************************/

nsync_state_t centos_get_config(net_sync_info_t *info);

nsync_state_t centos_get_unsynced(net_sync_info_t *info);

nsync_state_t centos_check_persistent_files(net_sync_info_t *info);

nsync_state_t centos_create_and_write_to_file(net_sync_info_t *info);

nsync_state_t centos_compare_configs(net_sync_info_t *info);

nsync_state_t centos_keep_existing(net_sync_info_t *info);

nsync_state_t centos_backup_files(net_sync_info_t *info);

nsync_state_t centos_mark_synced(net_sync_info_t *info);

nsync_state_t centos_overwrite_configs(net_sync_info_t *info);

nsync_state_t centos_cleanup_and_free(net_sync_info_t *info);

#endif