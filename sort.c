#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{

    if (shm_open(1, 1, 3))
    {
        printf(1, "succseful\n");
    }

    if (!fork())
    {
        char *a = (char *)shm_attach(1);
        *a = 'r';
        a[1] = '\0';
        printf(1, "%s", a);
        shm_close(1);
    }

    else
        for (int i = 0; i < 10; i++)
        {
            wait();
        }

    exit();
}
