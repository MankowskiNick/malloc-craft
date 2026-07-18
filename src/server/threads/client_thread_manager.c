#include "client_thread_manager.h"

#include "../../util/settings.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static pthread_key_t thread_socket_key;
static pthread_once_t thread_socket_key_once = PTHREAD_ONCE_INIT;

static void close_thread_socket(void *ptr) {
  int *fd_ptr = (int *)ptr;
  if (fd_ptr) {
    if (*fd_ptr >= 0)
      close(*fd_ptr);
    free(fd_ptr);
  }
}

static void init_thread_socket_key(void) {
  pthread_key_create(&thread_socket_key, close_thread_socket);
}

// Returns this thread's socket fd, connecting (or reconnecting) as needed.
int get_thread_fd(void) {
  pthread_once(&thread_socket_key_once, init_thread_socket_key);

  int *fd_ptr = (int *)pthread_getspecific(thread_socket_key);
  if (fd_ptr == NULL) {
    fd_ptr = malloc(sizeof(int));
    *fd_ptr = -1;
    pthread_setspecific(thread_socket_key, fd_ptr);
  }

  // Return the cached fd if it's valid.
  if (*fd_ptr >= 0) {
    return *fd_ptr;
  }

  // Connect (or reconnect after a previous failure or broken socket).
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "ERROR: Failed to create client socket\n");
    return -1;
  }

  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_port = htons(SERVER_PORT)};
  inet_pton(AF_INET, SERVER_HOST, &addr.sin_addr);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "ERROR: Failed to connect to chunk server\n");
    close(fd);
    return -1;
  }

  *fd_ptr = fd;
  return fd;
}
