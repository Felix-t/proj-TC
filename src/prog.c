#include "prog.h"

static uint8_t parse_number_of_modules(config_t *cfg);
static void parse_config_from_file(config_t *file_cfg, configuration *cfg, 
		uint8_t nb);
static configuration **get_current_config(uint8_t **nb);
static uint8_t save_config_to_file(char *path, configuration *conf, uint8_t nb);
static void print_header(FILE *fp);
static uint8_t uint_checker(int n);

// @TODO : gestion d'erreur
uint8_t start_acquisition()
{
	float freq, duration;
	char file[100];
	FILE *fp;
	time_t start_time;

	do
	{
		printf("Frequence (mesures/minutes) : ");
		scanf("%f", &freq);
		if(freq == 0)
			freq = 0xFFFFFFFFFFFF;
	}while(freq < 0);
	do
	{
		printf("Fichier de capture : ");
		scanf("%s", file);
	}while((fp = fopen(file, "a+")) == NULL);
	do
	{
		printf("Durée (minutes) : ");
		scanf("%f", &duration);
	}while(duration <= 0);

	start_time = time(NULL);
	print_header(fp);

	while(difftime(time(NULL), start_time) < (double) 60*duration )
	{
		read_all(fp);
		fflush(fp);
		sleep(60 / freq);
	}
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

	for (i = 0; i< *nb_modules; i++)
	{
		if(!get_all_data((*conf[i]).module_address.code, (data+8*i)))
		{
			printf("Error while reading module %02hhx"
					", skipping to next module\n", i);
			continue;
		}
		for(j = 0; j < NB_CHANNELS; j++)
			fprintf(fp, "%f, ", data[i+8*j]);
		fprintf(fp, "\n");
	}
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
	printf("valeur : %i, addresse : %p\n", *nb_modules, nb_modules);
	printf("conf : %p\n", conf);
	printf("conf* : %p\n", *conf);

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
		if((get_configuration(add_modules[i], &*conf[i])) == 0)
			return 0;
		print_configuration(&*conf[i], stdout);
	}

	printf("valeur : %i, addresse : %p\n", *nb_modules, nb_modules);

	free(add_modules);
	return 1;
}


uint8_t load_config_from_file(char * path)
{
	uint8_t i;
	uint8_t *nb_modules = NULL;

	// Get configuration pointer & dealoccate old config mem
	configuration **conf = get_current_config(&nb_modules);
	printf("Address nb modules : %p", nb_modules);


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
	char usr_choice = 0;
	char save_path[100] = "";
	uint8_t *follow = number;

	printf("address number spe : %p\n", &number);

	while(usr_choice != 'q')
	{
		printf("valeur : %i, addresse : %p\n", *number, number);
		printf("\t\t[%c] : Chargement fichier de configuration\n", 'c');
		printf("\t\t[%c] : Configuration par scan\n", 'm');
		printf("\t\t[%c] : Afficher configuration actuelle\n", 'a');
		printf("\t\t[%c] : Sauvegarder configuration\n", 's');
		printf("\t\t[%c] : Quitter\n", 'q');
		printf("Follow ---> valeur : %i, addresse : %p\n", *follow, follow);
		scanf("%s", &usr_choice);

		switch(usr_choice)
		{
			case 'c':
				printf("Fichier : ");
				scanf("%s", save_path);
				load_config_from_file(save_path);
				break;
			case 'm':
				load_config_from_modules(0x0c);
				break;
			case 's':
				printf("number : %i\n", *follow);
				printf("Fichier : ");
				scanf("%s", save_path);
				save_config_to_file(save_path, *conf, *follow);
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
	}
	return;
}


int main(int argc, char *argv[])
{

	init_connexion("/dev/ADAM", 0);

	char usr_choice = 0;

	while(usr_choice != 'q')
	{
		printf("\t\t[%c] : Gestion des modules\n", 'g');
		printf("\t\t[%c] : Paramètres d'acquisition\n", 'p');
		printf("\t\t[%c] : Lancer l'acquisition\n", 'm');
		printf("\t\t[%c] : Lire les températures\n", 'l');
		printf("\t\t[%c] : Quitter\n", 'q');
		scanf("%s", &usr_choice);

		switch(usr_choice)
		{
			case 'g':
				module_management();
				break;
			case 'p':
				start_acquisition();
				break;
			case 'l' :
				read_all(stdout);
				break;
			default:
				break;
		};
	}
	close_connexion();
	configuration **conf = get_current_config(NULL);
	free(*conf);
	return 0;
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
	uint8_t i, j, nb_ch;
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
			for(j = 0; j < 3; j++)
			{
				if(!strcmp(tmp, input_type[j].name))
				{
					cfg[i].ch_input[j].code = 
						input_type[j].code;
					strncpy(cfg[i].ch_input[j].name, 
							input_type[j].name, 30);
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
				config_setting_set_int_elem(set, -1, 
						conf[i].channel_enable[j]);
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
	for(j = 0 ; j < NB_CHANNELS; j++)
		fprintf(fp, "Ch%i\t", j);
	fprintf(fp, "\n");
}

