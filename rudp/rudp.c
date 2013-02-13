#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "event.h"
#include "rudp.h"
#include "rudp_api.h"
#include "structs.h"

#define SENT 1
#define NOTSENT 0
#define DROP 2
#define EVENT 1
#define SYN_RECEIVED 1
#define FINISHED 2

int rudp_receive_data(int fd, void *arg);
/***************************************************/
/**Socket List functions					       *
***************************************************/

void printSockList(){
	rudp_socket_node *tmp;
	tmp = SockList;
	
	printf("Print SockList\n"); 
	
	while (tmp!=NULL) {
		printf("sd=%d\t l_port=%d\n",tmp->sockfd,tmp->port);
		
		if (tmp->Next == NULL) break;
		tmp = tmp->Next;
	}
        return;

}
 /**addtoList****************************************
 * Adds a socket to the SockList and Initializes it*/
rudp_socket_node * addtoList(rudp_socket_node *tmp)  
{  
	
        rudp_socket_node *node = NULL;

        if (SockList==NULL){
		SockList = malloc(sizeof(rudp_socket_node));
		node = SockList;
		}
		else {
			node = SockList;
			while (node->Next!=NULL) {
				node = node->Next;
			}
			node->Next=malloc(sizeof(rudp_socket_node));
			node = node->Next;
		}
        
        node->sockfd = tmp->sockfd;
        node->port = tmp->port;
        node->saddr = tmp->saddr;
        node->last_seq = 0;
        node->Receivers = NULL;
        node->Senders = NULL;
        node->myrecvfrom_handler = tmp->myrecvfrom_handler;
        node->myevent_handler = tmp->myevent_handler;
        node->Next=NULL;
        
        return node;
}

/***************************************************/
rudp_socket_node *searchSockList(rudp_socket_t rudp_socket,struct sockaddr_in to) 
{		//printf("Search for previous socket session\n");
        rudp_socket_node *cur = SockList;
        rudp_socket_node *arg = (rudp_socket_node *) rudp_socket;//
        while(cur != NULL)
        {	
                if(cur->sockfd == arg->sockfd && cur->port == arg->port)
                {		receivers_node *receiver = cur->Receivers;
						while(receiver!=NULL){
							if(ntohs(to.sin_port) == ntohs(receiver->to.sin_port) && !strcmp(inet_ntoa(to.sin_addr),inet_ntoa(receiver->to.sin_addr))){
								return cur;}
						receiver = receiver->Next;
						}
						
                        
                }
                cur = cur->Next;
        }
        return NULL;
}


rudp_socket_node *searchSocket(rudp_socket_t rudp_socket) 
{		
        rudp_socket_node *cur = SockList;
        rudp_socket_node *arg = (rudp_socket_node *) rudp_socket;//
        while(cur != NULL)
        {	
                if(cur->sockfd == arg->sockfd && cur->port == arg->port )
                {		//printf("Found sd=%d\n",cur->sockfd);
                        return cur;
                }
                cur = cur->Next;
        }
        return NULL;
}

//////////////////////////////
void printPacketQueue(rudp_socket_node * socket){
	printf("--Print packets--\n");
	rudp_packet_queuenode *tmp;
	//tmp = socket->PacketQueue;
	while(tmp!=NULL){
		//printf("seq=%0x state=%d\n",tmp->rudppacket.header.seqno,tmp->state);
		tmp=tmp->Next;
	}
}
///////////////////
senders_node *addSender(rudp_socket_node *socket,struct sockaddr_in addr) {
		
						
	rudp_socket_node *tmpSockList;
	senders_node *tmpSenders;
	int flag;
	tmpSockList = SockList;
	
	flag = 0;
	while (tmpSockList!=NULL) {
		if (tmpSockList->sockfd == socket->sockfd && (tmpSockList->port == socket->port) ){
			flag = 1;
			break;
		}
		else if (tmpSockList->Next == NULL) {
			printf("Sockfd not found\n");
			break;
		}
		else tmpSockList = tmpSockList->Next;
	}
	//if socket found
	if (flag){

	//for the first node
	if (tmpSockList->Senders==NULL){
		tmpSockList->Senders = malloc(sizeof(senders_node));
		tmpSockList->Senders->Next = NULL;
		tmpSenders = tmpSockList->Senders;
		}
	
	else{tmpSenders = tmpSockList->Senders;
	while (tmpSenders!=NULL) {
		if(tmpSenders->Next==NULL){break;}
		tmpSenders = tmpSenders->Next;
	}
	
	
	tmpSenders->Next = malloc(sizeof(senders_node));
	tmpSenders = tmpSenders->Next;
	}
	
	tmpSenders->status = SYN_RECEIVED;
	tmpSenders->to = addr;
	
	tmpSenders->Next = NULL;
	
	
	}
      
       //printf("Sender Added in sd=%d with port=%d\n",socket->sockfd,ntohs(tmpSenders->to.sin_port));
       return tmpSenders;

}
////////////////////////////
receivers_node *addReceiver(rudp_socket_node *socket,struct sockaddr_in addr) {
		
						
	rudp_socket_node *tmpSockList;
	receivers_node *tmpReceivers;
	int flag;
	tmpSockList = SockList;
	
	flag = 0;
	while (tmpSockList!=NULL) {
		if (tmpSockList->sockfd == socket->sockfd && (tmpSockList->port == socket->port) ){
			flag = 1;
			break;
		}
		else if (tmpSockList->Next == NULL) {
			printf("Sockfd not found\n");
			break;
		}
		else tmpSockList = tmpSockList->Next;
	}
	//if socket found
	if (flag){

	//for the first node
	if (tmpSockList->Receivers==NULL){
		tmpSockList->Receivers = malloc(sizeof(receivers_node));
		tmpSockList->Receivers->Next = NULL;
		tmpReceivers = tmpSockList->Receivers;
		}
	
	else{tmpReceivers = tmpSockList->Receivers;
	while (tmpReceivers!=NULL) {
		if(tmpReceivers->Next==NULL){break;}
		tmpReceivers = tmpReceivers->Next;
	}
	
	
	tmpReceivers->Next = malloc(sizeof(receivers_node));
	tmpReceivers = tmpReceivers->Next;
	}

        tmpReceivers->last_seq = 0;
        tmpReceivers->SYN_ACK_RECEIVED = 0;
        tmpReceivers->FIN_ACK_RECEIVED = 0;
        tmpReceivers->SYN_seqno = 0;
        tmpReceivers->FIN_seq = 0;
        tmpReceivers->SYNpacket = NULL;
        tmpReceivers->SlidingWindow = NULL;
        tmpReceivers->PacketQueue = NULL;
		tmpReceivers->to = addr;
		tmpReceivers->Next = NULL;
	
	
	}
      
       //printf("Receiver Added in sd=%d with port=%d\n",socket->sockfd,ntohs(tmpReceivers->to.sin_port));
       return tmpReceivers;

}
////////////////////////////
receivers_node *searchReceiver(rudp_socket_node *sock,struct sockaddr_in addr) {
		receivers_node *tmpReceivers;
        tmpReceivers = sock->Receivers;
		while(tmpReceivers!=NULL){
			if(ntohs(tmpReceivers->to.sin_port) == ntohs(addr.sin_port) && !strcmp(inet_ntoa(tmpReceivers->to.sin_addr),inet_ntoa(addr.sin_addr)))
								return tmpReceivers;
			tmpReceivers = tmpReceivers->Next;
		}
                        
        return NULL;
}
///////////////////////////////


senders_node *searchSender(rudp_socket_node *sock,struct sockaddr_in addr) {
		senders_node *tmpSenders;
        tmpSenders = sock->Senders;
		while(tmpSenders!=NULL){
			if(ntohs(tmpSenders->to.sin_port) == ntohs(addr.sin_port) && !strcmp(inet_ntoa(tmpSenders->to.sin_addr),inet_ntoa(addr.sin_addr)))
								return tmpSenders;
			tmpSenders = tmpSenders->Next;
		}
                        
                
        
        return NULL;
}

////////////////////////////////////////////

rudp_packet_queuenode * addtoPacketQueue(receivers_node * receiver,int data_len,rudp_rawpacket rudppacket,struct sockaddr_in to)  
{  
	receivers_node *tmpReceiverList;
	rudp_packet_queuenode *tmpQueueNode;
	int flag;
	tmpReceiverList = receiver;
	
	flag = 0;
	while (tmpReceiverList!=NULL) {
		if (ntohs(to.sin_port) == ntohs(tmpReceiverList->to.sin_port) && !strcmp(inet_ntoa(tmpReceiverList->to.sin_addr),inet_ntoa(to.sin_addr))) {//if (tmpSockList->sockfd == socket->sockfd) {
			flag = 1;
			break;
		}
		else if (tmpReceiverList->Next == NULL) {
			printf("Receiver not found\n");
			break;
		}
		else tmpReceiverList = tmpReceiverList->Next;
	}
	//if socket found
	if (flag){

	//for the first node
	if (tmpReceiverList->PacketQueue==NULL){
		tmpReceiverList->PacketQueue = malloc(sizeof(rudp_packet_queuenode));
		tmpReceiverList->PacketQueue->Next = NULL;
		//set sliding window to point the start of PacketQueue
		tmpReceiverList->SlidingWindow = tmpReceiverList->PacketQueue;
		tmpQueueNode = tmpReceiverList->PacketQueue;
		}
	
	else{tmpQueueNode = tmpReceiverList->PacketQueue;
	while (tmpQueueNode!=NULL) {
		if(tmpQueueNode->Next==NULL){break;}
		tmpQueueNode = tmpQueueNode->Next;
	}
	
	
	tmpQueueNode->Next = malloc(sizeof(rudp_packet_queuenode));
	tmpQueueNode = tmpQueueNode->Next;
	}
	tmpQueueNode->to = to;
	tmpQueueNode->retries = 0;
	tmpQueueNode->state = NOTSENT;
	tmpQueueNode->data_len = data_len;
	tmpQueueNode->rudppacket = rudppacket;
	tmpQueueNode->TimeoutDel = 0;
	tmpQueueNode->Next = NULL;
	
	
	}
      
       // printf("Packet Added datalen=%d in sd:%d\n",tmpQueueNode->data_len,socket->sockfd);
       return tmpQueueNode;
}

/**searchPacketQueue**********************************
 * search and returns a packet from a PacketQueue                 */
/*****************************************************/
rudp_packet_queuenode * searchPacketQueue(receivers_node * receiver,u_int32_t seq)  
{  
 	rudp_packet_queuenode *tmpPacketQueue;
	tmpPacketQueue = receiver->PacketQueue;
	
	//printf("---Searchfor sockfd=%d \n",socket->sockfd);
	 
		while (tmpPacketQueue!=NULL) {
			if(seq == receiver->SYN_seqno)
				return tmpPacketQueue;
			if(tmpPacketQueue->rudppacket.header.seqno == seq){
				return tmpPacketQueue;
			}
			
			if (tmpPacketQueue->Next == NULL) break;
			
			tmpPacketQueue = tmpPacketQueue->Next;
		}
	
	return NULL;
}
////////////////////////////////////////////

/***********************************************************/
int retransmitSYN(int fd,void *arg){
		if(EVENT)event_timeout_delete(retransmitSYN,arg);
		rudp_packet_queuenode *SYNpacket = (rudp_packet_queuenode *)arg;
	
		struct timeval timer, t0, t1;           // timer variables
		// initialize timer variables
        timer.tv_sec = timer.tv_usec = 0;
        t0.tv_sec = t0.tv_usec = 0;
        t1.tv_sec = t1.tv_usec = 0;
		struct sockaddr_in to = SYNpacket->to;
		int len = SYNpacket->data_len;
		rudp_socket_node * tmpSockList = SockList;
		int found =0;
		
		rudp_packet_queuenode * tmpSYNpacket;
		receivers_node *receiver;
		//search in which packet Queue of the receivers this pakcet belongs
		
		while(tmpSockList!=NULL){
			receiver = tmpSockList->Receivers;
			while(receiver!=NULL){
			tmpSYNpacket = receiver->SYNpacket;
			
				if(tmpSYNpacket->rudppacket.header.seqno == SYNpacket->rudppacket.header.seqno && ntohs(tmpSYNpacket->to.sin_port) == ntohs(SYNpacket->to.sin_port) && !strcmp(inet_ntoa(SYNpacket->to.sin_addr),inet_ntoa(tmpSYNpacket->to.sin_addr))){
					found = 1;
					break;}
			
			
			if(found) break;
				receiver = receiver->Next;
			}
			if(found) break;
			tmpSockList=tmpSockList->Next;
		}
        if (found && SYNpacket->retries < RUDP_MAXRETRANS){
		rudp_rawpacket * packet = (rudp_rawpacket *)malloc(sizeof(rudp_rawpacket));
        packet->header = SYNpacket->rudppacket.header; 
		memcpy(packet->data,SYNpacket->rudppacket.data,len) ;
		
		char * buffer = malloc(len*sizeof(char)+sizeof(struct rudp_hdr));
		memcpy(buffer,&packet->header,sizeof(struct rudp_hdr));
		memcpy(buffer+sizeof(struct rudp_hdr),SYNpacket->rudppacket.data,len);
		
		
		//register again timeout
		// Start the timeout callback with event_timeout
		timer.tv_sec = RUDP_TIMEOUT/1000;               // convert to second
        timer.tv_usec = (RUDP_TIMEOUT%1000) * 1000;     // convert to micro
        gettimeofday(&t0, NULL);                        // current time of the day
        timeradd(&t0, &timer, &t1);  //add the timeout time with thecurrent time of the day
		if(EVENT){
        // register timeout
        if(event_timeout(t1, &retransmitSYN,arg, "timer_callback") == -1)
        {
			fprintf(stderr,"Error registering event_timeout\n");
			return -1;
		}
		}
		//search in which packet Queue of the sockets this pakcet belongs
		
		
		
		//struct sockaddr_in to = tmpSockList->to;
		printf("==Retransmitting SYN packet to %s...\n",inet_ntoa(SYNpacket->to.sin_addr));
		if(sendto((int)tmpSockList->sockfd, buffer, len+sizeof(struct rudp_hdr), 0, (struct sockaddr *)&to, sizeof(struct sockaddr)) <= 0)
		{
			fprintf(stderr,"Error : sendto\n");
			return -1;
		}
		tmpSYNpacket->retries++;
		
		}else{
			printf("Max Retransmits for SYN reached...\n");
			if(EVENT)event_timeout_delete(retransmitSYN,arg);
			receiver->FIN_ACK_RECEIVED = 1; //fake solution
			receivers_node* tmpreceiver = tmpSockList->Receivers;
			int allFIN=1;
			while(tmpreceiver!=NULL){
				if(tmpreceiver->FIN_ACK_RECEIVED == 0)
					allFIN=0;
				tmpreceiver = tmpreceiver->Next;
			}
			if(allFIN){
				tmpSockList->myevent_handler((rudp_socket_t)tmpSockList, RUDP_EVENT_CLOSED, NULL);
				event_fd_delete(rudp_receive_data, (void *)tmpSockList);
			}
			
			//event_fd_delete(rudp_receive_data, (void *)tmpSockList);
			tmpSockList->myevent_handler((rudp_socket_t)tmpSockList, RUDP_EVENT_TIMEOUT, &to);
		}
		return 1;
        
}
int retransmit(int fd,void *arg){ 
		if(EVENT)event_timeout_delete(retransmit,arg);
		
		struct sockaddr_in to;
		rudp_packet_queuenode *queuepacket = (rudp_packet_queuenode *)arg;
		receivers_node *receiver;
		struct timeval timer, t0, t1;           // timer variables
		// initialize timer variables
        timer.tv_sec = timer.tv_usec = 0;
        t0.tv_sec = t0.tv_usec = 0;
        t1.tv_sec = t1.tv_usec = 0;
		
		int len = queuepacket->data_len;
		rudp_socket_node * tmpSockList = SockList;
		
		//search in which packet Queue of the receivers this pakcet belongs
		int found =0;
		while(tmpSockList!=NULL){
			receiver = tmpSockList->Receivers;
			while(receiver!=NULL){
			rudp_packet_queuenode * tmpPacketQueue = receiver->PacketQueue;
			while(tmpPacketQueue!=NULL){
				if(tmpPacketQueue->rudppacket.header.seqno == queuepacket->rudppacket.header.seqno && ntohs(tmpPacketQueue->to.sin_port) == ntohs(queuepacket->to.sin_port) && !strcmp(inet_ntoa(queuepacket->to.sin_addr),inet_ntoa(tmpPacketQueue->to.sin_addr))){
					found = 1;
					break;}
				tmpPacketQueue=tmpPacketQueue->Next;
			}
			if(found) break;
				receiver = receiver->Next;
			}
			if(found) break;
			tmpSockList=tmpSockList->Next;
		}
        if (found && queuepacket->retries < RUDP_MAXRETRANS){
        rudp_rawpacket * packet = (rudp_rawpacket *)malloc(sizeof(rudp_rawpacket));
        packet->header = queuepacket->rudppacket.header; 
		memcpy(packet->data,queuepacket->rudppacket.data,len) ;
		///////////////////////////////
		char * buffer = malloc(len*sizeof(char)+sizeof(struct rudp_hdr));
		memcpy(buffer,&packet->header,sizeof(struct rudp_hdr));
		memcpy(buffer+sizeof(struct rudp_hdr),queuepacket->rudppacket.data,len);
		
		
		//register again timeout
		// Start the timeout callback with event_timeout
		timer.tv_sec = RUDP_TIMEOUT/1000;               // convert to second
        timer.tv_usec = (RUDP_TIMEOUT%1000) * 1000;     // convert to micro
        gettimeofday(&t0, NULL);                        // current time of the day
        timeradd(&t0, &timer, &t1);  //add the timeout time with thecurrent time of the day
		if(EVENT){
        // register timeout
        if(event_timeout(t1, &retransmit,queuepacket, "timer_callback") == -1)
        {
			fprintf(stderr,"Error registering event_timeout\n");
			return -1;
		}
		}
		to = receiver->to;
		printf("==Retransmiting packet with seqno %0x\n",packet->header.seqno);
		if(sendto((int)tmpSockList->sockfd, buffer, len+sizeof(struct rudp_hdr), 0, (struct sockaddr *)&to, sizeof(struct sockaddr)) <= 0)
		{
			fprintf(stderr,"Error : sendto\n");
			return -1;
		}
		if(packet->header.type == RUDP_FIN){
			printf("==========FIN retransmitted\n");
			receiver->FIN_seq = packet->header.seqno;
				
		}
		queuepacket->retries++;
		}else{
			printf("Max retransmits reached!\nHanging up...\n");
			if(EVENT)event_timeout_delete(retransmit,arg);
			receiver->FIN_ACK_RECEIVED = 1; //fake solution
			receivers_node* tmpreceiver = tmpSockList->Receivers;
			int allFIN=1;
			while(tmpreceiver!=NULL){
				if(tmpreceiver->FIN_ACK_RECEIVED == 0)
					allFIN=0;
				tmpreceiver = tmpreceiver->Next;
			}
			if(allFIN){
				tmpSockList->myevent_handler((rudp_socket_t)tmpSockList, RUDP_EVENT_CLOSED, NULL);
				event_fd_delete(rudp_receive_data, (void *)tmpSockList);
			}
			
			tmpSockList->myevent_handler((rudp_socket_t)tmpSockList, RUDP_EVENT_TIMEOUT, &to);
			
			}
		
	return 1;
}
/**sendpacket***********************************************
 * sends a packet in queue and updates sw for this socket  */
/***********************************************************/
int sendpacket(rudp_socket_node * socket,rudp_packet_queuenode *queuepacket,struct sockaddr_in to){
		receivers_node *receiver;
		
		struct timeval timer, t0, t1;           // timer variables
		// initialize timer variables
        timer.tv_sec = timer.tv_usec = 0;
        t0.tv_sec = t0.tv_usec = 0;
        t1.tv_sec = t1.tv_usec = 0;
		
		int len = queuepacket->data_len;
        rudp_rawpacket * packet = (rudp_rawpacket *)malloc(sizeof(rudp_rawpacket));
        packet->header = queuepacket->rudppacket.header; 
		memcpy(packet->data,queuepacket->rudppacket.data,len) ;
		///////////////////////////////
		char * buffer = malloc(len*sizeof(char)+sizeof(struct rudp_hdr));
		memcpy(buffer,&packet->header,sizeof(struct rudp_hdr));
		memcpy(buffer+sizeof(struct rudp_hdr),queuepacket->rudppacket.data,len);
		
		
		// Start the timeout callback with event_timeout
		timer.tv_sec = RUDP_TIMEOUT/1000;               // convert to second
        timer.tv_usec = (RUDP_TIMEOUT%1000) * 1000;     // convert to micro
        gettimeofday(&t0, NULL);                        // current time of the day
        timeradd(&t0, &timer, &t1);  //add the timeout time with thecurrent time of the day
if(EVENT){
        // register timeout

        if(event_timeout(t1, &retransmit, (void*)queuepacket, "timer_callback") == -1) //queuepacket
        {
			fprintf(stderr,"Error registering event_timeout\n");
			return -1;
		}
	}
		printf("RUDP:Sending packet with seqno=%0x - ",packet->header.seqno);
		if (DROP != 0 && rand() % DROP == 1){
			printf("packet dropped!\n");
			return 1;}
		if(sendto((int)socket->sockfd, buffer, len+sizeof(struct rudp_hdr), 0, (struct sockaddr *)&to, sizeof(struct sockaddr)) <= 0)
		{
			fprintf(stderr,"Error : sendto\n");
			return -1;
		}
		if(packet->header.type == RUDP_FIN){
			receiver=searchReceiver(socket,to);
			printf("==========FIN sent\n");
			receiver->FIN_seq = packet->header.seqno;
		}
	return 1;
}
/////////////////////////////////////////////////////////////////////
int rudp_receive_data(int fd, void *arg) //called when data arrives
{ 
	
	    rudp_socket_node *rsocket = (rudp_socket_node *) arg;
        struct sockaddr_in addr; //for the receiver sider here it is the address of the sender
        rudp_rawpacket packet;
        int addr_size;
        int bytes=0;
        receivers_node *receiver;
		//Init packet
        memset(&packet, 0x0, sizeof(rudp_rawpacket));
        
        addr_size = sizeof(addr);
        bytes = recvfrom((int)fd, (void*)&packet, sizeof(packet),0, (struct sockaddr*)&addr, (socklen_t*)&addr_size);
		
        printf("-Received %d bytes\n", bytes);
        //check for error after receiving
        if(bytes <= 0)
        {
                fprintf(stderr,"Error: recvfrom failed!\n");
                return -1;
        }
        
        int data_len=bytes-sizeof(struct rudp_hdr);
        int error;
        rudp_socket_node *rsock;
        senders_node * sender;
        //check for same version
        if(packet.header.version!=RUDP_VERSION){
			printf("RUDP Version not %d\n",RUDP_VERSION);
			return -1;
		}
        switch(packet.header.type)
        {
        case RUDP_DATA: //used by receiver -> Send back ACK
                //find the socket created
                rsock = searchSocket(rsocket);
                //find the sender of the packet
                sender = searchSender(rsocket,addr); 
				if(rsock==NULL){
					return -1;
				}
				
				if(sender == NULL){
					printf("NULL sender\n");
					return -1;
				}
				
				printf("RUDP Received: RUDP_DATA seq: %0x - expected: %0x\n", packet.header.seqno,(sender->last_seq+1));
                
				/////////////////////////////////////////////////////////////////
				if(packet.header.seqno == (sender->last_seq+1)){ //check if data seq no is the expected one
					
					//Call app handler 
					rsock->myrecvfrom_handler(rsock, &addr, packet.data, data_len); 
					//update last sequence number
					sender->last_seq=packet.header.seqno; 
					//prepare packet to send ACK
					packet.header.version = RUDP_VERSION;
					packet.header.type = RUDP_ACK;
					packet.header.seqno = sender->last_seq+1; //update header
					printf("-->Sending DATA ACK seqno=%0x\n",packet.header.seqno);
					//Drop probability
					
					//send packet
					error = sendto((int)rsock->sockfd, (void *)&packet.header, sizeof(struct rudp_hdr), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
					
					if(error <= 0)
					{
						fprintf(stderr, "RUDP Received: sendto failed\n");
						return -1;
					}
					
				}else{printf("!Not expected ACK..\n");
					//prepare packet to send ACK
					packet.header.version = RUDP_VERSION;
					packet.header.type = RUDP_ACK;
					packet.header.seqno = sender->last_seq + 1;
					printf("Sending *again* ACK seqno=%0x\n",packet.header.seqno);
					error = sendto((int)rsock->sockfd, (void *)&packet.header, sizeof(struct rudp_hdr), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
					
						if(error <= 0)
						{
							fprintf(stderr, "RUDP Received: sendto failed\n");
							return -1;
						}
					}
                break;
        case RUDP_ACK: //used by sender send next packet or if SYN_ACK set last_seq
               
                rsock=searchSocket(rsocket);
                receiver = searchReceiver(rsocket,addr);
                //rsock->to = addr; //////////
                //rsock=searchSockList(rsocket,addr); 
                if(rsock==NULL){
					return -1;
				}
				printf("RUDP Received: RUDP_ACK seq: %0x from %d", packet.header.seqno,ntohs(receiver->to.sin_port));
				
				//check if this ACK is for the SYN packet
				if(packet.header.seqno == (receiver->SYN_seqno+1) && !receiver->SYN_ACK_RECEIVED){
					//this ACK is for SYN
					printf("ACK for SYN received - ");
		if(EVENT){ 
					//delete register timeout for SYN
					 if(event_timeout_delete(retransmitSYN, receiver->SYNpacket) == -1)
                     {	  
						  fprintf(stderr,"Error deleting SYNtimeout\n");
                          return -1;
					 }
					 printf("Event timeout for SYN deleted\n");
				 }
					receiver->last_seq=packet.header.seqno;
					receiver->SYN_ACK_RECEIVED=1;
				}
				
				
				if(receiver->SYN_ACK_RECEIVED){ //if SYN has been ACKed
				
					
					//update last seqno received
					receiver->last_seq=packet.header.seqno;
					//search in PacketQueue(packet.header.seqno-1) and
					//delete event timeout for this and all previous packet in PacketQueue
					
					rudp_packet_queuenode * tmpPacketQueue = receiver->PacketQueue;
					
					while(tmpPacketQueue!=NULL){
						//for all packets with seqno< received packet seqno delete timeout as
						// they have surely been received
						if(SEQ_LT(tmpPacketQueue->rudppacket.header.seqno,packet.header.seqno) && tmpPacketQueue->state == SENT && !tmpPacketQueue->TimeoutDel){
					if(EVENT){
							 if(event_timeout_delete(retransmit,tmpPacketQueue) == -1)
                              {		 printf("\n\nSeq %0x ",tmpPacketQueue->rudppacket.header.seqno);
                                     printf("--Error deleting timeout\n\n");
                                     return -1;
                              }
                              tmpPacketQueue->TimeoutDel = 1;

                              }
						}
						tmpPacketQueue = tmpPacketQueue->Next; 
					}
					//search for all FIN 
				    
					//ACK for FIN received
					if(packet.header.seqno == (receiver->FIN_seq+1)){
					printf("ACK for FIN received\n");
						receiver->FIN_ACK_RECEIVED = 1;
						receivers_node *tmpReceiverList = rsock->Receivers;
						int allFIN=1;
						while(tmpReceiverList!=NULL){
							if(tmpReceiverList->FIN_ACK_RECEIVED == 0){
								printf("==Not all Finished\n");
								allFIN=0;
								break;
							}
							tmpReceiverList = tmpReceiverList->Next;
						}
						if(allFIN==1){
							rsock->myevent_handler((rudp_socket_t)rsock, RUDP_EVENT_CLOSED, NULL);
							event_fd_delete(rudp_receive_data, (void *)rsock);
						}
						
					}else{
					
					int window = 0;
					
					rudp_packet_queuenode *tmpSlidingWindow;
					
					//search the packet that was acked and assign the sliding window there
					
					
					if(searchPacketQueue(receiver,packet.header.seqno-1)!=NULL)
						receiver->SlidingWindow=searchPacketQueue(receiver,packet.header.seqno-1);//-1
						
					
					//if found and it's not the first ACK for SYN then put Sliding window to next packet in queue
					if(receiver->SlidingWindow != NULL && (packet.header.seqno-1) != receiver->SYN_seqno){
						receiver->SlidingWindow = receiver->SlidingWindow->Next;
					}
					
					tmpSlidingWindow=receiver->SlidingWindow;
					u_int32_t seqno;
					
					seqno=receiver->last_seq;
					
					//for current sliding window position till rudp max window 
					while( window <  RUDP_WINDOW && tmpSlidingWindow!=NULL){ 
						
							//send packet and increase the sw
						if(tmpSlidingWindow->state != SENT){
								tmpSlidingWindow->rudppacket.header.seqno = seqno;
								tmpSlidingWindow->state = SENT;
							
						
								if( sendpacket(rsock,tmpSlidingWindow,addr) > 0 ){
								printf("packet with seqno %0x sent\n",tmpSlidingWindow->rudppacket.header.seqno);
								}
							
						}
					
						window ++;
						seqno ++;
						if(tmpSlidingWindow->Next==NULL)break;
						tmpSlidingWindow = tmpSlidingWindow->Next;// go to next packet in queue
					}
				}
				}
				break;
        case RUDP_SYN: //used by receiver -> Send back ACK
					printf("RUDP Received: RUDP_SYN seq: %0x\n",packet.header.seqno); 
					rsock = searchSocket(rsocket);
					
					if(rsock==NULL){
						return -1;
					}
					
					sender=addSender(rsocket,addr);
					
					if(sender == NULL)
						return -1;
					
					//update last sequence number
					sender->last_seq = packet.header.seqno; 
					//prepare packet to send ACK
					packet.header.type=RUDP_ACK;
					packet.header.seqno=packet.header.seqno+1; //update header
					
					printf("Sending SYN ACK seqno=%0x to senderport:%d\n",packet.header.seqno,ntohs(addr.sin_port));
					
					//send packet
					error = sendto((int)rsock->sockfd, (void *)&packet.header, sizeof(struct rudp_hdr), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
					if(error <= 0)
					{
						fprintf(stderr, "RUDP Received: sendto failed\n");
						return -1;
					}
				//	printSockList();
				
				break;
        case RUDP_FIN: //used by receiver ->send back ACK
					printf("RUDP Received: RUDP_FIN seq: %0x\n",packet.header.seqno);
					senders_node *sender;
					rsock = searchSocket(rsocket);
					sender=searchSender(rsocket,addr);
                
					if(sender==NULL){
						return -1;
					}
					
					
					//update last sequence number
					sender->last_seq=packet.header.seqno; 
					//prepare packet to send ACK
					packet.header.type=RUDP_ACK;
					packet.header.seqno=packet.header.seqno+1; //update header
					printf("Sending FIN ACK seqno=%0x to senderport:%d\n",packet.header.seqno,ntohs(addr.sin_port));
					//send packet
					error = sendto((int)rsock->sockfd, (void *)&packet.header, sizeof(struct rudp_hdr), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
					if(error <= 0)
					{
						fprintf(stderr, "RUDP Received: sendto failed\n");
						return -1;
					}
					sender->status = FINISHED;
					//sender->last_seq = 0;
			
				
                break;
        default:
                break;
        }
        

        
        return 0;
	
}
//////////////////////////////////////////////////////////////////

/* 
 *rudp_recvfrom_handler: Register receive callback function 
 */ 

int rudp_recvfrom_handler(rudp_socket_t rsocket,int (*handler)(rudp_socket_t, struct sockaddr_in *, char *, int)) {
		rudp_socket_node *socket;
		socket = (rudp_socket_node *) rsocket;
		socket->myrecvfrom_handler=handler;				 
		return 0;
}

/* 
 *rudp_event_handler: Register event handler callback function 
 */ 
int rudp_event_handler(rudp_socket_t rsocket, int (*handler)(rudp_socket_t, rudp_event_t, struct sockaddr_in *)) {
	rudp_socket_node *socket;
	socket = (rudp_socket_node *) rsocket;
	socket->myevent_handler=handler;
	return 0;
}

/////////////////////////////////////////////////////////////////////
int rudp_sendto(rudp_socket_t rsocket, void* data, int len, struct sockaddr_in* to) {
	
	if(len > RUDP_MAXPKTSIZE){
		printf("Datalength more than RUDP_MAXPKTSIZE!\n");
		return -1;
	}
	
	struct sockaddr_in addr = *to; //each time has the address of a receiver
	
	//rsocket created for each file sent
	rudp_socket_node *socket;

	rudp_rawpacket packet;
	memset(&packet, 0x0, sizeof(rudp_rawpacket));
	
	//search in the List for previous socket session
	socket = searchSocket(rsocket);
	if(socket==NULL)
		return -1;
	receivers_node *receiver = searchReceiver(socket,addr);
	if (receiver == NULL)
		receiver = addReceiver(socket,addr);
	if(receiver == NULL)
		return -1;
	    
	if(receiver->SYN_ACK_RECEIVED==0){ // ACK for SYN has not been received
		if(receiver->SYN_seqno==0){ //SYN has not been sent
		packet.header.type=RUDP_SYN;
		packet.header.version=RUDP_VERSION;
		packet.header.seqno=rand() % 0xFFFFFFFF + 1;
		memcpy(&(receiver->to) ,&addr,sizeof(struct sockaddr_in)); 
		
		//update SYN_seqno 
		receiver->SYN_seqno = packet.header.seqno;
		receiver->SYNpacket = malloc(sizeof(rudp_packet_queuenode));
		receiver->SYNpacket->retries = 0;
		receiver->SYNpacket->rudppacket = packet;
		memcpy(&(receiver->SYNpacket->to),&addr,sizeof(struct sockaddr_in)); 
		printf("Sending SYN seq = %0x to recevport:%d\n",receiver->SYN_seqno,ntohs(receiver->to.sin_port));
		
		//register timeout
		struct timeval timer, t0, t1;
		// initialize timer variables
        timer.tv_sec = timer.tv_usec = 0;
        t0.tv_sec = t0.tv_usec = 0;
        t1.tv_sec = t1.tv_usec = 0;
        
		// Start the timeout callback with event_timeout
		timer.tv_sec = RUDP_TIMEOUT/1000;  //convert to sec
        timer.tv_usec = (RUDP_TIMEOUT%1000)*1000;  //convert to microsec
        gettimeofday(&t0, NULL);  //time of the day
        timeradd(&t0, &timer, &t1);  //add the timeout time with the current time of the day
 
        if(EVENT){
        // register timeout
        if(event_timeout(t1, &retransmitSYN,receiver->SYNpacket,"timeout_callback") == -1)
        {
			fprintf(stderr,"Error registering event_timeout\n");
			return -1;
		}
		}
		
		if(sendto((int)socket->sockfd, (unsigned char *) &packet,sizeof(struct rudp_hdr), 0, (struct sockaddr *)to, sizeof(struct sockaddr)) <= 0)
		{   
			fprintf(stderr,"Error : sendto\n");
			return -1;
		}
		}
		
		//SYN sent, DATA packet from app should be added  to the PacketQueue
		packet.header.type = RUDP_DATA;
		packet.header.version = RUDP_VERSION;
		packet.header.seqno = 0;
		//Don't have to take care about seqno it will be set when ACK is received
		memcpy(&(packet.data),data,len);
		addtoPacketQueue(receiver,len,packet,addr);
		
	}else{ //SYN ACK has been received - Data packet are added to the PacketQueue
	    packet.header.type = RUDP_DATA;
		packet.header.version = RUDP_VERSION;
		packet.header.seqno = 0;
		
		
		//add packet to PacketQueue
		memcpy(&(packet.data),data,len);
		addtoPacketQueue(receiver,len,packet,addr);
		//Try to send now
		//Sliding window
			int window = 0;
			rudp_packet_queuenode *tmpSlidingWindow;
			
			if(searchPacketQueue(receiver,receiver->last_seq)!=NULL)
						receiver->SlidingWindow=searchPacketQueue(receiver,receiver->last_seq);
			
			tmpSlidingWindow=receiver->SlidingWindow;
			u_int32_t seqno=receiver->last_seq;
			//for current sliding window position till rudp max window 
			while( window < RUDP_WINDOW && tmpSlidingWindow!=NULL){
						
							//send packet and increase the sw
						if(tmpSlidingWindow->state!=SENT){
							    tmpSlidingWindow->rudppacket.header.seqno = seqno;
								printf("Sending packet with seqno=%0x\n",seqno);
						if( sendpacket(socket,tmpSlidingWindow,*to) > 0 ){
							tmpSlidingWindow->state = SENT;
							//packets sent are increased

						}
						
						}
						window++;
						seqno++;
						if(tmpSlidingWindow->Next==NULL)break;
						tmpSlidingWindow = tmpSlidingWindow->Next;
			}
	
	}
	return 0;
}
//////////////////////////////////////////////////////////////////
rudp_socket_t rudp_socket(int port) {
		
        struct rudp_socket_type *rudp_socket=malloc(sizeof(struct  rudp_socket_type));
        int sockfd;
        struct sockaddr_in in_addr;  
		socklen_t sin_len;
        int errcode = 0;
         
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0)
        {
                fprintf(stderr,"RUDP socket() error!");
                return NULL;
        }
        
        

        bzero(&in_addr, sizeof(in_addr));
        in_addr.sin_family = AF_INET;
        in_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        in_addr.sin_port = htons(port);
		sin_len = sizeof(struct sockaddr_in);
		//Even if port is 0 bind will use the first unused port number
        if(bind(sockfd, (struct sockaddr *)&in_addr, sizeof(in_addr)) == -1)
        {
                fprintf(stderr, "rudp: bind error\n");
                return NULL;
        }
        //find out the port used
        errcode = getsockname (sockfd, (struct sockaddr *)&in_addr, &sin_len);
		 if (errcode == -1) {
			fprintf(stderr,"getsockname() failed");
			return NULL;
		 }
		port=ntohs(in_addr.sin_port);
		rudp_socket->port = port;
		rudp_socket->sockfd = sockfd;
		rudp_socket->saddr = in_addr;
		// create rudp_socket and add to socketList
        rudp_socket=addtoList(rudp_socket); ////////////////////////
        printf("Create socket: sockfd: %d, port: %d\n", rudp_socket->sockfd, rudp_socket->port);
        //printSockList();
		
        // Registeration callback function for receiving data from this sock fd
     
        if(event_fd((int)sockfd, &rudp_receive_data, (void*)rudp_socket, "RUDP:receive_data") < 0)
        {
                printf("event_fd Error: rudp_receive_data()!\n");
                return NULL;
        }
	
        return rudp_socket;
}
////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
/* 
 *rudp_close: Close socket 
 */ 

int rudp_close(rudp_socket_t rsocket) {
	
	rudp_socket_node *socket;
	rudp_rawpacket packet;
	memset(&packet, 0x0, sizeof(rudp_rawpacket));
	 
	 
	socket=searchSocket(rsocket);
	if(socket==NULL)
		return -1;
	printf("==Add FIN to all receivers\n");
	packet.header.type = RUDP_FIN;
	packet.header.seqno = 0;
	packet.header.version = RUDP_VERSION;
	
	receivers_node *receiver = socket->Receivers;
	while(receiver!=NULL){
		//Add FIN packet to PacketQueue
		addtoPacketQueue(receiver,0,packet,receiver->to);
		receiver = receiver->Next;
	}
	
	return 0;
}


