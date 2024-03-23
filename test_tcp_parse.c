/*****************
**  Vit Pavlik  **
**   xpavli0a   **
**    251301    **
**              **
**   Created:   **
**  2024-03-23  **
**              **
**    Edited:   **
**  2024-03-23  **
*****************/

/**
 * test IPK24CHAT TCP strings for the parse result (enum parse_result)
 * in tcp_parse.c
*/

#include <assert.h>
#include "tcp_parse.h"

#define concatconcat(x, y) x ## y
#define concat(x, y) concatconcat(x, y)
#define unique_name concat(test_string_, __LINE__)

#define test(str, expected) \
char unique_name [200] = str; \
res = tcp_parse_any(unique_name); \
assert((str, res == expected))


int main() {
    enum parse_result res;
    test("mSg frOm server iS hello, this is a message\r\n",  PR_MSG);
    test("eRr froM server iS hello, this is an error\r\n",   PR_ERR);
    test("rePly nOk iS yo this is a unsuccessful reply\r\n", PR_REPLY_NOK);
    test("rEply oK iS yo this is a successful reply\r\n",    PR_REPLY_OK);
    test("bYe\r\n",                                          PR_BYE);
}
