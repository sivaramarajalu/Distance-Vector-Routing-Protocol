/**
 * @author
 * @author  Swetank Kumar Saha <swetankk@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * AUTHOR [Control Code: 0x00]
 */

#include <string.h>
#include <netinet/in.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"

#define AUTHOR_STATEMENT "I, sramadur, have read and understood the course academic integrity policy."

/*code taken from stack overflow*/
void print_ip(uint32_t ip, char msg[16]) {
	unsigned char bytes[4];
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	sprintf(msg, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

void init_extract_data(char *cntrl_payload)
{
	uint16_t t_router_count;
	memcpy(&t_router_count, cntrl_payload, sizeof(t_router_count));
	routerCount = ntohs(t_router_count);

	uint16_t t_time_interval;
	memcpy(&t_time_interval, cntrl_payload + 2, sizeof(t_time_interval));
	updateInterval = ntohs(t_time_interval);

	int payloadoffset = 4; //skipping initial 4 bytes
	int i;
	for (i = 0; i < routerCount; i++) {
		struct INIT_PAYLOAD *payload = (struct INIT_PAYLOAD *) (cntrl_payload
				+ payloadoffset);

		/* Insert into list of active control connections */
		struct MAIN_INFO* node = malloc(sizeof(struct MAIN_INFO));
		node->router_id = ntohs(payload->router_id);
		node->router_port = ntohs(payload->router_port);
		node->data_port = ntohs(payload->data_port);
		node->cost = ntohs(payload->cost);
		node->router_ip_32bit = ntohl(payload->router_ip_addr);
		print_ip(node->router_ip_32bit, node->router_ip_addr);

		node->timer_fd_value = -1;
		
		if (node->cost == INFINITY) {
			node->nexthop_id = INFINITY;
			node->neighbour = FALSE;
		} 
		else if(node->cost == 0)
		{
			node->nexthop_id = node->router_id;
			node->neighbour = FALSE;
		}
		else {
			node->nexthop_id = node->router_id;
			node->neighbour = TRUE;
		}
		
		struct MAIN_INFO* temp;
		/*Insert into Linked List in sorted order of router Id */
		if (LIST_EMPTY(&MAIN_LIST)){
			LIST_INSERT_HEAD(&MAIN_LIST, node, next);
		}
		else {
			LIST_FOREACH(temp, &MAIN_LIST, next) {
				//insert in right location
				if (temp->router_id > node->router_id) {
					LIST_INSERT_BEFORE(temp, node, next);
					break;
				} 
				else {
					if (LIST_NEXT(temp, next) == NULL ) {
						LIST_INSERT_AFTER(temp, node, next);
						break;
					}
				}
			}
		}

		//self node - set all local parameters
		if (node->cost == 0) {
			myRouterID = node->router_id;
			myIp32bit = node->router_ip_32bit;
			strcpy(myIp, node->router_ip_addr);
			myRouterPort = node->router_port;
			myDataPort = node->data_port;
		}

		payloadoffset += 12;
	}

	/* Create UDP and TCP Sockets with new ports */
	create_required_sockets();

	/* Fill in router index value */
	assign_router_index_in_mainlist();

	/* Update my Routing Index */
	set_my_routing_index();

	/* Fill cost in global array */
	fill_global_cost_array();

	/* setup initial cost through Init */
	Fill_distance_matrix(myRouterIndex, link_cost_array);

	/* update dv matrix */
	update_distance_matrix();
	
}



void Init_response(int sock_index)
{
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = 0;//sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
	cntrl_response_payload = (char *) malloc(payload_len);
	memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

	cntrl_response_header = create_response_header(sock_index, 1, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
	
	
	printf("\nINIT SUCCESSFULLY COMPLETED ----------- MATRIX UPDATED | LIST FILLED\n");
	
	struct MAIN_INFO* temp;
		LIST_FOREACH(temp, &MAIN_LIST, next)
		{
			printf(
					"routerid %d, routerindex %d, cost %d, next hop %d, dataport %d, ipaddress %s \n",
					temp->router_id, temp->router_index,
					temp->cost, temp->nexthop_id,
					temp->data_port, temp->router_ip_addr);
		}

	print_distance_vector_matrix();
	printf("---------------------------------------------------------------\n");	
	
	
}
