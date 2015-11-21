#include <stdint.h>
#include <ucos_ii.h>
#include "buffer.h"

static OS_EVENT *fullSlot;
static OS_EVENT *freeSlot;
static OS_EVENT *bufMutex;

static message_t buffer[BUF_SIZE];
static uint8_t front = 0;
static uint8_t back = 0;

void putBuffer(message_t const * const msg) {
  buffer[back] = *msg;
  back = (back + 1) % BUF_SIZE;
}

void getBuffer(message_t * const msg) {
  *msg = buffer[front];
  front = (front + 1) % BUF_SIZE;
}

void safeBufferInit(void) {
  fullSlot = OSSemCreate(0);
  freeSlot = OSSemCreate(BUF_SIZE);
  bufMutex = OSSemCreate(1);
}

void safeBufferPut(message_t const * const msg) {
  uint8_t osStatus;

  OSSemPend(freeSlot, 0, &osStatus);
  OSMutexPend(bufMutex, 0, &osStatus);
  putBuffer(msg);
  osStatus = OSMutexPost(bufMutex);
  osStatus = OSSemPost(fullSlot);
}


void safeBufferGet(message_t * const msg) {
  uint8_t osStatus;
  
  OSSemPend(fullSlot, 0, &osStatus);
  OSMutexPend(bufMutex, 0, &osStatus);
  getBuffer(msg);
  osStatus = OSMutexPost(bufMutex);
  osStatus = OSSemPost(freeSlot);
}

