#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "BufferTypes.h"


Buffer buffer_init(size_t size) {
    Buffer buf;

    buf.array = malloc(size * sizeof(BufferElement));
    buf.head = 0;
    buf.tail = 0;

    buf.max_value = size;
    buf.size = 0;

    return buf;
}


void push_in_buffer(Buffer *buf, BufferElement request) {
    if (!buffer_is_full(*buf)) {
        buf->array[buf->head] = request;
        buf->head = (buf->head + 1) % buf->max_value;
        buf->size = buf->size++;
    }
}


BufferElement pop_from_buffer(Buffer *buf) {
    BufferElement temp;

    if (!buffer_is_empty(*buf)) {
        temp = buf->array[buf->tail];
        buf->tail = (buf->tail + 1) % buf->max_value;
        buf->size = buf->size--;
    }
    return temp;
}


void free_buffer(Buffer buf) {
    free(buf.array);
}

int buffer_is_empty(Buffer buf) {
    return buf.head == buf.tail;
}

int buffer_is_full(Buffer buf) {
    return buf.head + 1 == buf.tail;
}

void printElement(BufferElement request){
    struct sockaddr_in ip_addr;
    ip_addr.sin_addr.s_addr = request.ipaddr;
    printf("--REQUEST has: ipaddr: %s\nport: %u\nfilename: %s \nversion: %lu\nfd: %d\n",
    inet_ntoa(ip_addr.sin_addr),request.port,request.filename,request.version,request.fd);
}