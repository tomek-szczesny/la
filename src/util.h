#ifndef UTIL_H
#define UTIL_H

float parse_float_arg(int argc, char *argv[], const char *flag, float default_val);
int parse_int_arg(int argc, char *argv[], const char *flag, int default_val);
int has_flag(int argc, char *argv[], const char *flag);

#endif

