#include "gui/include/gui.h"

int main(void)
{
        struct sysm_app app;

        sysm_app_init(&app);
        sysm_app_run(&app);
        sysm_app_destroy(&app);

        return 0;
}