//============================================================================
// Name        : RUDP.cpp
// Author      : Yuanfeng Peng
// Version     :
// Copyright   : All rights reserved.
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "RConn.hpp"
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
	string ip="158.130.24.207";
	int port =9526;
	int remote_port=9527;
	int status=0;
	if(bind(port)==-1)
	{
		perror("bind() failure.");
		return -1;
	}

	pthread_t t;
	pthread_create(&t,NULL,wait_to_accept,NULL);

	RConn * conn=connect(ip,remote_port,&status);
	if(conn==NULL)
		cout<<"connect failed!"<<endl;
	else
	{
		cout<<"connect success!"<<endl;
		unsigned char msg[4096]="hello,world!";
		int size=conn->send(msg,4096);
		cout<<"sent. size="<<size<<endl;
		unsigned char buf[10240];
		size=conn->recv(buf,10240);
		cout<<"received. size="<<size<<endl;
		cout<<buf<<endl;
		size=conn->recv(buf,10240);
		cout<<"received. size="<<size<<endl;
		cout<<buf<<endl;
		size=conn->send(msg,4096);
		cout<<"sent. size="<<size<<endl;
		size=conn->recv(buf,10240);
		cout<<"received. size="<<size<<endl;
		cout<<buf<<endl;
		string wtf;
		cin>>wtf;
		//close(conn);
		delete conn;
	}
	return 0;
}
