/*
 * RUDP.hpp
 *
 *  Created on: May 1, 2014
 *      Author: stilltracy
 */

#ifndef RUDP_HPP_
#define RUDP_HPP_
#include <string>
#include "RConn.hpp"
namespace rudp{
using namespace std;
 RConn * connect(string ip, int port, int * status);
 RConn * accept(int port);
 int bind(int port);
 void close(RConn * rconn);


}
#endif /* RUDP_HPP_ */
