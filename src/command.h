#include "query_parser.h"
int load(const char *csv_path);
int query(ParsedQuery *pq);
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));