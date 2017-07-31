/*
   Simple test with libxml2 <http://xmlsoft.org>. It displays the name
   of the root element and the names of all its children (not
   descendents, just children).

   On Debian, compiles with:
   gcc -Wall -o read-xml2 $(xml2-config --cflags) $(xml2-config --libs) \
   read-xml2.c    

*/

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static xmlNodePtr get_val(char *type, char *name, xmlNodePtr cur);
static xmlNodePtr get_int_val(char *name, xmlNodePtr cur);
static xmlNodePtr get_bool_val(char *name, xmlNodePtr cur);
static xmlNodePtr get_first_node(char *name, xmlNodePtr cur);
static void parseDoc(char *docname);
static uint8_t get_node_id(xmlNodePtr cur, char **id);
static inline uint8_t end_doc(xmlNodePtr cur);

int main(int argc, char **argv)
{
	parseDoc("config");		


}


static uint8_t get_node_id(xmlNodePtr cur, char **id)
{
	if(*id != NULL)
		xmlFree(*id);
	if((*id = xmlGetProp(cur, "id")) == NULL)
		return 0;
	return 1;
}


static inline uint8_t end_doc(xmlNodePtr cur)
{
    if(cur == NULL || (cur->next == NULL && cur->children == NULL))
        return 1;
    return 0;
}


static xmlNodePtr get_first_node(char *name, xmlNodePtr cur)
{	
    static xmlNodePtr last_node = NULL;

    if(name == NULL)
    {
            printf("Error : name is NULL\n");
            return NULL;
    }
    else if(cur == NULL && last_node == NULL)   
    {
        printf("No node specified\n");
        return NULL;
    }
    else if(cur == NULL)
    {
        cur = last_node;
    }
        else
    {
        last_node = cur;

        if(!xmlStrcmp(cur->name, name))
            return cur;
    }

	xmlNodePtr save_cur = cur;
    if(save_cur->children != NULL)
    {
	    cur = get_first_node(name, save_cur->children);
    }
	if((save_cur->children == NULL || !cur) && save_cur->next != NULL)
    {
        cur = get_first_node(name, save_cur->next);
    }
    if(end_doc(cur))
        cur = NULL;
	return cur;
}


static xmlNodePtr get_val(char *type, char *name, xmlNodePtr cur)
{
    char * cur_name = NULL;

    cur = get_first_node(type, cur);

    while(cur != NULL)
    {
        if((cur_name = xmlGetProp(cur, "name")) && !strcmp(cur_name, name))
            break;
        cur = get_first_node(type, NULL);
    }

    return cur;
}

static xmlNodePtr get_int_val(char *name, xmlNodePtr cur)
{
    static char *name_int = "integer";
	char * cur_name;
	cur = get_first_node(name_int, cur);
	if(cur == NULL)
	{
		// printf("Not found : %s\n", name);
		return NULL;
	}
	else if((cur_name = xmlGetProp(cur, "name")) != NULL
			&& !strcmp(cur_name, name))
	{
	    // printf("Found val : %s  of %s\n", xmlGetProp(cur, "val"), cur_name);
		return cur;
	}
	else
	{
        name_int = NULL;
		cur = get_int_val(name, cur);
	}

	return cur;
}


static xmlNodePtr get_bool_val(char *name, xmlNodePtr cur)
{
    static char *name_bool = "bool";
	char * cur_name;
	cur = get_first_node(name_bool, cur);
	if(cur == NULL)
	{
		printf("Not found : %s\n", name);
		return NULL;
	}
	else if((cur_name = xmlGetProp(cur, "name")) != NULL
			&& !strcmp(cur_name, name))
	{
	    printf("Found val : %s  of %s\n", xmlGetProp(cur, "val"), cur_name);
		return cur;
	}
	else
	{
        name_bool = NULL;
		cur = get_bool_val(name, cur);
	}

	return cur;
}


static void parseDoc(char *docname) 
{
    uint8_t i = 0, j = 0, k = 0;
    uint8_t tab[10] = {0};
    uint8_t tab2[8] = {0};
    uint8_t tab3[8] = {0};
    char name_node[20] = "";
	char *id = NULL;
	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr tmp_cur;

	doc = xmlParseFile(docname);

	if (doc == NULL ) {
		fprintf(stderr,"Document not parsed successfully. \n");
		return;
	}

	root_node = xmlDocGetRootElement(doc);

	if (root_node == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return;
	}

	if (xmlStrcmp(root_node->name, (const xmlChar *) "obj")) {
		fprintf(stderr,"document of the wrong type, root node != obj\n");
		xmlFreeDoc(doc);
		return;
	}
   
/*
    cur = get_val("str", "sensor_type_M1_ch6", cur);
    printf("cur name : %s\n", xmlGetProp(cur, "val"));
  */                        
    /*
    while(i < 255)
    {
        sprintf(name_node, "ad_mod_%i", i++);
        tmp_cur = get_val("integer", name_node, root_node);
        if(tmp_cur != NULL)
            tab[j++] = strtol(xmlGetProp(tmp_cur, "val"), NULL, 0);
    }
    j = 0;    
    while(tab[j] != 0)
    {
        for(i = 0; i < 8; i++)
        {
            sprintf(name_node, "M%i_ch%i_enable", tab[j], i + 1);
            tmp_cur = get_val("bool", name_node, root_node);
            if(tmp_cur != NULL)
            {
                if(!strcmp("true", xmlGetProp(tmp_cur, "val")))
                    tab2[i] = 1;
            }
            sprintf(name_node, "sensor_type_M%i_ch%i", tab[j], i + 1);
            tmp_cur = get_val("str", name_node, root_node);
            if(tmp_cur != NULL)
            {
                    tab3[i] = xmlGetProp(tmp_cur, "val")[0];
            }

        }
        printf("\n Module %i\nEnable :\t", tab[j]);
        for(k = 0; k < 8; k++)
            printf("%i - ", tab2[k]);
        printf("\nType :\t\t");
        for(k = 0; k < 8; k++)
            printf("%c - ", tab3[k]);
        j++;
        memset(tab2, 0, 8);
    }
    printf("\n");

    */
    xmlFreeDoc(doc);
    xmlCleanupParser();
    return;
}


