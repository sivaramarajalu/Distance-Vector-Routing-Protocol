/*
 * crash_response.c
 *
 *  Created on: May 4, 2016
 *      Author: sivaram
 */

#include <string.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"


void crash_response(int sock_index)
{
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = 0; // one router takes 8 bytes of payload data

	cntrl_response_header = (char *) malloc(CNTRL_RESP_HEADER_SIZE);
	cntrl_response_payload = (char *) malloc(payload_len);


	//cntrl_response_payload = create_router_table_response_payload();

	cntrl_response_header = create_response_header(sock_index, 4, 0, payload_len);

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
}


