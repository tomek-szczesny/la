#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "util.h"

float parse_float_arg(int argc, char *argv[], const char *flag, float default_val) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return atof(argv[i + 1]);
        }
    }
    return default_val;
}

int parse_int_arg(int argc, char *argv[], const char *flag, int default_val) {
    float result = parse_float_arg(argc, argv, flag, (float)default_val);
    return (int)roundf(result);
}

int has_flag(int argc, char *argv[], const char *flag) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return 1;
        }
    }
    return 0;
}

