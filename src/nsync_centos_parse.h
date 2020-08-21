/**
 * @file nsync_centos_parse.h
 * @author Ben London
 * @date 7/30/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#ifndef NSYNC_CENTOS_PARSE_H
#define NSYNC_CENTOS_PARSE_H
 
#include "nsync_utils.h"
#include "nsync_info.h"

/** GLOBAL ERROR BUFFER */
extern char err_msg[ERR_LEN];

/** MACRO CONSTANTS */
#define MAX_NUM_IF 255
#define MAX_ROUTES 100
#define MAX_IF_NAME 100

#define MAX_NUM_IP 100
#define MAX_NUM_GW 100
#define MAX_NUM_NM 100
#define CFG_LINE_LEN 300

#define MAX_OPT_LEN 100
#define MAX_VAL_LEN CFG_LINE_LEN-MAX_OPT_LEN-1



/**********************************************************************/
/*                              ENUMS                                 */
/**********************************************************************/
/** 
 * @enum ifcfg_opt_t 
 * @brief enumeration of the possible options that can be found in an ifcfg file
*/
typedef enum {
    COMMENT = 0,
    TYPE,
    DEVICE,
    ONBOOT,
    BOOTPROTO,
    IPADDR,
    GATEWAY,
    NETMASK,
    DNS1,
    DNS2,
    IPV4_FAILURE_FATAL,
    IPV6ADDR,
    IPV6INIT,
    NM_CONTROLLED,
    USERCTL,
    DEFROUTE,
    VLAN,
    MTU,
    HWADDR,
    UUID,
    NETWORK,
    BROADCAST,
    NAME,
    IPV6_AUTOCONF,
    PROXY_METHOD,
    BROWSER_ONLY,
    ARPING_WAIT,
    NUM_OPT,
    UNKNOWN_OPT,
} ifcfg_opt_t;

extern char **ifcfg_opts;

/** 
 * @enum addr_show_opts_t 
 * @brief enumeration of the possible fields of the active configuration
 */
typedef enum {
    STATE,
    LINK,
    INET,
    MASK,
    INET6,
    MTU_VAL,
    NUM_ADR_SHW_OPT,
    INVALID_ADR_SHW_OPT,
} addr_show_opts_t;

/**********************************************************************/
/*                             STRUCTS                                */
/**********************************************************************/
/**
 * @struct route_data
 * @brief stores the 3 fields needed for a route: ADDRESS, GATEWAY, NETMASK
 */
typedef char * centos_route_t;


/** 
 * @struct if_list_parsed
 * @brief struct to store the list of all
 * network interfaces and the length of the list
 */
typedef struct if_list_parsed{
    char** if_list;
    int num_if;
} if_list_parsed_t;


/** 
 * @struct routes_parsed
 * @brief struct to store the list of all
 * active routes and the length of the list
 */
typedef struct routes_parsed{
    centos_route_t*  route_list;
    int num_route;
} routes_parsed_t;


typedef routes_parsed_t** map_routes_if_t;

/** 
 * @struct interface_config_fields
 * @brief Stores the values that might exist in the ifcfg-<interface> files
 */
typedef struct interface_config_fields{
    char *comment; // Stores any line starting with a # symbol
    char *type;
    char *device;
    char *onboot;
    char *bootproto;

    char *ipaddr;
    char *gateway;
    char *netmask;
    char *dns[2];
    
    char *ipv4_failure_fatal;
    char *ipv6addr;
    char *ipv6_autoconf;
    char *ipv6init;
    char *ipv6_failure_fatal;
    char *ipv6_addr_gen_mode;

    char *nm_controlled;
    char *userctl;
    char *defroute;

    char *vlan;
    char *mtu;
    char *hwaddr;
    char *uuid;

    char *network;
    char *broadcast;

    char *name;
    char *proxy_method;
    char *browser_only;
    char *arping_wait;

    char *unknown;
} ifcfg_fields_t;


/** 
 * @struct ip_addr_show_fields
 * @brief Stores the important values that might appear
 * in the output of the `ip addr show <interface>` command
 */
typedef struct ip_addr_show_fields{
    char *name;
    char *mtu;
    char *link;
    char *inet;
    char *inet_mask;
    char *inet6;
    char *inet6_mask;
    bool dynamic;
}ip_show_fields_t;


typedef struct route_config {
    centos_route_t routes[MAX_ROUTES];
    int num_routes;
    char *comments[MAX_ROUTES];
    char *gaps[MAX_ROUTES];
}rt_cfg_t;

/** 
 * @struct parse_func
 * @brief used to store function pointers to various parsing functions
 */
typedef struct centos_parse_func {
        if_list_parsed_t *(*parse_if_list)(const char *cmd);
        routes_parsed_t *(*parse_routes)(const char *cmd);
        map_routes_if_t (*map_routes_to_if)(routes_parsed_t *rp, if_list_parsed_t *ilp);
        ifcfg_fields_t *(*parse_ifcfg)(const char *path);
        ip_show_fields_t *(*parse_ip_show)(const char *cmd);
        rt_cfg_t *(*parse_persist_routes)(const char *path);
}centos_parse_func_t;

extern centos_parse_func_t centos_parsers;


/** 
 * The fatal_err_ptr is used in place of a NULL to check for the failure
 * of a function that uses NULL as a valid return value;
 */
extern void *fatal_err_ptr;

/**********************************************************************/
/*                             PARSERS                                */
/**********************************************************************/

if_list_parsed_t* centos_parse_if_list(const char *cmd);

routes_parsed_t *centos_parse_routes(const char *cmd);

map_routes_if_t centos_map_routes_to_if(routes_parsed_t *rp, if_list_parsed_t *ilp);

ifcfg_fields_t *centos_parse_ifcfg(const char *path);

ip_show_fields_t *centos_parse_ip_show(const char *cmd);

rt_cfg_t *centos_parse_route_cfg(const char *path);

/**********************************************************************/
/*                         GENERAL FUNCTIONS                          */
/**********************************************************************/

ifcfg_opt_t opt_str_to_enum(const char *str);

/** Free Structs */

void free_ifcfg_fields(ifcfg_fields_t *free_cfg);

void free_ip_show_fields(ip_show_fields_t *free_ip_show);

void free_route_config(rt_cfg_t *free_rt_cfg);

#endif