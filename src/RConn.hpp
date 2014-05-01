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
	static pthread_mutex_t lock_conn_count;
	static int conn_count;
	static int socket_fd;//all the connections share the same UDP socket
	static pthread_mutex_t lock_buffer_router;
	static map<in_addr_t, Buffer *> buffer_router;
	static pthread_mutex_t lock_listening_rconn;
	static RConn * listening_rconn;
	string ip;
	int port;
	in_addr_t remote_addr;
	static pthread_t t_receiver;
	static pthread_mutex_t lock_receiver_alive;
	static bool receiver_alive;
	RConnState state;
	Config * config;
	pthread_mutex_t lock_rx_buffer;
	Buffer * rx_buffer;
	pthread_mutex_t lock_sendable;
	bool sendable;
	pthread_mutex_t lock_recv_interrupted;
	bool recv_interrupted;
	unsigned int localSeq;
	unsigned int remoteSeq;
	void send_control_msg(RUDPMsgType type,unsigned int seq);
	void send_syn(unsigned int seq);
	void send_fin1(unsigned int seq);
	void send_fin2(unsigned int seq);
	ErrorCode recv_control_msg(RUDPMsgType type, unsigned int expected_seq);
	ErrorCode recv_control_msg(RUDPMsgType type, unsigned int expected_seq, unsigned int * p_remote_seq);
	ErrorCode recv_syn(unsigned int seq);
	ErrorCode recv_control_msg_timeout(RUDPMsgType type,unsigned int expected_seq,int timeout);
	ErrorCode recv_control_msg_timeout(RUDPMsgType type,unsigned int expected_seq,unsigned int * p_remote_seq,int timeout);
	ErrorCode recv_ack(unsigned int expected_seq);
	ErrorCode recv_fin2(unsigned int * remote_seq);
	int set_nonblocking();
	int set_blocking();
	bool check_timeout(__suseconds_t timeout, struct timeval start);
	void startReceiver();
	void endReceiver();
	static void * receiver(void * args);
	static void on_receiver_exit();
	void send_ack(unsigned int seq);
	void send_packet(Packet * p);
	Packet * recv_next_packet();
	Packet * recv_next_packet(RUDPMsgType type);
	void on_close(unsigned int seq);

	void disable_send();
	void disable_recv();
	bool is_sendable();
	void wipe_static_traces();
public:
	RConn();
	RConn(string ip,int port);
	virtual ~RConn();
	bool is_closed();
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
