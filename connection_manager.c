/**
 * @connection_manager
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
 * Connection Manager listens for incoming connections/messages from the
 * controller and other routers and calls the desginated handlers.
 */

#include <sys/select.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"

fd_set master_list, watch_list;
int head_fd;

void create_required_sockets() {
	//router_socket and data_socket creation
	router_socket = create_router_sock();
	FD_SET(router_socket, &master_list);
	head_fd = router_socket;

	data_socket = create_tcp_socket(myDataPort);
	FD_SET(data_socket, &master_list);
	head_fd = data_socket;
}


void set_TimerFd_in_MasterList(int myTimerFd)
{
	FD_SET(myTimerFd, &master_list);
	if(head_fd< myTimerFd)
	{
	head_fd = myTimerFd;	
	}
}


/* find crashed router index */
int find_crashed_router_index(int sock_index)
{
	int router_index = -1;
	
	struct MAIN_INFO* temp;

	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		if (temp->timer_fd_value == sock_index)
			router_index = temp->router_index;
	}
		
	return router_index;	
}

/* update crashed router information */
void update_crashed_router_details(int crashed_router_index)
{
	struct MAIN_INFO* temp;

	link_cost_array[crashed_router_index] = INFINITY;

	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		if (temp->router_index == crashed_router_index) {
			temp->neighbour = FALSE;
		}
	}
}



void main_loop() {
	int selret, sock_index, fdaccept;

	while (TRUE) {
		watch_list = master_list;
		selret = select(head_fd + 1, &watch_list, NULL, NULL, NULL );

		if (selret < 0)
			ERROR("select failed.");

		/* Loop through file descriptors to check which ones are ready */
		for (sock_index = 0; sock_index <= head_fd; sock_index += 1) {

			if (FD_ISSET(sock_index, &watch_list)) {

				/* control_socket */
				if (sock_index == control_socket) {
					fdaccept = new_control_conn(sock_index);

					/* Add to watched socket list */
					FD_SET(fdaccept, &master_list);
					if (fdaccept > head_fd)
						head_fd = fdaccept;
				}
				
				else if(isTimer(sock_index))
				{
					printf("------------------------------------------TIMER FD RECEIVED IN SELECT------------------------------------------------\n");
				
					
					uint64_t readbytes, bytes;
					readbytes = read(sock_index,&bytes, sizeof(bytes));
					
					printf("READ BYTESSSSSSSSSSSSSSSSSSSSSS %d\n  SOCK FD %d ", readbytes,sock_index);
					
					//Send Udp Updates to all neighbours
					if (sock_index == myTimerFd) {
						printf("TIME TO SEND UPDATES - FD matches my FD\n");
						send_udp_update();
					}
					
					// find the black sheep - crashed router 
					else {
					
						printf("SOMEONE CRASHED - FIND THE NODE\n");
						int crashed_router_index;
						crashed_router_index = find_crashed_router_index(sock_index);
						
						printf("crashed router Index %d \n",crashed_router_index);
						
						update_crashed_router_details(crashed_router_index);//make neighbour false, update link cost array
						update_distance_matrix(); //update matrix
					}
					printf("------------------------------------------TIMER FD JOB IN SELECT COMPLETED--------------------------------------\n");
	
				}

				/* router_socket */
				else if (sock_index == router_socket) {
					//call handler that will call recvfrom() .....
					
					printf("----------------------------------------UDP RECEIVE HOOK------------------------------------------------------\n");
					udp_recv_hook(sock_index);

					printf("----------------------------------------UDP RECEIVE HOOK COMPLETED------------------------------------------------------\n");
					
				}

				/* data_socket */
				else if (sock_index == data_socket) {
					//new_data_conn(sock_index);
				}

				/* Existing connection */
				else {
					if (isControl(sock_index)) {
						if (!control_recv_hook(sock_index))
							FD_CLR(sock_index, &master_list);
						//printf("message received from controller\n");
					}
					//else if isData(sock_index);
					else
						perror("Unknown socket index");
				}
			}
		}
	}
}

void init() {
	control_socket = create_control_sock();

	//router_socket and data_socket will be initialized after INIT from controller

	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);

	/* Register the control socket */
	FD_SET(control_socket, &master_list);
	head_fd = control_socket;

	/* Initializing distance vector matrix*/
	int i, j;
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			distance_vector_matrix[i][j] = INFINITY;
		}
	}

	/* Initializing our main list*/
	LIST_INIT(&MAIN_LIST);

	main_loop();
}

