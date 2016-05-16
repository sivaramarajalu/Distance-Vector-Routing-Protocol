#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/queue.h>

typedef enum {FALSE, TRUE} bool;

#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

/* https://scaryreasoner.wordpress.com/2009/02/28/checking-sizeof-at-compile-time/ */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)])) // Interesting stuff to read if you are interested to know how this works

#define INFINITY UINT16_MAX

/*Linked List for complete user information */
struct MAIN_INFO {
	int router_index;
	
	// All the fields below correspond to host format
	
	uint16_t router_id;
	uint16_t router_port;
	uint16_t data_port;
	
	// Routing table information
	uint16_t nexthop_id;
	uint16_t cost; // minimum cost to the router

	uint32_t router_ip_32bit;
	char router_ip_addr[16];
	
	int timer_fd_value;
	bool neighbour;

	LIST_ENTRY(MAIN_INFO) next;
};

// List sorted based on Router ID
LIST_HEAD(MAIN_LIST_HEAD, MAIN_INFO) MAIN_LIST;

uint16_t distance_vector_matrix[5][5];
uint16_t link_cost_array[5];

uint16_t routerCount;
uint16_t myRouterID;
int myRouterIndex;
int myTimerFd;
uint16_t updateInterval;

uint32_t myIp32bit;
char myIp[16];

uint16_t myControlPort;
uint16_t myRouterPort;
uint16_t myDataPort;

#endif
