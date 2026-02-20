#include "world.h"
#include "chunk.h"
#include "chunk_io.h"
#include "../../server/server.h"
#include "../../server/compression/compression.h"
#include <hashmap.h>


#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define mod(x, y) fmod(x, y) < 0 ? fmod(x, y) + (y) : fmod(x,y)

// Per-thread socket: each thread gets its own connection to the chunk server.
// This eliminates lock contention — threads send/recv independently in parallel.
static pthread_key_t  thread_socket_key;
static pthread_once_t thread_socket_key_once = PTHREAD_ONCE_INIT;

static void close_thread_socket(void* ptr) {
    int* fd_ptr = (int*)ptr;
    if (fd_ptr) {
        if (*fd_ptr >= 0) close(*fd_ptr);
        free(fd_ptr);
    }
}

static void init_thread_socket_key(void) {
    pthread_key_create(&thread_socket_key, close_thread_socket);
}

// Returns this thread's socket fd, connecting (or reconnecting) as needed.
static int get_thread_socket(void) {
    pthread_once(&thread_socket_key_once, init_thread_socket_key);

    int* fd_ptr = (int*)pthread_getspecific(thread_socket_key);
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

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT)
    };
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "ERROR: Failed to connect to chunk server\n");
        close(fd);
        return -1;
    }

    *fd_ptr = fd;
    return fd;
}

// Mark this thread's socket as broken so the next get_thread_socket() reconnects.
static void reset_thread_socket(void) {
    int* fd_ptr = (int*)pthread_getspecific(thread_socket_key);
    if (fd_ptr && *fd_ptr >= 0) {
        close(*fd_ptr);
        *fd_ptr = -1;
    }
}

DEFINE_HASHMAP(chunk_map, chunk_coord, chunk*, chunk_hash, chunk_equals);
typedef chunk_map_hashmap chunk_map;
chunk_map chunks;

const char* get_worlds_dir(void) {
    return WORLDS_DIR;
}

void w_init() {
    c_init();

    chunks = chunk_map_init(CHUNK_CACHE_SIZE);

    // Initialize the worlds directory
    if (init_worlds_directory(WORLDS_DIR) == -1) {
        fprintf(stderr, "Warning: Failed to initialize worlds directory\n");
    }

    // Pre-connect the main thread so errors surface at startup.
    get_thread_socket();
}

void w_cleanup() {
    for (size_t i = 0; i < chunks.capacity; ++i) {
        chunk_map_entry* current = chunks.buckets[i];
        while (current) {
            chunk* c = current->value;

            if (c->modified) {
                if (chunk_save_to_disk(c, WORLDS_DIR) == -1) {
                    fprintf(stderr, "Warning: Failed to save chunk at (%d, %d)\n", c->x, c->z);
                }
            }

            free(c);  // Free the chunk pointer
            current = current->next;
        }
    }
    chunk_map_free(&chunks);

    // Close the main thread's socket. Worker thread sockets are closed by
    // the TLS destructor (close_thread_socket) when each thread exits.
    int* fd_ptr = (int*)pthread_getspecific(thread_socket_key);
    if (fd_ptr && *fd_ptr >= 0) {
        close(*fd_ptr);
        *fd_ptr = -1;
    }
}

chunk* get_chunk(int x, int z) {
    chunk_coord coord = {x, z};
    chunk** found = chunk_map_get(&chunks, coord);
    chunk* c = found ? *found : NULL;
    if (c == NULL) {
        // c = malloc(sizeof(chunk));

        // c->x = x;
        // c->z = z;
        // if (chunk_load_from_disk(c, WORLDS_DIR) == -1) {
        //     chunk_create(c, x, z);
        // }

        int fd = get_thread_socket();
        if (fd < 0) {
            fprintf(stderr, "ERROR: No server connection\n");
            return NULL;
        }

        chunk_request req = {
            .x = x,
            .z = z
        };

        if (send(fd, &req, sizeof(req), MSG_NOSIGNAL) < 0) {
            fprintf(stderr, "ERROR: Failed to send chunk request to server\n");
            reset_thread_socket();
            return NULL;
        }

        int packet_size = 0;
        int n1 = recv(fd, &packet_size, sizeof(int), MSG_WAITALL);
        if (n1 <= 0) {
            fprintf(stderr, "ERROR: Failed to receive chunk size from server\n");
            reset_thread_socket();
            return NULL;
        }

        byte* c_comp = malloc(packet_size);
        int n2 = recv(fd, c_comp, packet_size, MSG_WAITALL);
        if (n2 <= 0) {
            fprintf(stderr, "ERROR: Failed to receive chunk data from server\n");
            free(c_comp);
            reset_thread_socket();
            return NULL;
        }

        c = decompress_chunk(c_comp, packet_size);
        free(c_comp);

        chunk_map_insert(&chunks, coord, c);
    }
    return c;
}

chunk* get_chunk_at(float x, float z, int* chunk_x, int* chunk_z) {

    // adjust for negative coordinates
    if (x < 0) {
        x -= CHUNK_SIZE;
    }
    if (z < 0) {
        z -= CHUNK_SIZE;
    }
    // Calculate chunk coordinates
    int cx = (int)(x / CHUNK_SIZE);
    int cz = (int)(z / CHUNK_SIZE);

    chunk* c = get_chunk(cx, cz);

    // Calculate coordinate within chunk
    *chunk_x = (int)(mod(x, CHUNK_SIZE));
    *chunk_z = (int)(mod(z, CHUNK_SIZE));

    return c;
}
