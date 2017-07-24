/*H**********************************************************************
 * FILENAME :        prog.c         
 *
 * DESCRIPTION :
 * 	Implements a user interface to manage a network of ADAM-4018
 *       & associated thermocouples
 *
 *
 * AUTHOR :    Félix Tamagny        START DATE :    23 June 2017
 *
 *H*/

#include "adam_mgmnt.h"


// Pointer to the array of struct configuration used
static configuration **get_current_config(uint8_t **nb);

// Open a file in DEFAULT_OUTPUT_PATH with the day's date
static uint8_t open_new_file(FILE **fp);
// Find number of modules from config file
static uint8_t parse_number_of_modules(config_t *cfg);
//Return 1 if n is in range 0,255
static inline uint8_t uint_checker(int n);
// Fill a struct configuration[nb] with the data from a config file
static void parse_config_from_file(config_t *file_cfg, configuration *cfg, 
		uint8_t nb);
// Fill a config file from a struct configuration[nb]
static uint8_t save_config_to_file(char *path, configuration *conf, uint8_t nb);
// Print "Channel 1, Channel 2, ..." in the stream
static void print_header(FILE *fp);
// Prompt user for new address and exec the change
static uint8_t change_address(configuration *cfg);
// Prompt user for calibration and exec the change
static uint8_t change_calibration(configuration *cfg);
// Prompt user for new integration time and exec the change
static uint8_t change_integration_time(configuration *cfg);
// print the configuration of all channels
static void print_channels(configuration *cfg);
// Prompt user for new channels status(enable/disable) and input type
// and exec the change
static uint8_t change_channels_type_status(configuration *cfg);
// Check if the given address is already in use (0 if it is)
static uint8_t check_add(uint8_t add);


const char *argp_program_version  = "v1.0";
const char *argp_program_bug_address = "<felix.tamagny@ifsttar.fr>";

/* Program documentation. */
static char doc[] = "Program to configure and start data acquisition for"
" thermocouples connected with ADAM modules\n";

/* A description of the arguments we accept. */
static char args_doc[] = "[OPTION...ARG]";


/* The options we understand. */
static struct argp_option options[] = {
	{"output",   'o', "FILE",  0,
		"Output to FILE. Default is Data/xx_xx_xxxx_data (date)."
			" Value of stdout output to terminal" },
	{"config",   'c', "FILE",  0,
		"Get the configuration from FILE instead of by scanning" },
	{"freq", 'f', "mesures/min", 0, 
		"Set the acquisition frequency (default is 0.2/min)"},
	{"duration", 't', "DURATION", 0, "Set the acquisition duration" 
		" (default is infinite)"},
	{"interactive", 'i', 0, 0, 
		"Start program in interactive mode (ignore other options)"},
	{"device", 'd', "FILE", 0, "Specify specific device for communication"},
	{"baudrate", 'b', "Hz", 0, "Specify specific baudrate for"
		" communication (default is 9600)"},
	{ 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	double freq;             	/* Frequency in data/min */
	uint32_t baud, duration;
	char *device, *output_file_path, *config_file_path;
	uint8_t interactive;   		/* ‘-s’, ‘-v’, ‘--abort’ */
};

/* Parse a single option. */
	static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 'o':
			arguments->interactive = 0;
			arguments->output_file_path = arg;
			printf("-%s-\n", arg);
			break;
		case 'c':      
			arguments->interactive = 0;
			arguments->config_file_path = arg;
			break;
		case 'b':      
			arguments->interactive = 0;
			arguments->baud = atoi(arg);
			break;
		case 'd':      
			arguments->device = arg;
			break;
		case 'f':
			arguments->interactive = 0;
			arguments->freq = arg ? atof (arg) : 1;
			break;
		case 't':
			arguments->interactive = 0;
			if(atoi(arg) != 0)
				arguments->duration = atoi(arg);
			break;
		case 'i':
			arguments->interactive = 1;
			state->next = state->argc;
			break;
		case ARGP_KEY_NO_ARGS:
			//argp_usage (state);

		case ARGP_KEY_ARG:
			/* Here we know that state->arg_num == 0, since we
			   force argument parsing to end before any more 
			   arguments can get here. */

			/* Now we consume all the rest of the arguments.
			   state->next is the index in state->argv of the
			   next argument to be parsed, which is the first string
			   we’re interested in, so we can just use
			   &state->argv[state->next] as the value for
			   arguments->strings.

			   In addition, by setting state->next to the end
			   of the arguments, we can force argp to stop parsing
			   here and return. */
			state->next = state->argc;

			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };


uint8_t start_acquisition(FILE *fp, uint32_t duration, double freq)
{
	time_t start_time = time(NULL);
	print_header(fp);

	while(difftime(time(NULL), start_time) < 60*duration )
	{
		if(!read_all(fp))
			printf("Error while reading data\n");
		fflush(fp);
		sleep(60 / freq);
		if(ftell(fp) > SIZE_MAX_FILE*1024)
		{
			open_new_file(&fp);
			print_header(fp);
		}	

	}
	return 1;
}


// @TODO : gestion d'erreur
uint8_t prompt_acquisition()
{
	size_t input_size, in;
	float freq, duration;
	char *input;
	FILE *fp;

	do
	{
		printf("Frequence (mesures/minutes) : ");
		in = getline(&input, &input_size, stdin);
		input[in - 1] = '\0';
		if((freq = strtof(input, NULL)) <= 0)
			printf("Invalid input : must be a positive number\n");
	}while(freq < 0);
	do
	{
		printf("Fichier de capture : ");
		in = getline(&input, &input_size, stdin);
		input[in - 1] = '\0';
	}while((fp = fopen(input, "a+")) == NULL);
	do
	{
		printf("Durée (minutes) : ");
		in = getline(&input, &input_size, stdin);
		input[in - 1] = '\0';
		if((duration = strtof(input, NULL)) <= 0)
			printf("Invalid input : must be a positive number\n");
	}while(duration <= 0);

	start_acquisition(fp, duration, freq);

	free(input);
	fclose(fp);
	return 1;

}


uint8_t exec_config()
{
	uint8_t i, success = 1, *nb_modules = NULL;
	configuration **conf = get_current_config(&nb_modules);

	// Error management
	if(*conf == NULL)
	{
		printf("No configuration loaded\n");
		return 0;
	}
	if(*nb_modules == 0)
	{
		printf("No modules found in configuration...");
		return 0;
	}

	// Apply modifications module after module, continue even if 
	// an error happened
	printf("Applying new configuration...\n");
	for(i = 0; i < *nb_modules; i++)
	{
		if(!set_configuration((*conf + i)->module_address.code, *conf + i))
		{
			printf("Error with module %02hhx,"
					" continuing with next module...\n", i);
			success = 0;
			continue;
		}
	}
	return success;
}


uint8_t read_all(FILE *fp)
{
	uint8_t i, j, *nb_modules = NULL;
	float *data;
	char *time_string;
	time_t t;

	configuration **conf = get_current_config(&nb_modules);
	data = malloc(sizeof(float) * 8 * (*nb_modules));
	// Error management
	if(*conf == NULL)
	{
		printf("No configuration loaded\n");
		return 0;
	}
	if(*nb_modules == 0)
	{
		printf("No modules found in configuration...");
		return 0;
	}

	if (fp == stdout)
		print_header(fp);

	//print time
	t = time(NULL);
	time_string = ctime(&t);
	time_string[strlen(time_string) - 1] = '\0';
	fprintf(fp, "%s, ", time_string);
	for (i = 0; i< *nb_modules; i++)
	{
		if(!get_all_data((*(*conf + i)).module_address.code, (data+8*i)))
		{
			printf("Error while reading module %02hhx"
					", skipping to next module\n", i);
			continue;
		}
		for(j = 0; j < NB_CHANNELS; j++)
		{

			if((*(*conf + i)).channel_enable[j] == 1)
				fprintf(fp, "%.1f \t,", data[8*i+j]);
			else
				fprintf(fp, "\t,");
		}
	}
	fprintf(fp, "\n");
	return 1;
}


//@TODO : Ajouter limite superieure scan : max_add
uint8_t load_config_from_modules(uint8_t max_add)
{
	uint8_t i;
	uint8_t *nb_modules = NULL, *add_modules = NULL;

	// Get configuration pointer & dealoccate old config mem
	configuration **conf = get_current_config(&nb_modules);

	// Get the list of addresses with modules
	if((*nb_modules = scan_modules(&add_modules, max_add)) == 0)
	{
		printf("Error while scanning or no modules connected");
		return 0;
	}
	printf("Detected number of modules : %i\n", *nb_modules);
	if(*conf != NULL)
		free(*conf);
	if((*conf = malloc(sizeof(configuration)*(*nb_modules))) == NULL)
	{
		printf("Memory error\n");
		return 0;
	}
	//memset(*conf, 0, sizeof(configuration)*(*nb_modules));

	for(i = 0; i < *nb_modules; i++)
	{
		if((get_configuration(add_modules[i], (*conf + i))) == 0)
			return 0;
		print_configuration((*conf + i), stdout);
	}

	free(add_modules);
	return 1;
}


uint8_t load_config_from_file(char * path)
{
	uint8_t i;
	uint8_t *nb_modules = NULL;

	// Get configuration pointer & dealoccate old config mem
	configuration **conf = get_current_config(&nb_modules);


	config_t cfg;	
	config_init(&cfg);
	config_set_auto_convert(&cfg, 1);

	if(config_read_file(&cfg, path) == CONFIG_FALSE)
	{
		printf("Error reading file\n");
		return 0;
	}

	if((*nb_modules = parse_number_of_modules(&cfg)) == 0)
	{
		printf("No modules found in config\n");
		return 0;
	}

	if(*conf != NULL)
		free(*conf);
	if((*conf = malloc(sizeof(configuration)*(*nb_modules))) == NULL)
	{
		printf("Memory error\n");
		return 0;
	}
	memset(*conf, 0, sizeof(configuration)*(*nb_modules));

	parse_config_from_file(&cfg, *conf, *nb_modules);

	for(i = 0; i < *nb_modules; i++)
		print_configuration(*conf + i, stdout);

	config_destroy(&cfg);

	return 1;
}


void module_management() 
{
	uint8_t *number;
	uint8_t i;
	configuration **conf = get_current_config(&number);
	char *save_path = NULL, *usr_choice = NULL;
	size_t input_size = 100, in = 0;

	save_path = malloc(100);
	usr_choice = malloc(100);

	do
	{
		printf("\n\t\t*************************\t\n\n");
		printf("\t\t[%c] : Afficher configuration actuelle\n", 'a');
		printf("\t\t[%c] : Configuration par fichier\n", 'f');
		printf("\t\t[%c] : Configuration par scan\n", 'm');
		printf("\t\t[%c] : Sauvegarder configuration\n", 's');
		printf("\t\t[%c] : Modifier configuration\n", 'c');
		printf("\t\t[%c] : Quitter\n", 'q');

		getline(&usr_choice, &input_size, stdin);


		switch(usr_choice[0])
		{
			case 'f':
				printf("Fichier : ");
				in = getline(&save_path, &input_size, stdin);
				save_path[in - 1] = '\0';
				load_config_from_file(save_path);
				break;
			case 'm':
				load_config_from_modules(0x0c);
				break;
			case 's':
				printf("Fichier : ");
				in = getline(&save_path, &input_size, stdin);
				save_path[in - 1] = '\0';
				save_config_to_file(save_path, *conf, *number);
				break;
			case 'c':
				change_config();
				break;
			case 'a':
				if((conf = get_current_config(&number)) != NULL
						&& *number != 0)
				{
					for(i = 0; i < *number; i++)
						print_configuration(*conf + i, stdout);
				}
				else
					printf("No configuration loaded\n");
				break;
			default:
				break;

		};
		input_size = 0;
	}while(usr_choice[0] != 'q');
	free(usr_choice);
	return;
}


uint8_t change_config()
{
	uint8_t *number = NULL, i = 0, usr_choice = 0, base_add = 0;
	char *input = NULL;
	size_t input_size = 0;

	configuration *cfg = NULL;
	configuration **conf = get_current_config(&number);

	printf("Module : ");
	getline(&input, &input_size, stdin);
	base_add = (uint8_t) strtol(input, NULL, 0);
	for(i = 0; i < *number; i++)
	{
		if(base_add == (*(*conf + i)).module_address.code)
			cfg = &(*conf)[i];
	}

	if(cfg == NULL)
	{
		printf("Error : modules does not exist in config\n");
		return 0;
	}

	do
	{
		printf("\t\t[%c] : Addresse du module\n", '1');
		printf("\t\t[%c] : Integration time\n", '2');
		printf("\t\t[%c] : CJC calibration\n", '3');
		printf("\t\t[%c] : Channel type\n", '4');
		printf("\t\t[%c] : Channel enabled\n", '5');
		printf("\t\t[%c] : Quitter\n", 'q');

		getline(&input, &input_size, stdin);
		usr_choice = input[0];

		switch(usr_choice)
		{
			case '1':
				change_address(cfg);
				break;
			case '2':
				change_integration_time(cfg);
				break;
			case '3' :
				change_calibration(cfg);
				break;
			case '4' :
				change_channels_type_status(cfg);
				break;
			case '5' :
				break;
			default:
				break;
		};
	}while(usr_choice != 'q');
	return 1;
}


// @TODO : check error management for config file and output file
int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	char *usr_choice = NULL;
	size_t max_input_size = 0;
	struct arguments arguments;
	uint8_t *number;
	configuration **conf  = get_current_config(&number);

	/* Default values. */
	arguments.output_file_path = NULL;
	arguments.config_file_path = NULL;
	arguments.device = NULL;
	arguments.interactive = 1;
	arguments.duration = UINT32_MAX;
	arguments.freq = 0.2;
	arguments.baud = 0;

	/* Parse our arguments; every option seen by parse_opt will be
	   reflected in arguments. */
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	sleep(5);

	if(arguments.device != NULL)
	{
		if(!init_connexion(arguments.device, arguments.baud))
			return 0;
	}
	else if(!init_connexion("/dev/ADAM", arguments.baud))
		return 0;
	if(!arguments.interactive)
	{
		// If no output path is given, set it to default
		// Otherwise, try to open the given file
		if(arguments.output_file_path == NULL)
		{
			if( !open_new_file(&fp))
				return 0;
		}
		else if(strcmp(arguments.output_file_path, "stdout") == 0)
			fp = stdout;
		else if((fp = fopen(arguments.output_file_path, "a+")) == NULL)
		{
			printf("Couldn't open output file : %s\n",
					arguments.output_file_path);
			return errno;
		}

		// If no config path is given, scan the modules to get the 
		// values in place. Otherwise, try to get config from file
		if(arguments.config_file_path != NULL)
		{
			if(load_config_from_file(arguments.config_file_path) == 0)
				return errno;
		}
		else
			load_config_from_modules(0x10);

		// Start acquisition with the given parameters
		start_acquisition(fp, arguments.duration, arguments.freq);
	}

	else 
	{
		do
		{
			printf("\n\t\t*************************\t\n\n");
			printf("\t\t[%c] : Gestion des modules\n", 'g');
			printf("\t\t[%c] : Paramètres d'acquisition\n", 'p');
			printf("\t\t[%c] : Lancer l'acquisition\n", 'm');
			printf("\t\t[%c] : Lire les températures\n", 'l');
			printf("\t\t[%c] : Quitter\n", 'q');

			getline(&usr_choice, &max_input_size, stdin);

			switch(usr_choice[0])
			{
				case 'g':
					module_management();
					break;
				case 'p':
					prompt_acquisition();
					break;
				case 'l' :
					read_all(stdout);
					break;
				default:
					break;
			};
		}while(usr_choice[0] != 'q');
	}

	fclose(fp);
	close_connexion();
	free(*conf);
	return 0;
}


static uint8_t open_new_file(FILE **fp)
{
	if(*fp != NULL)
		fclose(*fp);

	static DIR* FD;

	static char *path_base = DEFAULT_OUTPUT_PATH;
	char *path = malloc(100);

	time_t t = time(NULL);
	struct tm *tt = localtime(&t);

	// Path specified in the config, add a suffix to it

	if(NULL == (FD = opendir(path_base)))
	{
		printf("Creating directory %s ...", path_base);
		mkdir(path_base, S_IRWXU | S_IRWXG | S_IRWXO);
		if(NULL == (FD = opendir(path_base)))
		{
			printf("Error opening output directory");
			return 0;
		}
	}
	closedir(FD);

	sprintf(path, "%s%02i-%02i-%i_data", path_base, 
			tt->tm_mday, tt->tm_mon + 1, tt->tm_year + 1900);

	if(!(*fp = fopen(path, "a+")))
	{
		printf("Can't open file %s\n", path);
		return 0;
	}

	free(path);
	return 1;
}


static configuration **get_current_config(uint8_t **nb)
{
	static uint8_t nb_modules = 0;
	static configuration *cfg = NULL;
	if(nb != NULL)
		*nb = &nb_modules;
	return &cfg;
}


static uint8_t parse_number_of_modules(config_t *cfg)
{
	const char *path = "modules_address";
	config_setting_t *t;

	t = config_lookup (cfg, path);

	return config_setting_length(t);
}


static inline uint8_t uint_checker(int n)
{
	if(n > 0xFF || n < 0)
		return 0;
	return 1;
}


static void parse_config_from_file(config_t *file_cfg, configuration *cfg, 
		uint8_t nb)
{
	uint8_t i, j, l, nb_ch;
	int k = 0;
	const char *tmp;
	char tmp2[20];
	config_setting_t *set, *add, *module, *status, *type;

	init_struct_configuration(cfg);

	if((add = config_lookup(file_cfg, "modules_address")) == NULL
			|| config_setting_type(add) != CONFIG_TYPE_ARRAY)
	{
		printf("Invalid syntax : couldn't find %s", "modules_address");
		return;
	}

	for(i = 0; i < nb; i++)
	{
		// Get the address of the module number i
		if(config_setting_type(config_setting_get_elem(add, i)) !=
				CONFIG_TYPE_INT)
		{
			printf("Invalid syntax in modules_address[%i]\n", i);
			continue;
		}


		k = config_setting_get_int_elem(
				config_lookup(file_cfg, "modules_address"), i);
		if(!uint_checker(k))
		{
			printf("Invalid address for module nb %i : %02hhX."
					" Addresses must be between 0x0 and"
					" 0xFF \n", i, k);
			continue;
		}

		cfg[i].module_address.code = k;
		sprintf(cfg[i].module_address.name, "%02hhX",
				cfg[i].module_address.code);

		// Get the setting grouping information for a particular module
		// module_1, module_2...
		module = config_setting_get_elem(config_lookup(file_cfg,
					"modules"), i);

		if((set = config_setting_get_member(module, "baudrate")) != NULL)
		{
			k = config_setting_get_int(set);
			for(j = 0; j < 8; j++)
			{
				if(k == baudrate_conf[j].code)
				{
					cfg[i].baudrate.code = 
						baudrate_conf[j].code;
					sprintf(cfg[i].baudrate.name, 
							baudrate_conf[j].name);
					break;
				}
			}
		}

		if((set = config_setting_get_member(module, "format")) != NULL
				&& config_setting_type(set) == CONFIG_TYPE_STRING)
		{
			tmp = config_setting_get_string(set);
			for(j = 0; j < 3; j++)
			{
				if(!strcmp(tmp, data_format[j].name))
				{
					cfg[i].format.code = 
						data_format[j].code;
					sprintf(cfg[i].format.name, 
							data_format[j].name);
					break;
				}
			}
		}

		if((set = config_setting_get_member(module, "checksum")) != NULL
				&& config_setting_type(set) == CONFIG_TYPE_INT)
		{
			k = config_setting_get_int(set);
			for(j = 0; j < 2; j++)
			{
				if(k == checksum_status[j].code)
				{
					cfg[i].checksum.code = 
						checksum_status[j].code;
					sprintf(cfg[i].checksum.name, 
							checksum_status[j].name);
					break;
				}
			}
		}

		if((set = config_setting_get_member(module, "integration_time")) != NULL
				&& config_setting_type(set) == CONFIG_TYPE_INT)
		{
			k = config_setting_get_int(set);
			for(j = 0; j < 2; j++)
			{
				sprintf(tmp2, "%i ms", k); 
				if(!strcmp(tmp2, integration_time[j].name))
				{
					cfg[i].integration.code = 
						integration_time[j].code;
					sprintf(cfg[i].integration.name, 
							integration_time[j].name);
					break;
				}
			}
		}

		if((set = config_setting_get_member(module, "module_name")) != NULL
				&& config_setting_type(set) == CONFIG_TYPE_STRING)
		{
			tmp = config_setting_get_string(set);
			strncpy(cfg[i].module_name, tmp, MAX_SIZE_MSG);
		}

		if((set = config_setting_get_member(module, "firmware_version")) != NULL
				&& config_setting_type(set) == CONFIG_TYPE_STRING)
		{
			tmp = config_setting_get_string(set);
			strncpy(cfg[i].firmware_version, tmp, MAX_SIZE_MSG);
		}

		if((set = config_setting_get_member(module, "CJC")) != NULL
				&& config_setting_type(set) == CONFIG_TYPE_FLOAT)
		{
			cfg[i].temp_CJC = config_setting_get_float(set);
		}

		if((status = config_setting_get_member(module, "status")) == NULL
				|| (nb_ch = config_setting_length(status)) == 0)
		{
			printf("No active channels for module %02hhX\n",
					cfg[i].module_address.code);
			continue;
		}	       
		if((type = config_setting_get_member(module, "type")) == NULL
				|| config_setting_length(type) != nb_ch)
			continue;
		for(j = 0; j < nb_ch; j++)
		{
			set = config_setting_get_elem(status, j);
			// Get first enabled channel
			if(config_setting_type(set) == CONFIG_TYPE_INT)
				k = config_setting_get_int(set);

			cfg[i].channel_enable[k] = 1;

			set = config_setting_get_elem(type, j);
			if(config_setting_type(set) != CONFIG_TYPE_STRING)
				continue;
			// get the type name	
			tmp = config_setting_get_string(set);
			for(l = 0; l < 8; l++)
			{
				if(!strcmp(tmp, input_type[l].name))
				{
					cfg[i].ch_input[k].code = 
						input_type[l].code;
					strncpy(cfg[i].ch_input[k].name, 
							input_type[l].name, 30);
					break;
				}
			}
		}
	}
	return;

}


static uint8_t save_config_to_file(char *path, configuration *conf, uint8_t nb)
{
	uint8_t i, j;
	char mod_name[12];

	config_t config;
	config_init(&config);

	config_setting_t *root = config_root_setting (&config);
	config_setting_t *module, *set, *tmp;

	config_setting_add (root, "modules_address", CONFIG_TYPE_ARRAY);
	config_setting_add (root, "modules", CONFIG_TYPE_GROUP);

	for(i = 0; i < nb; i++)
	{
		sprintf(mod_name, "module_%i", i + 1);

		// -1 = add new value to an array
		config_setting_set_int_elem(
				config_lookup(&config, "modules_address"),
				-1, conf[i].module_address.code);

		module = config_setting_add(config_lookup(&config, "modules"),
				mod_name,
				CONFIG_TYPE_GROUP);

		// Default to 9600
		set = config_setting_add(module, "baudrate", CONFIG_TYPE_STRING);
		config_setting_set_string(set, 
				conf[i].baudrate.code == 0 ? "9600" : conf[i].baudrate.name); 

		// Default to "Engineering Units"
		set = config_setting_add(module, "format", CONFIG_TYPE_STRING);
		config_setting_set_string(set, conf[i].format.name); 

		// Default to 0 (no checksum)
		set = config_setting_add(module, "checksum", CONFIG_TYPE_INT);
		config_setting_set_int(set, conf[i].checksum.code); 

		// Default to 50 ms
		set = config_setting_add(module, "integration_time", CONFIG_TYPE_STRING);
		config_setting_set_string(set, conf[i].integration.name); 

		// Default to "undefined"
		set = config_setting_add(module, "input", CONFIG_TYPE_STRING);
		config_setting_set_string(set, conf[i].input.name); 

		// Default to ""
		set = config_setting_add(module, "module_name", CONFIG_TYPE_STRING);
		config_setting_set_string(set, conf[i].module_name); 

		// Default to ""
		set = config_setting_add(module, "firmware_version", CONFIG_TYPE_STRING);
		config_setting_set_string(set, conf[i].firmware_version); 

		// Default to 0
		set = config_setting_add(module, "CJC", CONFIG_TYPE_FLOAT);
		config_setting_set_float(set, conf[i].temp_CJC); 

		set = config_setting_add(module, "status", CONFIG_TYPE_ARRAY);
		tmp = config_setting_add(module, "type", CONFIG_TYPE_ARRAY);
		for(j = 0; j < NB_CHANNELS; j++)
		{
			if(conf[i].channel_enable[j] == 1)
			{
				// -1 = add new value to an array
				config_setting_set_int_elem(set, -1, j);
				config_setting_set_string_elem(tmp, -1, 
						conf[i].ch_input[j].name);
			}
		}
	}

	if(config_write_file(&config, path) == CONFIG_FALSE)
	{
		printf("Couldn't write to file %s\n", path);
		return 0;
	}

	config_destroy(&config);
	return 1;
}


static void print_header(FILE *fp)
{
	uint8_t j;
	uint8_t *nb;
	get_current_config(&nb);
	for(j = 0; j < 27; j++)
		fprintf(fp, " ");
	if(*nb > 1)
	{
		for(j = 0 ; j < *nb; j++)
			fprintf(fp, ",Module %i\t\t\t\t\t\t\t\t", j);
	}
	fprintf(fp, "\n");
	fprintf(fp, "Date,");
	for(j = 0; j < 27 - strlen("Date,"); j++)
		fprintf(fp, " ");
	for(j = 0 ; j < NB_CHANNELS*(*nb); j++)
		fprintf(fp, "M%iCh%i,\t",j/8, j%8);
	fprintf(fp, "\n");
}


static uint8_t change_address(configuration *cfg)
{
	char * input = NULL;
	uint8_t base_add = cfg->module_address.code;
	long int parse;

	printf("New address for module %02hhX : ", base_add);
	getline(&input, NULL, stdin);
	parse = strtol(input, NULL, 0);
	free(input);
	if(parse < 0 || parse > 0xFF)
	{
		printf("Address must be between 0x0 and 0xFF\n");
		return 0;;
	}
	if(!check_add(parse))
		return 0;

	cfg->module_address.code = parse;
	sprintf(cfg->module_address.name, "%02hhX", (uint8_t) parse);

	if(!set_configuration_status(base_add, cfg))
		return 0;
	return 1;
}


static uint8_t change_calibration(configuration *cfg)
{
	printf("Changing calibration...\n");

	printf("Not implemented\n");
	return 1;
}


static uint8_t change_integration_time(configuration *cfg)
{
	long int parse;
	char *input = NULL;

	printf("Change integration time to : \n\t[0] : 50ms\n\t[1] : 60ms\n");

	getline(&input, NULL, stdin);
	parse = strtol(input, NULL, 0);
	free(input);

	if(parse < 0 || parse > 1)
	{
		printf("Please choose 0 or 1\n");
		return 0;
	}

	cfg->integration.code = parse;
	strncpy(cfg->integration.name, integration_time[parse].name, 6);

	if(!set_configuration_status(cfg->module_address.code, cfg))
		return 0;
	return 1;
}


static void print_channels(configuration *cfg)
{
	uint8_t i;

	// Print channels
	printf("\t\t\t\t");
	for(i = 0; i< NB_CHANNELS; i++)
		printf("%i - ", i);
	printf("\n\tChannel input type : \t");
	for(i = 0; i< NB_CHANNELS; i++)
	{
		if(cfg->ch_input[i].name[3] == 0)
			cfg->ch_input[i].name[3] = '0';
		printf("%c - ", cfg->ch_input[i].name[3]);
	}
	printf("\n\tChannel status : \t");
	for(i = 0; i< NB_CHANNELS; i++)
		printf("%i - ", cfg->channel_enable[i]);
	printf("\n\tChannel state : \t");
	for(i = 0; i< NB_CHANNELS; i++)
		printf("%i - ", cfg->channel_condition[i]);
	printf("\n");
}


static uint8_t change_channels_type_status(configuration *cfg)
{
	long int parse;
	char *input = NULL, t[6] = "TC_";
	size_t in = 0, input_size = 0;
	uint8_t ch = -1, i = 0;

	print_channels(cfg);

	printf("Channel to configure :");
	getline(&input, &input_size, stdin);
	parse = strtol(input, NULL, 0);

	if(parse < 0 || parse > 7)
	{
		printf("Please choose between 0 and 7\n");
		return 0;
	}
	ch = (uint8_t) parse;

	printf("Enable ? (1/0) ");
	getline(&input, &input_size, stdin);
	parse = strtol(input, NULL, 0);
	if(parse == 0 || parse == 1)
	{
		cfg->channel_enable[ch] = parse;
		if(!set_channels_status(cfg->module_address.code, cfg->channel_enable))
			return 0;
	}
	printf("Type (J, K, T, E, R, S, B) ? ");
	in = getline(&input, &input_size, stdin);
	input[in-1] = '\0';
	strncat(t, input, 2);
	free(input);

	// Pass if only \n or if multiple letters have been entered
	if(in == 2)
	{
		for(i = 0; i < 8; i++)
		{
			if(strcmp(t, input_type[i].name) == 0)
			{
				cfg->ch_input[ch].code = input_type[i].code;
				strncpy(cfg->ch_input[ch].name,
						input_type[i].name, 
						30);
				if(!set_channel_type(cfg->module_address.code,
							ch, &cfg->ch_input[ch]))
					return 0;
			}
		}

	}

	return 1;
}


static uint8_t check_add(uint8_t add)
{
	uint8_t i, *number = NULL;
	configuration **conf = get_current_config(&number);

	for(i = 0; i < *number; i++)
	{
		if(add == (*(*conf + i)).module_address.code)
			return 1;
	}
	printf("Adress already existing\n");
	return 0;
}


