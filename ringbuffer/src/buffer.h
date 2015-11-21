#ifndef __BUFFER_H
#define __BUFFER_H
	
#include <stdint.h>

enum {
  BUF_SIZE = 6UL
};

typedef struct message {
  uint32_t id;
  uint32_t data[2];
	float fdata[2];
} message_t;

void putBuffer(message_t const * const);
void getBuffer(message_t * const);
void safeBufferInit(void);
void safeBufferPut(message_t const * const msg);
void safeBufferGet(message_t * const msg);

#endif
