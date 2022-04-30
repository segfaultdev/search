#include <search.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <duck.h>
#include <html.h>
#include <load.h>

entry_t *duck_search(const char *raw_query) {
  char url[100 + strlen(raw_query)];
  sprintf(url, "https://lite.duckduckgo.com/lite?q=%s", raw_query);
  
  size_t size;
  char *buffer = load_url(url, &size);
  
  HtmlDocument *doc;
  HtmlParseState *state = html_parse_begin();
  
  html_parse_stream(state, buffer, buffer, size);
  doc = html_parse_end(state);
  
  // html_print_dom(doc);
  
  HtmlElement *element = doc->root_element->child->child->sibling->child->sibling->sibling->sibling->sibling->sibling->sibling->sibling->sibling->child;
  
  entry_t *entries = NULL;
  int entry_count = 0;
  
  while (element && element->sibling) {
    entry_t entry;
    entry.is_pdf = 0;
    
    if (!element->child->sibling->child->child) break;
    
    if (element->child->sibling->child->child->child) {
      strcpy(entry.title, element->child->sibling->child->child->sibling->text);
      entry.is_pdf = 1;
    } else {
      strcpy(entry.title, element->child->sibling->child->child->text);
    }
    
    HtmlAttrib *attrib = element->child->sibling->child->attrib;
    entry.url[0] = '\0';
    
    while (attrib) {
      if (attrib->key == HTML_ATTRIB_HREF) {
        strcpy(entry.url, attrib->value);
        break;
      }
      
      attrib = attrib->next;
    }
    
    HtmlElement *data_element = element->sibling->child->sibling->child;
    entry.data[0] = '\0';
    
    while (data_element) {
      if (data_element->child) {
        char *data_ptr = data_element->child->text;
        while (data_ptr[0] == '\n') data_ptr++;
        
        strcat(entry.data, "<b>");
        strcat(entry.data, data_ptr);
        
        int length = strlen(entry.data);
        
        if (entry.data[length - 1] == '\n') {
          entry.data[length - 1] = '\0';
        }
        
        strcat(entry.data, "</b>");
      } else {
        char *data_ptr = data_element->text;
        while (data_ptr[0] == '\n') data_ptr++;
        
        strcat(entry.data, data_ptr);
        
        int length = strlen(entry.data);
        
        if (entry.data[length - 1] == '\n') {
          entry.data[length - 1] = '\0';
        }
      }
      
      data_element = data_element->sibling;
    }
    
    entries = realloc(entries, (entry_count + 1) * sizeof(entry_t));
    entries[entry_count++] = entry;
    
    element = element->sibling->sibling->sibling->sibling;
  }
  
  entries = realloc(entries, (entry_count + 1) * sizeof(entry_t));
  entries[entry_count].url[0] = '\0';
  
  html_free_document(doc);
  free(buffer);
  
  return entries;
}
