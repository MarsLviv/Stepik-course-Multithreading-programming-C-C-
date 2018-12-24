#include "server.h"

#include <iostream>			// asio HTTP Server 6.0
#include <cstdlib>			// 
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

	server::server(const std::string& address, const std::string& port, const std::string& folder) : 
			acceptor_(io_service_), folder_(folder) {
		boost::asio::ip::tcp::resolver resolver(io_service_);
		boost::asio::ip::tcp::resolver::query query(address, port);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		//boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(address), stoi(port));// not works???
		acceptor_.bind(endpoint);
		acceptor_.listen();
	
		start_accept();
	}

void server::start_accept(){
	session* new_session = new session(io_service_, folder_);
	acceptor_.async_accept(	new_session->socket(), // reference to socket
				boost::bind(&server::handle_accept, this, new_session, boost::asio::placeholders::error));
}

void server::handle_accept(session* new_session, const boost::system::error_code& error){
	if (!error){
		new_session->start();
	}else{
		delete new_session;
	}

	start_accept();
 }

void server::run(){
	const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;
	unsigned int thread_pool_size = std::thread::hardware_concurrency() * 2;
	if (thread_pool_size == 0)
			thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
	std::vector<boost::shared_ptr<boost::thread> > threads;// Create a pool of threads to run all of the io_services.
	for (std::size_t i = 0; i < thread_pool_size; ++i){
		boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
		threads.push_back(thread);
	}

	for (std::size_t i = 0; i < threads.size(); ++i)// Wait for all threads in the pool to exit.
		threads[i]->join();
}

void server::stop(){  io_service_.stop(); }


