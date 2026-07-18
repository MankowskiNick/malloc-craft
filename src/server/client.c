#include <server/client.h>

#include <server/models.h>
#include <util.h>

#include "compression/compression.h"
#include "threads/client_thread_manager.h"

#include <stdio.h>

// TODO: convert this to use send_data
void send_chunk_request(int fd, int x, int z) {
  if (!send_msg_header(CHUNK_REQ, fd)) {
    return;
  }

  chunk_request req = {.x = x, .z = z};
  if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) < 0) {
    fprintf(stderr, "ERROR: Failed to send chunk request to server\n");
    return;
  }

  return;
}

byte *receive_chunk(int fd, int *packet_size) {
  if (recv(fd, packet_size, sizeof(int), MSG_WAITALL) != sizeof(int) ||
      *packet_size <= 0) {
    printf("ERROR: Failed to receive chunk size from server\n");
    return NULL;
  }

  byte *compressed_chunk = malloc(*packet_size);
  if (compressed_chunk == NULL) {
    printf("ERROR: Could not allocate buffer for compressed chunk\n");
    return NULL;
  }
  if (recv(fd, compressed_chunk, *packet_size, MSG_WAITALL) != *packet_size) {
    printf("ERROR: Could not receive compressed chunk from server (fd %i)\n",
           fd);
    free(compressed_chunk);
    return NULL;
  }

  return compressed_chunk;
}

chunk *request_chunk(int x, int z) {
  int fd = get_thread_fd();
  send_chunk_request(fd, x, z);

  int packet_size = -1;
  byte *compressed_chunk = receive_chunk(fd, &packet_size);

  if (compressed_chunk == NULL || packet_size < 0) {
    printf("ERROR: Could not receive chunk from server.\n");
    return NULL;
  }

  chunk *c = decompress_chunk(compressed_chunk, packet_size);
  free(compressed_chunk);
  return c;
}

void send_compressed_chunk(int fd, byte *data, int size) {
  if (!send_msg_header(CHUNK_UPDATE, fd)) {
    return;
  }

  if (!send_data(fd, &size, sizeof(int)) || !send_data(fd, data, size)) {
    printf("ERROR: Failed to send chunk to client (fd %d)\n", fd);
  }
}

void send_chunk_to_server(chunk *c) {
  int fd = get_thread_fd();

  int packet_size = -1;
  byte *compressed_chunk = compress_chunk(c, &packet_size);

  send_compressed_chunk(fd, compressed_chunk, packet_size);

  free(compressed_chunk);
}
