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
	int port =9527;
	int status=0;
	RConn * conn=connect(ip,port,&status);
	if(conn==NULL)
		cout<<"connect failed!"<<endl;
	else
		cout<<"connect success!"<<endl;
	return 0;
}
