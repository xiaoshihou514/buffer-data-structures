#include <wchar.h>

typedef struct {

} PieceTree;

wchar_t pt_get_char(PieceTree *pt, int row, int col);

void pt_insert(PieceTree *pt, int row, int col, wchar_t *wc);

void pt_search(PieceTree *pt, char *needle);

void pt_free(PieceTree *pt);
