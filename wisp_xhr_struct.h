typedef struct WispXHR {
    JSContext *ctx;
    JSValue self;
    int readyState;
    int status;
    char *statusText;
    char *method;
    struct nsurl *url;
    bool async;
    struct fetch *fetch_handle;
    uint8_t *response_buf;
    size_t response_len;
    size_t response_alloc;
    char *response_headers;
    struct fetch_multipart_data *out_headers;
    struct dom_document *response_xml;
    struct WispXHR *next;
} WispXHR;
