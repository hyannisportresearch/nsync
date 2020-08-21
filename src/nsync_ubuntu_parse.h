/**
 * @file nsync_ubuntu_parse.h
 * @author Ben London
 * @date 8/11/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_info.h"
#include "nsync_utils.h"

#ifndef NSYNC_UBUNTU_PARSE_H
#define NSYNC_UBUNTU_PARSE_H


/**********************************************************************/
/*                             CONSTANTS                              */
/**********************************************************************/
#define MAX_UBUNTU_IF_VAL 100

/**********************************************************************/
/*                         STRUCTS AND TYPES                          */
/**********************************************************************/

typedef char*  ubuntu_route_t;

typedef struct route_list{
    ubuntu_route_t routes[MAX_ROUTES];
    int num_routes;
} route_list_t;

typedef struct interface{
    char *name;
    char *linktype;
    char *address;
    char *netmask;
    char *broadcast;
    char *metric;
    char *hwaddress;
    char *gateway;
    char *mtu;
    char *scope;
    char *unmanaged;
    bool auto_opt;

    route_list_t* mapped_routes;

}interface_t;

typedef struct sys_interfaces
{
    char *if_name_list[MAX_NUM_IF];
    interface_t *interfaces[MAX_NUM_IF];
    int num_if;
} if_data_t;

#define IF_I_NAME(ifs, i)       ifs[i]->name 
#define IF_I_LINKTYPE(ifs, i)   ifs[i]->linktype
#define IF_I_ADDRESS(ifs, i)    ifs[i]->address 
#define IF_I_NETMASK(ifs, i)    ifs[i]->netmask 
#define IF_I_BROADCAST(ifs, i)  ifs[i]->broadcast 
#define IF_I_METRIC(ifs, i)     ifs[i]->metric 
#define IF_I_HWADDRESS(ifs, i)  ifs[i]->hwaddress 
#define IF_I_GATEWAY(ifs, i)    ifs[i]->gateway 
#define IF_I_MTU(ifs, i)        ifs[i]->mtu 
#define IF_I_SCOPE(ifs, i)      ifs[i]->scope 
#define IF_I_UNMANAGED(ifs, i)  ifs[i]->unmanaged 
#define IF_I_AUTO_OPT(ifs, i)   ifs[i]->auto_opt

#define IF_I_ROUTES(ifs, i)     ifs[i]->mapped_routes->routes
#define IF_I_ROUTE_NUM(ifs, i)  ifs[i]->mapped_routes->num_routes
#define IF_I_ROUTE_J(ifs, i, j) ifs[i]->mapped_routes->routes[j]



typedef struct ubuntu_parse_func {
        if_data_t *(*ubuntu_parse_active_interfaces)(const char *if_list_cmd,const char *if_details_cmd);
        route_list_t *(*ubuntu_parse_active_routes)(const char *cmd);
        if_data_t *(*ubuntu_parse_persist_interfaces)(const char *file_loc);
        route_list_t *(*ubuntu_parse_persist_routes)(const char *file_loc);
        bool (*ubuntu_map_routes)(if_data_t *sys_ifs, route_list_t *route_list);
}ubuntu_parse_func_t;

/**********************************************************************/
/*                         PARSING FUNCTIONS                          */
/**********************************************************************/
extern ubuntu_parse_func_t ubuntu_parsers;

if_data_t *ubuntu_parse_active_interfaces(const char *if_list_cmd, const char *if_details_cmd);

route_list_t *ubuntu_parse_active_routes(const char *cmd);

if_data_t *ubuntu_parse_persist_interfaces(const char *file_loc);

route_list_t *ubuntu_parse_persist_routes(const char *file_loc);

bool map_routes_to_if(if_data_t *sys_ifs, route_list_t *route_list);

void free_interface(interface_t *iface);

#endif