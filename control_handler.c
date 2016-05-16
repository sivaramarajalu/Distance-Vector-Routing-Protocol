/**
 * @control_handler
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
 * Handler for the control plane.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/queue.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <netdb.h>


#include <sys/timerfd.h>
#include <time.h>

#include "../include/global.h"
#include "../include/network_util.h"
#include "../include/control_header_lib.h"
#include "../include/author.h"



/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
};

struct ControlConn *connection, *conn_temp;

LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;


/* code taken from man page - starts a timer fd and returns it to user*/
int create_timer_fd(uint16_t timer_interval)
{
	int fd;
	struct itimerspec new_value;
	
	fd = timerfd_create(CLOCK_REALTIME, 0);
	
	new_value.it_value.tv_sec = timer_interval;
	new_value.it_interval.tv_sec = timer_interval;
	new_value.it_value.tv_nsec=0;
	new_value.it_interval.tv_nsec =0;
	
	if (timerfd_settime(fd, 0, &new_value, NULL) == -1)
	{
	 perror("timerfd_settime");
	}

	printf("timer intervalllllllllllllllllllllllllllll Created FD IS  %d %u \n",fd,timer_interval);
	
	return fd;
}


void update_my_timer_in_list(int timerFd)
{
	struct MAIN_INFO* temp;

	LIST_FOREACH(temp, &MAIN_LIST, next) {
		 if(temp->router_index == myRouterIndex)
			 temp->timer_fd_value = timerFd;
	}
}

int create_tcp_socket(uint16_t port)
{
	int sock;
	struct sockaddr_in server_addr;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		ERROR("socket() failed");

	/* Make socket re-usable */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[] ) { 1 }, sizeof(int))
			< 0)
		ERROR("setsockopt() failed");

	bzero(&server_addr, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY );
	server_addr.sin_port = htons(port);

	if (bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
		ERROR("TCP  bind() failed");

	if (listen(sock, 10) < 0)
		ERROR("listen() failed");

	return sock;
}

int create_control_sock()
{
	int sock = create_tcp_socket(myControlPort);
   	LIST_INIT(&control_conn_list);
    return sock;
}

int create_router_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(myRouterPort);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("UDP bind() failed");

    return sock;
}


int new_control_conn(int sock_index)
{
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection = malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}


// FIXME  uncomment the for loop
void remove_control_conn(int sock_index)
{
//	struct ControlConn* temp;

	LIST_FOREACH(connection, &control_conn_list, next)
	{
		if (connection->sockfd == sock_index)
			LIST_REMOVE(connection, next);
		// this may be unsafe?
		free(connection);
	}

	close(sock_index);
}


/* function to receive DV updates */
void udp_recv_hook(int sock_index)
{
	struct sockaddr_in control_addr;
	socklen_t addr_size;
	
	addr_size = sizeof control_addr;
	
	char *cntrl_payload;
	
	/*8 bytes of header + (number of routers* 12 bytes) */
	int UDP_PAYLOAD_LENGTH = 8 + (routerCount*12);
		     	
	cntrl_payload = (char *) malloc(UDP_PAYLOAD_LENGTH);
    
	int received_bytes = recvfrom(sock_index,cntrl_payload,UDP_PAYLOAD_LENGTH,0,(struct sockaddr *)&control_addr, 
    		&addr_size);
	
	if(received_bytes <= 0) {
	   perror("UDP RECEIVE FAILED: Control Handler");
	}	    
	
	printf("Message Received successfully of size %d!\n",received_bytes);

	udp_extract_data(cntrl_payload);
	
	free(cntrl_payload);    
}


void send_udp_update()
{	
	int UDP_PAYLOAD_LENGTH = 8 + (routerCount * 12);
	char cntrl_response[UDP_PAYLOAD_LENGTH];
	
	printf("----------UDP SEND------------------\n");
	
	struct MAIN_INFO* temp;
	LIST_FOREACH(temp, &MAIN_LIST, next)
	{		
		printf("current Router ID in UDP send %d\n",temp->router_id);
		
		//if (temp->router_id == myRouterID || temp->cost == INFINITY)
		if(!temp->neighbour) {	
		continue; }
	
		memset(cntrl_response, 0, UDP_PAYLOAD_LENGTH);
			
		int sock;
		struct sockaddr_in control_addr;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0)
			ERROR("socket() failed");

		bzero(&control_addr, sizeof(control_addr));

		control_addr.sin_family = AF_INET;
		control_addr.sin_port = htons(temp->router_port);
		inet_pton(AF_INET, temp->router_ip_addr,
				&(control_addr.sin_addr));

		printf("Used Ip address and port %s  %d\n",
				temp->router_ip_addr, temp->router_port);

		create_udp_response_payload(cntrl_response);
		   
		if (sendto(sock, cntrl_response, UDP_PAYLOAD_LENGTH, 0,
				(struct sockaddr *) &control_addr, sizeof control_addr) <= 0) {
			perror("UDP SEND FAILED: Control Handler");
		}
		
		close(sock);
	}
	
	printf("--------Message sending success---------!\n");
	
	
}


bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}


bool isTimer(int sock_index)
{
	
	struct MAIN_INFO* temp;
	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		printf("Comparing Timer fd value in List and myTimerFD Value %d  %d\n",temp->timer_fd_value,myTimerFd);
		
		if(temp->timer_fd_value == sock_index)
			return TRUE;
	}
	
    return FALSE;
}


bool control_recv_hook(int sock_index)
{
	char *cntrl_header, *cntrl_payload;
	uint8_t control_code;
	uint16_t payload_len;

	/* Get control header */
	cntrl_header = (char *) malloc(sizeof(char) * CNTRL_HEADER_SIZE);
	bzero(cntrl_header, CNTRL_HEADER_SIZE);

	if (recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0) {
		remove_control_conn(sock_index);
		free(cntrl_header);
		return FALSE;
	}

	struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
	control_code = header->control_code;
	payload_len = ntohs(header->payload_len);

	free(cntrl_header);

	/* Get control payload */
	if (payload_len != 0) {
		cntrl_payload = (char *) malloc(sizeof(char) * payload_len);
		bzero(cntrl_payload, payload_len);

		if (recvALL(sock_index, cntrl_payload, payload_len) < 0) {
			remove_control_conn(sock_index);
			free(cntrl_payload);
			return FALSE;
		}
	}

	/* Triage on control_code */
	switch (control_code) {
	case 0: {
		author_response(sock_index);
		break;
	}

	case 1: {
		
		/* Init operations */
		init_extract_data(cntrl_payload);
		Init_response(sock_index);
		
		/*My Timer Fd operations */
		myTimerFd = create_timer_fd(updateInterval);
		update_my_timer_in_list(myTimerFd);
		
		set_TimerFd_in_MasterList(myTimerFd);
		
		/*Send initial Udp updates*/
		send_udp_update(); 
		break;
	}

	case 2: {
		routing_table_response(sock_index);
		break;
	}
	case 3: {
		update_link(cntrl_payload);
		update_link_response(sock_index);
		break;
	}

	case 4: {
		crash_response(sock_index);
		exit(0);
		break;
	}

		/* .......
		 case 1: init_response(sock_index, cntrl_payload);
		 break;

		 .........
		 ....... 
		 ......*/
	}

	if (payload_len != 0)
		free(cntrl_payload);
	return TRUE;
}
