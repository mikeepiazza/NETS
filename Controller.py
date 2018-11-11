import socket

bufferSize = 1500 # MTU is 1500B

serverIP = "192.168.0.1" # server address
localPort = 5002 # local port, same for clients

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
serverAddress = (serverIP, localPort) # server can only bind to self
sock.bind(serverAddress) # bind socket to address

while True:
    data, clientAddress = sock.recvfrom(bufferSize) # listen for packet
    print("Received Message: ", data)
    print("From: ", clientAddress)
    if (data == bytes(b'\x01')):
        udp_ip = "192.168.0.181" # recipient
        udp_port = 5002 # same for server/client
        message = bytes(b'\xFF\xFF') # data to send
        sock.sendto(message, clientAddress) # send datagram
        print("sent message: ", message)

