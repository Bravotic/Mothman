#include "mothman.h"
#include <curl/curl.h>

static size_t callback(void *data, size_t size, size_t nmemb, void *clientp) {
    parserState_t *ps;
    size_t total = size * nmemb;
    int i;
    ps = clientp;

    for(i = 0; i < total; i++) {
        parserHTMLAddChar(ps, ((char*)data)[i]);
    }

    return total;
}

void openURL(const char *url, parserState_t *ps) {
    if (strncmp(url, "file://", 7) == 0) {
        FILE *source;
        char *path;
        int c;

        printf("URL is %s\n", &url[7]);

        /* Strip the protocol from our URL and open it */
        path = &url[7];
        source = fopen(path, "r");

        /* Read and parse our HTML */
        while((c = fgetc(source)) != EOF) {
           parserHTMLAddChar(ps, c);
        }
        parserHTMLAddChar(ps, 0);
        fclose(source);
    }
    else if(strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        CURL *curl_handle;
        CURLcode res;

        curl_handle = curl_easy_init();
        if(curl_handle) {
            curl_easy_setopt(curl_handle, CURLOPT_URL, url);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, ps);
            res = curl_easy_perform(curl_handle);
            curl_easy_cleanup(curl_handle);
            parserHTMLAddChar(ps, 0);
        }
        else {
            printf("Error performing curl\n");
        }
    }
    else {
        printf("Unknown protocol\n");
    }
}
