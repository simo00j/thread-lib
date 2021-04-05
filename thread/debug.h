#ifndef PROJET_D_OS_S8_DEBUG_H
#define PROJET_D_OS_S8_DEBUG_H

#ifdef USE_DEBUG

	#include <stdio.h>
	#include <stdarg.h>

	#define internal_log(file, level, message, ...) fprintf(file, "[" level "] " message "\n\tin %s:%d, %s\n", __VA_ARGS__, __FILE__, __LINE__, __func__);

#else // USE_DEBUG

	#define internal_log(...) ;

#endif // USE_DEBUG

#define debug(message, ...) internal_log(stdout, "DEBUG", message, __VA_ARGS__)
#define info(message, ...) internal_log(stdout, "INFO", message, __VA_ARGS__)
#define warn(message, ...) internal_log(stderr, "WARN", message, __VA_ARGS__)
#define error(message, ...) internal_log(stderr, "ERROR", message, __VA_ARGS__)

#endif //PROJET_D_OS_S8_DEBUG_H
