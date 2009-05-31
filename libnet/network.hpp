//THREADSAFE, THREAD-SPAWNING
#ifndef H_NETWORKING
#define H_NETWORKING

//boost
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//include
#include <logger.hpp>
#include <reactor.hpp>
#include <reactor_select.hpp>

//std
#include <cstdlib>
#include <limits>
#include <map>
#include <queue>
#include <vector>

class network : private boost::noncopyable
{
public:
	//passed to call backs
	typedef reactor::socket_data::socket_data_visible socket;

	/* ctor parameters
	boost::bind is the nicest way to do a callback to the member function of a
	specific object. ex:
		boost::bind(&http::connect_call_back, &HTTP, _1, _2, _3)

	connect_call_back:
		Called when server connected to. The direction indicates whether we
		established connection (direction OUTGOING), or if someone established a
		connection to us (direction INCOMING). The send_buff is to this function
		in case something needs to be sent after connect.
	recv_call_back:
		Called after data received (received data in recv_buff). A return value of 
		true will keep the server connected, but false will cause it to be
		disconnected.
	send_call_back:
		Called after data sent. The send_buff will contain data that hasn't yet
		been sent. A return value of true will keep the server connected, but
		false will cause it to be disconnected.
	disconnect_call-back:
		Called when a connected socket disconnects.
	connect_time_out:
		Seconds before connection attempt times out.
	socket_time_out:
		Seconds before socket with no I/O times out.
	listen_port:
		Port to listen on for incoming connections. The default (if no parameter
		specified) is to not listen for incoming connections.
	*/
	network(
		boost::function<void (socket &)> failed_connect_call_back_in,
		boost::function<void (socket &)> connect_call_back_in,
		boost::function<void (socket &)> disconnect_call_back_in,
		const std::string port = "-1"
	):
		failed_connect_call_back(failed_connect_call_back_in),
		connect_call_back(connect_call_back_in),
		disconnect_call_back(disconnect_call_back_in)
	{
		//instantiate networking subsystem
		Reactor = boost::shared_ptr<reactor>(new reactor_select(port));

		//create workers to do call backs
		for(int x=0; x<8; ++x){
			Workers.create_thread(boost::bind(&network::worker_pool, this));
		}
	}

	~network()
	{
		Workers.interrupt_all();
		Workers.join_all();
	}

	bool connect_to(const std::string & IP, const std::string & port)
	{
		return Reactor->connect_to(IP, port);
	}

private:
	//reactor for networking
	boost::shared_ptr<reactor> Reactor;

	//worker threads to handle call backs
	boost::thread_group Workers;

	//call backs
	boost::function<void (socket &)> failed_connect_call_back;
	boost::function<void (socket &)> connect_call_back;
	boost::function<void (socket &)> disconnect_call_back;

	void worker_pool()
	{
		boost::shared_ptr<reactor::socket_data> info;
		while(true){
			boost::this_thread::interruption_point();
			Reactor->get_job(info);
			if(info->failed_connect_flag){
				failed_connect_call_back(info->Socket_Data_Visible);
			}else{
				if(!info->connect_flag){
					info->connect_flag = true;
					connect_call_back(info->Socket_Data_Visible);
				}
				assert(info->Socket_Data_Visible.recv_call_back);
				assert(info->Socket_Data_Visible.send_call_back);
				if(info->recv_flag){
					info->recv_flag = false;
					info->Socket_Data_Visible.recv_call_back(info->Socket_Data_Visible);
				}
				if(info->send_flag){
					info->send_flag = false;
					info->Socket_Data_Visible.send_call_back(info->Socket_Data_Visible);
				}
				if(info->disconnect_flag){
					disconnect_call_back(info->Socket_Data_Visible);
				}
			}
			Reactor->finish_job(info);
		}
	}
};
#endif
