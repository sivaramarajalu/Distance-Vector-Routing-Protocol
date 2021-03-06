#ifndef CONTROL_HANDLER_LIB_H_
#define CONTROL_HANDLER_LIB_H_

#define CNTRL_HEADER_SIZE 8
#define CNTRL_RESP_HEADER_SIZE 8

#define PACKET_USING_STRUCT // Comment this out to use alternate packet crafting technique

#ifdef PACKET_USING_STRUCT
    struct __attribute__((__packed__)) CONTROL_HEADER
    {
        uint32_t dest_ip_addr;
        uint8_t control_code;
        uint8_t response_time;
        uint16_t payload_len;
    };

    struct __attribute__((__packed__)) CONTROL_RESPONSE_HEADER
    {
        uint32_t controller_ip_addr;
        uint8_t control_code;
        uint8_t response_code;
        uint16_t payload_len;
    };

    struct __attribute__((__packed__)) CONTROL_RESPONSE_ROUTING_TABLE
    {
            uint16_t router_id;
            uint16_t padding;
            uint16_t nexthop_id;
            uint16_t cost;
    };

    struct __attribute__((__packed__)) INIT_PAYLOAD
    {
            uint16_t router_id;
            uint16_t router_port;
            uint16_t data_port;
            uint16_t cost;
            uint32_t router_ip_addr;
    };
    
    
    struct __attribute__((__packed__)) UDP_PAYLOAD
        {
    			uint32_t router_ip_addr;
    			uint16_t router_port;
                uint16_t padding;
                uint16_t router_id;
                uint16_t cost;
        };
    

#endif

char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len);

#endif
