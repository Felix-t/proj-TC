/*H**********************************************************************
* FILENAME :        prog.c         
*
* DESCRIPTION :
* 	Library providing access to an ADAM-4018 module.
* 	Used for reading data, informations about the module and the associated 
* 	themocouples.
* 	Used for configuring the module and the channels.
*
* AUTHOR :    Félix Tamagny        START DATE :    16 June 2017
*
*H*/


#include "adam_4018p.h"
#include "serial.h"
#include <time.h>
#include <termios.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>


static inline uint8_t str_to_hex(char msb, char lsb);
static char * hex_to_str(uint8_t add);
static void hex_to_uppercase(char *str);
static char * remove_address_from_cmd(char * response);
static uint8_t parse_answer(uint8_t module_address, char * msg);
static uint8_t parse_answer_data(char * msg);
static void parse_config(char *msg, uint8_t module_address, configuration *cfg);
static float str_to_float(char *msg);


int fd = -1;


void init_struct_configuration(configuration *c)
{
	memset(c, 0, sizeof(configuration));	

	// Defaults values :
	// baudrate
	sprintf(c->baudrate.name, baudrate_conf[3].name);
	c->baudrate.code = baudrate_conf[3].code;
	// Data format
	sprintf(c->format.name, data_format[0].name);
	c->baudrate.code = data_format[0].code;
	// checksum
	sprintf(c->checksum.name, checksum_status[0].name);
	c->checksum.code = checksum_status[0].code;
	// Integration
	sprintf(c->integration.name, integration_time[0].name);
	c->integration.code = integration_time[0].code;
	// Input
	sprintf(c->input.name, "Undefined");
	c->input.code = 0xFF;
}

uint8_t init_connexion(char * port_name, int baudrate)
{
	if(baudrate == 0)
		baudrate = B9600;
	// Speed macro defined in termios.h are between 0 and 15, 
	// or between 4097 and 4099. See termios.h
	else if(baudrate < 1 
			|| (baudrate > 15 && baudrate < 4097)
			|| baudrate > 4099)
	{
		printf("Invalid baudrate, see \"man termios\"\n");
		return 0;
	}	
	fd = open (port_name, O_RDWR | O_NOCTTY | O_SYNC);

	if (fd < 0)
	{
		printf("error %d opening %s: %s\n", errno, port_name, strerror(errno));
		printf("\"USB to RS-485\" may be disconnected\n");
		return 0;
	}

	if(!set_interface_attribs (fd, baudrate, 0) ) // set speed to 9600 bps, 8n1 (no parity)
	{
		printf("Error setting up serial interface\n");
		return 0;
	}
	if(!set_blocking (fd, 0))
	{
		close(fd);
		printf("Error setting up serial blocking properties\n");
		return 0;
	}

	printf("Serial connection set\n");
	return 1;
}


uint8_t close_connexion()
{
	if(close(fd) < 0)
	{
		printf("error %d closing : %s\n", errno, strerror(errno));
		return 0;
	}
	fd = -1;
	return 1;
}


uint8_t send_command(char * command, char * reception)
{
	struct timespec tt = {
		.tv_sec = 0,        /* seconds */
		.tv_nsec = 1000000000 / CMD_TIMEOUT       /* nanoseconds */
	};
	memset(reception, 0, strlen(reception));
	if(fd < 0)
	{
		printf("Connection not initialized\n");
		return 0;
	}

	char cmd[20] = "";
	int size_command, bytes_received = 0;

	size_command = strlen(command);

	// Add cariage return : \r
	memcpy(cmd, command, size_command);
	strcat(cmd, "\r");

	if(write(fd, cmd, size_command + 1) < 0)
	{
		printf("Serial write operation failed : %s\n", strerror(errno));
		return 0;
	}

	nanosleep(&tt, NULL);

	if((bytes_received = read(fd, reception, MAX_SIZE_MSG)) < 0)  
	{
		printf("Serial read operation failed : %s\n", strerror(errno));
		return 0;
	}

	return 1;
}


uint8_t get_configuration(uint8_t module_address, configuration *config)
{ 
	printf("Address conf get_configuration : %p\n", config);
	// Temporary configuration struct : in case of failure,
	// the configuration given is not overwritten
	configuration cfg;
	cfg.module_address.code = module_address;

	if(!get_configuration_status(module_address, &cfg) 
			|| !get_name(module_address, cfg.module_name) 
			|| !get_firmware_version(module_address, cfg.firmware_version)
			|| !get_all_channels_type(module_address, cfg.ch_input)
			|| !get_CJC_status(module_address, &cfg.temp_CJC)
			|| !get_channels_status(module_address, cfg.channel_enable)
			|| !get_channels_conditions(module_address, cfg.channel_condition))
		return 0;

	// Copy the configuration informations in the struct passed as argument
	if(config != NULL)
	{
		cfg.module_address.code = module_address;
		memcpy(config, &cfg, sizeof(configuration));
	}
	else
		print_configuration(&cfg, stdout);

	return 1;
}


uint8_t set_configuration(uint8_t module_address, configuration *config)
{ 
	if(!set_configuration_status(module_address, config) 
			|| !set_all_channels_type(module_address, config->ch_input)
			|| !set_channels_status(module_address, config->channel_enable))
		return 0;
	return 1;
}


void print_configuration(configuration *cfg, FILE *fp)
{
	if(cfg == NULL)
	{
		printf("Configuration passed in argument is NULL pointer\n");
		return;
	}
	fprintf(fp, "\n\tAddress : %s\t Name : %s\n\tBaudrate : %s\n\t"
			"Input type : %s\n\tData format : %s\n\t"
			"Checksum %s\n\tIntegration time = %s\n\t"
			"Firmware version : %s\n\t"
			"CJC Status : %0.1f\n\t", 
			cfg->module_address.name, cfg->module_name,
			cfg->baudrate.name, cfg->input.name,
			cfg->format.name, cfg->checksum.name,
			cfg->integration.name, cfg->firmware_version,
			cfg->temp_CJC);
	uint8_t i;
	printf("Channels number : \t");
	for(i = 0; i< NB_CHANNELS; i++)
		fprintf(fp, "%i - ", i);
	fprintf(fp, "\n\tChannel input type : \t");
	for(i = 0; i< NB_CHANNELS; i++)
	{
		if(cfg->ch_input[i].name[3] == 0)
			cfg->ch_input[i].name[3] = '0';
		fprintf(fp, "%c - ", cfg->ch_input[i].name[3]);
	}
	fprintf(fp, "\n\tChannel status : \t");
	for(i = 0; i< NB_CHANNELS; i++)
		fprintf(fp, "%i - ", cfg->channel_enable[i]);
	fprintf(fp, "\n\tChannel state : \t");
	for(i = 0; i< NB_CHANNELS; i++)
		fprintf(fp, "%i - ", cfg->channel_condition[i]);
	fprintf(fp, "\n");
}


uint8_t get_all_data(uint8_t module_address, float *data)
{
	uint8_t i = 0;
	char command[20] = "#", reception[100] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	// Build command
	strcat(command, prefix);

	if(!send_command(command, reception) 
			|| !parse_answer_data(reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	for(i = 0; i < NB_CHANNELS; i++)
	{
		// Data sent is either a 7-characters float number, 
		// or 7 ' ' characters
		if(reception[1+7*i] == ' ')
			data[i] = -888888.0;
		else if((data[i] = str_to_float(&reception[1+7*i])) == 0)
		{
			printf("Conversion error:%s\n",&reception[1+7*i]);
			return 0;
		}
	}
	return 1;
}

uint8_t get_channel_data(uint8_t module_address, uint8_t ch_number, float *data)
{
	if(ch_number > 7)
	{
		printf("Channel number incorrect : %i", ch_number);
		return 0;
	}	

	char command[20] = "#", reception[MAX_SIZE_MSG] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[1];

	// Build command
	strcat(command, prefix);
	sprintf(suffix, "%i", ch_number);
	strcat(command, suffix);

	if(!send_command(command, reception) 
			|| !parse_answer_data(reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}
	
	
	// Data sent is either a 7-characters float number, 
	// or 7 ' ' characters
	if(reception[1] == ' ')
		*data = -888888.0;
	else if((*data = str_to_float(&reception[1])) == 0)
	{
		printf("Conversion error:%s\n",&reception[1]);
		return 0;
	}

	return 1;
}
/* --- 			Specific command functions 		--- 	*/

uint8_t set_configuration_status(uint8_t module_address, configuration *cfg)
{
	if(cfg == NULL)
	{
		printf("Configuration passed in argument is NULL pointer\n");
		return 0;
	}

	char command[20] = "%", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	strcat(command, prefix);

	char new_add[2] = "", *new_baudrate, *new_param;
	uint8_t param_int = 0;

	// --- Module address
	// @TODO : ??
	// Change only if name is given (0 mays be possible address
	if (strcmp(cfg->module_address.name, "") == 0)
		sprintf(new_add, "%02hhX", module_address);
	else
		sprintf(new_add, "%02hhX", cfg->module_address.code);
	strcat(command, new_add);

	// --- Input range/type
	// @TODO : see possible input values module wise, for now don't change
	strcat(command, "FF");

	// --- Baudrate
	configuration tmp_cfg;
	if(!get_configuration(module_address, &tmp_cfg))
		return 0;
	if(tmp_cfg.baudrate.code != cfg->baudrate.code)
		printf("Cannot change baudrate value in normal mode, used" 
				" current value : %s\n", tmp_cfg.baudrate.name);
	new_baudrate = hex_to_str(tmp_cfg.baudrate.code);
	strcat(command, new_baudrate);

	// Data format, checksum, integration time
	printf("checksum received : %i\n", cfg->checksum.code);
	if(tmp_cfg.checksum.code != cfg->checksum.code)
		printf("Cannot enable/disable checksum in normal mode, used" 
				" current value : %s\n", 
				tmp_cfg.checksum.name);
	param_int = cfg->integration.code << 7
		| tmp_cfg.checksum.code << 6
		| cfg->format.code ;

	new_param = hex_to_str(param_int);
	strcat(command, new_param);


	printf("%s\n", command);

	if(!send_command(command, reception)
			|| !parse_answer(str_to_hex(new_add[0], new_add[1]), reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	free(new_baudrate);
	free(new_param);
	return 1;
}


uint8_t get_configuration_status(uint8_t module_address, configuration *cfg)
{
	if(cfg == NULL)
	{
		printf("Configuration passed in argument is NULL pointer\n");
		return 0;
	}

	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	// Different channels may have different input type and ranges,
	// so module input configuration may be irrelevant(0xFF not in doc)
	strcpy(cfg->input.name, "Undefined"); 

	// Build command
	strcat(command, prefix);
	strcat(command, "2");
	if(!send_command(command, reception)
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	parse_config(reception, module_address, cfg);

	// Add remaining information to the struct
	if(module_address < 0x10)
		sprintf(cfg->module_address.name, "0%hhx",
				module_address);
	else
		sprintf(cfg->module_address.name, "%hhx",
				module_address);

	return 1;
}

uint8_t set_all_channels_type(uint8_t module_address, tuple *ch_type)
{
	uint8_t i;
	for(i = 0; i<NB_CHANNELS; i++)
	{
		if(!set_channel_type(module_address, i, &ch_type[i]))
			return 0;
	}
	return 1;
}

uint8_t get_all_channels_type(uint8_t module_address, tuple *ch_type)
{
	uint8_t i;
	for(i = 0; i<NB_CHANNELS; i++)
	{
		if(!get_channel_type(module_address, i, &ch_type[i]))
			return 0;
	}
	return 1;
}


uint8_t get_channel_type(uint8_t module_address, uint8_t ch_number, 
		tuple *ch_type)
{	
	if(ch_number > 7)
	{
		printf("Channel number incorrect : %i", ch_number);
		return 0;
	}	

	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[3];
	sprintf(suffix, "8C%i", ch_number);

	// Build command
	strcat(command, prefix);
	strcat(command, suffix);
	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	ch_type->code = str_to_hex(reception[6], reception[7]);

	if(ch_type->code == 0x06)
		strcpy(ch_type->name, input_type[0].name);
	else if(ch_type->code >= 0x0E && ch_type->code <= 0x14)
		strcpy(ch_type->name, input_type[ch_type->code - 0x0D].name);
	else
	{
		printf("Input type non recognized : %hhx\n", ch_type->code);
		return 0;
	}

	return 1;
}

uint8_t set_channel_type(uint8_t module_address, uint8_t ch_number, 
		tuple *ch_type)
{	
	if(ch_number > 7)
	{
		printf("Channel number incorrect : %i", ch_number);
		return 0;
	}
	else if(ch_type->code == 0)
		return 1;
	else if(ch_type->code != 0x06 
			&& (ch_type->code < 0x0E || ch_type->code > 0x14))
	{
		printf("Input code non recognized : %hhx\n", ch_type->code);
		return 0;
	}


	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[6];
	sprintf(suffix, "7C%iR%s", ch_number, hex_to_str(ch_type->code));

	// Build command
	strcat(command, prefix);
	strcat(command, suffix);
	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	ch_type->code = str_to_hex(reception[6], reception[7]);

	if(ch_type->code == 0x06)
		strcpy(ch_type->name, input_type[0].name);
	else if(ch_type->code >= 0x0E && ch_type->code <= 0x14)
		strcpy(ch_type->name, input_type[ch_type->code - 0x0D].name);

	return 1;
}


uint8_t calibrate_channel_span(uint8_t module_address, uint8_t ch_number)
{
	if(ch_number > 7)
	{
		printf("Channel number incorrect : %i", ch_number);
		return 0;
	}	

	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[3];
	sprintf(suffix, "0C%i", ch_number);

	// Build command
	strcat(command, prefix);
	strcat(command, suffix);

	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}
	printf("Waiting 7 seconds...\n");
	sleep(7);

	return 1;
}


uint8_t calibrate_channel_offset(uint8_t module_address, uint8_t ch_number)
{
	if(ch_number > 7)
	{
		printf("Channel number incorrect : %i", ch_number);
		return 0;
	}	

	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[3];
	sprintf(suffix, "1C%i", ch_number);

	// Build command
	strcat(command, prefix);
	strcat(command, suffix);

	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}
	printf("Waiting 7 seconds...\n");
	sleep(7);

	return 1;
}

uint8_t calibrate_CJC_offset(uint8_t module_address, float offset)
{
	if((int) (offset/COUNT_VALUE) > 0xFFFF)
	{
		printf("Offset sent is too large : %i maximum\n", (int) (COUNT_VALUE*0xFFFF));
		return 0;
	}
	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[3];

	if(offset >= 0)
	sprintf(suffix, "9+%04X", (uint) (offset / COUNT_VALUE));
	else
	sprintf(suffix, "9-%04X", (uint) -(offset / COUNT_VALUE));

	// Build command
	strcat(command, prefix);
	strcat(command, suffix);

	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	printf("Waiting 7 seconds...\n");
	sleep(7);

	return 1;
}

uint8_t get_name(uint8_t module_address, char *name)
{	
	if(name == NULL)
	{
		printf("Array passed in argument is NULL pointer\n");
		return 0;
	}

	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	char * module_name;

	// Build command
	strcat(command, prefix);
	strcat(command, "M");
	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	module_name = remove_address_from_cmd(reception);
	module_name[strlen(module_name)-1] = '\0';
	if(module_name == NULL)
		return 0;
	strncpy(name, module_name, MAX_SIZE_MSG);

	free(module_name);
	return 1;
}


uint8_t scan_modules(uint8_t **modules_add, uint8_t max_add)
{
	if(max_add == 0)
		max_add = 0xFF;
	uint8_t i, nb_mod = 0;
	uint8_t mod_add_tmp[max_add];
	// Build command
	char command[20] = "", reception[50] = "";

	struct timespec tt = {
		.tv_sec = 0,
		.tv_nsec = 1000000000
	};


	printf("Scanning modules...   \n");

	for(i = 0; i < max_add; i++)
	{
		printf("\b\b%02hhX", i);
		fflush(stdout);
		nanosleep(&tt, NULL);
		sprintf(command, "$%02hhX", i);
		if(send_command(command, reception) 
				&& strcmp(reception, "") != 0)
		{
			mod_add_tmp[nb_mod++] = i;
		}
	}
	printf("\n");
	*modules_add = malloc(nb_mod);
	memcpy(*modules_add, &mod_add_tmp, nb_mod);

	return nb_mod;
}

uint8_t get_CJC_status(uint8_t module_address, float *temperature)
{
	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	// Build command
	strcat(command, prefix);
	strcat(command, "3");

	if(!send_command(command, reception) 
			|| !parse_answer_data(reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	if((*temperature = str_to_float(&reception[1])) == 0)
	{
		printf("Conversion error:%s\n",&reception[1]);
		return 0;
	}
	return 1;
}


uint8_t get_firmware_version(uint8_t module_address, char *version)
{	
	if(version == NULL)
	{
		printf("Array passed in argument is NULL pointer\n");
		return 0;
	}

	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	char * tmpchar;

	// Build command
	strcat(command, prefix);
	strcat(command, "F");
	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}
	tmpchar = remove_address_from_cmd(reception);
	tmpchar[strlen(tmpchar)-1] = '\0';
	if(tmpchar == NULL)
		return 0;
	strncpy(version, tmpchar, MAX_SIZE_MSG);

	free(tmpchar);
	return 1;
}


uint8_t set_channels_status(uint8_t module_address, uint8_t *ch)
{
	if(ch == NULL)
	{
		printf("Array passed in argument is NULL pointer\n");
		return 0;
	}

	uint8_t i = 0, hex_status = 0;
	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);
	char suffix[3];

	for(i = 0; i < NB_CHANNELS; i++)
		hex_status |= (ch[i] != 0) << i;

	sprintf(suffix, "5%s", hex_to_str(hex_status));

	// Build command
	strcat(command, prefix);
	strcat(command, suffix);
	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	return 1;
}


uint8_t get_channels_status(uint8_t module_address, uint8_t *ch)
{	
	if(ch == NULL)
	{
		printf("Array passed in argument is NULL pointer\n");
		return 0;
	}

	uint8_t i = 0;
	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	uint8_t hex_status;

	// Build command
	strcat(command, prefix);
	strcat(command, "6");
	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	hex_status = str_to_hex(reception[3], reception[4]);

	for(i = 0; i < NB_CHANNELS; i++)
		ch[i] = (hex_status >> i) & 1;

	return 1;
}


uint8_t get_channels_conditions(uint8_t module_address, uint8_t *ch)
{	
	if(ch == NULL)
	{
		printf("Array passed in argument is NULL pointer\n");
		return 0;
	}

	uint8_t i = 0;
	char command[20] = "$", reception[50] = "";
	char prefix[2] = "";
	sprintf(prefix, "%02hhX", module_address);

	uint8_t hex_status;

	// Build command
	strcat(command, prefix);
	strcat(command, "B");

	if(!send_command(command, reception) 
			|| !parse_answer(module_address, reception))
	{
		printf("Failed to send/receive command %s\n", command);
		return 0;
	}

	hex_status = str_to_hex(reception[3], reception[4]);

	for(i = 0; i < NB_CHANNELS; i++)
		ch[i] = (hex_status >> i) & 1;

	return 1;
}



/* --- 			Static functions 			---	*/

/* Function : Build an int from two ASCII characters in hexadecimal notation
 * 	Ex : "00" -> 0, "FF" -> 255, "A7" -> 167 ...
 * Params : 	-msb : 4 most significant bits of the number in ASCII
 * 		-lsb : 4 least significant bits of the number in ASCII
 * Return : uint8_t number
 */ 
static inline uint8_t str_to_hex(char msb, char lsb)
{
	return 16*((msb > '9' ? msb - 7 : msb) -'0')
		+ (lsb > '9' ? lsb - 7 : lsb) -'0';
}


static char * hex_to_str(uint8_t add)
{
	char * str = malloc(3);

	if (add >= 0x10)
		sprintf(str, "%hhx", add);
	else
		sprintf(str, "0%hhx", add);

	hex_to_uppercase(str);

	return str;
}

static void hex_to_uppercase(char *str)
{
	// Lowercase --> Uppercase
	if(str[0] >= 'a')
		str[0] -= 32;
	if(str[1] >= 'a')
		str[1] -= 32;
}



static char * remove_address_from_cmd(char * response)
{
	char * str = malloc(MAX_SIZE_MSG);
	memcpy(str, &response[3], strlen(response) - 2);
	return str;
}


static uint8_t parse_answer_data(char * msg)
{
	if(strcmp(msg, "") == 0)
	{
		printf("Received empty message, check device address\n");
		return 0;
	}
	else if(msg[0] == '?')
	{
		printf("Invalid syntax\n");
		return 0;
	}
	return 1;
}



static uint8_t parse_answer(uint8_t module_address, char * msg)
{
	uint8_t rcv_add;

	if(strcmp(msg, "") == 0)
	{
		printf("Received empty message, check device address\n");
		return 0;
	}
	else if(msg[0] == '?')
	{
		printf("Invalid syntax\n");
		return 0;
	}
	else if(module_address != 0)
	{
		rcv_add = str_to_hex(msg[1], msg[2]);
		if(rcv_add != module_address)
		{
			printf("Error, received message from wrong address\n");
			return 0;
		}
	}
	return 1;
}


/* Function : Parse a configuration string received from an ADAM module as 
 * 	response to an $XX2 command
 * 	See doc p.121 for information
 * Params : 	- msg is the string received
 * 		- module_address is compared with the message source address
 * 		- pointer to a configuration structure to fill
 */
static void parse_config(char * msg, uint8_t module_address, configuration *cfg)
{
	uint8_t i, parameters;

	// Get input type and name, baudrate name, 
	cfg->input.code = str_to_hex(msg[3], msg[4]);
	cfg->baudrate.code = str_to_hex(msg[5], msg[6]);

	parameters = str_to_hex(msg[7], msg[8]);
	cfg->format.code = 0b00000011 & parameters;
	cfg->checksum.code =  (0b01000000 & parameters) > 6;
	cfg->integration.code =  (0b10000000 & parameters) > 7;

	for(i = 0; i < 8; i++)
	{
		if (cfg->input.code == input_type[i].code)
			strcpy(cfg->input.name, input_type[i].name);
		if (cfg->baudrate.code == baudrate_conf[i].code)
			strcpy(cfg->baudrate.name, baudrate_conf[i].name);
	}

	strcpy(cfg->format.name, data_format[cfg->format.code].name);	
	strcpy(cfg->checksum.name, checksum_status[cfg->checksum.code].name);	
	strcpy(cfg->integration.name, integration_time[cfg->integration.code].name);	
}


static float str_to_float(char *msg)
{
	char tmp[7] = "";
	strncpy(tmp, msg, 7);

	char * err_Check;
	float f = strtof(tmp, &err_Check);
	if(err_Check == tmp) {
		return 0;
	}
	return f;
}


