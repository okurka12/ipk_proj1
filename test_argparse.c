/**
 *
 * this file utilizes the argparse module and returns 0 or 1 depending on
 * whether the arguments are correct - this code itself does not test argparse
 *
*/

#include "argparse.h"

int main(int argc, char *argv[]) {
    conf_t conf;
    return args_ok(argc, argv, &conf) ? 0 : 1;
}