#define _GNU_SOURCE

#include <mongoose.h>
#include <search.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <duck.h>
#include <load.h>

const char *listen_url = "http://0.0.0.0:80";

const char *entry_format = \
  "<tr>" \
  "  <td class=\"row-url\"><a class=\"span-row-url\" href=\"%s\">%s</a></td>" \
  "</tr>" \
  "<tr>" \
  "  <td class=\"row-title\"><a class=\"span-row-title\" href=\"%s\">%s</a></td>" \
  "</tr>" \
  "<tr>" \
  "  <td class=\"row-data\"><span class=\"span-row-data\">%s</span></td>" \
  "</tr>" \
;

const char *entry_format_pdf = \
  "<tr>" \
  "  <td class=\"row-url\"><a class=\"span-row-url\" href=\"%s\">%s</a></td>" \
  "</tr>" \
  "<tr>" \
  "  <td class=\"row-title\">[PDF] <a class=\"span-row-title\" href=\"%s\">%s</a></td>" \
  "</tr>" \
  "<tr>" \
  "  <td class=\"row-data\"><span class=\"span-row-data\">%s</span></td>" \
  "</tr>" \
;

char *template = NULL;
int do_exit = 0;

static void handle_exit(int signo) {
  do_exit = 1;
}

static void handle_event(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data;
    
    struct mg_http_serve_opts opts = (struct mg_http_serve_opts){
      .root_dir = "public",
      .ssi_pattern = "#.html"
    };
    
    printf("path: '%.*s'\n", hm->uri.len, hm->uri.ptr);
    
    const char *find = "query=";
    char *ptr = memmem(hm->query.ptr, hm->query.len, find, strlen(find));
    
    if (ptr) {
      int index = (ptr - hm->query.ptr) + strlen(find);
      
      char query[(hm->query.len - index) + 1];
      int length = 0;
      
      char raw_query[hm->query.len + 1];
      int raw_length = 0;
      
      while (hm->query.len - index) {
        if (hm->query.ptr[index] == '&') {
          break;
        }
        
        raw_query[raw_length++] = hm->query.ptr[index];
        
        if (hm->query.ptr[index] == '+') {
          query[length++] = ' ';
        } else if (strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~-._", hm->query.ptr[index])) {
          query[length++] = hm->query.ptr[index];
        } else if (hm->query.ptr[index] == '%') {
          const char *hex_str = "0123456789ABCDEF";
          int value = 0;
          
          index++;
          if (!(hm->query.len - index)) continue;
          
          char *hex_ptr = strchr(hex_str, toupper(hm->query.ptr[index]));
          
          if (!hex_ptr) continue;
          value += (hex_ptr - hex_str) * 16;
          
          index++;
          if (!(hm->query.len - index)) continue;
          
          hex_ptr = strchr(hex_str, toupper(hm->query.ptr[index]));
          
          if (!hex_ptr) continue;
          value += hex_ptr - hex_str;
          
          if (!value) continue;
          query[length++] = value;
        }
        
        index++;
      }
      
      query[length] = '\0';
      raw_query[raw_length] = '\0';
      
      if (length) {
        printf("- query: '%s'\n", query);
        
        entry_t *entries = duck_search(raw_query);
        
        char *buffer = calloc(1, 1);
        int length = 0;
        
        for (int i = 0; entries[i].url[0]; i++) {
          // printf("-> '%s'\n", entries[i].title);
          
          char sub_buffer[strlen(entry_format) + strlen(entries[i].title) + strlen(entries[i].url) * 3 + strlen(entries[i].data) + 1];
          sprintf(sub_buffer, entries[i].is_pdf ? entry_format_pdf : entry_format, entries[i].url, entries[i].url, entries[i].url, entries[i].title, entries[i].data);
          
          buffer = realloc(buffer, length + strlen(sub_buffer) + 1);
          strcat(buffer, sub_buffer);
          
          length += strlen(sub_buffer) + 1;
        }
        
        mg_http_reply(c, 200, "Content-Type: text/html\r\n", template, query, query, query, buffer);
        
        free(entries);
        return;
      }
    }
    
    mg_http_serve_dir(c, hm, &opts);
  }
}

int main(int argc, const char **argv) {
  FILE *file = fopen("search.html", "r");
  if (!file) return 1;
  
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  
  rewind(file);
  
  template = calloc(size + 1, 1);
  fread(template, 1, size, file);
  
  fclose(file);
  
  struct mg_mgr mgr;
  
  signal(SIGINT, handle_exit);
  signal(SIGTERM, handle_exit);
  
  mg_mgr_init(&mgr);
  
  if (!mg_http_listen(&mgr, listen_url, handle_event, &mgr)) {
    printf("error: cannot start server!\n");
    exit(1);
  }
  
  printf("server started at %s!\n", listen_url);
  
  while (!do_exit) {
    mg_mgr_poll(&mgr, 1000);
  }
  
  mg_mgr_free(&mgr);
  return 0;
}
