/*
 * buffer.hpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#ifndef RECEIVER_HPP_
#define RECEIVER_HPP_
#include <cstdlib>
#include "enums.hpp"

#include "rudp_msg.hpp"
namespace rudp{
class Packet{
public:
	unsigned char * buf;
	int size;
	Packet * next;
	Packet(int size)
	{
		this->size=size;
		this->next=NULL;
		this->buf=new unsigned char[size];
	}
	RUDPMsgHdr * make_msg(int type,int seq, unsigned char * body)
	{
		RUDPMsgHdr * hdr=(RUDPMsgHdr *)buf;
		memcpy(hdr->magic,rudp_magic,4);
		hdr->sequence=seq;
		hdr->size=this->size-sizeof(RUDPMsgHdr);
		hdr->type=type;
		if(body!=NULL)
			memcpy(hdr->body,body,size);
		return hdr;
	}
	~Packet()
	{
		delete[] buf;
	}
};

class Buffer{
private:
	Packet * buffers;
	int size;//total size of bytes in buffers
	int maxVolume;
public:
	Packet * getPacket();
	ErrorCode putPacket(unsigned char * buf, int size);
	Buffer(int maxVolume);
	~Buffer();

};
}



#endif /* RECEIVER_HPP_ */
