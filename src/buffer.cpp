/*
 * receiver.cpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#include "buffer.hpp"
#include <iostream>
namespace rudp{
Buffer::Buffer(void * owner,int maxVolume)
{
	this->lock=PTHREAD_MUTEX_INITIALIZER;
	this->packets=NULL;
	this->size=0;
	this->maxVolume=maxVolume;
	this->owner=owner;
}
Buffer::~Buffer()
{

}
void * Buffer::getOwner()
{
	return this->owner;
}
Packet * Buffer::getPacket()
{
	pthread_mutex_lock(&this->lock);
	Packet * p=NULL;
	if(this->packets!=NULL)
	{
		p=this->packets;
		this->packets=p->next;
	}
	pthread_mutex_unlock(&this->lock);
	return p;
}

ErrorCode Buffer::putPacket(Packet * nb)
{
	pthread_mutex_lock(&this->lock);
	ErrorCode code=ErrorCode::SUCCESS;

	if(this->size+nb->size>this->maxVolume)
	{
		code=ErrorCode::BUFFER_FULL;
	}
	else
	{
		/*insert into the tail of the buffer list*/
		if(this->packets==NULL)
			this->packets=nb;
		else
		{
			Packet * p=this->packets;
			for(;p->next!=NULL;p=p->next);
			p->next=nb;
		}
	}
	pthread_mutex_unlock(&this->lock);
	return code;
}

}



