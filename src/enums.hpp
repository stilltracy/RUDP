/*
 * enums.hpp
 *
 *  Created on: Apr 24, 2014
 *      Author: stilltracy
 */

#ifndef ENUMS_HPP_
#define ENUMS_HPP_
namespace rudp{

enum class RConnState{
	VOID,LISTEN,CONNECTING,
	SYN_SENT,SYN_RECEIVED,ESTABLISHED,
	FIN_WAIT_1,FIN_WAIT_2,LAST_ACK,CLOSED
};
enum class ErrorCode{
	SUCCESS,
	TIME_OUT,
	BUFFER_FULL
};

}
#endif /* ENUMS_HPP_ */
