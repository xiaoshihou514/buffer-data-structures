#include <wchar.h>

typedef struct {

} PieceTree;

wchar_t pt_get_char(PieceTree *pt, size_t row, size_t col);

void pt_insert(PieceTree *pt, size_t row, size_t col, wchar_t *wc);

void pt_search(PieceTree *pt, char *needle);

void pt_free(PieceTree *pt);
