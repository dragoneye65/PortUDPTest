/*
* This program will test if a UDP port is open and accessable from the internet
* -----------------------------------------------------------------------------
* This is what the program does:
* - Gets your public IP address from checkip.amazonaws.com
* - Creats a local server that listens on the specified udp port and wait for the message.
* - Sends a message to the public IP of the router and wait for a response from the server
* - If a response is received, the port is open and accessable from the internet
* - If no response is received, the port is closed and not accessable from the internet
* - The message sent is "Hello, server!"
* 
* Be sure to not run your game/application before running this program,
* if failure to do so, this test will not be conclusive as the port will be occupied
* 
*   Disclaimer: use at own risk!    (c) CMDR Tyroshious (@DragoneEye) January, 2025
*
* Usage: program <port> [-v|--verbose]
* Example: program 12345
* Example: program 12345 -v

    Known issues: Assertion: cannot dereference string iterator because the iterator was invalidated... 
    Been looking for it, can't find it. :-/
*/

#include <asio.hpp>
#include <iostream>
#include <array>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>


// yea yea I know, I know... globals bad bad, but this is a small program   =)
struct Globals {
    std::string router_ip;
    std::string public_ip;
	UINT16 port = 0;
	std::string sendmsg = "Hello, server!"; 
	std::string server_get_public_ip = "checkip.amazonaws.com";
	std::chrono::seconds timeout = std::chrono::seconds(5);
	bool port_open = false;
	bool verbose = false;   
};

Globals globs;

std::string trim(const std::string& str) {
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start)) {
        start++;
    }

    auto end = str.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return std::string(start, end + 1);
}

using asio::ip::udp;


class PublicIPFetcher {
public:
    explicit PublicIPFetcher(asio::io_context& io_context)
        : resolver_(io_context), socket_(io_context) {
    }

    std::string fetch() {
        try {
            asio::ip::tcp::resolver::query query( globs.server_get_public_ip, "http");
            auto endpoints = resolver_.resolve(query);

            asio::connect(socket_, endpoints);

            std::string request = "GET / HTTP/1.1\r\nHost: " + globs.server_get_public_ip + "\r\nConnection: close\r\n\r\n";
            socket_.send(asio::buffer(request));

            std::string response;
            std::array<char, 1024> buffer;
            asio::error_code error;
            while (socket_.available() || !error) {
                size_t bytes = socket_.read_some(asio::buffer(buffer), error);
                response.append(buffer.data(), bytes);
            }

            auto body_start = response.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                // std::string public_ip = response.substr(body_start + 4);
                //globs.public_ip = public_ip;
                std::string public_ip = response.substr(body_start + 4);
                return public_ip.substr(0, public_ip.find_first_of("\r\n"));
                // return response.substr(body_start + 4);
            }
        }
        catch (std::exception& e) {
            globs.router_ip = "Unknown";
            std::cerr << "Failed to fetch public IP: " << e.what() << " from " << globs.server_get_public_ip << std::endl;
        }
        return "Unknown";
    }

private:
    asio::ip::tcp::resolver resolver_;
    asio::ip::tcp::socket socket_;
};

class UdpEchoServer {
public:
    UdpEchoServer(asio::io_context& io_context, unsigned short port, std::atomic<bool>& response_received, std::atomic<bool>& should_exit)
        : socket_(io_context, udp::endpoint(udp::v4(), port)),
        response_received_(response_received),
        should_exit_(should_exit),
        io_context_(io_context),
        // timer_(io_context, std::chrono::seconds(30)) {
        timer_(io_context, globs.timeout) {
        start_receive();
        start_timer();
    }

private:
    void start_receive() {
        socket_.async_receive_from(
            asio::buffer(recv_buffer_), remote_endpoint_,
            [this](std::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    handle_receive(bytes_recvd);
                }
                else if (!should_exit_) {
                    start_receive();
                }
            });
    }

    void start_timer() {
        timer_.async_wait([this](const asio::error_code& error) {
            if (!error && !response_received_) {
                if (globs.verbose)
                    std::cout << "Timeout occurred. No response received within " << globs.timeout << " seconds.\n";
                should_exit_ = true;
                io_context_.stop();
            }
            });
    }

    void handle_receive(std::size_t bytes_recvd) {
        if (globs.verbose)
            std::cout << "Your Router's IP: " << remote_endpoint_.address().to_string() << std::endl;
        globs.router_ip = remote_endpoint_.address().to_string();

        std::string message(recv_buffer_.data(), bytes_recvd);
        socket_.async_send_to(
            asio::buffer(message), remote_endpoint_,
            [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
                response_received_ = true;
                should_exit_ = true;
                timer_.cancel();
                io_context_.stop();
            });
    }

    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    std::array<char, 1024> recv_buffer_;
    std::atomic<bool>& response_received_;
    std::atomic<bool>& should_exit_;
    asio::io_context& io_context_;
    asio::steady_timer timer_;
};

void send_udp_message(const std::string& server_ip, unsigned short port, const std::string& message) {
    try {
        asio::io_context io_context;
        udp::socket socket(io_context);
        socket.open(udp::v4());

        udp::endpoint server_endpoint(asio::ip::make_address(server_ip), port);

        socket.send_to(asio::buffer(message), server_endpoint);
        if (globs.verbose)
            std::cout << "Sent message: " << message << " to " << server_ip << ":" << port << "\n";

        std::array<char, 1024> recv_buffer;
        socket.async_receive_from(
            asio::buffer(recv_buffer), server_endpoint,
            [&](const asio::error_code& error, std::size_t bytes_transferred) {
                if (!error && globs.verbose) {
                    std::cout << "Received response: " << std::string(recv_buffer.data(), bytes_transferred) << std::endl;
                }
            });

        io_context.run_for(std::chrono::seconds(5));
    }
    catch (std::exception& e) {
        std::cerr << "Client exception: " << e.what() << std::endl;
    }
}

void usage( char* prog) {
    std::cout << "\nThis program will test if a UDP port is open and accessable from the internet\n";
    std::cout << "-----------------------------------------------------------------------------\n";
    std::cout << "It will send a message to the public IP of the router and wait for a response\n\n";
    std::cout << "  - If a response is received, the port is open and accessable from the internet\n";
    std::cout << "  - If no response is received, the port is closed and not accessable from the internet\n";
    std::cout << "\n";
    std::cout << "Be sure to not run your game/application before running this program, \n";
    std::cout << "if failure to do so, this test will not be conclusive as the port will be occupied\n" << std::endl;
    std::cout << "   Disclaimer: use at own risk!    (c) CMDR Tyroshious (@DragoneEye) January, 2025\n\n";
    std::cerr << "Usage: " << prog << " <port> [-v|--verbose]" << std::endl;
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
		usage( argv[0]);
		return 1;
	}

	bool port_set = false;
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-v" || arg == "--verbose") {
			globs.verbose = true;
		}
        else {
            unsigned short port = static_cast<unsigned short>(std::stoi(argv[i]));
            if (port > 0 && port < 65535) {
                globs.port = port;
				port_set = true;
            }
            else {
				globs.port = 0;
            }
        }
	}

	if (!port_set) {
		std::cerr << "\nInvalid port number. Exiting.\n";
        usage(argv[0]);
        return 1;
	}

    try {
        asio::io_context io_context;

        PublicIPFetcher ip_fetcher(io_context);
        std::string public_ip = ip_fetcher.fetch();

        if (globs.verbose)
            std::cout << "Public IP: " << public_ip << std::endl;

        if (public_ip == "Unknown" || public_ip.empty()) {
            std::cerr << "Failed to fetch a valid public IP address. Exiting.\n";
            return 1;
        }
		globs.public_ip = public_ip;

        std::atomic<bool> response_received(false);
        std::atomic<bool> should_exit(false);
		unsigned short port = globs.port;   // due to lambda capture issue, or capture globaly
        std::thread server_thread([&io_context, port, &response_received, &should_exit]() {
            UdpEchoServer server(io_context, port, response_received, should_exit);
            if (globs.verbose)
                std::cout << "Server is running on localhost port " << port << "...\n";
            io_context.run();
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

		// TODO: send message to public IP
        send_udp_message(public_ip, port, globs.sendmsg);

        // Wait for the server to finish (either by receiving a response or timing out)
        server_thread.join();

        if (response_received) {
            if (globs.verbose)
                std::cout << "Response received. Router IP: " << globs.router_ip << ", Public IP: " << globs.public_ip << std::endl;
			globs.port_open = true;
        }
        else {
            if (globs.verbose)
                std::cout << "No response received. " << std::endl;
			globs.port_open = false;
        }

        // Give the user the result to chew on...
        std::cout << "\n";

        if (!globs.verbose) {
            std::cout << "Public IP: " << globs.public_ip << std::endl;
            if (globs.port_open)
                std::cout << "Router IP: " << globs.router_ip << std::endl;
            std::cout << "\n";
        }

        std::cout << "Port " << globs.port << " is ";
        if (!globs.port_open) {
			std::cout << "closed and cannot be accessed from the internet\n" << std::endl;
        }
        else {
			std::cout << "open and accessable from the internet\n" << std::endl;
        }

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
