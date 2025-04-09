#include "debugger_startup.h"

#ifndef DEBUG

int main(int argc, char **argv)
{
    return debugger_main(argc, argv);
}

#endif