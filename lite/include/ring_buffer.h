#ifndef DELOOP_RING_BUFFER_H
#define DELOOP_RING_BUFFER_H

struct deloop_ring_buffer {
  void *buffer;
  size_t capacity;
  size_t write;
  size_t read;
} deloop_ring_buffer_t;

#endif // DELOOP_RING_BUFFER_H
