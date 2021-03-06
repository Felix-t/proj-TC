#include "config_io.h"
#include <libconfig.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/* @TODO : */
static uint8_t save_file_xml(char *path);
static uint8_t save_file_lconfig(char *path);
/*         */

static uint8_t load_file_xml(char *path);
static uint8_t load_file_lconfig(char *path);
//Return 1 if n is in range 0,255
static inline uint8_t uint_checker(int n);
// Find number of modules from config file
static uint8_t parse_number_of_modules(config_t *cfg);
// Fill a struct configuration[nb] with the data from a config file
static void parse_config_from_file(config_t *file_cfg, configuration *cfg, 
		uint8_t nb);
static void parseDoc(char *docname);
static void get_node (xmlDocPtr doc, xmlNodePtr cur);

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

// @TODO :
static uint8_t save_file_xml(char *path)
{
	return 1;
}

// @TODO :
static uint8_t load_file_xml(char *path)
{
	parseDoc(path);

	return 1;
}


static void get_node(xmlDocPtr doc, xmlNodePtr cur)
{

	xmlChar *uri;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"reference"))) {
		    uri = xmlGetProp(cur, "uri");
		    printf("uri: %s\n", uri);
		    xmlFree(uri);
	    }
	    cur = cur->next;
	}
	return;
}


static void parseDoc(char *docname) 
{

	xmlDocPtr doc;
	xmlNodePtr cur;

	doc = xmlParseFile(docname);
	
	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}
	
	cur = xmlDocGetRootElement(doc);
	
	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}
	
	if (xmlStrcmp(cur->name, (const xmlChar *) "Resource")) {
		fprintf(stderr,"document of the wrong type, root node != story");
		xmlFreeDoc(doc);
		return;
	}
	
	get_node (doc, cur);
	xmlFreeDoc(doc);
	return;
}


static uint8_t load_file_lconfig(char *path)
{
	uint8_t *nb_modules, i;
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


static uint8_t save_file_lconfig(char *path)
{
	uint8_t i, j, *nb;
	char mod_name[12];

	configuration **conf = get_current_config(&nb);
	config_t config;
	config_init(&config);

	config_setting_t *root = config_root_setting (&config);
	config_setting_t *module, *set, *tmp;

	config_setting_add (root, "modules_address", CONFIG_TYPE_ARRAY);
	config_setting_add (root, "modules", CONFIG_TYPE_GROUP);

	for(i = 0; i < *nb; i++)
	{
		sprintf(mod_name, "module_%i", i + 1);

		// -1 = add new value to an array
		config_setting_set_int_elem(
				config_lookup(&config, "modules_address"),
				-1, (*conf+i)->module_address.code);

		module = config_setting_add(config_lookup(&config, "modules"),
				mod_name,
				CONFIG_TYPE_GROUP);

		// Default to 9600
		set = config_setting_add(module, "baudrate", CONFIG_TYPE_STRING);
		config_setting_set_string(set, 
				(*conf + i)->baudrate.code == 0 ? "9600" : (*conf + i)->baudrate.name); 

		// Default to "Engineering Units"
		set = config_setting_add(module, "format", CONFIG_TYPE_STRING);
		config_setting_set_string(set, (*conf + i)->format.name); 

		// Default to 0 (no checksum)
		set = config_setting_add(module, "checksum", CONFIG_TYPE_INT);
		config_setting_set_int(set, (*conf + i)->checksum.code); 

		// Default to 50 ms
		set = config_setting_add(module, "integration_time", CONFIG_TYPE_STRING);
		config_setting_set_string(set, (*conf + i)->integration.name); 

		// Default to "undefined"
		set = config_setting_add(module, "input", CONFIG_TYPE_STRING);
		config_setting_set_string(set, (*conf + i)->input.name); 

		// Default to ""
		set = config_setting_add(module, "module_name", CONFIG_TYPE_STRING);
		config_setting_set_string(set, (*conf + i)->module_name); 

		// Default to ""
		set = config_setting_add(module, "firmware_version", CONFIG_TYPE_STRING);
		config_setting_set_string(set, (*conf + i)->firmware_version); 

		// Default to 0
		set = config_setting_add(module, "CJC", CONFIG_TYPE_FLOAT);
		config_setting_set_float(set, (*conf + i)->temp_CJC); 

		set = config_setting_add(module, "status", CONFIG_TYPE_ARRAY);
		tmp = config_setting_add(module, "type", CONFIG_TYPE_ARRAY);
		for(j = 0; j < NB_CHANNELS; j++)
		{
			if((*conf + i)->channel_enable[j] == 1)
			{
				// -1 = add new value to an array
				config_setting_set_int_elem(set, -1, j);
				config_setting_set_string_elem(tmp, -1, 
						(*conf + i)->ch_input[j].name);
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


uint8_t load_config_from_file(char * path)
{
	FILE *fp;

	if((fp = fopen(path, "r")) == NULL)
	{
		int err = errno;
		printf("Error opening %s : %s\n", path, strerror(err));
		return 0;
	}
	fclose(fp);

	if(strcmp(&path[strlen(path) - 4], ".xml") == 0)
		load_file_xml(path);
	else
		load_file_lconfig(path);
	return 1;
}

uint8_t save_config_to_file(char *path)
{
	FILE *fp;

	if((fp = fopen(path, "w")) == NULL)
	{
		int err = errno;
		printf("Error opening %s : %s\n", path, strerror(err));
		return 0;
	}
	fclose(fp);

	if(strcmp(&path[strlen(path) - 4], ".xml") == 0)
		save_file_xml(path);
	else
		save_file_lconfig(path);
	return 1;
}
