#include <wchar.h>

typedef struct {

} RopeNode;

typedef struct {
    RopeNode *root;
} Rope;

wchar_t rp_get_char(Rope *rp, int row, int col);

void rp_insert(Rope *rp, int row, int col, wchar_t *wc);

void rp_search(Rope *rp, char *needle);

void rp_free(Rope *rp);
