#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#define FIFO_NAME "/tmp/call.fifo"

struct call_data {
    unsigned long tv_sec, tv_usec;
    int id;
};

int main () {

    int n = 0;
    FILE *fp = fopen (FIFO_NAME, "wb");
    if (NULL == fp) {
        fprintf (stderr, "Failed to open FIFO: %s\n", strerror (errno));
        return 1;
    }

    srand(time (NULL));

    while (n < 1000) {
        struct call_data cd;
        struct timeval tv, ts;
        ssize_t nw = 0;
        tv.tv_sec = 0;
        tv.tv_usec = rand () % 25000;
        select (0, NULL, NULL, NULL, &tv);
        gettimeofday (&ts, NULL);
        cd.id = ++n;
        cd.tv_sec = ts.tv_sec;
        cd.tv_usec = ts.tv_usec;
        nw = fwrite (&cd, sizeof (struct call_data), 1, fp);
        if (nw != 1) {
            fprintf (stderr, "Failed write (bail): %s\n", strerror (errno));
            fclose (fp);
            return 1;
        }
        fflush (fp);
        fprintf (stderr, "%d\n", n);
    }

    fclose (fp);
    return 0;
}

