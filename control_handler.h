#ifndef CONTROL_HANDLER_H_
#define CONTROL_HANDLER_H_

int create_control_sock();
int create_router_sock();
int new_control_conn(int sock_index);
bool isControl(int sock_index);
bool control_recv_hook(int sock_index);
void udp_recv_hook(int sock_index);
void udp_send_final_message(int sock_index, char *cntrl_payload);
void send_udp_update();


#endif
