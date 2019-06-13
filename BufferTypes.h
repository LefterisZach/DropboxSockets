

#ifndef BUFFERTYPES_H
#define BUFFERTYPES_H


#include <stdint.h>


typedef struct BufferElement {
    uint32_t ipaddr;
    uint16_t port;
    char filename[128];
    unsigned long version;
    int fd;
} BufferElement;

typedef struct BufferElement *BufferElementptr;


typedef struct Buffer {
    BufferElement *array;
    int size;
    int max_value;
    int head;
    int tail;
} Buffer;

Buffer buffer_init(size_t size);
void push_in_buffer(Buffer *buf, BufferElement request);
BufferElement pop_from_buffer(Buffer *buf);
void free_buffer(Buffer buf);
int buffer_is_empty(Buffer buf);
int buffer_is_full(Buffer buf);
void printElement(BufferElement request);

#endif //BUFFERTYPES_H
