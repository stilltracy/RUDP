/*
 * RConn.cpp
 *
 *  Created on: Apr 24, 2014
 *      Author: stilltracy
 */


#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <sstream>
#include "RConn.hpp"
#include <sys/types.h>
#include <sys/socket.h>
namespace rudp {
using namespace std;

/*Initialization of static variables*/

pthread_mutex_t RConn::lock_conn_count=PTHREAD_MUTEX_INITIALIZER;
int RConn::conn_count=0;
int RConn::socket_fd=socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP);
map<string, Buffer *> RConn::buffer_router;
pthread_mutex_t RConn::lock_buffer_router=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RConn::lock_receiver_alive=PTHREAD_MUTEX_INITIALIZER;
bool RConn::receiver_alive=false;
pthread_t RConn::t_receiver=-1;
//pthread_mutex_t RConn::lock_connecting_rconn=PTHREAD_MUTEX_INITIALIZER;
//RConn * RConn::connecting_rconn=NULL;
pthread_mutex_t RConn::lock_listening_rconn=PTHREAD_MUTEX_INITIALIZER;
RConn * RConn::listening_rconn=NULL;
//pthread_mutex_t RConn::lock_connect=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t RConn::lock_accept=PTHREAD_MUTEX_INITIALIZER;
/*used by listening peer*/
RConn::RConn() {
	pthread_mutex_lock(&lock_conn_count);
		conn_count++;
	pthread_mutex_unlock(&lock_conn_count);
	//memset(&remote_addr,0,sizeof(remote_addr));
	lock_send=PTHREAD_MUTEX_INITIALIZER;
	lock_recv=PTHREAD_MUTEX_INITIALIZER;

	lock_syn_buffer=PTHREAD_MUTEX_INITIALIZER;
	syn_buffer=new Buffer(this,DEFAULT_CONFIG->SYN_BUFFER_MAX_VOLUME);
	lock_rx_buffer=PTHREAD_MUTEX_INITIALIZER;
	lock_sendable=PTHREAD_MUTEX_INITIALIZER;
	sendable=true;
	lock_recv_interrupted=PTHREAD_MUTEX_INITIALIZER;
	recv_interrupted=false;
	port=0;
	ip="0.0.0.0";
	config=DEFAULT_CONFIG;
	rx_buffer=new Buffer(this,config->BUFFER_MAX_VOLUME);
	//rx_buffer=NULL;
	state=RConnState::VOID;
	localSeq=0;
	remoteSeq=0;
	startReceiver();
}
/*called by connecting peer*/
RConn::RConn(string ip, int port)
{
	pthread_mutex_lock(&lock_conn_count);
		conn_count++;
	pthread_mutex_unlock(&lock_conn_count);
	//memset(&remote_addr,0,sizeof(remote_addr));
	lock_send=PTHREAD_MUTEX_INITIALIZER;
	lock_recv=PTHREAD_MUTEX_INITIALIZER;
	lock_syn_buffer=PTHREAD_MUTEX_INITIALIZER;
	syn_buffer=new Buffer(this,DEFAULT_CONFIG->SYN_BUFFER_MAX_VOLUME);
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
	stringstream ss;
	ss<<port;
	this->buffer_key=ip+ss.str();
	pthread_mutex_lock(&lock_buffer_router);
	buffer_router[buffer_key]=rx_buffer;
	pthread_mutex_unlock(&lock_buffer_router);
	startReceiver();
}
RConn::~RConn() {
	pthread_mutex_lock(&lock_conn_count);
	if(conn_count<=0)
	{
		conn_count=0;
		endReceiver();
	}
	else
	{
		conn_count--;
	}
	pthread_mutex_unlock(&lock_conn_count);
	wipe_static_traces();
	pthread_mutex_lock(&lock_rx_buffer);
	if(rx_buffer!=NULL)
		delete rx_buffer;
	pthread_mutex_unlock(&lock_rx_buffer);
}
void RConn::send_syn(unsigned int seq)
{
	send_control_msg(MSG_TYPE_SYN,seq);
}
ErrorCode RConn::recv_control_msg(RUDPMsgType type, unsigned int expected_seq)
{
	return recv_control_msg(type,expected_seq,NULL);
}
/*If p_remote_req is not NULL, the value of expected_seq is ignored*/
ErrorCode RConn::recv_control_msg(RUDPMsgType type, unsigned int expected_seq, unsigned int * p_remote_seq)
{
	 ErrorCode err=ErrorCode::SUCCESS;
	 while(true)
	 {
		 Packet * packet=recv_next_packet(type);
		 if(packet==NULL)
			 continue;
		 RUDPMsgHdr * hdr=packet->parse_hdr();
		 if(hdr!=NULL)
		 {
			 if(p_remote_seq!=NULL||hdr->sequence==expected_seq)
			 {
				 if(p_remote_seq!=NULL)
					 *p_remote_seq=hdr->sequence;
				 delete packet;
				 break;//no body for copying
			 }

		 }
		 delete packet;
	 }
	 return err;
}
Packet * RConn::get_syn_packet()
{
	pthread_mutex_lock(&lock_listening_rconn);
	pthread_mutex_lock(&this->lock_syn_buffer);
	/*ip and port should have been filled by the receiver() before putting the packet on syn_buffer*/
	Packet * p=syn_buffer->getPacket(MSG_TYPE_SYN,this->ip,this->port);
	pthread_mutex_unlock(&this->lock_syn_buffer);
	pthread_mutex_unlock(&lock_listening_rconn);
	return p;
}
ErrorCode RConn::recv_syn(unsigned int expected_seq)
{
	//return recv_control_msg(MSG_TYPE_SYN,expected_seq);
	ErrorCode err=ErrorCode::SUCCESS;
	 while(true)
	 {
		 Packet * packet=get_syn_packet();
		 if(packet==NULL)
			 continue;
		 RUDPMsgHdr * hdr=packet->parse_hdr();
		 if(hdr!=NULL)
		 {
			 if(hdr->sequence==expected_seq)
			 {
				 delete packet;
				 break;//no body for copying
			 }

		 }
		 delete packet;
	 }
	 return err;
}
ErrorCode RConn::recv_fin2(unsigned int * remote_seq)
{
	return recv_control_msg_timeout(MSG_TYPE_FIN2,0,remote_seq,config->RECV_FIN2_TIMEOUT);
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
bool RConn::check_timeout(__suseconds_t timeout, struct timeval start)
{
	struct timeval cur={0,0};
	gettimeofday(&cur,NULL);
	if(cur.tv_usec-start.tv_usec>=timeout||cur.tv_sec-start.tv_sec>timeout/1000000)
		return true;
	return false;
}
/*If p_remote_req is not NULL, the value of expected_seq is ignored*/
ErrorCode RConn::recv_control_msg_timeout(RUDPMsgType type,unsigned int expected_seq,unsigned int * p_remote_seq,int timeout)
{
	 ErrorCode err=ErrorCode::SUCCESS;
	 //set_nonblocking();
	 struct timeval start={0,0};
	 gettimeofday(&start,NULL);
	 while(true)
	 {
		 if(check_timeout(timeout,start))
		 {
			 err=ErrorCode::TIME_OUT;
			 break;
		 }
		 Packet * packet=recv_next_packet(type);
		 if(packet==NULL)
			 continue;
		 RUDPMsgHdr * hdr=packet->parse_hdr();
		 if(hdr!=NULL)
		 {
			 if(hdr->type==type&&(p_remote_seq!=NULL||hdr->sequence==expected_seq))
			 {
				 if(p_remote_seq!=NULL)
					 *p_remote_seq=hdr->sequence;
				 delete packet;
				 break;//no body for copying
			 }

		 }
		 delete packet;
	 }
	 //set_blocking();
	 return err;
}
 ErrorCode RConn::recv_ack(unsigned int expected_seq)
{
	 //return recv_msg(MSG_TYPE_ACK,expected_seq,NULL);
	 return recv_control_msg_timeout(MSG_TYPE_ACK,expected_seq,NULL,config->CONNECT_TIMEOUT);
}
void RConn::startReceiver()
{
	pthread_mutex_lock(&this->lock_receiver_alive);
	if(this->receiver_alive)//a receiver thread has already been started.
	{
		pthread_mutex_unlock(&this->lock_receiver_alive);
		return;
	}
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
Packet * RConn::recv_next_packet(RUDPMsgType type)
{
	pthread_mutex_lock(&this->lock_rx_buffer);
	Packet * p=this->rx_buffer->getPacket(type);
	pthread_mutex_unlock(&this->lock_rx_buffer);
	return p;
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
void RConn::on_receiver_exit()
{
	pthread_mutex_lock(&lock_listening_rconn);
	listening_rconn=NULL;
	pthread_mutex_unlock(&lock_listening_rconn);
	pthread_mutex_lock(&lock_conn_count);
	conn_count=0;
	pthread_mutex_unlock(&lock_conn_count);
	pthread_mutex_lock(&lock_buffer_router);
	buffer_router.clear();
	pthread_mutex_unlock(&lock_buffer_router);
}
void RConn::on_syn_received(Packet * p)
{
	pthread_mutex_lock(&lock_listening_rconn);
	listening_rconn->ip=p->ip;
	listening_rconn->port=p->port;
	pthread_mutex_lock(&listening_rconn->lock_syn_buffer);
	listening_rconn->syn_buffer->putPacket(p);
	pthread_mutex_unlock(&listening_rconn->lock_syn_buffer);
	pthread_mutex_unlock(&lock_listening_rconn);
}
/*caller to this must have held lock_buffer_router*/
Buffer * RConn::find_rx_buffer(RConn * conn,string key)
{
	Buffer * buffer=NULL;
	pthread_mutex_lock(&conn->lock_rx_buffer);
	if(conn->rx_buffer==NULL)
	{
		buffer=new Buffer(conn,DEFAULT_CONFIG->BUFFER_MAX_VOLUME);
		conn->rx_buffer=buffer;
	}
	else
	{
		buffer=conn->rx_buffer;
	}
	buffer_router[key]=buffer;

	pthread_mutex_unlock(&conn->lock_rx_buffer);

	return buffer;
}
Buffer * RConn::find_buffer(string key,string ip_str, int port)
{
	pthread_mutex_lock(&RConn::lock_buffer_router);
	map<string,Buffer *>::iterator it=buffer_router.find(key);
	Buffer * buffer=NULL;
	if(it==buffer_router.end())//allocate a buffer for the address
	{
		pthread_mutex_lock(&RConn::lock_listening_rconn);
		if(RConn::listening_rconn!=NULL)
		{
			/*after this, an entry for the rx_buffer will have been created in buffer_router*/
			buffer =find_rx_buffer(RConn::listening_rconn,key);

			RConn::listening_rconn->ip=ip_str;
			RConn::listening_rconn->port=port;
				//RConn::listening_rconn->remote_addr=addr;
			RConn::listening_rconn->buffer_key=key;
		}
		pthread_mutex_unlock(&RConn::lock_listening_rconn);
	}
	else
	{
		buffer=it->second;
	}
	pthread_mutex_unlock(&RConn::lock_buffer_router);
	return buffer;
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
				delete p;
				continue;
			}
			char buf_ip[16];
			const char * ip_cstr=inet_ntop(AF_INET,&si_client.sin_addr.s_addr,buf_ip,sizeof(si_client));
			stringstream ss;
			ss<<htons(si_client.sin_port);
			string key=string(ip_cstr)+ss.str();
			if(p->is_syn())
			{
				p->ip=string(ip_cstr);
				p->port=htons(si_client.sin_port);
				on_syn_received(p);
				//delete p;
				continue;
			}

			Buffer * buffer=find_buffer(key,string(ip_cstr),htons(si_client.sin_port));
			if(buffer!=NULL)
			{
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
			else
			{
				delete p;
			}
		}

		/*check whether the receiver should die at this point*/
		pthread_mutex_lock(&lock_receiver_alive);
		if(!receiver_alive)//die happily
		{
			on_receiver_exit();//die elegantly
			pthread_mutex_unlock(&lock_receiver_alive);
			break;
		}
		pthread_mutex_unlock(&lock_receiver_alive);
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
void RConn::send_control_msg(RUDPMsgType type,unsigned int seq)
{
	Packet * p =new Packet(sizeof(RUDPMsgHdr));
	p->make_msg(type,seq,NULL);
	send_packet(p);
	delete p;
}
void RConn::send_fin1(unsigned int seq)
{
	send_control_msg(MSG_TYPE_FIN1,seq);
}
void RConn::send_fin2(unsigned int seq)
{
	send_control_msg(MSG_TYPE_FIN2,seq);
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
void RConn::on_close(unsigned int fin_seq)
{
	this->disable_send();
	this->send_ack(fin_seq);
	this->send_fin2(this->localSeq);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<this->config->CLOSE_MAX_RETRIES;i++)
	{
		err=this->recv_control_msg_timeout(MSG_TYPE_ACK,this->localSeq,NULL,this->config->CLOSE_TIMEOUT);
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
	this->wipe_static_traces();
	this->state=RConnState::CLOSED;

}
void RConn::wipe_static_traces()
{
	pthread_mutex_lock(&lock_buffer_router);
	buffer_router.erase(this->buffer_key);
	pthread_mutex_unlock(&lock_buffer_router);
	pthread_mutex_lock(&lock_listening_rconn);
	if(listening_rconn==this)
	{
		listening_rconn=NULL;
	}
	pthread_mutex_unlock(&lock_listening_rconn);
}
bool RConn::is_closed()
{
	return this->state==RConnState::CLOSED;
}
void RConn::send_ack(unsigned int seq)
{
	send_control_msg(MSG_TYPE_ACK,seq);
}

} /* namespace rudp */
