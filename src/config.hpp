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
	int BUFFER_MAX_VOLUME;//maximum buffer of 10KB per connection
	unsigned int RECEIVER_BUFFER_MAX_VOLUME;//maximum buffer of 100KB for the receiver
	Config();
	~Config();

};
static Config * DEFAULT_CONFIG=new Config();

}
#endif /* CONFIG_HPP_ */
