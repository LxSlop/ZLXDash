#include <stdio.h>
#include <string.h>
#include <Hw.h>
#include <elf/elf.h>

#include "dash.h"

using namespace ZLX;

int main(int argc, char **argv) {
    char resources[256];

    if (argc != 0 && argv[0])
        snprintf(resources, sizeof resources, "%s/ressources/", argv_GetFilepath(argv[0]));
    else
        strcpy(resources, "uda0:/ressources/");

    Hw::SystemInit(INIT_SOUND | INIT_VIDEO | INIT_USB | INIT_NETWORK | INIT_ATA | INIT_ATAPI | INIT_FILESYSTEM | INIT_SFCX);
    App.Run(resources);
}
