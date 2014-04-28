/*
 * receiver.cpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#include "buffer.hpp"
namespace rudp{
Buffer::Buffer(int maxVolume)
{
	this->buffers=NULL;
	this->size=0;
	this->maxVolume=maxVolume;
}
Buffer::~Buffer()
{

}
//TODO: needs to be atomic
Packet * Buffer::getPacket()
{
	Packet * p=NULL;
	if(this->buffers!=NULL)
	{
		p=this->buffers;
		this->buffers=p->next;
	}
	return p;
}
//TODO: needs to be atomic
ErrorCode Buffer::putPacket(unsigned char * buf, int size)
{
	ErrorCode code=ErrorCode::SUCCESS;
	if(this->size+size>this->maxVolume)
	{
		code=ErrorCode::BUFFER_FULL;
	}
	else
	{
		/*insert into the head of the buffer list*/

		Packet * nb=new Packet(size);
		memcpy(nb->buf,buf,size);
		nb->next=this->buffers;
		this->buffers=nb;
	}
	return code;
}
}



