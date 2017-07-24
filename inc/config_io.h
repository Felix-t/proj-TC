#ifndef _CONFIG_IO_H_
#define _CONFIG_IO_H_
#include <stdint.h>
#include "adam_4018p.h"
#include <errno.h>

// Fill a config file from a struct configuration[nb]
uint8_t save_config_to_file(char *path);
uint8_t load_config_from_file(char * path);

#endif
