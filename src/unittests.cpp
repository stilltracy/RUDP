/*
 * unittests.cpp
 *
 *  Created on: Apr 28, 2014
 *      Author: stilltracy
 */
#include "buffer.hpp"
#include <iostream>
using namespace rudp;
int main()
{
	Buffer * buffer=new Buffer(10);
	unsigned char wtf[4]="wtf";
	buffer->putPacket(wtf,4);
	std::cout<<buffer<<std::endl;
	return 0;
}



