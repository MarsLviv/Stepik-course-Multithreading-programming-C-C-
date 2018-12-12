#include "session.h"
#include "Service.h"

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

	session::session(boost::asio::io_service& io_service, const std::string& folder) : socket_(io_service), folder_(folder) {  }

	tcp::socket& session::socket(){ return socket_; }

	void session::start(){// start reading in session
		(new Service(socket_, folder_))->start_handling();
	}
	
	void session::finish(){
		delete this;
	}
