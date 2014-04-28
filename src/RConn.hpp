/*
 * RConn.h
 *
 *  Created on: Apr 24, 2014
 *      Author: stilltracy
 */

#ifndef RCONN_H_
#define RCONN_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <map>
#include "enums.hpp"
#include "buffer.hpp"
#include "rudp_msg.hpp"
#include "config.hpp"
namespace rudp {
using namespace std;

class RConn {
private:

	static int socket_fd;//all the connections share the same UDP socket
	static map<in_addr_t, Buffer *> buffer_router;
	static RConn * listening_rconn;

	string ip;
	int port;
	pthread_t t_receiver;
	//TODO: needs to be atomic
	static bool receiver_alive;
	struct sockaddr_in remote_addr;
	RConnState state;
	Config * config;
	Buffer * rx_buffer;

	int localSeq;
	int remoteSeq;
	static void initStatic();
	void send_syn(int seq);
	ErrorCode recv_msg(RUDPMsgType type, unsigned int expectedSeq, unsigned char * buf);
	ErrorCode recv_syn(unsigned int seq);
	ErrorCode recv_msg_timeout(RUDPMsgType type,unsigned int expectedSeq,int timeout);
	ErrorCode recv_syn_ack(unsigned int expectedSeq);
	int set_nonblocking();
	int set_blocking();
	bool check_timeout(int timeout, struct timeval start);
	void recvToConn();

	void startReceiver();
	void endReceiver();
	static void * receiver(void * args);
	void send_ack(int seq);
	void send_packet(Packet * p);
	Packet * recv_next_packet();
public:
	RConn();
	RConn(string ip,int port);
	virtual ~RConn();
	int send(char * buffer, int size);
	int recv(char * buffer, int size);
	void close();
	friend RConn * connect(string ip, int port, int * status);
	friend RConn * accept(int port);
	friend int bind(int port);

};

 RConn * connect(string ip, int port, int * status);
 RConn * accept(int port);
 int bind(int port);

} /* namespace rudp */



#endif /* RCONN_H_ */
