#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "../err/err.h"

#define BUFFER_SIZE 1000
#define DECIMAL_BASIS 10

int main(int argc, char *argv[]) {
  int sock;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  char *end_ptr;
  int i, flags, sflags;
  char buffer[BUFFER_SIZE];
  ssize_t snd_len, rcv_len;
  struct sockaddr_in my_address;
  struct sockaddr_in srvr_address;
  socklen_t rcva_len;

  if (argc < 3) {
    fatal("Usage: %s host port message ...\n", argv[0]);
  }

  // 'converting' host/port in string to struct addrinfo
  (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_DGRAM;
  addr_hints.ai_protocol = IPPROTO_UDP;
  addr_hints.ai_flags = 0;
  addr_hints.ai_addrlen = 0;
  addr_hints.ai_addr = NULL;
  addr_hints.ai_canonname = NULL;
  addr_hints.ai_next = NULL;
  if (getaddrinfo(argv[1], NULL, &addr_hints, &addr_result) != 0) {
    syserr("getaddrinfo");
  }

  my_address.sin_family = AF_INET; // IPv4
  my_address.sin_addr.s_addr =
    ((struct sockaddr_in *) (addr_result->ai_addr))->sin_addr.s_addr; // address IP
  my_address.sin_port = htons((uint16_t) atoi(argv[2])); // port from the command line

  freeaddrinfo(addr_result);

  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    syserr("socket");

  uint32_t number_of_messages = strtol(argv[3], &end_ptr, DECIMAL_BASIS); // Number of different messages.

  uint32_t size_of_message = strtol(argv[4], &end_ptr, DECIMAL_BASIS); // Size is the same for all of the messages.

  char *message = malloc(size_of_message * sizeof(char));
  if (message == NULL) {
    syserr("memory allocation");
  }

  for (i = 0; i < number_of_messages; i++) {
    if (size_of_message >= BUFFER_SIZE) {
      (void) fprintf(stderr, "ignoring long parameter %d\n", i);
      continue;
    }

    scanf("%s", message); // Read a message.

    if (strlen(message) != size_of_message) {
      syserr("message length");
    }

    (void) printf("sending to socket: %s\n", message);
    sflags = 0;
    rcva_len = (socklen_t) sizeof(my_address);
    snd_len = sendto(sock, message, size_of_message, sflags, (struct sockaddr *) &my_address, rcva_len);
    if (snd_len != (ssize_t) size_of_message) {
      syserr("partial / failed write");
    }

    (void) memset(buffer, 0, sizeof(buffer));
    flags = 0;
    rcva_len = (socklen_t) sizeof(srvr_address);
    rcv_len = recvfrom(sock, buffer, size_of_message, flags, (struct sockaddr *) &srvr_address, &rcva_len);

    if (rcv_len < 0) {
      syserr("read");
    }
    (void) printf("read from socket: %zd bytes: %s\n", rcv_len, buffer);
  }

  free(message);

  if (close(sock) == -1) { //very rare errors can occur here, but then
    syserr("close"); //it's healthy to do the check
  }

  return 0;
}