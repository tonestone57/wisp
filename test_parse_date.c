#include <curl/curl.h>
#include <stdio.h>
int main() {
    time_t t = curl_getdate("Sun, 06 Nov 1994 08:49:37 GMT", NULL);
    printf("%ld\n", (long)t);
    return 0;
}
