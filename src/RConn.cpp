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
pthread_mutex_t RConn::lock_buffer_router=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RConn::lock_receiver_alive=PTHREAD_MUTEX_INITIALIZER;
bool RConn::receiver_alive=false;
pthread_t RConn::t_receiver=-1;
pthread_mutex_t RConn::lock_listening_rconn=PTHREAD_MUTEX_INITIALIZER;
RConn * RConn::listening_rconn=NULL;

/*used by listening peer*/
RConn::RConn() {
	lock_rx_buffer=PTHREAD_MUTEX_INITIALIZER;
	lock_sendable=PTHREAD_MUTEX_INITIALIZER;
	sendable=true;
	lock_recv_interrupted=PTHREAD_MUTEX_INITIALIZER;
	recv_interrupted=false;
	port=0;
	ip="0.0.0.0";
	config=DEFAULT_CONFIG;
	rx_buffer=new Buffer(this,config->BUFFER_MAX_VOLUME);
	state=RConnState::VOID;
	localSeq=0;
	remoteSeq=0;
	startReceiver();
}
/*called by connecting peer*/
RConn::RConn(string ip, int port)
{
	this->lock_rx_buffer=PTHREAD_MUTEX_INITIALIZER;
	this->lock_sendable=PTHREAD_MUTEX_INITIALIZER;

	this->sendable=true;
	this->lock_recv_interrupted=PTHREAD_MUTEX_INITIALIZER;
	this->recv_interrupted=false;
	this->ip=ip;
	this->port=port;

	config=DEFAULT_CONFIG;
	rx_buffer=new Buffer(this,config->BUFFER_MAX_VOLUME);
	localSeq=0;
	remoteSeq=0;
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
		 RUDPMsgHdr * hdr=packet->parse_hdr();
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
		 RUDPMsgHdr * hdr=packet->parse_hdr();
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
 ErrorCode RConn::recv_ack(unsigned int expectedSeq)
{
	 //return recv_msg(MSG_TYPE_ACK,expectedSeq,NULL);
	 return recv_msg_timeout(MSG_TYPE_ACK,expectedSeq,config->CONNECT_TIMEOUT);
}
void RConn::startReceiver()
{
	pthread_mutex_lock(&this->lock_receiver_alive);
	if(this->receiver_alive)//a receiver thread has already been started.
		return;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	int status=pthread_create(&this->t_receiver,&attr,&receiver,NULL);
	if(status!=0)
	{
		perror("pthread_create failed.");
		this->t_receiver=-1;
	}
	else
	{
		this->receiver_alive=true;
	}
	pthread_mutex_unlock(&this->lock_receiver_alive);
}

void RConn::endReceiver()
{
	pthread_mutex_lock(&this->lock_receiver_alive);
	this->receiver_alive=false;
	pthread_mutex_unlock(&this->lock_receiver_alive);
}

Packet * RConn::recv_next_packet()
{
	pthread_mutex_lock(&this->lock_rx_buffer);
	Packet * p=this->rx_buffer->getPacket();
	pthread_mutex_unlock(&this->lock_rx_buffer);
	return p;
}
void RConn::disable_send()
{
	pthread_mutex_lock(&lock_sendable);
	sendable=false;
	pthread_mutex_unlock(&lock_sendable);
}
void * RConn::receiver(void * args)
{
	//RConn * conn=(RConn *)args;
	unsigned char * buf=new unsigned char[DEFAULT_CONFIG->RECEIVER_BUFFER_MAX_VOLUME]();

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
			Packet * p =new Packet(buf,size);
			if(!p->validate())//illegal msg
			{
				continue;
			}


			in_addr_t addr=si_client.sin_addr.s_addr;
			//todo: support multiple connections with the same IP

			pthread_mutex_lock(&RConn::lock_buffer_router);
			map<in_addr_t,Buffer *>::iterator it=buffer_router.find(addr);
			Buffer * buffer=NULL;
			if(it==buffer_router.end())//allocate a buffer for the address
			{
				//TODO: needs to set the maximum age for a buffer that isn't associated with a connection

				pthread_mutex_lock(&RConn::lock_listening_rconn);
				pthread_mutex_lock(&RConn::listening_rconn->lock_rx_buffer);
				if(RConn::listening_rconn->rx_buffer==NULL)
				{
					buffer=new Buffer(RConn::listening_rconn,DEFAULT_CONFIG->BUFFER_MAX_VOLUME);
					RConn::listening_rconn->rx_buffer=buffer;
				}
				else
				{
					buffer=RConn::listening_rconn->rx_buffer;
				}
				buffer_router[addr]=buffer;

				pthread_mutex_unlock(&RConn::listening_rconn->lock_rx_buffer);
				char buf_ip[16];
				const char * ip_cstr=inet_ntop(AF_INET,&si_client.sin_addr.s_addr,buf_ip,sizeof(si_client));
				RConn::listening_rconn->ip=string(ip_cstr);
				RConn::listening_rconn->port=htons(si_client.sin_port);
				pthread_mutex_unlock(&RConn::lock_listening_rconn);
			}
			else
			{
				buffer=it->second;
			}
			pthread_mutex_unlock(&RConn::lock_buffer_router);

			if(p->is_fin1())
			{
				RConn * conn=(RConn *)buffer->getOwner();
				conn->on_close(p->parse_hdr()->sequence);
				//TODO: free structures corresponding to conn inside the receiver thread
			}
			else
			{
				buffer->putPacket(p);
			}
		}

		/*check whether the receiver should die at this point*/
		if(!receiver_alive)//die happily
		{
			break;
		}
	}
	delete[] buf;
	pthread_exit(NULL);
}
void RConn::send_packet(Packet * p)
{
	sockaddr_in si_peer;
	memset(&si_peer,0,sizeof(si_peer));
    si_peer.sin_family = AF_INET;
    si_peer.sin_addr.s_addr=inet_addr(this->ip.c_str());
    si_peer.sin_port=htons(this->port);
	int slen=sizeof(si_peer);
	if(sendto(socket_fd,(void *)(p->buf),p->size,0,(sockaddr *)&si_peer, slen)==-1)
	{
		perror("sendto() failure.");
	}
}
void RConn::send_fin1(int seq)
{

}
void RConn::send_fin2(int seq)
{

}
void RConn::disable_recv()
{
	pthread_mutex_lock(&this->lock_recv_interrupted);
	this->recv_interrupted=true;
	pthread_mutex_unlock(&this->lock_recv_interrupted);
}
bool RConn::is_sendable()
{
	int ret=false;
	pthread_mutex_lock(&lock_sendable);
	ret=sendable;
	pthread_mutex_unlock(&lock_sendable);
	return ret;
}
/* called by the receiver thread upon receiving FIN*/
void RConn::on_close(int fin_seq)
{
	this->disable_send();
	this->send_ack(fin_seq);
	this->send_fin2(this->localSeq);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<this->config->ACCEPT_MAX_RETRIES;i++)
	{
		err=this->recv_msg_timeout(MSG_TYPE_ACK,this->localSeq,this->config->CLOSE_TIMEOUT);
		if(err==ErrorCode::SUCCESS)
			break;
		else
		{
			this->send_ack(fin_seq);
			this->send_fin2(this->localSeq);
		}
	}
	if(err!=ErrorCode::SUCCESS)
	{
		perror("No ACK to FIN-ACK received.");
	}
	this->disable_recv();
	this->state=RConnState::CLOSED;
}
int RConn::send(unsigned char * buffer, int size)
{
	if(!is_sendable())
	{
		perror("connection closed by peer.");
		return -1;
	}
	int ret=size;
	Packet * p=new Packet(sizeof(RUDPMsgHdr)+size);
	p->make_msg(MSG_TYPE_DATA,this->localSeq,buffer);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<this->config->TRANSMIT_MAX_RETRIES;i++)
	{
		send_packet(p);
		err=recv_ack(this->localSeq);
		if(err==ErrorCode::SUCCESS)
		{
			this->localSeq++;
			delete p;
			return size;
		}
	}
	delete p;
	ret=-1;
	return ret;
}
int RConn::recv(unsigned char * buffer, int size)
{
	int ret=0;
	Packet * p=NULL;
	struct timeval start;
	gettimeofday(&start,NULL);
	while(true)
	{
		pthread_mutex_lock(&this->lock_recv_interrupted);
		if(this->recv_interrupted)
		{
			ret=0;
			break;
		}
		pthread_mutex_unlock(&this->lock_recv_interrupted);
		if((p=this->recv_next_packet())!=NULL
				&&this->remoteSeq==p->parse_hdr()->sequence)
		{
			ret=p->unpack_data(buffer,size);
			send_ack(this->remoteSeq);
			this->remoteSeq++;
			break;
		}
		else
		{
			if(check_timeout(this->config->RECV_REACK_TIMEOUT,start))
			{
				send_ack(this->remoteSeq-1);
				gettimeofday(&start,NULL);
			}
			if(p!=NULL)
				delete p;
		}
	}
	delete p;
	return ret;
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
	pthread_mutex_lock(&RConn::lock_listening_rconn);
	RConn::listening_rconn=conn;
	pthread_mutex_unlock(&RConn::lock_listening_rconn);
	conn->send_syn(0);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<conn->config->CONNECT_MAX_RETRIES;i++)
	{
		err=conn->recv_ack(0);
		if(err==ErrorCode::SUCCESS)
			break;
		else
			conn->send_syn(0);
	}
	if(err!=ErrorCode::SUCCESS)
	{
		perror("No SYN_ACK received.");
		pthread_mutex_lock(&RConn::lock_listening_rconn);
		RConn::listening_rconn=NULL;
		pthread_mutex_unlock(&RConn::lock_listening_rconn);
		return NULL;
	}
	conn->send_ack(1);
	conn->state=RConnState::ESTABLISHED;
	conn->localSeq=2;
	conn->remoteSeq=2;
	pthread_mutex_lock(&RConn::lock_listening_rconn);
	RConn::listening_rconn=NULL;
	pthread_mutex_unlock(&RConn::lock_listening_rconn);
	return conn;
}
RConn * accept(int port)
{
	RConn * conn =new RConn();
	pthread_mutex_lock(&RConn::lock_listening_rconn);
	RConn::listening_rconn=conn;
	pthread_mutex_unlock(&RConn::lock_listening_rconn);
	conn->recv_syn(0);
	conn->send_ack(0);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<conn->config->ACCEPT_MAX_RETRIES;i++)
	{
		err=conn->recv_msg_timeout(MSG_TYPE_ACK,1,conn->config->CONNECT_TIMEOUT);
		if(err==ErrorCode::SUCCESS)
			break;
		else
		{
			conn->send_ack(0);
		}
	}
	if(err!=ErrorCode::SUCCESS)
	//if(conn->recv_msg(MSG_TYPE_ACK,1,NULL)!=ErrorCode::SUCCESS)
	{
		perror("No ACK(3) received.");
		delete conn;
		RConn::listening_rconn=NULL;
		return NULL;
	}
	conn->state=RConnState::ESTABLISHED;
	conn->localSeq=2;
	conn->remoteSeq=2;
	pthread_mutex_lock(&RConn::lock_listening_rconn);
	RConn::listening_rconn=NULL;
	pthread_mutex_unlock(&RConn::lock_listening_rconn);
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
void close(RConn * rconn)
{

}
} /* namespace rudp */
