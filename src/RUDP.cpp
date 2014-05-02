/*
 * RUDP.cpp
 *
 *  Created on: May 1, 2014
 *      Author: stilltracy
 */
#include "RUDP.hpp"
namespace rudp{
/*concurrent calls to this method of the same object will be serialized.*/
int RConn::send(unsigned char * buffer, int size)
{
	pthread_mutex_lock(&this->lock_send);
	if(!is_sendable())
	{
		perror("connection closed by peer.");
		pthread_mutex_unlock(&this->lock_send);
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
			pthread_mutex_unlock(&this->lock_send);
			return size;
		}
	}
	delete p;
	ret=-1;
	pthread_mutex_unlock(&this->lock_send);
	return ret;
}
/*concurrent calls to this method of the same object will be serialized.*/
int RConn::recv(unsigned char * buffer, int size)
{
	pthread_mutex_lock(&this->lock_recv);
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
			pthread_mutex_unlock(&this->lock_recv_interrupted);
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
	pthread_mutex_unlock(&this->lock_recv);
	return ret;
}

RConn * connect(string ip, int port, int * status)
{

	RConn * conn =new RConn(ip,port);
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

		return NULL;
	}
	conn->send_ack(1);
	conn->state=RConnState::ESTABLISHED;
	conn->localSeq=2;
	conn->remoteSeq=2;

	return conn;
}
/*concurrent calls to this method will be serialized.*/
RConn * accept(int port)
{
	pthread_mutex_lock(&RConn::lock_accept);
	RConn * conn =new RConn();
	pthread_mutex_lock(&RConn::lock_listening_rconn);
	RConn::listening_rconn=conn;
	pthread_mutex_unlock(&RConn::lock_listening_rconn);
	conn->recv_syn(0);
	conn->send_ack(0);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<conn->config->ACCEPT_MAX_RETRIES;i++)
	{
		err=conn->recv_control_msg_timeout(MSG_TYPE_ACK,1,NULL,conn->config->CONNECT_TIMEOUT);
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
		pthread_mutex_lock(&RConn::lock_listening_rconn);
		RConn::listening_rconn=NULL;
		pthread_mutex_unlock(&RConn::lock_listening_rconn);
		pthread_mutex_unlock(&RConn::lock_accept);
		return NULL;
	}
	conn->state=RConnState::ESTABLISHED;
	conn->localSeq=2;
	conn->remoteSeq=2;
	pthread_mutex_lock(&RConn::lock_listening_rconn);
	RConn::listening_rconn=NULL;
	pthread_mutex_unlock(&RConn::lock_listening_rconn);
	pthread_mutex_unlock(&RConn::lock_accept);
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
	rconn->disable_send();
	unsigned int local_seq=rconn->localSeq;
	rconn->send_fin1(local_seq);
	ErrorCode err=ErrorCode::SUCCESS;
	for(int i=0;i<rconn->config->CLOSE_MAX_RETRIES;i++)
	{
		err=rconn->recv_ack(local_seq);
		if(err==ErrorCode::SUCCESS)
			break;
		else
			rconn->send_fin1(local_seq);
	}
	if(err!=ErrorCode::SUCCESS)
	{
		perror("No ACK for FIN1 received.");
	}
	unsigned int seq=0;
	err=rconn->recv_fin2(&seq);
	if(err!=ErrorCode::SUCCESS)
	{
		perror("No FIN2 received.");
	}
	else
	{
		rconn->send_ack(seq);
	}
	//TODO: disable sender and receiver
	rconn->disable_recv();
}
}
