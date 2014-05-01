/*
 * buffer.hpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#ifndef RECEIVER_HPP_
#define RECEIVER_HPP_
#include <cstdlib>
#include <string>
#include <pthread.h>
#include "enums.hpp"
#include "rudp_msg.hpp"
namespace rudp{
using namespace std;
class Packet{
private :
	bool is_of_type(RUDPMsgType type)
	{
		RUDPMsgHdr * hdr = this->parse_hdr();
		if(hdr!=NULL&&hdr->type==type)
		{
			return true;
		}
		return false;
	}
public:
	unsigned char * buf;
	int size;//the size of all the bytes that will be transmitted, including the header
	Packet * next;
	/*additional information about the sender (ip,port) */
	string ip;
	int port;
	Packet(int size)
	{
		this->size=size;
		this->next=NULL;
		this->buf=new unsigned char[size]();
		this->ip="";
		this->port=0;
	}
	Packet(unsigned char * buf, int size)
	{
		this->size=size;
		this->next=NULL;
		this->buf=new unsigned char[size]();
		memcpy(this->buf,buf,size);
		this->ip="";
		this->port=0;
	}
	bool validate()
	{
		return !this->size<4&&check_magic(this->buf);
	}
	bool is_syn()
	{
		return is_of_type(MSG_TYPE_SYN);
	}
	bool is_fin1()
	{
		return is_of_type(MSG_TYPE_FIN1);
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
	unsigned int size;//total size of bytes in buffers
	unsigned int maxVolume;
	void * owner;
public:
	void * getOwner();
	Packet * getPacket(RUDPMsgType,string ip, int port);
	Packet * getPacket(RUDPMsgType type);
	Packet * getPacket();
	ErrorCode putPacket(Packet * p);
	Buffer(void * owner,unsigned int maxVolume);
	~Buffer();

};
}



#endif /* RECEIVER_HPP_ */
