#include "stdio.h"
#include "syscall.h"
//#include "screen.h"

static char blank[] = {"                   "};
static char plane1[] = {"    ___         _  "};
static char plane2[] = {"| __\\_\\______/_| "};
static char plane3[] = {"<[___\\_\\_______| "};
static char plane4[] = {"|  o'o             "};

void __attribute__((section(".entry_function"))) _start(int location)
{
    int i = 22, j = location;

    while (1)
    {
        for (i = 60; i > 0; i--)
        {
            /* move */
            sys_move_cursor(i, j + 0);
            printf("%s", plane1);

            sys_move_cursor(i, j + 1);
            printf("%s", plane2);

            sys_move_cursor(i, j + 2);
            printf("%s", plane3);

            sys_move_cursor(i, j + 3);
            printf("%s", plane4);
        }

        sys_move_cursor(1, j + 0);
        printf("%s", blank);

        sys_move_cursor(1, j + 1);
        printf("%s", blank);

        sys_move_cursor(1, j + 2);
        printf("%s", blank);

        sys_move_cursor(1, j + 3);
        printf("%s", blank);
    }
}
