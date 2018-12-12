#include "Service.h"			// asio HTTP Server 5.0

#include <iostream>			
#include <cstdlib>			// 
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
using boost::asio::ip::tcp;

	Service::Service(tcp::socket & socket_, const std::string& folder) :
		m_sock(socket_), m_request(4096), m_response_status_code(200), m_resource_size_bytes(0), folder_(folder) {};

	void Service::start_handling() {
		//std::cout << "HTTP SERVER service started." << '\n';
		boost::asio::async_read_until(m_sock, m_request, "\r\n",
			[this](	const boost::system::error_code& ec, std::size_t bytes_transferred) {
				on_request_line_received(ec, bytes_transferred); }
		);
	};

	void Service::on_request_line_received(const boost::system::error_code& ec, std::size_t bytes_transferred){
		if (ec != 0) {
			/*std::cout << "Error occured in on_request_line_received(). Error code = " << ec.value() 
			<< ". Message: " << ec.message();*/
			on_finish();	return;
		}

		// Parse the request line.
		std::string request_line;
		std::istream request_stream(&m_request);//  m_request is streambuf with request line
		std::getline(request_stream, request_line, '\r');// Get line from stream into string; '\r' is optional delimiter
		// So removing symbol '\n' from the buffer.
		request_stream.get();// get()  Reads one character and returns it if available. ???
		// Otherwise, returns Traits::eof() and sets failbit and eofbit.
		//std::cout << "1:" << request_line << std::endl;

		// Parse the request line.
		std::string request_method;
		std::istringstream request_line_stream(request_line);// pass request_line to istringstream
		request_line_stream >> request_method;
		//std::cout << "2: request_method:" << request_method << std::endl;

		// support only GET method.
		if (request_method.compare("GET") != 0) {
			//std::cout << "Unsupported method. err code 501" << std::endl;
			on_finish();	return;
		}

		request_line_stream >> m_requested_resource;
		std::string delimiter = "?";
		m_requested_resource = m_requested_resource.substr(0, m_requested_resource.find(delimiter));
		//std::cout << "3: requested_resource:" << m_requested_resource << std::endl;

		std::string request_http_version;
		request_line_stream >> request_http_version;
		//std::cout << "4: request_http_version:" << request_http_version << std::endl;

		if (request_http_version.compare("HTTP/1.0") != 0) {
			//std::cout << "Unsupported HTTP version or bad request. err code = 505" << std::endl;
			on_finish();	return;
		}

		// At this point the request line is successfully received and parsed.
		// Now we have all we need to process the request.
		process_request();
		send_response();
		return;
	}

	void Service::process_request() {
		// Read file
		// trim first '/'
		if(m_requested_resource[0] =='/')
			m_requested_resource.erase(0, 1);
		std::string resource_file_path = m_requested_resource;

		// trim last '/'
		const char whitespace[] {"/"};
		const size_t last (folder_.find_last_not_of(whitespace));
		folder_.substr(0, (last + 1));

		// combine file_path
		std::string file_path;
		file_path += folder_ + "/" + resource_file_path;
		resource_file_path = file_path;

		if (!boost::filesystem::exists(resource_file_path)) {
			m_response_status_code = 404; // Resource not found.
			m_resource_size_bytes = 0;
			m_response_headers += std::string("Content-Length") + ": " + std::to_string(m_resource_size_bytes) + "\r\n";
			m_response_headers += std::string("Content-Type") + ": " + std::string("text/html") + "\r\n";
			return;
		}

		std::ifstream resource_fstream( resource_file_path, std::ifstream::binary);

		if (!resource_fstream.is_open()) {
			//std::cout << "Could not open file. err code = 500" << std::endl;
			return;
		}

		// Find out file size.
		resource_fstream.seekg(0, std::ifstream::end);// seekg - set position in input sequence
		m_resource_size_bytes =	static_cast<std::size_t>(resource_fstream.tellg());//tellg - get position in input sequence

		m_resource_buffer.reset(new char[m_resource_size_bytes]);

		resource_fstream.seekg(std::ifstream::beg);
		resource_fstream.read(m_resource_buffer.get(), m_resource_size_bytes);

		m_response_headers += std::string("Content-Length") + ": " + std::to_string(m_resource_size_bytes) + "\r\n";
		m_response_headers += std::string("Content-Type") + ": " + std::string("text/html") + "\r\n";
		//std::cout << "m_response_headers:" << m_response_headers << std::endl;
		//std::cout << "nnn" << std::endl;
	}

	void Service::send_response()  {// GET mf HTTP/1.0  GET /mf HTTP/1.0  GET mf?par=45 HTTP/1.0  GET /mf?par=45 HTTP/1.0
		m_sock.shutdown(boost::asio::ip::tcp::socket::shutdown_receive);// GET CMakeLists.txt HTTP/1.0

		auto status_line = http_status_table.at(m_response_status_code);

		m_response_status_line = std::string("HTTP/1.0 ") + status_line + "\r\n";
		//std::cout << "m_response_status_line: " << m_response_status_line << std::endl;

		m_response_headers += "\r\n";

		std::vector<boost::asio::const_buffer> response_buffers;
		response_buffers.push_back(boost::asio::buffer(m_response_status_line));

		if (m_response_headers.length() > 0) {//std::cout << "+1" << std::endl;
			response_buffers.push_back(boost::asio::buffer(m_response_headers));
		}

		if (m_resource_size_bytes > 0) {//std::cout << "+2" << std::endl;
			response_buffers.push_back(boost::asio::buffer(m_resource_buffer.get(), m_resource_size_bytes));
		}

		// Initiate asynchronous write operation.
		boost::asio::async_write(m_sock, response_buffers,
			[this](	const boost::system::error_code& ec, std::size_t bytes_transferred){
				on_response_sent(ec, bytes_transferred); }
		);
	}

	void Service::on_response_sent(const boost::system::error_code& ec, std::size_t bytes_transferred){
		if (ec != 0) {
			//std::cout << "Error occured! Error code = "<< ec.value()<< ". Message: " << ec.message();
		}

		m_sock.shutdown(boost::asio::ip::tcp::socket::shutdown_both);

		on_finish();
	}

	// Here we perform the cleanup.
	void Service::on_finish() {//std::cout << "on_finish" << std::endl;
		delete this;
	}
