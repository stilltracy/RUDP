/*
 * config.hpp
 *
 *  Created on: Apr 24, 2014
 *      Author: stilltracy
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_
namespace rudp{
class Config{
public:

	/*the unit for timeouts is microsecond*/
	int CONNECT_TIMEOUT;
	int CLOSE_TIMEOUT;
	int RECV_REACK_TIMEOUT;
	int CONNECT_MAX_RETRIES;
	int ACCEPT_MAX_RETRIES;
	int TRANSMIT_MAX_RETRIES;

	int BUFFER_MAX_VOLUME;//maximum buffer of 10KB per connection
	unsigned int RECEIVER_BUFFER_MAX_VOLUME;//maximum buffer of 100KB for the receiver
	Config();
	~Config();

};
static Config * DEFAULT_CONFIG=new Config();

}
#endif /* CONFIG_HPP_ */
