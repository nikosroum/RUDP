////////////////////////////////////////////////////////////////////
//Struct used to send a packet
typedef struct 
{
        struct rudp_hdr header;
        char    data[RUDP_MAXPKTSIZE];
}__attribute__((packed)) rudp_rawpacket;
////////////////////////////////////////////////////////////////////
struct  rudp_packet_node{
    struct sockaddr_in to;
    int state;
    int retries;
    int data_len;
    int TimeoutDel;
    rudp_rawpacket rudppacket;
    struct rudp_packet_node *Next;
};
typedef struct rudp_packet_node rudp_packet_queuenode;
struct senders{
		int status;
		u_int32_t last_seq;
//		u_int32_t SYN_seqno;
		u_int32_t FIN_seq;
		int SYN_ACK_RECEIVED;
		
		int (*myrecvfrom_handler)(rudp_socket_t, struct sockaddr_in *, char *, int);
        int (*myevent_handler)(rudp_socket_t, rudp_event_t, struct sockaddr_in *);
        struct sockaddr_in to;
        
		struct senders *Next;
};
typedef struct senders senders_node;

struct receivers{
		int status;
		 
		u_int32_t last_seq;
		u_int32_t SYN_seqno;
		u_int32_t FIN_seq;
		int SYN_ACK_RECEIVED;
		int FIN_ACK_RECEIVED;
		int (*myrecvfrom_handler)(rudp_socket_t, struct sockaddr_in *, char *, int);
        int (*myevent_handler)(rudp_socket_t, rudp_event_t, struct sockaddr_in *);
        struct sockaddr_in to;
        rudp_packet_queuenode *SYNpacket;   
		rudp_packet_queuenode *PacketQueue;        
		rudp_packet_queuenode *SlidingWindow;   
		struct receivers *Next;
};
typedef struct receivers receivers_node;
/////////////////////////////////////////////////////////////////////
struct rudp_socket_type
{
        int sockfd;                         // socket file descriptor
        int port;
        u_int32_t last_seq;   //seq_no of last packet received
        /*Each socket has to assign its own handlers*/
        int (*myrecvfrom_handler)(rudp_socket_t, struct sockaddr_in *, char *, int);
        int (*myevent_handler)(rudp_socket_t, rudp_event_t, struct sockaddr_in *);
        struct sockaddr_in saddr;
		
		receivers_node *Receivers;
		senders_node *Senders;
		 
        struct rudp_socket_type *Next;
};
typedef struct rudp_socket_type rudp_socket_node;
//list of sockets for multiple sockets
rudp_socket_node *SockList = NULL;
///////////////////////////////////////////////////////////////////

