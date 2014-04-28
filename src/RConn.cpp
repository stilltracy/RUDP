/*
 * RConn.cpp
 *
 *  Created on: Apr 24, 2014
 *      Author: stilltracy
 */


#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include "RConn.hpp"
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
namespace rudp {
using namespace std;
int RConn::socket_fd=socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP);
map<in_addr_t, Buffer *> RConn::buffer_router;
bool RConn::receiver_alive=false;
RConn * RConn::listening_rconn=NULL;
RConn::RConn() {
	// TODO Auto-generated constructor stub
	//rxBuffer=new Buffer();
	port=0;
	ip="0.0.0.0";
	t_receiver=-1;
	config=DEFAULT_CONFIG;
	rx_buffer=new Buffer(config->BUFFER_MAX_VOLUME);
	state=RConnState::VOID;
	localSeq=0;
	remoteSeq=0;
	startReceiver();
}
RConn::RConn(string ip, int port)
{
	this->ip=ip;
	this->port=port;
	t_receiver=-1;
	config=DEFAULT_CONFIG;
	rx_buffer=new Buffer(config->BUFFER_MAX_VOLUME);
	localSeq=0;
	remoteSeq=0;
	this->remote_addr.sin_addr.s_addr=inet_addr(ip.c_str());
	this->remote_addr.sin_family=AF_INET;
	this->remote_addr.sin_port=htons(port);
	this->state=RConnState::CONNECTING;
	startReceiver();
}
RConn::~RConn() {
	endReceiver();
	delete rx_buffer;
}
void RConn::send_syn(int seq)
{
	Packet * p =new Packet(sizeof(RUDPMsgHdr));
	p->make_msg(MSG_TYPE_SYN,seq,NULL);
	send_packet(p);
	delete p;
}
ErrorCode RConn::recv_msg(RUDPMsgType type, unsigned int expectedSeq, unsigned char * buf)
{
	 ErrorCode err=ErrorCode::SUCCESS;
	 while(true)
	 {
		 Packet * packet=recv_next_packet();
		 if(packet==NULL)
			 continue;
		 RUDPMsgHdr * hdr=(RUDPMsgHdr *)packet->buf;
		 if(hdr!=NULL)
		 {
			 if(hdr->type==type&&hdr->sequence==expectedSeq)
			 {
				 if(type==MSG_TYPE_DATA)
				 {
					 memcpy(buf,hdr->body,hdr->size);
				 }
				 delete packet;
				 break;//no body for copying
			 }

		 }
		 delete packet;
	 }
	 return err;
}
ErrorCode RConn::recv_syn(unsigned int expectedSeq)
{
	return recv_msg(MSG_TYPE_SYN,expectedSeq,NULL);
}
int RConn::set_nonblocking()
{
	int flags;
    if (-1 == (flags = fcntl(socket_fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}
int RConn::set_blocking()
{
	int flags;
	if (-1 == (flags = fcntl(socket_fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(socket_fd, F_SETFL, flags &~O_NONBLOCK);
}
bool RConn::check_timeout(int timeout, struct timeval start)
{
	struct timeval cur;
	gettimeofday(&cur,NULL);
	if(cur.tv_usec-start.tv_usec>=timeout||cur.tv_sec-start.tv_sec>timeout/1000000)
		return true;
	return false;
}
ErrorCode RConn::recv_msg_timeout(RUDPMsgType type,unsigned int expectedSeq,int timeout)
{
	 ErrorCode err=ErrorCode::SUCCESS;
	 //set_nonblocking();
	 struct timeval start;
	 gettimeofday(&start,NULL);
	 while(true)
	 {
		 if(check_timeout(timeout,start))
		 {
			 err=ErrorCode::TIME_OUT;
			 break;
		 }
		 Packet * packet=recv_next_packet();
		 if(packet==NULL)
			 continue;
		 RUDPMsgHdr * hdr=(RUDPMsgHdr *)packet->buf;
		 if(hdr!=NULL)
		 {
			 if(hdr->type==type&&hdr->sequence==expectedSeq)
			 {

				 delete packet;
				 break;//no body for copying
			 }

		 }
		 delete packet;
	 }
	 //set_blocking();
	 return err;
}
 ErrorCode RConn::recv_syn_ack(unsigned int expectedSeq)
{
	 return recv_msg_timeout(MSG_TYPE_SYN_ACK,expectedSeq,config->CONNECT_TIMEOUT);
}
void RConn::startReceiver()
{
	if(this->receiver_alive)//a receiver thread has already been started.
		return;
	int status=pthread_create(&this->t_receiver,NULL,&receiver,NULL);
	if(status!=0)
	{
		perror("pthread_create failed.");
		this->t_receiver=-1;
	}
	else
	{
		//TODO:needs to be atomic
		this->receiver_alive=true;
	}
}
//TODO: needs to be atomic
void RConn::endReceiver()
{
	this->receiver_alive=false;
}
//needs to be atomic
Packet * RConn::recv_next_packet()
{
	Packet * p=this->rx_buffer->getPacket();
	return p;
}
void * RConn::receiver(void * args)
{
	//RConn * conn=(RConn *)args;
	unsigned char * buf=new unsigned char[DEFAULT_CONFIG->RECEIVER_BUFFER_MAX_VOLUME];

	sockaddr_in si_client;
	memset(&si_client,0,sizeof(si_client));
	unsigned int slen=sizeof(si_client);
	int size=0;
	while(true)
	{
		//the socket needs not be non-blocking
		if((size=recvfrom(socket_fd,buf,DEFAULT_CONFIG->RECEIVER_BUFFER_MAX_VOLUME,0,(sockaddr *)&si_client,&slen))==-1)
		{
			perror("recvfrom() failure.");
			break;
		}
		else
		{
			in_addr_t addr=si_client.sin_addr.s_addr;
			//todo: support multiple connections with the same IP
			//TODO: needs to be atomic
			Buffer * buffer=buffer_router[addr];
			if(buffer==NULL)//allocate a buffer for the address
			{
				//TODO: needs to set the maximum age for a buffer that isn't associated with a connection
				buffer=new Buffer(DEFAULT_CONFIG->BUFFER_MAX_VOLUME);
				buffer_router[addr]=buffer;
				RConn::listening_rconn->rx_buffer=buffer;
				char * buf=new char[16];
				const char * ip_cstr=inet_ntop(AF_INET,&si_client.sin_addr.s_addr,buf,sizeof(si_client));
				RConn::listening_rconn->ip=string(ip_cstr);
				RConn::listening_rconn->port=si_client.sin_port;
			}
			buffer->putPacket(buf,size);
		}

		/*check whether the receiver should die at this point*/
		if(!receiver_alive)//die happily
		{
			break;
		}
	}
	delete[] buf;
	return NULL;
}
void RConn::send_packet(Packet * p)
{
	sockaddr_in si_peer;
    si_peer.sin_family = AF_INET;
    si_peer.sin_addr.s_addr=inet_addr(this->ip.c_str());
    si_peer.sin_port=htons(this->port);
	int slen=sizeof(si_peer);
	if(sendto(socket_fd,(void *)(p->buf),p->size,0,(sockaddr *)&si_peer, slen)==-1)
	{
		perror("sendto() failure.");
	}
}
void RConn::send_ack(int seq)
{
	Packet * p =new Packet(sizeof(RUDPMsgHdr));
	p->make_msg(MSG_TYPE_ACK,seq,NULL);
	send_packet(p);
	delete p;

}
RConn * connect(string ip, int port, int * status)
{
	RConn * conn =new RConn(ip,port);
	conn->send_syn(0);
	ErrorCode err=conn->recv_syn_ack(0);
	if(err!=ErrorCode::SUCCESS)
	{
		perror("No SYN_ACK received.");
		return NULL;
	}
	conn->send_ack(1);
	conn->state=RConnState::ESTABLISHED;
	conn->localSeq=2;
	conn->remoteSeq=2;
	return conn;
}
RConn * accept(int port)
{
	RConn * conn =new RConn();
	RConn::listening_rconn=conn;
	conn->recv_syn(0);
	conn->send_ack(0);
	if(conn->recv_msg_timeout(MSG_TYPE_ACK,1,conn->config->CONNECT_TIMEOUT)!=ErrorCode::SUCCESS)
	{
		perror("No ACK(3) received.");
		delete conn;
		RConn::listening_rconn=NULL;
		return NULL;
	}
	conn->state=RConnState::ESTABLISHED;
	conn->localSeq=2;
	conn->remoteSeq=2;
	RConn::listening_rconn=NULL;
	return conn;
}
int bind(int port)
{
	struct sockaddr_in si_me;
	si_me.sin_family=AF_INET;
	si_me.sin_port=htons(port);
	si_me.sin_addr.s_addr=htonl(INADDR_ANY);
	return ::bind(RConn::socket_fd,(sockaddr *)&si_me, sizeof(si_me));
}
} /* namespace rudp */
