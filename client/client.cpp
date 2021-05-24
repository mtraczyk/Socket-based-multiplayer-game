#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

#include "client.h"
#include "../err/err.h"

namespace {
  int getGuiSocket(std::string const &guiServer, uint16_t guiServerPort) {
    int sock, err;
    struct addrinfo addrHints;
    struct addrinfo *addrResult;

    // 'converting' host/port in string to struct addrinfo
    memset(&addrHints, 0, sizeof(struct addrinfo));
    addrHints.ai_family = AF_INET6; // IPv6
    addrHints.ai_socktype = SOCK_STREAM; // TCP
    addrHints.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo(guiServer.c_str(), std::to_string(guiServerPort).c_str(), &addrHints, &addrResult);
    if (err == EAI_SYSTEM) { // system error
      syserr("getaddrinfo: %s", gai_strerror(err));
    } else if (err != 0) { // other error (host not found, etc.)
      fatal("getaddrinfo: %s", gai_strerror(err));
    }

    // initialize socket according to getaddrinfo results
    sock = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
    if (sock < 0)
      syserr("socket");

    // connect socket to the server
    if (connect(sock, addrResult->ai_addr, addrResult->ai_addrlen) < 0)
      syserr("connect");

    freeaddrinfo(addrResult);

    return sock;
  }
}

void client(std::string const &gameServer, std::string const &playerName,
            uint16_t gameServerPort, std::string const &guiServer, uint16_t guiServerPort) {
  int guiSocket = getGuiSocket(guiServer, guiServerPort); // obtains socket for client - gui connection
  int udpSocket;
  struct addrinfo addrHints;
  struct addrinfo *addrResult;
  struct sockaddr_in myAddress;

  // 'converting' host/port in string to struct addrinfo
  (void) memset(&addrHints, 0, sizeof(struct addrinfo));
  addrHints.ai_family = AF_INET; // IPv4
  addrHints.ai_socktype = SOCK_DGRAM;
  addrHints.ai_protocol = IPPROTO_UDP;
  addrHints.ai_flags = 0;
  addrHints.ai_addrlen = 0;
  addrHints.ai_addr = NULL;
  addrHints.ai_canonname = NULL;
  addrHints.ai_next = NULL;
  if (getaddrinfo(gameServer.c_str(), NULL, &addrHints, &addrResult) != 0) {
    syserr("getaddrinfo");
  }

  myAddress.sin_family = AF_INET6; // IPv6
  myAddress.sin_addr.s_addr =
    ((struct sockaddr_in *) (addrResult->ai_addr))->sin_addr.s_addr; // address IP
  myAddress.sin_port = htons(gameServerPort); // port from the command line

  freeaddrinfo(addrResult);

  udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    syserr("socket");
  }

  (void) close(guiSocket);
  (void) close(udpSocket);
}