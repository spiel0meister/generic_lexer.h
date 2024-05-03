/// USED OF TESING
/// NOT PART OF LIBRARY

#define GENERIC_LEXER_IMPLEMENTATION
#include "generic_lexer.h"

int main(void) {
    Lexer lexer = lexer_init("main.c", "int main(void) { bool success = true; if (success) return 0;\nelse return 1; }");
    Tokens tokens = lexer_lex(&lexer);

    for (size_t i = 0; i < tokens.size; i++) {
        printf("%s\n", tokens.items[i].value.items);
    }

    for (size_t i = 0; i < tokens.size; i++) {
        free(tokens.items[i].value.items);
    }
    free(tokens.items);
    return 0;
}
