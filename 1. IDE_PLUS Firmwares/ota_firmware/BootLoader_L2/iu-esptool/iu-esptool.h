#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
//#include <string.h>

#define LOGERR(...) infohelper_output(0, __VA_ARGS__)
#define LOGWARN(...) infohelper_output(1, __VA_ARGS__)
#define LOGINFO(...) infohelper_output(2, __VA_ARGS__)
#define LOGDEBUG(...) infohelper_output(3, __VA_ARGS__)
#define LOGVERBOSE(...) infohelper_output(4, __VA_ARGS__)

#define INFO(...) infohelper_output_plain(0, __VA_ARGS__)
