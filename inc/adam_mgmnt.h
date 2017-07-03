/*H**********************************************************************
* FILENAME :        prog.h         
* 
* DESCRIPTION :
* 	Implements a user interface to manage a network of ADAM-4018
*       & associated thermocouples
*
* AUTHOR :    Félix Tamagny        START DATE :    23 June 2017
*
*H*/

#ifndef _PROG_H_
#define _PROG_H_

#include "adam_4018p.h"
#include <argp.h>
#include <stdlib.h>
#include <error.h>
#include <libconfig.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <argp.h>

#define DEFAULT_OUTPUT_PATH "Data/"

/* Function : Apply changes to the ADAM modules according to the 
 * currently loaded configuration. Acts blindly( does not check whether modules
 * addresses or config values are correct). If an error occured while sending 
 * commands to a module, does not stop but continue with the next module in 
 * configuration
 * Return : 0 if no configuration/modules are found or if an error occured 
 * with the modules, 1 otherwise
*/
uint8_t exec_config();


/* Function : Read the data from all channels of all modules defined in 
 * current configuration and print it to a stream (stdout, a file...)
 * Params : A stream where data will be printed
 * Return : 0 if a communication error occured, 1 otherwise
*/
uint8_t read_all(FILE *fp);


/* Function : Prompt the user for acquisition parameters (file, freq, duration),
 * then start reading from the modules accordingly. Uses the current 
 * configuration
 * Return : Return 0 if a communication error occured, 1 when acquisition ends
*/
uint8_t start_acquisition(FILE *fp, uint32_t duration, double freq);


/* Function : Prompt the user for configuration values, then check their 
 * validity and apply them.
 * Return : 0 if value error or communication error, 1 if success
*/
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
