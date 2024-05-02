/// USED OF TESING
/// NOT PART OF LIBRARY

#define GENERIC_LEXER_IMPLEMENTATION
#include "generic_lexer.h"

int main(void) {
    Lexer lexer = lexer_init("main.c", "int main(void) { return 0; }");
    Tokens tokens = lexer_lex(&lexer);

    for (size_t i = 0; i < tokens.size; i++) {
        printf("%s\n", tokens.items[i].value.items);
    }

    return 0;
}
