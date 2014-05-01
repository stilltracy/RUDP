/*
 * receiver.cpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#include "buffer.hpp"
#include <iostream>
namespace rudp{
Buffer::Buffer(void * owner,unsigned int maxVolume)
{
	this->lock=PTHREAD_MUTEX_INITIALIZER;
	this->packets=NULL;
	this->size=0;
	this->maxVolume=maxVolume;
	this->owner=owner;
}
Buffer::~Buffer()
{
	Packet *p =this->packets;
	Packet *last=NULL;
	while(p!=NULL)
	{
		last=p;
		p=p->next;
		delete last;
	}
}
void * Buffer::getOwner()
{
	return this->owner;
}
Packet * Buffer ::getPacket(RUDPMsgType type,string ip, int port)
{
	pthread_mutex_lock(&this->lock);
	Packet * p=this->packets;
	Packet * prev=NULL;
	while(p!=NULL)
	{
		RUDPMsgHdr * h=p->parse_hdr();
		if(h!=NULL&&h->type==type&&p->ip==ip&&p->port==port)
		{
			this->size-=p->size;
			if(prev!=NULL)
				prev->next=p->next;
			else
				this->packets=p->next;
			break;
		}
		prev=p;
		p=p->next;
	}
	pthread_mutex_unlock(&this->lock);
	return p;
}
Packet * Buffer::getPacket(RUDPMsgType type)
{
	pthread_mutex_lock(&this->lock);
	Packet * p=this->packets;
	Packet * prev=NULL;
	while(p!=NULL)
	{
		RUDPMsgHdr * h=p->parse_hdr();
		if(h!=NULL&&h->type==type)
		{
			this->size-=p->size;
			if(prev!=NULL)
				prev->next=p->next;
			else
				this->packets=p->next;
			break;
		}
		prev=p;
		p=p->next;
	}
	pthread_mutex_unlock(&this->lock);
	return p;
}
Packet * Buffer::getPacket()
{
	return getPacket(MSG_TYPE_DATA);
}
/* this method is responsible for freeing nb*/
ErrorCode Buffer::putPacket(Packet * nb)
{
	pthread_mutex_lock(&this->lock);
	ErrorCode code=ErrorCode::SUCCESS;

	if(this->size+nb->size>this->maxVolume)
	{
		code=ErrorCode::BUFFER_FULL;
		delete nb;
	}
	else
	{
		/*insert into the tail of the buffer list*/
		if(this->packets==NULL)
		{
			this->packets=nb;
			this->size+=nb->size;
		}
		else
		{
			Packet * p=this->packets;
			for(;p->next!=NULL;p=p->next);
			p->next=nb;
			this->size+=nb->size;
		}
	}
	pthread_mutex_unlock(&this->lock);
	return code;
}

}



