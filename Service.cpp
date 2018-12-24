#include "Service.h"			// asio HTTP Server 7.0

#include <iostream>	
#include <fstream>		
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
		boost::asio::async_read_until(m_sock, m_request, "\r\n",	//	boost::asio::streambuf m_request;
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
		
		boost::asio::streambuf::const_buffers_type bufs = m_request.data();
		std::string line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + m_request.size());
		//std::cout << "input lineON_START:" << line << ". With size:" << line.size() << std::endl;
		// handling only first line; rest cutting
		const char whitespace1[] {"\n"};
		size_t space1 = line.find_first_of(whitespace1);
		line = line.substr(0, space1);
		//std::cout << "input lineAFTER_CUTTING:" << line << ". With size:" << line.size() << std::endl;

		std::string request_method;	// HANDLING REQUEST
		const char whitespace[] {" "};
		const char whitespaceQ[] {"?"};
		const char whitespaceN[] {"10"};
		size_t space (line.find_first_not_of(whitespace));//	cutting spaces before line
		line = line.substr(space, line.size() - space);
		space = line.find_first_of(whitespace);
		request_method = line.substr(0, space);
		//std::cout << "request_method:" << request_method << std::endl;
		line = line.substr(space + 1, line.size() - space - 1);	// cutting request_method from line
		if (request_method.compare("GET") != 0) {		// support only GET method
			//std::cout << "Unsupported method. err code 501" << std::endl;
			on_response_sent(ec, bytes_transferred);	return;
		}

		// handling resource
		space = line.find_first_not_of(whitespace);//	cutting spaces before line
		line = line.substr(space, line.size() - space);
		space = line.find_first_of(whitespace);
		m_requested_resource = line.substr(0, space);
		line = line.substr(space + 1, line.size() - space - 1);	// cutting requested_resource from line
		space = m_requested_resource.find_first_of(whitespaceQ);	// cutting params
		//std::cout << "space:" << space << std::endl;
		m_requested_resource = m_requested_resource.substr(0, space);
		//std::cout << "m_requested_resource:" << m_requested_resource << std::endl;

		// handling HTTP protocol
		space = line.find_first_not_of(whitespace);//	cutting spaces before line
		line = line.substr(space, line.size() - space);
		std::string request_http_version;
		if (line.size() == 8)
			request_http_version = line;
		else if (line.size() < 8){
			//std::cout << "Unsupported HTTP version or bad request. err code = 505" << std::endl;
			on_response_sent(ec, bytes_transferred);	return;
		} else {
			space = line.find_last_of(whitespaceN);
			request_http_version = line.substr(0, space + 1);
		}
		if (request_http_version.compare("HTTP/1.0") != 0) {
			//std::cout << "Unsupported HTTP version or bad request. err code = 505" << std::endl;
			on_response_sent(ec, bytes_transferred);	return;
		}
		//std::cout << "request_http_version:" << request_http_version << std::endl;
		
		//on_response_sent(ec, bytes_transferred);
		
		// At this point the request line is successfully received and parsed.
		// Now we have all we need to process the request.
		process_request();
		send_response();
		return;
	}

bool Service::find_file(const boost::filesystem::path& dir_path, const boost::filesystem::path& file_name, 
				boost::filesystem::path& path_found) {
	const boost::filesystem::recursive_directory_iterator end;
	const auto it = std::find_if(boost::filesystem::recursive_directory_iterator(dir_path), end,
				[&file_name](const boost::filesystem::directory_entry& e) {
					return e.path().filename() == file_name;
				});
	if (it == end) {
		return false;
	} else {
		path_found = it->path();
		return true;
	}
}

	void Service::process_request() {
		// Read file
		// trim first '/' in file name
		if(m_requested_resource[0] =='/')
			m_requested_resource.erase(0, 1);
		std::string resource_file_path = m_requested_resource;
		// trim last '/' in file name
		const char whitespace[] {"/"};
		size_t last (resource_file_path.find_last_not_of(whitespace));
		resource_file_path = resource_file_path.substr(0, (last + 1));

		// trim last '/' in server's path
		last = folder_.find_last_not_of(whitespace);
		std::string folder = folder_;
		//std::cout << "folder1:" << folder << std::endl;
		folder = folder.substr(0, (last + 1));
		//std::cout << "folder2:" << folder << std::endl;
		

		// combine file_path
		std::string file_path;
		file_path += folder + "/" + resource_file_path;
		std::string tmp = resource_file_path; // need for recursing search
		resource_file_path = file_path;

		std::string rpath;//			recursive search
		const boost::filesystem::path myPath {folder};
		const boost::filesystem::path myFile {tmp};
		boost::filesystem::path myFound;
		bool recpath = false;
		if (find_file(myPath, myFile, myFound)){
			//std::cout << "I found this File :" << myFound.string() << std::endl;
			recpath = true;
		} else {
			//std::cout << "I don't found this File" << std::endl;
		}
		//std::cout << "recpath:" << recpath << std::endl;
		if (boost::filesystem::exists(resource_file_path)) {
			
		}else if (!recpath && !boost::filesystem::exists(resource_file_path)) {
			m_response_status_code = 404; // Resource not found.
			m_resource_size_bytes = 0;
			m_response_headers += std::string("Content-Length") + ": " + std::to_string(m_resource_size_bytes) + "\r\n";
			m_response_headers += std::string("Content-Type") + ": " + std::string("text/html") + "\r\n";
			return;
		}else if(recpath){
			resource_file_path = myFound.string();	
		}
		
		std::ifstream resource_fstream( resource_file_path, std::ifstream::binary);//?1

		if (!resource_fstream.is_open()) {
			//std::cout << "Could not open file. err code = 500" << std::endl;
			return;
		}

		// Find out file size.
		resource_fstream.seekg(0, std::ifstream::end);// seekg - set position in input sequence //?2
		m_resource_size_bytes =	static_cast<std::size_t>(resource_fstream.tellg());//tellg - get position in input sequence

		m_resource_buffer.reset(new char[m_resource_size_bytes]);

		resource_fstream.seekg(std::ifstream::beg);//?3
		resource_fstream.read(m_resource_buffer.get(), m_resource_size_bytes);

		m_response_headers += std::string("Content-Length") + ": " + std::to_string(m_resource_size_bytes) + "\r\n";
		m_response_headers += std::string("Content-Type") + ": " + std::string("text/html") + "\r\n";
		resource_fstream.close();
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
		//boost::system::error_code ec;
		m_sock.close( /*ec*/ );
		on_finish();
	}

	// Here we perform the cleanup.
	void Service::on_finish() {//std::cout << "on_finish" << std::endl;
		delete this;
	}
