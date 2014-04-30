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
int main() {
	cout << "RUDP test!" << endl; // prints RUDP test!
	string ip="127.0.0.1";
	int port =9526;
	int remote_port=9527;
	int status=0;
	if(bind(port)==-1)
	{
		perror("bind() failure.");
		return -1;
	}
	RConn * conn=connect(ip,remote_port,&status);
	if(conn==NULL)
		cout<<"connect failed!"<<endl;
	else
	{
		cout<<"connect success!"<<endl;
		unsigned char msg[13]="hello,world!";
		int size=conn->send(msg,13);
		cout<<"sent. size="<<size<<endl;
		close(conn);
		delete conn;
	}
	return 0;
}
