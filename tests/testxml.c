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

static xmlNodePtr get_int_val(char *name, xmlNodePtr cur);
static xmlNodePtr get_first_node(char *name, xmlNodePtr cur);
static void parseDoc(char *docname);
static uint8_t get_node_id(xmlNodePtr cur, char **id);

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

static xmlNodePtr get_first_node(char *name, xmlNodePtr cur)
{	
	if(cur != NULL)
		printf("%s\n", cur->name);
	if(cur == NULL || !xmlStrcmp(cur->name, name))
	{
		printf("end\n");
		return cur;
	}

	xmlNodePtr save_cur = cur;
	cur = get_first_node(name, save_cur->children);
	if(!cur)
		cur = get_first_node(name, save_cur->next);
	return cur;
}

static xmlNodePtr get_int_val(char *name, xmlNodePtr cur)
{
	char * cur_name;
	printf("getintval\n");
	cur = get_first_node("integer", cur);
	printf("--->%s\n", cur->name); 
	printf("--->%s\n", xmlGetProp(cur, "name")); 
	if(cur == NULL)
	{
		printf("Not found : %s\n", name);
		return NULL;
	}
	else if((cur_name = xmlGetProp(cur, "name")) != NULL
			&& !strcmp(cur_name, name))
	{
		printf("Found val :  of %s\n", xmlGetProp(cur, "val"), cur_name);
		return cur;
	}
	else
	{
		sleep(1);
		cur = get_int_val(name, cur);
	}

	return cur;
}

static void parseDoc(char *docname) 
{

	char *id = NULL;
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

	printf("%s\n", cur->name);
	if (xmlStrcmp(cur->name, (const xmlChar *) "obj")) {
		fprintf(stderr,"document of the wrong type, root node != story\n");
		xmlFreeDoc(doc);
		return;
	}
	get_int_val("pe", cur);
/*
	cur = get_first_node("pe", cur);
	printf("Mark 1\n");
	if((id = xmlGetProp(cur, "name")) != NULL)
	{
		printf("%s\n", id);
		xmlFree(id);
	}
	*/
	/*for (cur = cur->children; cur != NULL; cur = cur->next) {
		fprintf(stdout, "\t Child is <%s> (%i) \n", cur->name, cur->type);
		if(cur->type == XML_ELEMENT_NODE && !xmlStrcmp(cur->name, "Data"))
		{
			cur = cur->children;
			while(cur)
			{
				if(get_node_id(cur, &id) == 0)
				{
					cur = cur->next;
					continue;
				}
				printf("ID : %s\n", id);
				cur = cur->next;
			}
		}
		printf("Mark 3 : %s\n", cur);
	}*/

	printf("Mark 4\n");
	xmlFreeDoc(doc);
	return;
}


