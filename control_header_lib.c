/**
    *  @control_header_lib
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
 * Routines to parse/generated control/control-response headers.
 */

#include <string.h>
#include <netinet/in.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"

char* create_router_table_response_payload()
{
	char *buffer, *temp;

	struct CONTROL_RESPONSE_ROUTING_TABLE *cntrl_resp_payload;

	int payloadoffset = 0;
	buffer = (char *) malloc(sizeof(char) * (routerCount * 8)); //payload of routing table conists 8 bytes of data for one router
	temp = (char *) malloc(sizeof(char) * 8);

	struct MAIN_INFO* MAIN_INFO_NODE;
	LIST_FOREACH(MAIN_INFO_NODE, &MAIN_LIST, next)
	{

		cntrl_resp_payload = (struct CONTROL_RESPONSE_ROUTING_TABLE *) temp;

		cntrl_resp_payload->router_id = htons(MAIN_INFO_NODE->router_id);
		cntrl_resp_payload->padding = 0;
		cntrl_resp_payload->nexthop_id = htons(MAIN_INFO_NODE->nexthop_id);
		cntrl_resp_payload->cost = htons(MAIN_INFO_NODE->cost); //NEED TO CHANGE THIS VALUE

		memcpy(buffer + payloadoffset, temp, 8);

		payloadoffset += 8;

		printf(
				"Routing table sent: router_id, %u,  nexthop_id, %u   cost  %u \n",
				MAIN_INFO_NODE->router_id, 
				MAIN_INFO_NODE->nexthop_id,
				MAIN_INFO_NODE->cost);

	}

	return buffer;

}




/* creating payload for controller */
char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len)
{
    char *buffer;

    #ifdef PACKET_USING_STRUCT
        /** ASSERT(sizeof(struct CONTROL_RESPONSE_HEADER) == 8)
          * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
          * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
          */
       BUILD_BUG_ON(sizeof(struct CONTROL_RESPONSE_HEADER) != CNTRL_RESP_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.

       struct CONTROL_RESPONSE_HEADER *cntrl_resp_header;
    #endif

    struct sockaddr_in addr;
    socklen_t addr_size;

    buffer = (char *) malloc(sizeof(char)*CNTRL_RESP_HEADER_SIZE);

    #ifdef PACKET_USING_STRUCT
        cntrl_resp_header = (struct CONTROL_RESPONSE_HEADER *) buffer;
    #endif

      addr_size = sizeof(struct sockaddr_in);
    getpeername(sock_index, (struct sockaddr *)&addr, &addr_size);

    #ifdef PACKET_USING_STRUCT
        /* Controller IP Address */
        memcpy(&(cntrl_resp_header->controller_ip_addr), &(addr.sin_addr), sizeof(struct in_addr));
        /* Control Code */
        cntrl_resp_header->control_code = control_code;
        /* Response Code */
        cntrl_resp_header->response_code = response_code;
        /* Payload Length */
        cntrl_resp_header->payload_len = htons(payload_len);
    #endif


    return buffer;
}



