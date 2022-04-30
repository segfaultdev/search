#ifndef __SEARCH_H__
#define __SEARCH_H__

typedef struct entry_t entry_t;

struct entry_t {
  char title[256];
  char url[256]; // url[0] == '\0' -> end of array
  
  char data[1024];
  int is_pdf;
};

#endif
