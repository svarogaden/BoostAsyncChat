﻿
#include "pch.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <set> 
#include <iterator>


#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include<mutex>


using namespace std;


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

mutex mtx;

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}


class session;
set<shared_ptr<session>>Connects;
unsigned int request = 0;





// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
	websocket::stream<tcp::socket> ws_;
	net::strand<net::io_context::executor_type> strand_;
	beast::multi_buffer buffer_;


public:
	// Take ownership of the socket
	explicit
		session(tcp::socket socket)
		: ws_(std::move(socket))
		, strand_(ws_.get_executor())
	{
	}

	~session()
	{
		//Deleted connect
		Connects.erase(shared_from_this());
		cout << "Deleted connect" << endl;
	}

	// Start the asynchronous operation
	void
		run()
	{
		// Accept the websocket handshake
		ws_.async_accept(
			net::bind_executor(
				strand_,
				std::bind(
					&session::on_accept,
					shared_from_this(),
					std::placeholders::_1)));
	}

	void
		on_accept(beast::error_code ec)
	{
		if (ec)
			return fail(ec, "accept");

		//Add connect

		Connects.emplace(shared_from_this());



		// Read a message
		do_read();
	}

	void
		do_read()
	{
		// Read a message into our buffer
		ws_.async_read(
			buffer_,
			net::bind_executor(
				strand_,
				std::bind(
					&session::on_read,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));




	}


	void
		on_read(
			beast::error_code ec,
			std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// This indicates that the session was closed
		if (ec == websocket::error::closed)
		{
			//Delete connect
			Connects.erase(shared_from_this());
			return;
		}

		if (ec)
		{
			fail(ec, "read");
			Connects.erase(shared_from_this());
		}

		// Echo the message
		ws_.text(ws_.got_text());


		request++;
		std::cout << " buffer_size - " << boost::asio::buffer_size(buffer_.data()) << " thread id  - " << std::this_thread::get_id() << " request - " << request << std::endl;



		//Send  messege all;


		string message = beast::buffers_to_string(buffer_.data());
		for (auto session : Connects)
		{
			session->Send(message);
		}




	}

	void
		on_write(
			beast::error_code ec,
			std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
		{
			Connects.erase(shared_from_this());
			return fail(ec, "write");
		}

		// Clear the buffer
		buffer_.consume(buffer_.size());

		// Do another read
		do_read();
	}


	void Send(std::string const & ss)
	{
		if (!strand_.running_in_this_thread())
			return net::post(ws_.get_executor(), net::bind_executor(
				strand_,
				std::bind(
					&session::Send,
					shared_from_this(),
					ss)));


		ws_.async_write(
			buffer_.data(),
			net::bind_executor(
				strand_,
				std::bind(
					&session::on_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));




	}



};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
	tcp::acceptor acceptor_;
	tcp::socket socket_;

public:
	listener(
		net::io_context& ioc,
		tcp::endpoint endpoint)
		: acceptor_(ioc)
		, socket_(ioc)
	{
		beast::error_code ec;

		// Open the acceptor
		acceptor_.open(endpoint.protocol(), ec);
		if (ec)
		{
			fail(ec, "open");
			return;
		}

		// Allow address reuse
		acceptor_.set_option(net::socket_base::reuse_address(true), ec);
		if (ec)
		{
			fail(ec, "set_option");
			return;
		}

		// Bind to the server address
		acceptor_.bind(endpoint, ec);
		if (ec)
		{
			fail(ec, "bind");
			return;
		}

		// Start listening for connections
		acceptor_.listen(
			net::socket_base::max_listen_connections, ec);
		if (ec)
		{
			fail(ec, "listen");
			return;
		}
	}

	// Start accepting incoming connections
	void
		run()
	{
		if (!acceptor_.is_open())
			return;
		do_accept();
	}

	void
		do_accept()
	{
		acceptor_.async_accept(
			socket_,
			std::bind(
				&listener::on_accept,
				shared_from_this(),
				std::placeholders::_1));
	}

	void
		on_accept(beast::error_code ec)
	{
		if (ec)
		{
			fail(ec, "accept");
		}
		else
		{
			// Create the session and run it
			std::make_shared<session>(std::move(socket_))->run();
		}

		// Accept another connection
		do_accept();
	}
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{

	unsigned int count_system_thread = std::thread::hardware_concurrency();
	std::cout << "Count_system_thread - " << count_system_thread << std::endl;

	auto const address = boost::asio::ip::make_address("0.0.0.0");
	auto const port = static_cast<unsigned short>(1234);
	auto const threads = std::max<int>(1, count_system_thread);


	// The io_context is required for all I/O
	net::io_context ioc{ threads };

	// Create and launch a listening port
	std::make_shared<listener>(ioc, tcp::endpoint{ address, port })->run();

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back(
			[&ioc]
	{
		ioc.run();
	});
	ioc.run();
	////////////////////////////////////////////////////////////////







	return EXIT_SUCCESS;
}