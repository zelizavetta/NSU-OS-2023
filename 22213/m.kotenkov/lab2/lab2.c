#include <stdio.h>
#include <stdlib.h>

#include <time.h>

int main() {
    time_t t;
    time(&t);

    putenv("TZ=UTC+7");
    struct tm *time = localtime(&t);
    printf("Time in California: %s", asctime(time));

    return 0;
}
