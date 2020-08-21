/**
 * @file nsync_ubuntu_parse.c
 * File to store network configuration parsing functions and data structures
 * for <= Ubuntu 16.04
 * 
 * @author Ben London
 * @date 8/11/2020
 * @copyright Copyright 2020, Hyannis Port Research, Inc. All rights reserved.
 */

#include "nsync_ubuntu_parse.h"

ubuntu_parse_func_t ubuntu_parsers = {
    .ubuntu_parse_active_interfaces     = &ubuntu_parse_active_interfaces,
    .ubuntu_parse_active_routes         = &ubuntu_parse_active_routes,
    .ubuntu_parse_persist_interfaces    = &ubuntu_parse_persist_interfaces,
    .ubuntu_parse_persist_routes        = &ubuntu_parse_persist_routes,
    .ubuntu_map_routes                  = &map_routes_to_if,
};

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
    pclose(fp);
    return false;
}

/**
 * @brief Gets and parses all details of all the active interfaces.
 * 
 * @param if_list_cmd command to get the list of all interfaces
 * @param if_details_cmd command to get all the details of each interface
 * 
 * @returns a pointer to an if_data_t struct containing all of the information
 * about the active interfaces
 */
if_data_t *ubuntu_parse_active_interfaces(const char *if_list_cmd, const char *if_details_cmd)
{
    FILE *fp;
    if_data_t *ifaces = calloc(1, sizeof(if_data_t));
    MEM_CHECK(ifaces, NULL);

    /** Attempt to read output of command */
    fp = popen(if_list_cmd, "r");
	if (fp == NULL){
		sprintf(err_msg, "Could not run command: %s", if_list_cmd);
        return NULL;
    }


    /** Get the list of interface names */
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
                continue;
            }

            ifaces->if_name_list[ifaces->num_if++] = if_name;
            if_name = calloc(MAX_IF_NAME, sizeof(char));
            MEM_CHECK(if_name, NULL);
        }
        line_num++;
    }

    free(if_line);
    free(if_name);
    pclose(fp);

    /** Get the details of all the interfaces */
    for(int i = 0; i < ifaces->num_if; i++){
        char cmd[MAX_CMD_LEN];
        sprintf(cmd, if_details_cmd, ifaces->if_name_list[i]);
        fp = popen(cmd, "r");
        
        if (fp == NULL){
		    sprintf(err_msg, "Could not run command: %s", if_list_cmd);
            return NULL;
        }
        char out_buffer[MAX_OUTPUT_LEN];
        clean_output(fp, out_buffer);
        pclose(fp);

        interface_t *if_ = calloc(1, sizeof(interface_t));
        MEM_CHECK(if_, NULL);

        char *name = calloc(MAX_IF_NAME, sizeof(char));
        MEM_CHECK(name, NULL);
        get_field_delim(name, out_buffer, 2, MAX_IF_NAME, " ");
        trim(name, ":");
        if_->name = name;

        char *link = strstr(out_buffer, "link");
        if ( link != NULL){
            char tmp[MAX_UBUNTU_IF_VAL];
            get_field_delim(tmp, link, 2, MAX_UBUNTU_IF_VAL, "/");
            char *link_val = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(link_val, NULL);
            get_field_delim(link_val, tmp, 1, MAX_UBUNTU_IF_VAL, " ");
            if(strcmp(link_val, "ether") == 0){
                free(link_val);
                link_val = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(link_val, NULL);
                char *check_dyn_cmd = "ps -A -o cmd | grep -E '(/| )dhclient .'";
                FILE *check_dynamic = popen(check_dyn_cmd, "r");
                if (check_dynamic == NULL){
                    sprintf(err_msg, "Could not run command: %s", check_dyn_cmd);
                    return NULL;
                }
                char check_dyn_line[MAX_OUTPUT_LEN];
                bool found = false;
                while(fgets(check_dyn_line, MAX_OUTPUT_LEN, check_dynamic)){
                    if(strstr(check_dyn_line, if_->name) != NULL){
                        memcpy(link_val, "dhcp", 4);
                        found = true;
                        break;
                    }
                }
                if (!found) memcpy(link_val, "static", 6);
                
                pclose(check_dynamic);
            }
            if_->linktype = link_val;
        }

        /** Gets line with inet on it and find the ip address */
        char *inet = strstr(out_buffer, "inet ");
        if ( inet != NULL){
            char *inet_val = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(inet_val, NULL);
            get_field_delim(inet_val, inet, 2, MAX_UBUNTU_IF_VAL, " ");
            get_field_delim(inet_val, inet_val, 1, MAX_UBUNTU_IF_VAL, "/");
            if_->address = inet_val;
            if_->auto_opt = true;
        }

        /* Attempt to find network mask */
        char *inet_mask = strstr(out_buffer, "inet ");
        if ( inet_mask != NULL){
            char *inet_mask_val = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(inet_mask_val, NULL);
            get_field_delim(inet_mask_val, inet_mask, 2, MAX_UBUNTU_IF_VAL, " ");
            get_field_delim(inet_mask_val, inet_mask_val, 2, MAX_UBUNTU_IF_VAL, "/");
            if_->netmask = bitmask_to_netmask_ipv4(atoi(inet_mask_val));
            free(inet_mask_val);
        }

        /* Attempt to find broadcast address */
        if ( inet != NULL){
            char check[MAX_UBUNTU_IF_VAL];
            get_field_delim(check, inet, 3, MAX_UBUNTU_IF_VAL, " ");
            if(strstr(check, "brd") != NULL) {            
                char *broadcast = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(broadcast, NULL);
                get_field_delim(broadcast, inet, 4, MAX_UBUNTU_IF_VAL, " ");            
                if_->broadcast = broadcast;
            }
        }
        /** Get inet scope */
        if ( inet != NULL){
            char check[MAX_UBUNTU_IF_VAL];
            get_field_delim(check, inet, 3, MAX_UBUNTU_IF_VAL, " ");
            if(strstr(check, "scope") != NULL) {            
                char *scope = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(scope, NULL);
                get_field_delim(scope, inet, 6, MAX_UBUNTU_IF_VAL, " ");            
                if_->scope = scope;
            }
        }
        
        /** Get MTU */
        char *mtu = strstr(out_buffer, "mtu");
        if ( mtu != NULL){
            char *mtu_val = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(mtu_val, NULL);
            get_field_delim(mtu_val, mtu, 2, MAX_UBUNTU_IF_VAL, " ");
            if_->mtu = mtu_val;
        }

        /** Get HW Address -- dependent on being on the same line as link/<opt>  */
        if ( link != NULL){
            char *hw_addr = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(hw_addr, NULL);
            get_field_delim(hw_addr, link, 2, MAX_UBUNTU_IF_VAL, " ");
            if_->hwaddress = hw_addr;
        }

        ifaces->interfaces[i] = if_;

    }

    return ifaces;
}

/**
 * @brief Parse and colloect data for all active routes and store in a struct
 * 
 * @param cmd the command to output the active routes
 * @returns a pointer to a route_list_t struct containing all of the active routes
 * and associated data.
 */
route_list_t *ubuntu_parse_active_routes(const char *cmd)
{    
    FILE *fp;
    route_list_t *route_lst = calloc(1, sizeof(route_list_t));
    MEM_CHECK(route_lst, NULL);

    /** Read the output of the cmd */
    fp = popen(cmd, "r");

	if (fp == NULL){
		sprintf(err_msg, "Could not run command: %s", cmd);
        return NULL;
    }

    /** Parse all the routes */
    ubuntu_route_t route = calloc(MAX_OUTPUT_LEN, sizeof(char));
    MEM_CHECK(route, NULL);
    
    while(fgets(route, MAX_OUTPUT_LEN, fp))
    {
        // Ignore routes that are made on boot. Those dont need to be specified.
        if (strstr(route, "proto kernel") != NULL) continue;

        if (route[strlen(route)-1] == '\n') route[strlen(route)-1] = '\0';

        route_lst->routes[route_lst->num_routes++] = route;
        route = calloc(MAX_OUTPUT_LEN, sizeof(char));
        MEM_CHECK(route, NULL);

    }
    free(route);
    pclose(fp);

    return route_lst;
}


/**
 * @brief Parses all of the persistant interface configurations 
 * and store the info in the struct
 * 
 * @param file_loc the absolute location of the file holding the 
 * persistant configs for the interfaces
 * @returns a pointer to an if_data_t stuct storing the persistant 
 * configuration data
 */
if_data_t *ubuntu_parse_persist_interfaces(const char *file_loc)
{
    FILE *fp = fopen(file_loc, "r");
    if (fp == NULL) {
        sprintf(err_msg, "couldn't open file: %s", file_loc);
        return NULL;
    }

    if_data_t *persist_ifs = calloc(1, sizeof(if_data_t));
    MEM_CHECK(persist_ifs, NULL);

    char line[MAX_OUTPUT_LEN];

    // We will add 1 when we find the first iface
    persist_ifs->num_if = -1;
    bool reached_auto = false;
    while(fgets(line, MAX_OUTPUT_LEN, fp)) {
        bool found_opt = false;
    
        /** iface */
        if(strstr(line, "iface") != NULL) {
            found_opt = true;

            /** Only iterate to the next saved interface if
             * this is actually the start of a new one */
            if(reached_auto)
                reached_auto = false;
            else {
                persist_ifs->num_if++;
                char *iface_name = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(iface_name, NULL);
                get_field_delim(iface_name, line, 2, MAX_UBUNTU_IF_VAL, " ");
                persist_ifs->if_name_list[persist_ifs->num_if] = trim(iface_name,NULL);
                char *if_name_cpy = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(if_name_cpy, NULL);
                memcpy(if_name_cpy, iface_name, strlen(iface_name));
                persist_ifs->interfaces[persist_ifs->num_if] = calloc(1, sizeof(interface_t));
                MEM_CHECK(persist_ifs->interfaces[persist_ifs->num_if], NULL);
                persist_ifs->interfaces[persist_ifs->num_if]->name = if_name_cpy;
                            
            }
        }
        /** auto */
        if(strstr(line, "auto") != NULL) {
            found_opt = true;
            reached_auto = true;
            persist_ifs->num_if++;
            char *iface_name = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(iface_name, NULL);
            get_field_delim(iface_name, line, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->if_name_list[persist_ifs->num_if] = trim(iface_name,NULL);;
            char *if_name_cpy = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(if_name_cpy, NULL);
            memcpy(if_name_cpy, iface_name, strlen(iface_name));
            persist_ifs->interfaces[persist_ifs->num_if] = calloc(1, sizeof(interface_t));
            MEM_CHECK(persist_ifs->interfaces[persist_ifs->num_if], NULL);
            persist_ifs->interfaces[persist_ifs->num_if]->name = if_name_cpy;
            trim(persist_ifs->interfaces[persist_ifs->num_if]->name,NULL);
            persist_ifs->interfaces[persist_ifs->num_if]->auto_opt = true;
            
        }
        
        /** linktype/inet */
        char *inet_loc = strstr(line, "inet");
        if(inet_loc != NULL){
            found_opt = true;
            char *linktype = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(linktype, NULL);
            get_field_delim(linktype, inet_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->linktype = trim(linktype, NULL);
        }

         /** hwaddress */
        char *hwaddr_loc = strstr(line, "hwaddress");
        if(hwaddr_loc != NULL) {
            found_opt = true;
            char *hwaddress = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(hwaddress, NULL);
            get_field_delim(hwaddress, hwaddr_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->hwaddress = trim(hwaddress, NULL);
        }
        else {
            /** address */
            char *addr_loc = strstr(line, "address");
            if(addr_loc != NULL){
                found_opt = true;
                char *address = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(address, NULL);
                get_field_delim(address, addr_loc, 2, MAX_UBUNTU_IF_VAL, " ");
                persist_ifs->interfaces[persist_ifs->num_if]->address = trim(address, NULL);
            }
        }


        /** netmask */
        char *mask_loc = strstr(line, "netmask");
        if(mask_loc != NULL){
            found_opt = true;
            char *netmask = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(netmask, NULL);
            get_field_delim(netmask, mask_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->netmask = trim(netmask, NULL);
        }

        /** broadcast */
        char *brd_loc = strstr(line, "broadcast");
        if(brd_loc != NULL) {
            found_opt = true;
            char *broadcast = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(broadcast, NULL);
            get_field_delim(broadcast, brd_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->broadcast = trim(broadcast, NULL);
        }

        /** metric */
        char *metric_loc = strstr(line, "metric");
        if(metric_loc != NULL) {
            found_opt = true;
            char *metric = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(metric, NULL);
            get_field_delim(metric, metric_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->metric = trim(metric, NULL);
        }

       

        /** gateway */
        char *gw_loc = strstr(line, "gateway");
        if(gw_loc != NULL) {
            found_opt = true;
            char *gateway = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(gateway, NULL);
            get_field_delim(gateway, gw_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->gateway = trim(gateway, NULL);
        }

        /** mtu */
        char *mtu_loc = strstr(line, "mtu");
        if(mtu_loc != NULL) {
            found_opt = true;
            char *mtu = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
            MEM_CHECK(mtu, NULL);
            get_field_delim(mtu, mtu_loc, 2, MAX_UBUNTU_IF_VAL, " ");
            persist_ifs->interfaces[persist_ifs->num_if]->mtu = trim(mtu, NULL);
        }
        
        /** If its a route then it will be handled by the mapper */
        if(strstr(line, "up") != NULL)
            found_opt = true;
        else{
            /** scope */
            char *scope_loc = strstr(line, "scope");
            if(scope_loc != NULL) {
                found_opt = true;
                char *scope = calloc(MAX_UBUNTU_IF_VAL, sizeof(char));
                MEM_CHECK(scope, NULL);
                get_field_delim(scope, scope_loc, 2, MAX_UBUNTU_IF_VAL, " ");
                persist_ifs->interfaces[persist_ifs->num_if]->scope = trim(scope, NULL);
                printf("%s\n", persist_ifs->interfaces[persist_ifs->num_if]->scope);
            }
        }
        /** Uncategorized/managed */
        if (!found_opt && persist_ifs->num_if >= 0) {
            if(!persist_ifs->interfaces[persist_ifs->num_if]->unmanaged){
                persist_ifs->interfaces[persist_ifs->num_if]->unmanaged = calloc(strlen(line)+1, sizeof(char));
                MEM_CHECK(persist_ifs->interfaces[persist_ifs->num_if]->unmanaged, NULL);
                safe_strncpy(persist_ifs->interfaces[persist_ifs->num_if]->unmanaged, line, strlen(line));
            }
            else {
                // curr_size+1 to include null terminator
                int curr_size = strlen(persist_ifs->interfaces[persist_ifs->num_if]->unmanaged)+1;
                int line_size = strlen(line);
                char *new_unmanaged = calloc(curr_size+line_size, sizeof(char));
                MEM_CHECK(new_unmanaged, NULL);
                safe_strncpy(new_unmanaged, persist_ifs->interfaces[persist_ifs->num_if]->unmanaged, curr_size);
                safe_strncpy(&new_unmanaged[curr_size-1], line, line_size);
                free(persist_ifs->interfaces[persist_ifs->num_if]->unmanaged); 
                new_unmanaged[strlen(new_unmanaged)] = '\n';
                persist_ifs->interfaces[persist_ifs->num_if]->unmanaged = new_unmanaged; 
            }
        }
    }
    // To avoid segfaults w/ indexing we initialize from -1 and then need to add 1 at the end so the count is right
    persist_ifs->num_if++;

    fclose(fp);
    return persist_ifs;
}


/**
 * @brief Parses all of the persistant routes and stores them in the struct
 * 
 * @param file_loc the path to the file containing the persistent routes
 * @returns a pointer to a route_lsit_t struct containing the persistent routes
 */
route_list_t *ubuntu_parse_persist_routes(const char *file_loc)
{
    /** Open the file for reading */
    FILE *fp = fopen(file_loc, "r");
    if (fp == NULL) {
        sprintf(err_msg, "couldn't open file: %s", file_loc);
        return NULL;
    }

    route_list_t *persist_routes = calloc(1, sizeof(route_list_t));
    MEM_CHECK(persist_routes, NULL);

    /** parse all of the routes from the files */
    char line[MAX_OUTPUT_LEN];
    while(fgets(line, MAX_OUTPUT_LEN, fp)) {
        if (!strstr(line, "up")) {
            continue;
        }
        char *mark = strstr(line,"add");
        if (mark){
            ubuntu_route_t route = calloc(MAX_OUTPUT_LEN, sizeof(char));
            MEM_CHECK(route, NULL);
            memcpy(route, &mark[4], MAX_OUTPUT_LEN); // copy everything after the "up"
            trim(route, "\n");
            persist_routes->routes[persist_routes->num_routes++] = route;
        }
    }
    fclose(fp);

    return persist_routes;
}

/**
 * @brief Maps each route to its corresponding interface by adding
 * routes to the interface's route list.
 * 
 * @param sys_ifs struct containing interface configuration details
 * @param route_list struct containing a list of routes
 * 
 * @returns boolean that is true if successful and false if there was an error
 */
bool map_routes_to_if(if_data_t *sys_ifs, route_list_t *route_list)
{
    for (int i = 0; i < sys_ifs->num_if; i++) {
        sys_ifs->interfaces[i]->mapped_routes = calloc(1, sizeof(route_list_t));
        MEM_CHECK(sys_ifs->interfaces[i]->mapped_routes, false);
        if (strncmp(sys_ifs->interfaces[i]->linktype, "dhcp", 4) == 0){
            continue;
        } 
        for (int j = 0; j < route_list->num_routes; j++) {
            if (strstr(route_list->routes[j], sys_ifs->if_name_list[i]) == NULL) {
                continue;
            }
            sys_ifs->interfaces[i]->mapped_routes->
                routes[sys_ifs->interfaces[i]->
                    mapped_routes->num_routes++] = route_list->routes[j];
        }
    }
    return true;
}

/**
 * @brief Frees all of the fields in an interface_t struct
 * 
 * @param iface the interface whose fields will be freed.
 */
void free_interface(interface_t *iface)
{
    if (!iface) return;

    if(iface->name)         free(iface->name);
    if(iface->linktype)     free(iface->linktype);
    if(iface->address)      free(iface->address);
    if(iface->netmask)      free(iface->netmask);
    if(iface->broadcast)    free(iface->broadcast);
    if(iface->metric)       free(iface->metric);
    if(iface->hwaddress)    free(iface->hwaddress);
    if(iface->gateway)      free(iface->gateway);
    if(iface->mtu)          free(iface->mtu);
    if(iface->scope)        free(iface->scope);
    if(iface->unmanaged)    free(iface->unmanaged);
    
    free(iface->mapped_routes);
}
