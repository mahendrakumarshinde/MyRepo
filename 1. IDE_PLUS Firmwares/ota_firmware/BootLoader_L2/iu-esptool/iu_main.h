#include "iu-esptool.h"
//#include "argparse.h"
#include "espcomm.h"
//#include "elf/esptool_elf.h"
//#include "elf/esptool_elf_object.h"
//#include "binimage/esptool_binimage.h"

// Invocation is ./esptool -vv -cd ck -cb 115200 -cp $IDE_PATH -ca 0x00000 -cf wifistation_ESP8285.bin

static char infolevel = 0;
void infohelper_set_infolevel(char lvl)
{
	//infolevel = lvl;
}

void infohelper_output(int loglevel, const char* format, ...)
{
//	if (infolevel < loglevel)
//		return;
//
//	const char* log_level_names[] = {"error: ", "warning: ", "", "\t", "\t\t"};
//	const int log_level_names_count = sizeof(log_level_names) / sizeof(const char*);
//	if (loglevel >= log_level_names_count)
//		loglevel = log_level_names_count - 1;
//    
//	va_list v;
//
//	printf("%s", log_level_names[loglevel]);
//	va_start(v, format);
//	vprintf(format, v);
//	va_end(v);
//	printf("\n");
}

void infohelper_output_plain(int loglevel, const char* format, ...)
{
//	if (infolevel < loglevel)
//		return;
//
//	va_list v;
//
//	va_start(v, format);
//	vprintf(format, v);
//	va_end(v);
}

int iu_main(void)
{
//	int num_args;
//	int num_args_parsed;
//	char **arg_ptr;
//
//	num_args = argc-1;
//	arg_ptr = argv;
//	arg_ptr++;
//
//	if (argc < 2) {
//		LOGERR("No arguments given. Use -h for help.");
//		return 0;
//	}
//	infohelper_set_infolevel(1);
//	infohelper_set_argverbosity(num_args, arg_ptr);
	infohelper_set_infolevel(3);

//	LOGINFO("esptool v" VERSION " - (c) 2014 Ch. Klippel <ck@atelier-klippel.de>");

//	For -cd ck
	espcomm_set_board("ck");
//	For -cb 115200
//	espcomm_set_baudrate("115200");
	//espcomm_set_baudrate(115200);
//	For -cp $IDE_PATH
//	espcomm_set_port($IDE_PATH);
	//espcomm_set_port("/dev/ttyUSB0");
   
//	For -ca 0x00000
//	espcomm_set_address("0x00000");
	espcomm_set_address(0x00000);
//	For -cf wifistation_ESP8285.bin
	espcomm_upload_file("wifistation_ESP8285.bin");
/*
	while (num_args) {
		num_args_parsed = parse_arg(num_args, arg_ptr);
		if (num_args_parsed == 0) {
			LOGERR("Invalid argument or value after %s (argument #%d)", arg_ptr[0], arg_ptr - argv + 1);
			goto EXITERROR;
		} else if (num_args_parsed < 0) {
			// Command failed, error is printed by the command
			goto EXITERROR;
		}

		num_args -= num_args_parsed;
		arg_ptr += num_args_parsed;
    }
*/

	if (espcomm_file_uploaded()) {
		espcomm_start_app(0);
	}
/*
	close_elf_object();
	binimage_write_close(16);
*/
	return 0;

EXITERROR:
/*
	close_elf_object();
	binimage_write_close(16);
*/
	return 2;
}
