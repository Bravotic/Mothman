#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

typedef struct buffer_t {
    unsigned char *buf;
    size_t ptr;
    size_t size;
} buffer_t;

static buffer_t *createBuffer() {
    buffer_t *buf;
    buf = malloc(sizeof(buffer_t));

    buf->buf = malloc(64 * sizeof(unsigned char*));
    buf->ptr = 0;
    buf->size = 64;

    return buf;
}

static size_t callback(void *data, size_t size, size_t nmemb, void *clientp) {
    buffer_t *buffer;
    size_t total = size * nmemb;
    buffer = clientp;

    while(buffer->size <= buffer->ptr + total) {
        buffer->size += 64;
        buffer->buf = realloc(buffer->buf, buffer->size);
    }

    memcpy(&(buffer->buf[buffer->ptr]), data, total);
    buffer->ptr += total;
    buffer->buf[buffer->ptr] = 0;

    return total;
}

unsigned char *REQ_request(const char *url) {
    CURL *curl_handle;
    CURLcode res;
    buffer_t *buffer;

    curl_handle = curl_easy_init();
    if(curl_handle) {
        buffer = createBuffer();
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, buffer);
        res = curl_easy_perform(curl_handle);
        curl_easy_cleanup(curl_handle);
    }
}
