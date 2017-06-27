#include "prog.h"

static uint8_t parse_number_of_modules(config_t *cfg);
static void parse_config_to_struct(config_t *file_cfg, configuration *cfg, 
		uint8_t nb);
static configuration **get_current_config(uint8_t *nb);

uint8_t exec_config()
{
	uint8_t i, *nb_modules;
	configuration **conf = get_current_config(nb_modules);
	for(i = 0; i < *nb_modules; i++)
		set_configuration(conf[0]->module_address.code, conf[0]);
}


//@TODO : Ajouter limite superieure scan : max_add
uint8_t load_config_from_modules(uint8_t max_add)
{
	uint8_t i;
	uint8_t *nb_modules, *add_modules;

	// Get configuration pointer & dealoccate old config mem
	configuration **conf = get_current_config(nb_modules);


	// Get a list of addresses with modules
	if((*nb_modules = scan_modules(add_modules)) == 0)
	{
		printf("Error while scanning or no modules connected");
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

	for(i = 0; i < *nb_modules; i++)
	{
		if((get_configuration(add_modules[i], conf[i])) == 0)
			return 0;
		print_configuration(conf[i]);
	}

	free(add_modules);
	return 1;
}


uint8_t load_config_from_file(char * path)
{
	uint8_t *nb_modules;
	uint8_t i, j = 0;
	
	// Get configuration pointer & dealoccate old config mem
	configuration **conf = get_current_config(nb_modules);


	config_t cfg;	
	config_init(&cfg);
	config_set_auto_convert(&cfg, 1);

	config_setting_t *root;
	root = config_root_setting(&cfg);

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

	parse_config_to_struct(&cfg, *conf, *nb_modules);

	for(i = 0; i < *nb_modules; i++)
		print_configuration(conf[i]);

	free(conf);
	config_destroy(&cfg);

	return 1;
}




void module_management() 
{
	char usr_choice;
	printf("\t[%c] : Chargement fichier de configuration\n", 'c');
	printf("\t[%c] : Modules reconnus\n", 'm');
	printf("\t[%c] : Détails d'un module\n", 'd');
	scanf("%s", &usr_choice);

	switch(usr_choice)
	{
		case 'c':
			printf("c\n");
			load_config_from_file("/home/pi/TC_Eolienne/config/config_1");
			break;
		default:
			break;
	};
	return;
}


int main(int argc, char *argv[])
{

	init_connexion("/dev/ADAM", 0);
	configuration cfg;
	load_config_from_modules(0xFF);
	close_connexion();
	/*
	char usr_choice;
	printf("\t[%c] : Gestion des modules\n", 'g');
	printf("\t[%c] : Paramètres d'acquisition\n", 'p');
	printf("\t[%c] : Lancer l'acquisition\n", 'm');
	printf("\t[%c] : Lire les températures\n", 'l');
	scanf("%c", &usr_choice);

	switch(usr_choice)
	{
		case 'g':
			module_management();
			break;
		default:
			break;
	};
*/

	return 0;
}



static configuration **get_current_config(uint8_t *nb)
{
	static uint8_t nb_modules = 0;
	static configuration *cfg = NULL;
	nb = &nb_modules;
	return &cfg;
}

static uint8_t parse_number_of_modules(config_t *cfg)
{
	const char *path = "modules_address";
	config_setting_t *t;

	t = config_lookup (cfg, path);

	return config_setting_length(t);
}


static void parse_config_to_struct(config_t *file_cfg, configuration *cfg, 
		uint8_t nb)
{
	uint8_t i, j, k = 0;
	const char *type_name;
	config_setting_t *setting;
	config_setting_t *status_setting, *type_setting;

	setting = config_lookup (file_cfg, "modules");

	for(i = 0; i < nb; i++)
	{
		printf("££££££ Module %i\n", i);
		status_setting = config_setting_get_member(
				config_setting_get_elem(setting, i), "status");
		type_setting = config_setting_get_member(
				config_setting_get_elem(setting, i), "type");
		while((j = config_setting_get_int_elem(status_setting, k)))
		{
			cfg[i].channel_enable[j] = 1;
			type_name = config_setting_get_string_elem(type_setting, k); 
			printf("\t%s\n", type_name);
			strncpy(cfg[i].ch_input[j].name, type_name, 5);
			k++;
		}
		k = 0;
	}
}



