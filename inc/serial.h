#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>


uint8_t set_interface_attribs (int fd, int speed, int parity);
uint8_t set_blocking (int fd, int should_block);


#endif
