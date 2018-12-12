#ifndef SESS_H
#define SESS_H

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

class session{// responsible for one connection (client to server)
public:
	session(boost::asio::io_service& io_service, const std::string& folder);

	tcp::socket& socket();

	void start();
	
	void finish();
private:
	tcp::socket socket_;
	const std::string& folder_;
};

#endif // SESS_H
