#ifndef _ADAM_4018p_
#define _ADAM_4018p_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CMD_TIMEOUT 2 // = nb messages par sec
#define MAX_SIZE_MSG 58
#define NB_CHANNELS 8
#define COUNT_VALUE 0.009 //°C

typedef struct {
	char name[30];
	uint8_t code;
} tuple;


typedef struct {
	tuple ch_input[NB_CHANNELS];
	tuple baudrate;
	tuple format;
	tuple checksum;
	tuple integration;
	tuple input;
	tuple module_address;
	char module_name[MAX_SIZE_MSG];
	char firmware_version[MAX_SIZE_MSG];
	uint8_t channel_enable[NB_CHANNELS];
	uint8_t channel_condition[NB_CHANNELS];
	float temp_CJC;
} configuration;


uint8_t scan_modules(uint8_t *modules_add);


/* Function : Open a serial communication 
 * Params : 	- port_name is a device name : usually /dev/something
 * 		- baudrate indicates the speed of the connection, 
 * 		it defaults to 9600
 * Return : 0 if error, 1 otherwise
 */
uint8_t init_connexion(char * portname, int baudrate);

/* Function : Close the file descriptor used for serial communication
 * 	Calling this function before exiting the program is not necessary
 * Return : 0 if error(already closed, not opened...), 1 otherwise
*/
uint8_t close_connexion();

/* Function : Write a message to the device, and read the response
 * 	The function doesn't check whether a response has been received
 * 	The ADAM 4018 may not acknowledge wrong commands
 * Params : command is the string to send, without any line feed
 * Return : 0 if read/write failed (connection not init...), 1 otherwise
*/
uint8_t send_command(char * command, char * reception);


/* Function : Query the 4018+ module for information
 * Params : 	- module_address : 0x00 -> 0xFF
 * 		- pointer to the configuration to fill, NULL if only printing info
 * Return : 0 if error, 1 otherwise
*/
uint8_t get_configuration(uint8_t module_address, configuration *config);
uint8_t set_configuration(uint8_t module_address, configuration *config);

/* Function : Print the configuration
 * Params : Configuration to print
*/
void print_configuration(configuration *cfg);


/* Function : Read the data from all channels of a module.
 * 	Channels disabled are set to -888888.0, while non connected but 
 * 	enabled channels are set to 888888.0
 * Params : 	- module_address : 0x00 -> 0xFF
 * 		- array of 8 uint8_t to contain the data (does not check 
 * 		allocation of the array)
 * Return : 0 if communication error, 1 if success
*/
uint8_t get_all_data(uint8_t module_address, float *data);


/* Function : Read the data from a specific channel of a module.
 * 	If the channel is disabled, data is set to -888888.0, 
 * 	If the channel is unconnected but enabled, data is set to 888888.0
 * Params : 	- module_address : 0x00 -> 0xFF
 * 		- The channel number (0 -> 7)
 * 		- pointer to an uint8_t to contain the data
 * Return : 0 if communication error, 1 if success
*/
uint8_t get_channel_data(uint8_t module_address, uint8_t ch_number, float *data);


/* Function : Scan all 8 channels of a specified module and determines their 
 * 	input configuration (type, range)
 * Params : 	- The module address (0x00 -> 0xFF)
 * 		- An array of (at least) 8 tuples for the data : the function 
 * 		does not check if memory for the array has been allocated
 * Return : 0 if error, 1 otherwise
*/
uint8_t get_all_channels_type(uint8_t module_address, tuple *ch_type);
uint8_t set_all_channels_type(uint8_t module_address, tuple *ch_type);


/* Function : Scan the 8 channels of a specified module and determines if they
 * 	are connected, out of range or disabled
 * Params : 	- The module address (0x00 -> 0xFF)
 * 		- An array of (at least) 8 uint8_t for the data : each is set 
 * 		to either 1 if channel is enabled && unconnected or out of range
 * 		and 0 otherwise
 * Return : 0 if error, 1 otherwise
*/
uint8_t get_channels_conditions(uint8_t module_address, uint8_t *ch);


/* Function : Find the name of a module (ex. "ADAM4018P")
 * Params : the module address, and the char* to contain the name
 * Return : 0 if communication error, 1 if success
*/
uint8_t get_name(uint8_t module_address, char *name);


/* Function : Find the Cold Junction Compensation current value for a specified
 * 	module
 * Params : the module address, and a pointer to float to contain the data(°C)
 * Return : 0 if communication error, 1 if success
*/
uint8_t get_CJC_status(uint8_t module_address, float *temperature);


/* Function : Find the firmware version used by a specified module
 * Params : the module address, and the char* to contain the version
 * Return : 0 if communication error, 1 if success
*/
uint8_t get_firmware_version(uint8_t module_address, char *version);


/* Functions : Get/Set the Enable/Disable status of all channels for a specified module
 * Params : the module address
 * 	an array of 8 uint8_t where the index is the channel number,
 * 	and a value of 1 = Enabled, 0 = Disabled
 * Return : 0 if params or communication error, 1 otherwise
*/
uint8_t set_channels_status(uint8_t module_address, uint8_t *ch);
uint8_t get_channels_status(uint8_t module_address, uint8_t *ch);


/* Functions : Get/Set the configuration of a specified module
 * 	@TODO : refactor cfg -> (baudrate, input...) ??
 * Params : the module address
 * Return : 0 if params or communication error, 1 otherwise
*/
uint8_t set_configuration_status(uint8_t module_address, configuration *cfg);
uint8_t get_configuration_status(uint8_t module_address, configuration *cfg);


/* Function : Get/Set the input configuration (type, range) of a channel for a
 * 	specified module
 * Params : 	- The module address (0x00 -> 0xFF)
 * 		- The channel number (0 -> 7)
 * 		- Pointer to the data
 * Return : 0 if wrong channel, communication error or empty response, 1 success
*/
uint8_t set_channel_type(uint8_t module_address, uint8_t ch_number, 
		tuple *ch_type);
uint8_t get_channel_type(uint8_t module_address, uint8_t ch_number, 
		tuple *ch_type);

/* Function : Calibrate a channel of a specified module to correct for 
 * 	offset errors.
 * 	Calibration may take up to 7 seconds. See ADAM doc chapter 5.1.17
 * 	(A proper input signal should be connected to the module ?? @TODO : check)
 * Params :
 * Return
*/
uint8_t calibrate_channel_offset(uint8_t module_address, uint8_t ch_number);


/* Function : Calibrate a channel of a specified module to correct for 
 * 	gain errors. A proper input signal should be connected to the module.
 * 	Calibration may take up to 7 seconds. See ADAM doc chapter 5.1.16
 * Params : the module address, the channel number
 * Return : 0 if wront channel or communication error, 1 otherwise
*/
uint8_t calibrate_channel_span(uint8_t module_address, uint8_t ch_number);


/* Function :  Calibrate the Cold Junction Compensation (CJC) by 
 * 	increasing/decreasing the offset. See ADAM doc chapter 5.1.15
 * Params : The module address, the temperature offset in °C, with a 
 * 	precision of 0.1. Max offset is (+/-) 0xFFFF*COUNT_VALUE
 * Return : 0 if offset range error or communication error, 1 if success
*/
uint8_t calibrate_CJC_offset(uint8_t module_address, float offset);


/*
 * ---	Codes and signification defined in the ADAM documentation
*/

static tuple input_type[8] = {
	{.code = 0x06, .name = "mA"},
	{.code = 0x0E, .name = "TC_J"},
	{.code = 0x0F, .name = "TC_K"},
	{.code = 0x10, .name = "TC_T"},
	{.code = 0x11, .name = "TC_E"},
	{.code = 0x12, .name = "TC_R"},
	{.code = 0x13, .name = "TC_S"},
	{.code = 0x14, .name = "TC_B"}
};

static tuple data_format[3] = {
	{.code = 0, .name = "Engineering Units"},
	{.code = 1, .name = "\% of FSR"},
	{.code = 2, .name = "two's complement of hex"}
};

static tuple baudrate_conf[8] = {
	{.code = 0x03, .name = "1200"},
	{.code = 0x04, .name = "2400"},
	{.code = 0x05, .name = "4800"},
	{.code = 0x06, .name = "9600"},
	{.code = 0x07, .name = "19200"},
	{.code = 0x08, .name = "38400"},
	{.code = 0x09, .name = "57600"},
	{.code = 0x0A, .name = "115200"}
};

static tuple checksum_status[2] = {
	{.code = 0, .name = "Disabled"},
	{.code = 1, .name = "Enabled"}
};

static tuple integration_time[2] = {
	{.code = 0, .name = "50 ms"},
	{.code = 1, .name = "60 ms"}
};

/*
 * End
*/  


#endif
