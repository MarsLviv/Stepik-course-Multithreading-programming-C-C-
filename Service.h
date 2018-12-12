#ifndef SERV_H
#define SERV_H

#include <iostream>			// asio HTTP Server 5.0
#include <cstdlib>			// 
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <thread>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

class Service {// provide HTTP service
	tcp::socket & m_sock;
	boost::asio::streambuf m_request;
	std::string m_requested_resource;

	std::unique_ptr<char[]> m_resource_buffer;
	unsigned int m_response_status_code;
	std::size_t m_resource_size_bytes;
	std::string m_response_headers;
	std::string m_response_status_line;
	
	const std::map<unsigned int, std::string> http_status_table = {
		{ 200, "200 OK" },
		{ 404, "404 Not Found" },
	};
	enum { max_length = 1024 };
	char data_[max_length];
	const std::string& folder_;
public:
	Service(tcp::socket & socket_, const std::string& folder);
	void start_handling();
private:
	void on_request_line_received(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void process_request();
	void send_response();
	void on_response_sent(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void on_finish();
};

#endif // SERV_H
