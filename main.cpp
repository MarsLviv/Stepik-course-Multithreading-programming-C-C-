#include "server.h"
#include <fstream>

#include <iostream>			// asio HTTP Server 9.0
#include <cstdlib>			// 
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

int main(int argc, char* argv[]){//std::cout << "pid:" << getpid() << std::endl;
	// demonization
	pid_t pid;
	if ((pid = fork()) < 0){ std::cout << "fork() failed\n"; return 1; }
	if (pid != 0){  //	write pid to file
		std::ofstream ofile;
		ofile.open ("main.log", std::ofstream::out | std::ofstream::trunc);
		if ( (ofile.rdstate() & std::ifstream::failbit ) != 0 )
	    		std::cout << "Error opening main.log" << std::endl;
		std::string str = std::to_string(pid);
		ofile << "demonized with pid:" << str;
		ofile.close();
		_exit(0); 
	}// close parent
	close(0);	close(1);	close(2);
	if (setsid() == (pid_t)-1){ std::cout << "setsid() failed\n"; return 1; }/*  Creates a new session if the calling process
				 is not a process group leader. The calling process is the leader of the new session, 
				the process group leader of the new process group, and has no controlling terminal. 
				The process group ID and session ID of the calling process are set to the PID of the calling
			process. The calling process will be the only process in this new process group and in this new session.*/
	if (chdir("/") != 0){ std::cout << "chdir(/) failed\n"; return 1; }

	try {
		if (argc != 7){
			std::cerr << "Usage: -h <ip> -p <port> -d <directory>\n";
		return 1;
		}

		int rez=0;
		std::string user_ip, user_port, user_dir;
		while ( (rez = getopt(argc,argv,"h:p:d:")) != -1){
			switch (rez){
				case 'h': /*printf("found argument \"h = %s\".\n",optarg);*/ user_ip = optarg; break;
				case 'p': /*printf("found argument \"p = %s\".\n",optarg);*/ user_port = optarg; break;
				case 'd': /*printf("found argument \"d = %s\".\n",optarg);*/ user_dir = optarg; break;
				default: /*printf("getopt error found!\n");*/break;
			};
		};
		// checking corectness user_dir
		/*if (boost::filesystem::exists(user_dir)){
			//std::cout << "directory exist" << std::endl;
		} else {
			//std::cout << "directory not exist" << std::endl;
			return 1;
		}*/

		// Block all signals for background thread.
		sigset_t new_mask;
		sigfillset(&new_mask);// filling all with one's - block all signals
		sigset_t old_mask;
		pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

		server s(user_ip, user_port, user_dir);  //		run server
		boost::thread t(boost::bind(&server::run, &s));//	in separate thread

		pthread_sigmask(SIG_SETMASK, &old_mask, 0);// 		restore previous signals mask
    
		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);// filling with zeros - all signals not blocking
		sigaddset(&wait_mask, SIGINT);// add to mask one signal (SIGINT) - will be blocked
		sigaddset(&wait_mask, SIGQUIT);// ...
		sigaddset(&wait_mask, SIGTERM);// ... soft terminating
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
		int sig = 0;
		sigwait(&wait_mask, &sig);/*function suspends execution of the calling thread until one of the signals specified 
			in the wait_mask becomes pending. The function accepts the signal (removes it from the pending list 
			of signals), and returns the signal number in sig*/

		s.stop();// stop server
		t.join();// and it thread
	} catch (std::exception& e) {
		//std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}//CC=gcc-7 CXX=g++-7	https://stackoverflow.com/questions/45216648/update-g-but-still-old-version

