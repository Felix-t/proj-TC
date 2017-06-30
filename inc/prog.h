#ifndef _PROG_H_
#define _PROG_H_

#include "adam_4018p.h"
#include <argp.h>
#include <stdlib.h>
#include <error.h>
#include <libconfig.h>
#include <time.h>
#include <stdint.h>

/* Function : Apply changes to the ADAM modules according to the 
 * currently loaded configuration. Acts blindly( does not check whether modules
 * addresses or config values are correct). If an error occured while sending 
 * commands to a module, does not stop but continue with the next module in 
 * configuration
 * Return : 0 if no configuration/modules are found or if an error occured 
 * with the modules, 1 otherwise
*/
uint8_t exec_config();

uint8_t read_all(FILE *fp);

uint8_t start_acquisition();

uint8_t change_config();

/* Function : User interface to configure the ADAM modules */
void module_management();

/* Function : Creates configuration structure with the information in the 
 * specified file. Print the structures
 * Params : A path to a file : see config/ directory for file exemples
 * Return : 0if error 1 otherwise
*/
uint8_t load_config_from_file(char * path);

/* Function : Creates configuration structures by requesting informations 
 * 	from the modules. The serial connection must be initialised. May take up 
 * 	to 5 min to scan all available module addresses.
 * Params : An address (0x00 to 0xFF) where the scan stops
 * Return : 0 if error, 1 otherwise
*/
uint8_t load_config_from_modules(uint8_t max_add);


#endif
