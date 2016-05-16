/*
 * udp_response.c
 *
 *  Created on: May 4, 2016
 *      Author: sivaram
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timerfd.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"


void packi16(unsigned char *buf, unsigned int i)
{
    *buf++ = i>>8; *buf++ = i;
}

/*
** packi32() -- store a 32-bit int into a char buffer (like htonl())
*/ 
void packi32(unsigned char *buf, unsigned long int i)
{
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8;  *buf++ = i;
}


void create_udp_response_payload(char* buffer)
{
    int UDP_PAYLOAD_LENGTH = 8 + (routerCount*12);

    struct UDP_PAYLOAD *udp_resp_payload;
 
     //required declarations for payload 
    int payloadoffset =8;
    char tempbuffer[12];

     //copy intial fields 
    packi16(buffer,htons(routerCount));
    packi16(buffer+2,htons(myRouterPort));
    packi32(buffer+4,htonl(myIp32bit)); //checkkkkkkkkkkk IP
    
    struct MAIN_INFO* mytemp1;
	LIST_FOREACH(mytemp1, &MAIN_LIST, next)
	{
		memset(tempbuffer, 0, 12);
		udp_resp_payload = (struct UDP_PAYLOAD *) tempbuffer;

		udp_resp_payload->router_ip_addr = htonl(mytemp1->router_ip_32bit);
		udp_resp_payload->router_port = htons(mytemp1->router_port);
		udp_resp_payload->padding = 0;
		udp_resp_payload->router_id = htons(mytemp1->router_id); 
		udp_resp_payload->cost = htons(mytemp1->cost);
		
		printf("router id sent to other side with cost %u   %u  \n",
				mytemp1->router_id,mytemp1->cost);

		memcpy(buffer+payloadoffset, tempbuffer, 12);
		
		payloadoffset += 12;
	}
}



void udp_extract_data(char *cntrl_payload)
{
	uint16_t costArray[5] = {0};
	uint16_t sender_routerid;
	uint16_t sender_port;
	int sender_routerindex;
	
	uint32_t sender_ip;
	char sender_ip_char[16];
	

	int payloadoffset =8; //skipping initial 8 bytes
	
	memcpy(&sender_port, cntrl_payload+2, sizeof(sender_ip));
	memcpy(&sender_ip, cntrl_payload+4, sizeof(sender_ip));
	
	printf("\nExtracting UDP Data... \n");
	print_ip(sender_ip, sender_ip_char);
		
	struct MAIN_INFO* mytemp;	
	LIST_FOREACH(mytemp, &MAIN_LIST, next)
	{
		printf("IP address comparison %s %s  and router port is %u received Rout port %u\n",sender_ip_char, mytemp->router_ip_addr, 
																		mytemp->router_port,sender_port);
		if(!strcmp(mytemp->router_ip_addr,sender_ip_char) && (mytemp->router_port == sender_port))
		 sender_routerid = mytemp->router_id;	
	}
	
	int i;
	for (i = 0; i < routerCount; i++) {
		struct UDP_PAYLOAD *payload = (struct UDP_PAYLOAD *) (cntrl_payload
				+ payloadoffset);
	
		costArray[i] = ntohs(payload->cost);
		payloadoffset += 12;
	}
		
	printf("Router ID is %d\n", sender_routerid);
	
	/* Find correct Index */
	sender_routerindex = find_Router_index(sender_routerid); 
	printf("Router Index is %d\n", sender_routerindex);
	
	for(i =0; i < routerCount;i++)
	{
			printf("receieved cost %u\n", costArray[i]);
	}
	
	/* Reset timer for this router */
	int timerfd;
	struct MAIN_INFO* mytempiterator;	
	LIST_FOREACH(mytempiterator, &MAIN_LIST, next)
	{
		if(mytempiterator->router_index == sender_routerindex)
			timerfd = mytempiterator->timer_fd_value;
	}
	
	printf("TRYING RESET TIMER FOR RECEIVED SENDER  ID : %u  TIMER ID : %d\n",sender_routerid,timerfd);
	
	/* receiving for the first time */
	if(timerfd == -1)
	{
		int newfd;
		newfd = create_timer_fd(updateInterval*3);
		
		struct MAIN_INFO* mytemp1;	
		LIST_FOREACH(mytemp1, &MAIN_LIST, next)
		{
			if (mytemp1->router_index == sender_routerindex)
				mytemp1->timer_fd_value = newfd;
		}
		
		set_TimerFd_in_MasterList(newfd);
	}
	
	else {
		struct itimerspec new_value1;
		
		timerfd_gettime(timerfd,&new_value1);
		
		new_value1.it_value.tv_sec = updateInterval*3;
		new_value1.it_interval.tv_sec = 0;  //INTERVAL TIME CAN BE SET TO 0. EXPIRES ONCE.
		new_value1.it_value.tv_nsec = 0;
		new_value1.it_interval.tv_nsec = 0;
		
		if (timerfd_settime(timerfd, 0, &new_value1, NULL) == -1)
		{
			 perror("timerfd_settime IN UDP RECEIVE");
		}
		
		printf("_____________________________timer second occurance : time is %d updateinterval %u\n ",new_value1.it_value.tv_sec,updateInterval);
	}
	
	/* setup initial cost through Init */
	if(sender_routerindex != -1)
	Fill_distance_matrix(sender_routerindex, costArray);
	
	printf("\nfilled distance matrix \n");
	print_distance_vector_matrix();
	
	update_distance_matrix();
	
	printf("\nupdated distance matrix \n");
	print_distance_vector_matrix();
	
	
	struct MAIN_INFO* temp;			
	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		printf(
				"routerid %d, routerindex %d, cost %d, next hop %d, dataport %d, ipaddress %s \n",
				temp->router_id, temp->router_index, temp->cost,
				temp->nexthop_id, temp->data_port, temp->router_ip_addr);
	}
	
	
}
