/*
 * rudp_server.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: stilltracy
 */

#include <iostream>
#include "RConn.hpp"
using namespace std;
using namespace rudp;

int main() {
	cout << "RUDP test!" << endl; // prints RUDP test!
	string ip="127.0.0.1";
	int port =9527;
	int status=0;
	if(bind(port)==-1)
	{
		perror("bind() failure.");
		return -1;
	}
	RConn * conn=accept(port);
	if(conn==NULL)
		cout<<"accept failed!"<<endl;
	else
		cout<<"accept success!"<<endl;
	return 0;
}


