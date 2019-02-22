#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include <chrono>

using boost::asio::ip::tcp;

int numChannelsToTest = 1;

int sendCommand(const char* ip, const char* port, const char* command, size_t command_length)
{
  try
  {
    boost::asio::io_service io_service;

    tcp::socket s(io_service);
    tcp::resolver resolver(io_service);
    boost::asio::connect(s, resolver.resolve({ip, port}));

    boost::asio::write(s, boost::asio::buffer(command, command_length));
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

int main()
{
	const char* ipTable[] = {"192.168.0.2", "192.168.0.3", "192.168.0.4", "192.168.0.5", "192.168.0.6"};
	std::cout << "NETS UI V1.0\n";
	std::cout << "Press Enter to begin test...";
	std::cin.get();

	sendCommand("127.0.0.1", "13", "timestamp", 10); // send message to tcp server to hack timestamp
	
	// cycle through ip addresses and initiate test
	for (int i = 0; i < numChannelsToTest; i++)
	{
		const char* ip = ipTable[i];
		char port[] = "13";
		char command[] = {'t'};
    	size_t command_length = std::strlen(command);
		sendCommand(ip, port, command, command_length);
	}
	return 0;
}
