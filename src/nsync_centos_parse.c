/**
 * @file nsync_centos_parse.c
 * File to store network configuration parsing functions and data structures
 * for CentOS 6,7,8
 * 
 * @author Ben London
 * @date 7/30/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h> 
#include "nsync_centos_parse.h"


/** ifcfg file options */
char *ifcfg_opt[NUM_OPT] =
{
    [COMMENT]               = "#",
    [TYPE]                  = "TYPE",
    [DEVICE]                = "DEVICE",
    [ONBOOT]                = "ONBOOT",
    [BOOTPROTO]             = "BOOTPROTO",
    [IPADDR]                = "IPADDR",
    [GATEWAY]               = "GATEWAY",
    [NETMASK]               = "NETMASK",
    [DNS1]                  = "DNS1",
    [DNS2]                  = "DNS2",
    [IPV4_FAILURE_FATAL]    = "IPV4_FAILURE_FATAL",
    [IPV6ADDR]              = "IPV6ADDR",
    [IPV6INIT]              = "IPV6INIT",
    [NM_CONTROLLED]         = "NM_CONTROLLED",
    [USERCTL]               = "USERCTL",
    [DEFROUTE]              = "DEFROUTE",
    [VLAN]                  = "VLAN",
    [MTU]                   = "MTU",
    [HWADDR]                = "HWADDR",
    [UUID]                  = "UUID",
    [NETWORK]               = "NETWORK",
    [BROADCAST]             = "BROADCAST",
    [NAME]                  = "NAME",
    [IPV6_AUTOCONF]         = "IPV6_AUTOCONF",
    [PROXY_METHOD]          = "PROXY_METHOD",
    [BROWSER_ONLY]          = "BROWSER_ONLY",
    [ARPING_WAIT]           = "ARPING_WAIT",
};

centos_parse_func_t centos_parsers = {
    .parse_if_list          = &centos_parse_if_list,
    .parse_routes           = &centos_parse_routes,
    .map_routes_to_if       = &centos_map_routes_to_if,
    .parse_ifcfg            = &centos_parse_ifcfg,
    .parse_ip_show          = &centos_parse_ip_show,
    .parse_persist_routes   = &centos_parse_route_cfg,
};

static bool is_up(const char *interface)
{
    char cmd[MAX_CMD_LEN];
    sprintf(cmd, "ethtool %s 2> /dev/null", interface);

    FILE *fp = popen(cmd,"r");
    if (fp == NULL){
        fprintf(stderr, "\n%sIssue running command %s. Moving on to next interface%s\n", KYEL, cmd, KNRM);
        return false;
    }

    char line[MAX_OUTPUT_LEN];
    while(fgets(line, MAX_OUTPUT_LEN, fp)){
        if(strstr(line, "Link detected: yes")){
            pclose(fp);
            return true;
        }
    }
    return false;
}

/** 
 * @brief Parses the list of interfaces on any CentOS 6/7/8 system
 * and stores them in a specialized struct
 * 
 * @param cmd the string corresponding to the correct command to 
 * retrieve the list of interfaces
 * @returns a pointer to an if_list_parsed struct containing the
 * parsed information 
 */
if_list_parsed_t *centos_parse_if_list(const char *cmd)
{
    FILE *fp;
    if_list_parsed_t *parsed_list = calloc(1, sizeof(if_list_parsed_t));
    MEM_CHECK(parsed_list, NULL);

    parsed_list->num_if = 0;
    parsed_list->if_list = calloc(MAX_NUM_IF,sizeof(char *));
    MEM_CHECK(parsed_list->if_list, NULL);
    
    /** Attempt to read the output of the command */
	fp = popen(cmd, "r");
	if (fp == NULL){
		sprintf(err_msg, "Could not run command: %s", cmd);
        return NULL;
    }

    /** Read each line of the output and parse ouyt the interface names */
    char *if_line = calloc(MAX_OUTPUT_LEN, sizeof(char));
    MEM_CHECK(if_line, NULL);
    char *if_name = calloc(MAX_IF_NAME, sizeof(char));
    MEM_CHECK(if_name, NULL);
    int line_num = 0;
    char *delim = " ";
	while (fgets(if_line, MAX_OUTPUT_LEN, fp) != NULL){
        if (line_num % 2 == 0) {
            get_field_delim(if_name, if_line, 2, MAX_IF_NAME, delim);
            if_name[strlen(if_name)-1] = 0;
            char* at = strchr(if_name, '@');
            if (at) *at = 0;

            if (!is_up(if_name)){
                free(if_name);
                if_name = calloc(MAX_IF_NAME, sizeof(char));
                line_num++;
                continue;
            }
            
            parsed_list->if_list[parsed_list->num_if++] = if_name;
            if_name = calloc(MAX_IF_NAME, sizeof(char));
            MEM_CHECK(if_name, NULL);
        }
        line_num++;
    }
    free(if_line);
    free(if_name);
    pclose(fp);

    return parsed_list;
}

/** 
 * @brief Parses the list of routes on any CentOS 6/7/8 system
 * and stores them in a specialized struct. Note: this function
 * will ignore any routes with the the attribute proto set to kernel,
 * as these were done on boot and not added manually
 * 
 * @param cmd the string corresponding to the correct command to 
 * retrieve the list of active routes
 * @returns a pointer to an routes_parsed struct containing the
 * parsed information
 */
routes_parsed_t *centos_parse_routes(const char *cmd)
{
    routes_parsed_t *parsed_routes = calloc(1, sizeof(routes_parsed_t));
    MEM_CHECK(parsed_routes, NULL);

    parsed_routes->route_list = calloc(MAX_ROUTES,sizeof(char *));
    MEM_CHECK(parsed_routes->route_list, NULL);

    /** Attempt to read the command output */
    FILE *fp = popen(cmd, "r");
    if (fp == NULL){
        sprintf(err_msg, "could not run command: %s", cmd);
        return NULL;
    }
    
    /** Iterate through the lines of the output and store the route data */
    centos_route_t route = calloc(MAX_OUTPUT_LEN, sizeof(char));;
    MEM_CHECK(route, NULL);

    while(fgets(route, MAX_OUTPUT_LEN, fp))
    {
        /** Ignore routes that are made on boot. */
        if (strstr(route, "proto kernel") != NULL) 
            continue;

        parsed_routes->route_list[parsed_routes->num_route++] = trim(route, NULL);
        route = calloc(MAX_OUTPUT_LEN, sizeof(char));
        MEM_CHECK(route, NULL);

    }
    free(route);
    pclose(fp);

    return parsed_routes;   
}

/** 
 * @brief Maps each route to its corresponding interface so that all
 * routes relating to a specific interface can be quickly and easily
 * accessed.
 * 
 * @param rp a pointer to a routes_parsed struct containing routes
 * corresponding to interfaces
 * @param ilp a pointer to an if_list_parsed struct containing the information
 * about the network interfaces
 * @returns a map_routes_it_t object (a 2D char array) whose indices correspond
 * to an interface's index in the interface list of ilp. The string stored at
 * that index is the routes that go with the corresponding interface.
 */
map_routes_if_t centos_map_routes_to_if(routes_parsed_t *rp, if_list_parsed_t *ilp)
{
    map_routes_if_t mappings = calloc(ilp->num_if, sizeof(routes_parsed_t *));
    MEM_CHECK(mappings, NULL);
    int i_num, r_num;
    /** Iterate through all interfaces */
    for (i_num = 0; i_num < ilp->num_if; i_num++){
        mappings[i_num] = (routes_parsed_t *)calloc(1, sizeof(routes_parsed_t));
        MEM_CHECK( mappings[i_num], NULL);
        mappings[i_num]->num_route = 0;
        mappings[i_num]->route_list = calloc(MAX_ROUTES, sizeof(centos_route_t));
        MEM_CHECK(mappings[i_num]->route_list, NULL);
        
        /** Separate name from any alias */
        char sep_name[strlen(ilp->if_list[i_num])+1];
        safe_strncpy(sep_name, ilp->if_list[i_num], strlen(ilp->if_list[i_num])+1);
        sep_name[strlen(ilp->if_list[i_num])+1] = '\0';

        /** Iterate throguh all routes and check for interface name*/
        for (r_num = 0; r_num < rp->num_route; r_num++){
            if (strstr(rp->route_list[r_num], sep_name) == NULL)
                continue;
        
            /** If the interface name is found then add that route to the list */
            mappings[i_num]->route_list[mappings[i_num]->num_route] = rp->route_list[r_num];
            mappings[i_num]->num_route++;
        }
    }
    return mappings;
}

/**
 * @brief converts an ifcfg option string to its corresponding enum value
 * @param str the option string
 * @returns the enum value
 */
ifcfg_opt_t opt_str_to_enum(const char *str)
{
    for (ifcfg_opt_t i = 0; i < NUM_OPT; i++)
    {
        if (strcmp(str, ifcfg_opt[i]) == 0) 
            return i;
    }
    return UNKNOWN_OPT;
}

/**
 * @brief Basically a wrapper for a switch, will determine if the line has 
 * a valid field for the ifcfg file and set the appropriate struct field.
 * @param cfg_data pointer to the struct holding the ifcfg file's data
 * @param line the current line that needs to be parsed
 * @returns a success(0) or failure(-1) code
 */
static int parse_ifcfg_fields(ifcfg_fields_t *cfg_data, const char *line)
{
    char opt_str[MAX_OPT_LEN];
    char *opt_val = calloc(MAX_VAL_LEN, sizeof(char));
    MEM_CHECK(opt_val, -1);

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

    ifcfg_opt_t opt = opt_str_to_enum(opt_str);

    switch(opt){

    case COMMENT:
        if(!cfg_data->comment){
            cfg_data->comment = calloc(strlen(line)+1, sizeof(char));
            MEM_CHECK(cfg_data->comment, -1);
            safe_strncpy(cfg_data->comment, line, strlen(line));
        }
        else {
            int curr_size = strlen(cfg_data->comment)+1;
            int val_size = strlen(line);
            char *new_comment = calloc(curr_size+val_size, sizeof(char));
            MEM_CHECK(new_comment, -1);
            safe_strncpy(new_comment, cfg_data->comment, curr_size);
            safe_strncpy(&new_comment[curr_size], line, val_size);
            free(cfg_data->comment); 
            new_comment[strlen(new_comment)] = '\n';
            cfg_data->comment = new_comment;
        }
        free(cfg_data->comment);
        cfg_data->comment = opt_val;
        break;
    case TYPE:
        cfg_data->type = opt_val;
        break;
    case DEVICE:
        free(cfg_data->device);
        cfg_data->device = opt_val;
        break;
    case ONBOOT:
        cfg_data->onboot = opt_val;
        break;
    case BOOTPROTO:
        cfg_data->bootproto = opt_val;
        break;
    case IPADDR:
        cfg_data->ipaddr = opt_val;
        break;
    case GATEWAY:
        cfg_data->gateway = opt_val;
        break;
    case NETMASK:
        cfg_data->netmask = opt_val;
        break;
    case DNS1:
        cfg_data->dns[0] = opt_val;
        break;
    case DNS2:
        cfg_data->dns[1] = opt_val;
        break;
    case IPV4_FAILURE_FATAL:
        cfg_data->ipv4_failure_fatal = opt_val;
        break;
    case IPV6ADDR:
        cfg_data->ipv6addr = opt_val;
        break;
    case IPV6INIT:
        cfg_data->ipv6init = opt_val;
        break;
    case NM_CONTROLLED:
        cfg_data->nm_controlled = opt_val;
        break;
    case USERCTL:
        cfg_data->userctl = opt_val;
        break;
    case DEFROUTE:
        cfg_data->defroute = opt_val;
        break;
    case VLAN:
        cfg_data->vlan = opt_val;
        break;
    case MTU:
        cfg_data->mtu = opt_val;
        break;
    case HWADDR:
        cfg_data->hwaddr = opt_val;
        break;
    case UUID:
        cfg_data->uuid = opt_val;
        break;
    case NETWORK:
        cfg_data->network = opt_val;
        break;
    case BROADCAST:
        cfg_data->broadcast = opt_val;
        break;
    case NAME:
        cfg_data->name = opt_val;
        break;
    case IPV6_AUTOCONF:
        cfg_data->ipv6_autoconf = opt_val;
        break;
    case PROXY_METHOD:
        cfg_data->proxy_method = opt_val;
        break;
    case BROWSER_ONLY:
        cfg_data->browser_only = opt_val;
        break;
    case ARPING_WAIT:
        cfg_data->arping_wait = opt_val;
        break;
    case UNKNOWN_OPT:
        if(!cfg_data->unknown){
            cfg_data->unknown = calloc(strlen(line)+1, sizeof(char));
            MEM_CHECK(cfg_data->unknown, -1);
            safe_strncpy(cfg_data->unknown, line, strlen(line));
            cfg_data->unknown[strlen(cfg_data->unknown)] = '\n';
        }
        else {
            int curr_size = strlen(cfg_data->unknown)+1;
            int line_size = strlen(line);
            char *new_unknown = calloc(curr_size+line_size, sizeof(char));
            MEM_CHECK(new_unknown, -1);
            safe_strncpy(new_unknown, cfg_data->unknown, curr_size);
            safe_strncpy(&new_unknown[curr_size-1], line, line_size);
            free(cfg_data->unknown); 
            new_unknown[strlen(new_unknown)] = '\n';
            cfg_data->unknown = new_unknown; 
        }
        free(opt_val);
        break;

    default:
        sprintf(err_msg, "invalid opt: %s", opt_str);
        free(opt_val);
        return -1;
    }

    return 0;
}

/**
 * @brief Parses the existing persistent network configuration files of a
 * CentOS system and stores the fields in a struct.
 * 
 * @param path the path to a speicfic configuration file
 * @returns a pointer to an ifcfg_fields_t struct whose fields have been
 * initialized to match the data in the config file. If a field is not set
 * in the config file, then it is left empty (NULL) in the struct.
 */
ifcfg_fields_t *centos_parse_ifcfg(const char *path)
{
    char file_path[FILENAME_MAX];
    sprintf(file_path,"%s", path);
    
    /** Check for the existence of the file */
    if (!file_exists(file_path)){
        return NULL;
    }

    /** Open the file for reading */
    FILE *fp = fopen(file_path, "r");
    if (!fp) {
        char msg[FILENAME_MAX + ERR_LEN];
        sprintf(msg, "file %s could not be read", file_path);
        memcpy(err_msg, msg, ERR_LEN);
        return fatal_err_ptr;
    }

    char line[CFG_LINE_LEN];
    ifcfg_fields_t *cfg_data = calloc(1, sizeof(ifcfg_fields_t));
    MEM_CHECK(cfg_data, fatal_err_ptr);

    /** Iterate through the lines of the file and parse out the field of the configuration */
    while (fgets(line, CFG_LINE_LEN, fp)) {
        /** if we encounter an error then free the cfg_data and return the error ptr. */
        if (parse_ifcfg_fields(cfg_data, line) < 0){
            free(cfg_data);
            return fatal_err_ptr;
        }
    }
    fclose(fp);
    return cfg_data;
}


/**
 * @brief Cleans up the output of a file and stores it in a buffer.
 * Specifically used to clean the output for centos_parse_ip_show.
 * NOTE: Not for general use.
 * 
 * @param fp the file pointer to an open data stream
 * @param out_buff the buffer to store the cleaned-up file contents
 */
static void clean_output(FILE *fp, char *out_buff)
{
    char line[MAX_OUTPUT_LEN];

    int offset = 0;
    
    /** 
     * Make output all on single line
     * Replace all newlines with spaces
     */
    while(fgets(line, MAX_OUTPUT_LEN, fp))
    {
        char *pchar = strchr(line, '\n');
        if(pchar){
            *pchar = ' ';
        }
        safe_strncpy(&out_buff[offset], line, MAX_OUTPUT_LEN - offset);
        offset += strlen(line);
    }
    
    /** strip all extra spaces */
    int i, x;
    for(i=0, x=0; out_buff[i]; i++){
        if(!isspace(out_buff[i]) || (i > 0 && !isspace(out_buff[i-1])))
            out_buff[x++] = out_buff[i];
    }
    out_buff[x] = '\0';
}


/**
 * @brief Parses the active network configuration settings displayed by
 * the given command for a CentOS system.
 * 
 * @param cmd the command to output the active network configuration
 * @returns a pointer to an ip_show_fields_t struct whose fields have been
 * initialized to match the data parsed from the command's output. 
 * If a field is not set in the active config, then it is left empty 
 * (NULL) in the struct.
 */
ip_show_fields_t *centos_parse_ip_show(const char *cmd)
{   
    /** Try to read the output of the command */
    FILE *fp = popen(cmd, "r");
    if (fp == NULL){
        sprintf(err_msg, "Could not run command: %s", cmd);
        return fatal_err_ptr;
    }

    ip_show_fields_t *addr_show_data = calloc(1, sizeof(ifcfg_fields_t));
    MEM_CHECK(addr_show_data, fatal_err_ptr);

    char out_buffer[MAX_OUTPUT_LEN];
    /** Clean up the output */
    clean_output(fp, out_buffer);
    pclose(fp);

    /** Parse out all fields */
    char *name = calloc(MAX_VAL_LEN, sizeof(char));
    MEM_CHECK(name, fatal_err_ptr);
    get_field_delim(name, out_buffer, 2, MAX_VAL_LEN, " ");
    trim(name, ":");
    char* at = strchr(name, '@');
    if (at) *at = 0;
    addr_show_data->name = name;

    char *mtu = strstr(out_buffer, "mtu");
    if ( mtu != NULL){
        char *mtu_val = calloc(MAX_VAL_LEN, sizeof(char));
        MEM_CHECK(mtu_val, fatal_err_ptr);
        get_field_delim(mtu_val, mtu, 2, MAX_VAL_LEN, " ");
        addr_show_data->mtu = mtu_val;
    }

    char *link = strstr(out_buffer, "link");
    if ( link != NULL){
        char *link_val = calloc(MAX_VAL_LEN, sizeof(char));
        MEM_CHECK(link_val, fatal_err_ptr);
        get_field_delim(link_val, link, 2, MAX_VAL_LEN, "/ ");
        addr_show_data->link = link_val;
    }

    char *inet = strstr(out_buffer, "inet");
    if ( inet != NULL){
        char *inet_val = calloc(MAX_VAL_LEN, sizeof(char));
        MEM_CHECK(inet_val, fatal_err_ptr);
        get_field_delim(inet_val, inet, 2, MAX_VAL_LEN, " ");
        get_field_delim(inet_val, inet_val, 1, MAX_VAL_LEN, "/");
        addr_show_data->inet = inet_val;
    }

    char *inet_mask = strstr(out_buffer, "inet");
    if ( inet_mask != NULL){
        char *inet_mask_val = calloc(MAX_VAL_LEN, sizeof(char));
        MEM_CHECK(inet_mask_val, fatal_err_ptr);
        get_field_delim(inet_mask_val, inet_mask, 2, MAX_VAL_LEN, " ");
        get_field_delim(inet_mask_val, inet_mask_val, 2, MAX_VAL_LEN, "/");
        addr_show_data->inet_mask = bitmask_to_netmask_ipv4(atoi(inet_mask_val));
        free(inet_mask_val);
    }

    char *inet6 = strstr(out_buffer, "inet6");
    if ( inet6 != NULL){
        char *inet6_val = calloc(MAX_VAL_LEN, sizeof(char));
        MEM_CHECK(inet6_val, fatal_err_ptr);
        get_field_delim(inet6_val, inet6, 2, MAX_VAL_LEN, " ");
        get_field_delim(inet6_val, inet6_val, 1, MAX_VAL_LEN, "/");
        addr_show_data->inet6 = inet6_val;
    }
    
    char *inet6_mask = strstr(out_buffer, "inet6");
    if ( inet6_mask != NULL){
        char *inet6_mask_val = calloc(MAX_VAL_LEN, sizeof(char));
        MEM_CHECK(inet6_mask_val, fatal_err_ptr);
        get_field_delim(inet6_mask_val, inet6_mask, 2, MAX_VAL_LEN, " ");
        get_field_delim(inet6_mask_val, inet6_mask_val, 1, MAX_VAL_LEN, "/");
        addr_show_data->inet6_mask = inet6_mask_val;
    }

    /** Check if the interface is dynamic */
    char *check_dyn_cmd = "ps -A -o cmd | grep -E '(/| )dhclient .'";
    FILE *check_dynamic = popen(check_dyn_cmd, "r");
    if (check_dynamic == NULL){
        sprintf(err_msg, "Could not run command: %s", check_dyn_cmd);
        return fatal_err_ptr;
    }
    char *check_dyn_line = calloc(MAX_OUTPUT_LEN, sizeof(char));
    MEM_CHECK(check_dyn_line, fatal_err_ptr);
    while(fgets(check_dyn_line, MAX_OUTPUT_LEN, check_dynamic)){
        if(name && strstr(check_dyn_line, name) != NULL)
            addr_show_data->dynamic = true;
    }
    free(check_dyn_line);
    pclose(check_dynamic);

    return addr_show_data;
}

/**
 * @brief Basically a wrapper for a few conditionals. Determines the type
 * of line within the persistent route file and sets the appropriate fields
 * in the struct
 * @param route_cfg pointer to the struct holding the route file's data
 * @param route the current line of the route file to be parsed
 * @returns boolean true if successful, false if error
 */
static bool parse_route_cfg_fields(rt_cfg_t *route_cfg, centos_route_t route)
{
    /** case of being a blank line */
    if(route[0] == '\n'){
        if (!route_cfg->gaps[route_cfg->num_routes]){
            route_cfg->gaps[route_cfg->num_routes] = calloc(strlen(route)+1, sizeof(char));
            MEM_CHECK(route_cfg->gaps[route_cfg->num_routes], false);
            safe_strncpy(route_cfg->gaps[route_cfg->num_routes], route, strlen(route)+1);
        }
        else {
            int curr_size = strlen(route_cfg->gaps[route_cfg->num_routes])+1;
            int route_len = strlen(route);
            char *new_gap = calloc(curr_size+route_len, sizeof(char));
            MEM_CHECK(new_gap, false);
            safe_strncpy(new_gap,route_cfg->gaps[route_cfg->num_routes], curr_size);
            safe_strncpy(&new_gap[curr_size-1], route, route_len);
            free(route_cfg->gaps[route_cfg->num_routes]); 
            new_gap[strlen(new_gap)] = '\n';
            route_cfg->gaps[route_cfg->num_routes] = new_gap; 
        }
        return true;
    }

    /** Case of being a comment */
    if(route[0] == '#'){
        if (!route_cfg->comments[route_cfg->num_routes]){
            route_cfg->comments[route_cfg->num_routes] = calloc(strlen(route)+1, sizeof(char));
            MEM_CHECK(route_cfg->comments[route_cfg->num_routes], false);
            safe_strncpy(route_cfg->comments[route_cfg->num_routes], route, strlen(route)+1);
        }
        else {
            int curr_size = strlen(route_cfg->comments[route_cfg->num_routes])+1;
            int route_len = strlen(route);
            char *new_comment = calloc(curr_size+route_len, sizeof(char));
            MEM_CHECK(new_comment, false);
            safe_strncpy(new_comment,route_cfg->comments[route_cfg->num_routes], curr_size);
            safe_strncpy(&new_comment[curr_size-1], route, route_len);
            free(route_cfg->comments[route_cfg->num_routes]); 
            new_comment[strlen(new_comment)] = '\n';
            route_cfg->comments[route_cfg->num_routes] = new_comment; 
        }
        return true;
    }

    /** Typical route */
    route_cfg->routes[route_cfg->num_routes++] = trim(route, NULL);    
    return true;
}

/**
 * @brief Parses the persistent routes of a given interface
 * and stores them in a specialized struct
 * 
 * @param path the path to the route file
 * @returns a pointer to a route config struct that contains
 * all persistent routes of the interface. If the interface does
 * not have a persistent route file, then it return NULL.
 */
rt_cfg_t *centos_parse_route_cfg(const char *path)
{
    /** Attempt to open the route file */
    FILE *fp = fopen(path, "r");
    if(!fp){
        return NULL;
    }

    rt_cfg_t *route_cfg = calloc(1, sizeof(rt_cfg_t));
    MEM_CHECK(route_cfg, fatal_err_ptr);

    centos_route_t route = calloc(MAX_OUTPUT_LEN, sizeof(char));
    MEM_CHECK(route, fatal_err_ptr);

    while(fgets(route, MAX_OUTPUT_LEN, fp)){
        if(!parse_route_cfg_fields(route_cfg, route))
            return fatal_err_ptr;
        route = calloc(MAX_OUTPUT_LEN, sizeof(char));
        MEM_CHECK(route, fatal_err_ptr);
    }
    free(route);
    fclose(fp);

    return route_cfg;
}



/** 
 * @brief Frees each of the individual fields of the given struct.
 * 
 * @param free_cfg the ifcfg_fields_t struct whose fields need to be freed
 */
void free_ifcfg_fields(ifcfg_fields_t *free_cfg)
{
    if(!free_cfg) return;
    if (free_cfg->comment)              free(free_cfg->comment);
    if (free_cfg->type)                 free(free_cfg->type);
    if (free_cfg->device)               free(free_cfg->device);
    if (free_cfg->onboot)               free(free_cfg->onboot);
    if (free_cfg->bootproto)            free(free_cfg->bootproto);
    if (free_cfg->ipaddr)               free(free_cfg->ipaddr);
    if (free_cfg->gateway)              free(free_cfg->gateway);
    if (free_cfg->netmask)              free(free_cfg->netmask);
    if (free_cfg->dns[0])               free(free_cfg->dns[0]);
    if (free_cfg->dns[1])               free(free_cfg->dns[1]);
    if (free_cfg->ipv4_failure_fatal)   free(free_cfg->ipv4_failure_fatal);
    if (free_cfg->ipv6addr)             free(free_cfg->ipv6addr);
    if (free_cfg->ipv6_autoconf)        free(free_cfg->ipv6_autoconf);
    if (free_cfg->ipv6init)             free(free_cfg->ipv6init);
    if (free_cfg->ipv6_failure_fatal)   free(free_cfg->ipv6_failure_fatal);
    if (free_cfg->ipv6_addr_gen_mode)   free(free_cfg->ipv6_addr_gen_mode);
    if (free_cfg->nm_controlled)        free(free_cfg->nm_controlled);
    if (free_cfg->userctl)              free(free_cfg->userctl);
    if (free_cfg->defroute)             free(free_cfg->defroute);
    if (free_cfg->vlan)                 free(free_cfg->vlan);
    if (free_cfg->mtu)                  free(free_cfg->mtu);
    if (free_cfg->hwaddr)               free(free_cfg->hwaddr);
    if (free_cfg->uuid)                 free(free_cfg->uuid);
    if (free_cfg->network)              free(free_cfg->network);
    if (free_cfg->broadcast)            free(free_cfg->broadcast);
    if (free_cfg->name)                 free(free_cfg->name);
    if (free_cfg->proxy_method)         free(free_cfg->proxy_method);
    if (free_cfg->browser_only)         free(free_cfg->browser_only);
    if (free_cfg->arping_wait)          free(free_cfg->arping_wait);
    if (free_cfg->unknown)              free(free_cfg->unknown);

}


/** 
 * @brief Frees each of the individual fields of the given struct.
 * 
 * @param free_ip_show the ip_show_fields_t struct whose fields need to be freed
 */
void free_ip_show_fields(ip_show_fields_t *free_ip_show)
{
    if(!free_ip_show) 
        return;

    if(free_ip_show->name)          free(free_ip_show->name);
    if(free_ip_show->mtu)           free(free_ip_show->mtu);
    if(free_ip_show->link)          free(free_ip_show->link);
    if(free_ip_show->inet)          free(free_ip_show->inet);
    if(free_ip_show->inet_mask)     free(free_ip_show->inet_mask);
    if(free_ip_show->inet6)         free(free_ip_show->inet6);
    if(free_ip_show->inet6_mask)    free(free_ip_show->inet6_mask);
}

/** 
 * @brief Frees each of the individual fields of the given struct.
 * 
 * @param free_rt_cfg the rt_cfg_t struct whose fields need to be freed
 */
void free_route_config(rt_cfg_t *free_rt_cfg)
{
    if (!free_rt_cfg) return;
    for(int j = 0; j < MAX_ROUTES; j++){
            if(free_rt_cfg->comments[j]) 
                free(free_rt_cfg->comments[j]);
            
            if(free_rt_cfg->gaps[j]) 
                free(free_rt_cfg->gaps[j]);
        }
    for (int i = 0; i < free_rt_cfg->num_routes; i++){
        free(free_rt_cfg->routes[i]);
    }
}