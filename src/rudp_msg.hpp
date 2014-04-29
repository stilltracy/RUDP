/*
 * rudp_msg.hpp
 *
 *  Created on: Apr 25, 2014
 *      Author: stilltracy
 */

#ifndef RUDP_MSG_HPP_
#define RUDP_MSG_HPP_
#include <cstring>
namespace rudp{

typedef struct rudp_msg_hdr{
	__uint8_t magic[4];//Must be the ASCII code of 'R' 'U' 'D' 'P'
	__uint8_t type;
	__uint16_t size;//the size of body, excluding the msg_hdr
	__uint32_t sequence;
	__uint8_t body[0];
} RUDPMsgHdr;
typedef enum rudp_msg_type{
	MSG_TYPE_SYN,
	//MSG_TYPE_SYN_ACK,
	MSG_TYPE_ACK,
	MSG_TYPE_FIN1,
	MSG_TYPE_FIN2,
	MSG_TYPE_DATA
} RUDPMsgType;
const char rudp_magic[4]={'R','U','D','P'};
static inline bool check_magic(unsigned char * m)
{
	if(memcmp(m,rudp_magic,4)==0)
	{
		return true;
	}
	return false;
}
}
#endif /* RUDP_MSG_HPP_ */
