#include "adam_4018p.h"
#include "minunit.h"
#include <termios.h>

static char * all_tests();

int tests_run = 0;

mu_suite_start();

static char * test_unconnected_connexion() {
	mu_assert(init_connexion("/dev/ADAM", 0) == 0, "Failed test 1");
	return 0;
}

static char * test_connexion() {
	printf("Connect the ADAM 4561 module (Press any button to continue)");
	scanf("\n");
	
	printf("\nTesting incorrect values...\n");
	
	mu_assert(init_connexion("/dev/apmnz", B9600) == 0, "Failed test 2-1");
	mu_assert(init_connexion("/dev/ADAM", 750) == 0, "Failed test 2-2");
	mu_assert(init_connexion("/dev/apmnz", -12) == 0, "Failed test 2-3");
	
	printf("\nTesting correct values...\n");
	
	mu_assert(init_connexion("/dev/ADAM", B9600) == 1, "Failed test 2-4");
	mu_assert(close_connexion() != 0, "Failed test 2-5");
	
	printf("\nTesting incorrect closing...\n");
	
	mu_assert(close_connexion() == 0, "Failed test 2-6");
	
	return 0;
}


static char * test_send_command() {
	char * cmd = "$0BAUBEIZOA";
	char rcv[100] = "";

	printf("\nTesting incorrect call...\n");

	mu_assert(send_command(cmd, rcv) == 0, "Failed test 3-1");

	init_connexion("/dev/ADAM", B9600);
	
	mu_assert(send_command(cmd, rcv) == 1, "Failed test 3-2");
	printf("%s", rcv);
	mu_assert(strcmp(rcv, "?0B\r") == 0, "Failed test 3-3");

	cmd = "$0B2";
	printf("\nTesting correct call...\n");

	mu_assert(send_command(cmd, rcv) == 1, "Failed test 3-4");
	mu_assert(strcmp(rcv, "!0BFF0600\r") == 0, "Failed test 3-5");

	close_connexion();
	return 0;
}

/*

static char * test_add_address() {
	char *a = add_address_to_cmd(0xff);
	char *b = add_address_to_cmd(0x0e);
	mu_assert(strcmp(a, "$FF") == 0, "Failed test 4-1");
	mu_assert(strcmp(b, "$0E") == 0, "Failed test 4-2");
	free(a);
	free(b);
	return 0;
}


static char * test_remove_address() {
	char * a = remove_address_from_cmd("!ACFF0600");
	mu_assert(strcmp(a, "FF0600") == 0, "Failed test 5");
	free(a);
	return 0;
}
*/


static char * test_get_config() {
	configuration a;
	init_connexion("/dev/ADAM", 0);
	
	printf("\n Testing valid inputs...\n");
	mu_assert(0 == get_configuration(0x03, &a), "Failed test 6-1");

	printf("\n Testing valid inputs...\n");
	mu_assert(1 == get_configuration(0x0B, NULL), "Failed test 6-2");
	mu_assert(1 == get_configuration(0x0B, &a), "Failed test 6-3");
	mu_assert(a.baudrate.code == 0x06, "Failed test 6-4");
	mu_assert(a.format.code == 0, "Failed test 6-5");
	mu_assert(a.checksum.code == 0, "Failed test 6-6");
	mu_assert(a.integration.code == 0, "Failed test 6-7");
	mu_assert(a.input.code == 0xFF, "Failed test 6-8");
	mu_assert(a.module_address.code == 0x0b, "Failed test 6-9");

	printf("%s\n", a.module_name);

	mu_assert(strcmp(a.module_name, "4018P\r") == 0, "Failed test 6-10");
	close_connexion();

	return 0;
}

static char * test_set_config() {
	configuration a, b, c;
	init_connexion("/dev/ADAM", 0);
	get_configuration(0x0B, &a);
	get_configuration(0x0B, &c);

	//Change baudrate :
	a.baudrate.code = 0x09;
	mu_assert(0 == set_configuration_status(0x0B, NULL), "Failed test 8-1");
	//Change checksum :
	a.checksum.code = 1;	
	//Change integration :
	a.integration.code = 1;	
	a.module_address.code = 0x02;
	a.format.code = 2;
	strcpy(a.module_address.name, "02");;	
	mu_assert(1 == set_configuration_status(0x0B, &a), "Failed test 8-2");

	get_configuration(0x02, &b);
	mu_assert(b.baudrate.code == 0x06, "Failed test 8-4");
	mu_assert(b.format.code == a.format.code, "Failed test 8-5");
	mu_assert(b.checksum.code != a.checksum.code, "Failed test 8-6");
	mu_assert(b.integration.code == a.integration.code, "Failed test 8-7");
	mu_assert(b.input.code == 0xFF, "Failed test 8-8");
	mu_assert(a.module_address.code == 0x02, "Failed test 8-9");
	mu_assert(1 == set_configuration_status(0x02, &c), "Failed test 8-3");
	close_connexion();
	return 0;
}


static char * test_scan_channels() {
	init_connexion("/dev/ADAM", 0);
	tuple array[8];	
	mu_assert(get_all_channels_type(0x0B, array) == 1, "Failed test 7-1");
	mu_assert(strcmp(array[0].name, "TC_K") == 0, "Failed test 7-2");
	mu_assert(strcmp(array[1].name, "TC_T") == 0, "Failed test 7-3");
	mu_assert(array[0].code == 0x0F,  "Failed test 7-4");
	mu_assert(array[1].code == 0x10,  "Failed test 7-5");
	close_connexion();
	return 0;
}

static char * test_set_type() {
	init_connexion("/dev/ADAM", 0);
	tuple array[8];	
	tuple a = {
		.code = 0x11,
		.name = "TC_E" };
	tuple b = {
		.code = 0x10,
		.name = "TC_T" };
	
	mu_assert(set_channel_type(0x0B, 2, &a) == 1, "Failed test 9-2");

	mu_assert(get_all_channels_type(0x0B, array) == 1, "Failed test 9-1");
	mu_assert(strcmp(array[2].name, "TC_E") == 0, "Failed test 9-3");
	mu_assert(array[2].code == 0x11,  "Failed test 9-5");

	mu_assert(set_channel_type(0x0B, 2, &b) == 1, "Failed test 9-2");
	close_connexion();
	return 0;
}

static char * test_set_channels_status() {
	init_connexion("/dev/ADAM", 0);
	configuration a;
	uint8_t array[8] = {1,1,0,0,1,1,0,1};
	mu_assert(set_channels_status(0x0B, array) == 1, "Failed test 10-1"); 
	get_configuration(0x0B, &a);
	print_configuration(&a);
	array[2] = 72;
	mu_assert(set_channels_status(0x0B, array) == 1, "Failed test 10-1"); 
	get_configuration(0x0B, &a);
	print_configuration(&a);
	memset(array, 0, 8);
	array[0] = 1;
	mu_assert(set_channels_status(0x0B, array) == 1, "Failed test 10-1"); 
	get_configuration(0x0B, &a);
	print_configuration(&a);
	close_connexion();
	return 0;
}

static char * test_get_data() {
	uint8_t i;
	init_connexion("/dev/ADAM", 0);
	float data[8] = {0};
	get_all_data(0x0B, data);
	for(i = 0; i< 8; i++)
		printf("Channel %i : %f\n", i, data[i]);
	printf("Single measure\n");
	float single_data;
	get_channel_data(0x0B, 7, &single_data);
	printf("%f\n", single_data);
	close_connexion();
	return 0;
}

static char * test_calibrate() {
	init_connexion("/dev/ADAM", 0);
	calibrate_CJC_offset(0x0B, -375.0);
	close_connexion();
	return 0;
}

static char * all_tests() {
	//mu_run_test(test_unconnected_connexion);
	mu_run_test(test_connexion);
	mu_run_test(test_send_command);
	mu_run_test(test_scan_channels);
	mu_run_test(test_set_config);
	mu_run_test(test_set_type);
	mu_run_test(test_get_config);
	mu_run_test(test_set_channels_status);
	mu_run_test(test_get_data);
	//mu_run_test(test_calibrate);

	return 0;
}

RUN_TESTS(all_tests);

