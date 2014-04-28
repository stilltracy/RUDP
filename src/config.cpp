/*
 * config.cpp
 *
 *  Created on: Apr 26, 2014
 *      Author: stilltracy
 */
#include "config.hpp"
namespace rudp{
Config::Config()
{
	CONNECT_TIMEOUT=1000000;
	BUFFER_MAX_VOLUME=10240;//maximum buffer of 10KB
	RECEIVER_BUFFER_MAX_VOLUME=102400;//maximum buffer of 100KB
}
Config::~Config()
{

}
}

