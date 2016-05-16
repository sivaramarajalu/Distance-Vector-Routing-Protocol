/*
 * distance_vector.c
 *
 *  Created on: May 2, 2016
 *      Author: sivaram
 */

#include <string.h>

#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include "../include/connection_manager.h"
#include "../include/control_handler.h"

int find_Router_index(uint16_t RouterId) {
	struct MAIN_INFO* temp;

	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		if (temp->router_id == RouterId)
			return temp->router_index;
	}

	return -1;
}

uint16_t find_Router_Id(int RouterIndex) {
	if (RouterIndex == INFINITY) //call from update_main_list, sometimes have a value fo infinity
		return INFINITY;

	struct MAIN_INFO* temp;

	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		if (temp->router_index == RouterIndex)
			return temp->router_id;
	}

	return -1;
}


void print_distance_vector_matrix() {
	int i, j;

	printf("\t");
	int counter;
	for (counter = 0; counter < routerCount; counter++)
		printf("%d\t\t", find_Router_Id(counter));

	printf("\n");

	for (i = 0; i < routerCount; i++) {
		printf("%d\t", find_Router_Id(i));
		for (j = 0; j < routerCount; j++) {
			printf("%-15d", distance_vector_matrix[i][j]);
			if ((routerCount - 1) == j)
				printf("\n");
		}
	}
	printf("\n");
}

void set_my_routing_index() {
	struct MAIN_INFO* temp;
	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		if (temp->router_id == myRouterID)
			myRouterIndex = temp->router_index;
	}
}

void assign_router_index_in_mainlist() {
	int counter = 0;
	struct MAIN_INFO* temp;
	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		temp->router_index = counter;
		counter++;
	}
}

void fill_global_cost_array() {
	int counter = 0;
	struct MAIN_INFO* temp;
	LIST_FOREACH(temp, &MAIN_LIST, next)
	{
		link_cost_array[counter] = temp->cost;
		counter++;
	}
}

/* Generic for all received distance vector matrices*/
void Fill_distance_matrix(int routerindex, uint16_t cost[]) {
	int i;
	for (i = 0; i < routerCount; i++)
		distance_vector_matrix[routerindex][i] = cost[i];
}


void update_distance_matrix() {
	int entry = 0;
	int neighbour;
	
	struct MAIN_INFO* listtemp;

	LIST_FOREACH(listtemp,&MAIN_LIST,next)
	{	
		uint16_t minimum = INFINITY;
		int nexthop = INFINITY;

		if (entry == myRouterIndex)
		{
			entry++;
			continue;
		}
		
		for (neighbour = 0; neighbour < routerCount; neighbour++) {
			
			if (neighbour == myRouterIndex)
				continue;

			if (link_cost_array[entry] < minimum) {
				minimum = link_cost_array[entry];
				nexthop = entry; //gives index of nexthop
			}

			uint16_t tempcost;

			if (link_cost_array[neighbour] == INFINITY
					|| distance_vector_matrix[neighbour][entry] == INFINITY)
				tempcost = INFINITY;
			else
			{
				tempcost = link_cost_array[neighbour]
						+ distance_vector_matrix[neighbour][entry];
				
				//Handling Uint_16 wrap case
				if(tempcost < link_cost_array[neighbour] || tempcost < distance_vector_matrix[neighbour][entry])
					tempcost = INFINITY;
			}
			if (tempcost < minimum) {
				minimum = tempcost;
				nexthop = neighbour; //gives index of nexthop 	
			}
			
		}

		distance_vector_matrix[myRouterIndex][entry] = minimum;
		listtemp->nexthop_id = find_Router_Id(nexthop);
		listtemp->cost = minimum;

		entry++;
		
		printf("DV FINAL  Cost %u nexthop ID %u\n", listtemp->cost, listtemp->nexthop_id);
	
	}
	
}

