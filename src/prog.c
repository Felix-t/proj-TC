#include <argp.h>
#include <stdlib.h>
#include <error.h>
#include "adam_4018p.h"
#include <libconfig.h>


static uint8_t module_management();
static uint8_t load_config_from_file(char * path);
static uint8_t *get_config(configuration *cfg_ptr);
static void parse_config_to_struct(config_t *file_cfg, configuration *cfg, 
		uint8_t nb);

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


static uint8_t load_config_from_file(char * path)
{
	printf("Mark 1\n");
	uint8_t i, j = 0;
	configuration *conf = NULL;
	uint8_t *nb_modules = get_config(conf);

	printf("Mark 2\n");
	config_t cfg;
	config_setting_t *root;

	config_init(&cfg);
	config_set_auto_convert(&cfg, 1);
	config_read_file(&cfg, path);

	printf("Mark 3\n");
	root = config_root_setting(&cfg);

	*nb_modules = parse_number_of_modules(&cfg);

	printf("Mark 4\n");
	if(conf != NULL)
		free(conf);
	conf = malloc(sizeof(configuration)*(*nb_modules));
	memset(conf, 0, sizeof(configuration)*(*nb_modules));

	printf("Mark 5\n");
	printf("Number of modules : %i\n", *nb_modules);

	parse_config_to_struct(&cfg, conf, *nb_modules);

	printf("Mark 6\n");
	print_configuration(&conf[0]);
	print_configuration(&conf[1]);
	printf("Mark 7\n");

	free(conf);
	printf("Mark 8\n");
	config_destroy(&cfg);
}

static uint8_t *get_config(configuration *cfg_ptr)
{
	static uint8_t nb_modules = 0;
	static configuration *cfg = NULL;
	cfg_ptr = cfg;
	return &nb_modules;
}


static uint8_t module_management() 
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
	return 1;
}


int main(int argc, char *argv[]) {
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


	return 0;
}
