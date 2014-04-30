/*
 * buffer.hpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#ifndef RECEIVER_HPP_
#define RECEIVER_HPP_
#include <cstdlib>
#include <pthread.h>
#include "enums.hpp"
#include "rudp_msg.hpp"
namespace rudp{
class Packet{

public:
	unsigned char * buf;
	int size;//the size of all the bytes that will be transmitted, including the header
	Packet * next;

	Packet(int size)
	{
		this->size=size;
		this->next=NULL;
		this->buf=new unsigned char[size]();

	}
	Packet(unsigned char * buf, int size)
	{
		this->size=size;
		this->next=NULL;
		this->buf=new unsigned char[size]();
		memcpy(this->buf,buf,size);
	}
	bool validate()
	{
		return !this->size<4&&check_magic(this->buf);
	}
	bool is_fin1()
	{
		RUDPMsgHdr * hdr = this->parse_hdr();
		if(hdr!=NULL&&hdr->type==MSG_TYPE_FIN1)
		{
			return true;
		}
		return false;
	}
	int unpack_data(unsigned char * dest, int max_size)
	{
		int ret=size-sizeof(RUDPMsgHdr);
		ret=ret>max_size?max_size:ret;
		if(buf==NULL)
		{
			ret=0;
		}
		else
		{
			memcpy(dest,buf+sizeof(RUDPMsgHdr),ret);
		}
		return ret;
	}
	RUDPMsgHdr * parse_hdr()
	{
		return (RUDPMsgHdr *)this->buf;

	}
	RUDPMsgHdr * make_msg(int type,unsigned int seq, unsigned char * body)
	{
		RUDPMsgHdr * hdr=(RUDPMsgHdr *)buf;
		memcpy(hdr->magic,rudp_magic,4);
		hdr->sequence=seq;
		hdr->size=this->size-sizeof(RUDPMsgHdr);
		hdr->type=type;
		if(body!=NULL)
			memcpy(hdr->body,body,hdr->size);
		return hdr;
	}
	~Packet()
	{
		delete[] buf;
	}
};

class Buffer{
private:
	pthread_mutex_t lock;
	Packet * packets;
	int size;//total size of bytes in buffers
	int maxVolume;
	void * owner;
public:
	void * getOwner();
	Packet * getPacket();
	ErrorCode putPacket(Packet * p);
	Buffer(void * owner,int maxVolume);
	~Buffer();

};
}



#endif /* RECEIVER_HPP_ */
