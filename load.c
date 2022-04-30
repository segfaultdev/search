#include <mongoose.h>
#include <stdlib.h>
#include <stdio.h>
#include <load.h>

typedef struct state_t state_t;

struct state_t {
  const char *url;
  int done;
  
  char *data;
  int size;
};

static void handle_event(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  state_t *state = fn_data;
  
  if (ev == MG_EV_CONNECT) {
    struct mg_str host = mg_url_host(state->url);
    
    if (mg_url_is_ssl(state->url)) {
      struct mg_tls_opts opts = {.ca = "ca.pem", .srvname = host};
      mg_tls_init(c, &opts);
    }
    
    mg_printf(c, "GET %s HTTP/1.0\r\nHost: %.*s\r\n\r\n", mg_url_uri(state->url), (int)(host.len), host.ptr);
    printf("GET %s HTTP/1.0\r\nHost: %.*s\r\n\r\n", mg_url_uri(state->url), (int)(host.len), host.ptr);
    printf("waiting for response...\n");
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)(ev_data);
    
    size_t request_size = mg_http_get_request_len(hm->message.ptr, hm->message.len);
    
    char *data = hm->message.ptr + request_size;
    size_t size = hm->message.len - request_size;
    
    state->data = calloc(size + 1, 1);
    memcpy(state->data, data, size);
    
    state->size = size;
    
    c->is_closing = 1;
    state->done = 1;
  } else if (ev == MG_EV_ERROR) {
    state->done = 1;
  }
}

char *load_url(const char *url, size_t *size) {
  // TODO: fix good code and remove system()
  
  /*
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  
  state_t state = (state_t){url, 0, NULL, 0};
  
  printf("connecting to '%s'...\n", url);
  
  mg_http_connect(&mgr, url, handle_event, &state);
  
  printf("waiting for connection event...\n");
  
  while (!state.done) mg_mgr_poll(&mgr, 50);
  
  printf("closing connection...\n");
  
  mg_mgr_free(&mgr);
  
  if (size) *size = state.size;
  return state.data;
  */
  
  char buffer[320];
  sprintf(buffer, "wget -U \"Mozilla/5.0 (X11; Linux x86_64; rv:98.0) Gecko/20100101 Firefox/98.0\" %s -O temp.txt -4 --no-check-certificate 2> /dev/null", url);
  
  system(buffer);
  
  FILE *file = fopen("temp.txt", "rb");
  if (!file) return NULL;
  
  fseek(file, 0, SEEK_END);
  size_t new_size = ftell(file);
  
  rewind(file);
  
  char *data = calloc(new_size + 1, 1);
  fread(data, 1, new_size, file);
  
  fclose(file);
  remove("temp.txt");
  
  if (size) *size = new_size;
  return data;
}
