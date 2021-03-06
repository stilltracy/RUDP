/*
 * rudp_server.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: stilltracy
 */

#include <iostream>
#include "RUDP.hpp"
using namespace std;
using namespace rudp;

RConn * conn=NULL;

void * wait_to_accept(void * args)
{
	int port =9527;
	cout<<"accepting thread is running!"<<endl;
	while(true)
	{
		conn=accept(port);
		if(conn==NULL)
			cout<<"accept failed!"<<endl;
		else
		{
			cout<<"accept success!"<<endl;
		}
	}
	return NULL;
}
int main() {
	cout << "RUDP test!" << endl; // prints RUDP test!
	int port =9527;
	//int status=0;
	if(bind(port)==-1)
	{
		perror("bind() failure.");
		return -1;
	}

	pthread_t t;
	pthread_create(&t,NULL,wait_to_accept,NULL);
	while(conn==NULL);
	cout<<"conn get created!"<<endl;

	while(true)
	{
		cout<<"waiting for remote to say something:"<<endl;
		unsigned char buf[10240];
		int size=conn->recv(buf,10240);
		cout<<"received. size="<<size<<endl;
		cout<<"remote:"<<buf<<endl;
		unsigned char msg[4096]="hello,dlrow!";
		cout<<"say something:"<<endl;
		cin>>msg;
		size=conn->send(msg,4096);
		cout<<"sent. size="<<size<<endl;
		cout<<"local:"<<msg<<endl;
	}
	return 0;
}


