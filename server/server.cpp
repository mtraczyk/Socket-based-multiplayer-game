#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "server.h"
#include "err.h"

void server(uint32_t portNum, int64_t seed, int64_t turningSpeed,
            uint32_t roundPerSecond, uint32_t boardWidth, uint32_t boardHeight) {
  int sock;  // socket descriptor
  struct sockaddr_in6 server_address;
  struct sockaddr_in6 client_address;
  socklen_t client_address_len;

#warning REMEMBER ABOUT SOCK_NONBLOCK
  sock = socket(AF_INET6, SOCK_DGRAM, 0);  // IPv6 UDP

  if (sock < 0) {
    syserr("socket");
  }

  server_address.sin6_family = AF_INET6; // IPv6
  server_address.sin6_addr = in6addr_any; // listening on all interfaces
  server_address.sin6_port = htons(portNum); // port number is stored in portNum

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &server_address, (socklen_t) sizeof(server_address)) < 0) {
    syserr("bind, address taken.");
  }

  client_address_len = sizeof(client_address);
}