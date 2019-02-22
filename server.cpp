// V1.0 successfully server time to Arduino

// V1.1 successful test of Arduino TCP/UDP and packet rate (2701 from Arduino perspective)

// V1.2 stores UDP stream to file on desktop

// V1.3 performs rudimentary timestamping calculation on UDP stream, both ends measured within 1 packet of 2855 (per second)

// V1.4 sends initial configuration of node via TCP, reads from config.txt



// to compile:

// g++ server.cpp -o server -lboost_system -lpthread



// to run:

// ./server



#include <ctime>

#include <chrono>

#include <iostream>

#include <string>

#include <boost/array.hpp>

#include <boost/bind.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/enable_shared_from_this.hpp>

#include <boost/asio.hpp>

#include <fstream>

#include <algorithm>



using namespace boost::asio;

using ip::tcp;

using ip::udp;

using std::cout;

using std::endl;



std::ofstream dataFile; // data file will be saved locally

int numUDP = 0; // number of received UDP packets

char * memBlock; // dynamically allocated block to store config file

std::streampos size; // size of config file

uint64_t begin; // for testing

int numTestPackets = 200000; // number of packets to test

const int numChannelsToTest = 5; // num channels to test

int num[numChannelsToTest]; // num udp packets rx'd per channel



uint64_t end[numChannelsToTest]; // last packet received time

uint64_t latencyMax[numChannelsToTest]; // difference between last packet and now



std::string make_daytime_string() 

{

	using namespace std; // For time_t, time and ctime;

	time_t now = time(0);

	return ctime(&now);

}



class tcp_connection : public boost::enable_shared_from_this<tcp_connection> // kept alive as long as an operation refers to it

{ 

public:

	tcp_connection(boost::asio::io_service& io_service) : socket_(io_service) { }

	tcp::socket& socket() { return socket_; }

	void start() // calls read/write handlers

	{

		socket_.async_read_some(boost::asio::buffer(data_, macSize_), boost::bind(&tcp_connection::handle_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

	}

	typedef boost::shared_ptr<tcp_connection> pointer;

	static pointer create(boost::asio::io_service& io_service) { return pointer(new tcp_connection(io_service)); }	

private:

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred)

	{

    		if (!error)

    		{

				if (data_[0] == 't') // for testing DELETE when done!!!!!!!!!!!!!!!!!!!

				{

					auto duration = std::chrono::system_clock::now().time_since_epoch();

					begin = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();



				}

				else

				{

					bool matchFlag; // a match has been found when true

					for(int j = 0; j < size / rowSize_; j++) // j represents row in text file

					{

						matchFlag = true; // assume line matches until proven false

						int i; // value will be used outside i loop

						for (i = 0; i < macSize_; i++) // i represents column in text file

						{

							

							if (data_[i] != memBlock[i + j * rowSize_]) // not a match

							{

								matchFlag = false; // set flag to false

								break; // exit i loop

							}

						}

					

						if (matchFlag == true) // found mac address in text file

						{

							//cout << "Found a match.";

							

							std::string channelASCII = std::string(&memBlock[i + j * rowSize_ + 1], 3); // concatenate ASCII char

							int channel = stoi(channelASCII); // convert to int

							

							

							std::string groupASCII = std::string(&memBlock[i + j * rowSize_ + 5], 3); // concatenate ASCII char

							int group = stoi(groupASCII); // convert to int

							//cout << "Channel: " << channel << " Group: " << group;

							if (channel > 255 || group > 255) // incorrect assignemnt value, change to spare

							{

								//cout << "Out of range.";

								matchFlag = false; // set flag to false

								break; // exit j loop

								

							}

							data_[0] = channel; // assign channel

							data_[1] = group; // assign group

							break; // exit j loop

						}

					}

			

				if (matchFlag == false) // did not find match for mac address in text file, assign as spare

				{

					cout << "Assigned as spare.";

					data_[0] = 0;

					data_[1] = 0;

				}

				const int numBytesToSend = 2; // bytes for channel and group			

				socket_.async_write_some(boost::asio::buffer(data_, numBytesToSend), boost::bind(&tcp_connection::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)); // used to initiate write call

    		}

		}

		else

    	{

      		std::cerr << "Error: " << error.message() << std::endl;

			socket_.close();

    	}

  	}

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred)

	{

		if (!error)

    		{

			std:: cout << "Channel: " << int(data_[0]) << " group: " << int(data_[1]);

			

			std::cout << endl;

       		}

    		else

    		{

      			std::cerr << "Error: " << error.message() << std::endl;

			socket_.close();

    		}

	}

	tcp::socket socket_;

	static const int macSize_ = 17; // num bytes in mac address (from text file)

	static const int rowSize_ = 26; // size of row from text file

	char data_[tcp_connection::macSize_]; // data buffer



};



class tcp_server 

{

public:

	tcp_server(boost::asio::io_service& io_service) : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13)) // constructor initializes acceptor to listen on TCP port 13

	{

		start_accept(); // call start accept function

	}

private:

	void start_accept() // creates socket and initializes asynchronous accept operation to wait for new connection

	{

		tcp_connection::pointer new_connection = tcp_connection::create(acceptor_.get_io_service());

		acceptor_.async_accept(new_connection->socket(), boost::bind(&tcp_server::handle_accept, this, new_connection, boost::asio::placeholders::error));

	}

	void handle_accept(tcp_connection::pointer new_connection, const boost::system::error_code& error) // called when accept operation intiated by start_accept() finishes; services client request, then calls start_accept() to initiate next accept

	{

		if (!error) { new_connection->start(); }

		start_accept();

	}

	tcp::acceptor acceptor_;

};



class udp_server

{

public:

	udp_server(boost::asio::io_service& io_service) : socket_(io_service, udp::endpoint(udp::v4(), 13)) { start_receive(); } // intializes socket to listen on UDP port 13

private:

	void start_receive() // async_receive_from() causes application to listen for new request in background, loads recv_buffer_

	{ 

		socket_.async_receive_from( boost::asio::buffer(recv_buffer_), remote_endpoint_, boost::bind(&udp_server::handle_receive, this, boost::asio::placeholders::error));

	}

	void handle_receive(const boost::system::error_code& error)

	{ // services client request, called by io_service object when request is received

		if (!error || error == boost::asio::error::message_size) // error contains result of async operation, can ignore size error

		{

			auto duration = std::chrono::system_clock::now().time_since_epoch(); // time hack

			uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

			

			for (int i = 1; i <= numChannelsToTest; i++)

			{

				if (recv_buffer_[0] == i) // channel matches i

				{	

					if (++num[i] == 1) // first packet this channel

					{

						std::cout << "Channel: " << recv_buffer_[0] << " initial latency: " << micros - begin << " us.\n";

						begin = micros; // DELETE when done!!!!!!!!!!

					}

					else // remaining packets

					{

						uint64_t newLatency = micros - end[i];

						if (latencyMax[i] < newLatency) latencyMax[i] = newLatency;

						dataFile << recv_buffer_[0] << newLatency << '\n'; // write channel id and new latency to file

					}

					

					end[i] = micros; // update last packet time



					if (num[i] == numTestPackets)

					{

						std::cout << "Elapsed time: " << micros - begin << " us." << std::endl;

						uint64_t packetRate = numTestPackets / ((micros - begin) / 1000000); // rate calc

						std::cout << "Packet rate for channel " << recv_buffer_[0] << " was " << packetRate << " Hz" << std::endl;

						std::cout << "Max latency was: " << latencyMax[i] << std::endl;

						std::cout << num[i] << " packets were tested." << std::endl;

					}

				}

			}

  			// output channelID and received data to file

			//for (int i = 0; i < 9; i++) dataFile << recv_buffer_[i]; // write contents to file

			//dataFile << std::endl;

			

			start_receive(); // start listening for next client request

		}

		else

    	{

      		std::cerr << "Error: " << error.message() << std::endl;

    	}

	}

	void handle_send(boost::shared_ptr<std::string> /*message*/) { } // handles further actions for client request

	udp::socket socket_;

	udp::endpoint remote_endpoint_;

	boost::array<char, 9> recv_buffer_; // X-byte receive buffer

};



int main()

{	

	std::ifstream configFile("/home/pi/Desktop/config.txt", std::ios::in|std::ios::binary|std::ios::ate); // open config file, start at end

	if (configFile.is_open())

	{

		size = configFile.tellg(); // size equal to last byte index

		memBlock = new char [size]; // reserve that size of memory

		configFile.seekg(0, std::ios::beg); // set get position to beginning of file

		configFile.read(memBlock, size); // read contents into memory

		configFile.close(); // close file



		std::cout << "Configuration file stored in memory.\n";

	}

	else

	{

		std::cout << "Unable to open configuration file.";

		return 1;

	}



	std::string daytimeString = make_daytime_string(); // to generate file name

	std::string fileName = "/home/pi/Desktop/" + daytimeString + ".txt";

	dataFile.open(fileName, std::ios::app); // open file to store UDP data



	try

	{

		boost::asio::io_service io_service; // required for Boost to instantiate services

		tcp_server server1(io_service); // instantiate tcp service

		udp_server server2(io_service); // instantiate udp service

		io_service.run(); // run both services, blocking until complete

	}

	catch (std::exception& e) { std::cerr << e.what() << std::endl;	}



	delete[] memBlock; // deallocate memory block



	dataFile.close(); // close UDP file



	return 0;

}

