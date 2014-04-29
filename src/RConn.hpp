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
	static pthread_mutex_t lock_buffer_router;
	static map<in_addr_t, Buffer *> buffer_router;
	static pthread_mutex_t lock_listening_rconn;
	static RConn * listening_rconn;

	string ip;
	int port;
	static pthread_t t_receiver;
	static pthread_mutex_t lock_receiver_alive;
	static bool receiver_alive;
	RConnState state;
	Config * config;
	pthread_mutex_t lock_rx_buffer;
	Buffer * rx_buffer;
	pthread_mutex_t lock_sendable;
	bool sendable;
	unsigned int localSeq;
	unsigned int remoteSeq;
	static void initStatic();
	void send_syn(int seq);
	void send_fin1(int seq);
	void send_fin2(int seq);
	ErrorCode recv_msg(RUDPMsgType type, unsigned int expectedSeq, unsigned char * buf);
	ErrorCode recv_syn(unsigned int seq);
	ErrorCode recv_msg_timeout(RUDPMsgType type,unsigned int expectedSeq,int timeout);
	ErrorCode recv_ack(unsigned int expectedSeq);
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
	void on_close(int seq);
	void disable_sender();
	bool is_sendable();
public:
	RConn();
	RConn(string ip,int port);
	virtual ~RConn();
	int send(unsigned char * buffer, int size);
	int recv(unsigned char * buffer, int size);
	friend void close(RConn * rconn);
	friend RConn * connect(string ip, int port, int * status);
	friend RConn * accept(int port);
	friend int bind(int port);

};

 RConn * connect(string ip, int port, int * status);
 RConn * accept(int port);
 int bind(int port);
 void close(RConn * rconn);

} /* namespace rudp */



#endif /* RCONN_H_ */
