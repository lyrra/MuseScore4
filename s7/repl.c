#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "s7/s7.h"
#include "utils.h"

int run_scheme_eval_buffer (s7_scheme *s7, char *buffer)
{
    if (s7 && buffer)
        s7_eval_c_string(s7, buffer); /* evaluate input and write the result */
    return 0;
}

int main (int argc, char **argv) {
    s7_scheme *s7 = s7_init();
    for (int i = 1; i < argc; i++) {
        if (!s7_load(s7, argv[1]))
            fprintf(stderr, "can't load %s\n", argv[1]);
    }
    run_scheme_eval_buffer(s7, slurp_stream(stdin, 1));
    free(s7);
    return 0;
}
