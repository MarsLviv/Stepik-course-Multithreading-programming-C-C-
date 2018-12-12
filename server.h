#ifndef SERVER_H
#define SERVER_H

#include "session.h"
#include <iostream>			// asio HTTP Server 5.0
#include <cstdlib>			// 
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

class server{// handling all connections
public:
	server(const std::string& address, const std::string& port, const std::string& folder);
private:
	void start_accept();

	void handle_accept(session* new_session, const boost::system::error_code& error);
public:
	void run();

	void stop();
private:
	boost::asio::io_service io_service_;
	tcp::acceptor acceptor_;
	const std::string& folder_;
};

#endif // SERVER_H
