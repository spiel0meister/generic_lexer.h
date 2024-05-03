#ifndef GENERIC_LEXER_H
#define GENERIC_LEXER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#define da_append(da, item) \
    if ((da)->size == (da)->capacity) { \
        (da)->capacity = (da)->capacity == 0 ? 16 : (da)->capacity * 2; \
        (da)->items = realloc((da)->items, sizeof(*(da)->items) * (da)->capacity); \
    } \
    (da)->items[(da)->size++] = (item)
#define da_append_many(da, type, ...) do { \
        type items[] = { __VA_ARGS__ }; \
        size_t items_count = sizeof(items) / sizeof(type); \
        for (size_t i = 0; i < items_count; i++) { \
            da_append(da, items[i]); \
        } \
    } while (0)

typedef struct {
    char* items;
    size_t size;
    size_t capacity;
}StringBuilder;

typedef struct {
    char* items;
    size_t size;
}StringView;

#define sb_char_append da_append

#define sb_cstr_append(sb, cstr) do { \
        size_t cstr_len = strlen(cstr); \
        if ((sb)->size + cstr_len > (sb)->capacity) { \
            (sb)->capacity = (sb)->capacity == 0 ? cstr_len : (sb)->capacity * 2; \
            (sb)->items = (char*)realloc((sb)->items, (sb)->capacity); \
        } \
        memcpy((sb)->items + (sb)->size, cstr, cstr_len); \
        (sb)->size += cstr_len; \
    } while (0);
#define sb_cstr_append_many(sb, ...) do { \
        char* cstrs[] = { __VA_ARGS__, NULL }; \
        for (char* cstr = *cstrs; cstr != NULL; cstr++) { \
            size_t cstr_len = strlen(cstr); \
            if ((sb)->size + cstr_len > (sb)->capacity) { \
                (sb)->capacity = (sb)->capacity == 0 ? cstr_len : (sb)->capacity * 2; \
                (sb)->items = (char*)realloc((sb)->items, (sb)->capacity); \
            } \
            memcpy((sb)->items + (sb)->size, cstr, cstr_len); \
            (sb)->size += cstr_len; \
        } \
    } while (0);

typedef enum {
    TOKEN_EOF = 0,

    TOKEN_IDENT,
    TOKEN_NUMBER,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_DO,

    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GTE,
    TOKEN_LTE,

    TOKEN_SEMI,
    TOKEN_PERIOD,
    TOKEN_COMMA,

    TOKEN_LEFTBRACE,
    TOKEN_RIGHTBRACE,
    TOKEN_LEFTPAREN,
    TOKEN_RIGHTPAREN,
    TOKEN_LEFTBRACKET,
    TOKEN_RIGHTBRACKET,
}TokenType;

typedef struct {
    char* file;
    size_t line;
    size_t column;
}Loc;

typedef struct {
    TokenType type;
    StringView value;
    Loc loc;
}Token;

typedef struct {
    Token* items;
    size_t size;
    size_t capacity;
}Tokens;

typedef struct {
    char* file;
    char* content;
    size_t content_len;

    size_t cursor;
    size_t line;
    size_t column;
}Lexer;

StringView sb_export(StringBuilder sb);

void check_keywords(char* buf, Token* t);

Lexer lexer_init(char* file, char* content);
char lexer_peek(Lexer* lexer, size_t offset);
char lexer_consume(Lexer* lexer);

bool lexer_try_parse_literal(Lexer* lexer, char* lit, TokenType type, Tokens* tokens);
Token lexer_parse_identifier(Lexer* lexer);

Tokens lexer_lex(Lexer* lexer);

#endif // GENERIC_LEXER_H

#if 1 //def GENERIC_LEXER_IMPLEMENTATION

StringView sb_export(StringBuilder sb) {
    StringView view = {0};
    view.items = (char*)malloc(sizeof(char) * (sb.size + 1));
    memcpy(view.items, sb.items, sb.size);
    view.items[sb.size] = 0;
    view.size = sb.size;
    return view;
}

#define map_keyword(t, buf, keyword, ttype) \
    if (strcmp(buf, keyword) == 0) { \
        (t)->type = ttype; \
        return; \
    } \

void check_keywords(char* buf, Token* t) {
    map_keyword(t, buf, "if", TOKEN_IF);
    map_keyword(t, buf, "else", TOKEN_ELSE);
    map_keyword(t, buf, "return", TOKEN_RETURN);
    map_keyword(t, buf, "for", TOKEN_FOR);
    map_keyword(t, buf, "while", TOKEN_WHILE);
    map_keyword(t, buf, "do", TOKEN_WHILE);

    // if buf matched nothing, it is not a keyword, therefore it is an identifier
    t->type = TOKEN_IDENT;
}

Lexer lexer_init(char* file, char* content) {
    Lexer lexer = {0};

    lexer.file = file;
    lexer.content = content;
    lexer.content_len = strlen(content);

    lexer.cursor = 0;
    lexer.line = 1;
    lexer.column = 1;

    return lexer;
}

char lexer_peek(Lexer* lexer, size_t offset) {
    if (lexer->cursor + offset >= lexer->content_len) {
        return 0;
    }
    return lexer->content[lexer->cursor + offset];
}

char lexer_consume(Lexer* lexer) {
    if (lexer->cursor >= lexer->content_len) {
        fprintf(stderr, "ERROR: %s:%ld:%ld: Unexpected EOF\n", lexer->file, lexer->line, lexer->column);
        exit(EXIT_FAILURE);
    }
    lexer->column++;
    return lexer->content[lexer->cursor++];
}

bool lexer_try_parse_literal(Lexer* lexer, char* lit, TokenType type, Tokens* tokens) {
    if (lit == NULL || *lit == 0) return false;
    size_t lit_len = strlen(lit);

    StringBuilder buf = {0};
    while (*lit != 0) {
        if (lexer_peek(lexer, 0) != *lit++) {
            return false;
        }
    }

    for (; lit_len > 0; lit_len--) { 
        sb_char_append(&buf, lexer_consume(lexer));
    }

    Token t = {0};
    t.type = type;

    t.loc.file = lexer->file;
    t.loc.line = lexer->line;
    t.loc.column = lexer->column;

    t.value = sb_export(buf);
    da_append(tokens, t);

    free(buf.items);
    return true;
}


Token lexer_parse_number(Lexer* lexer) {
    StringBuilder buf = {0};
    sb_char_append(&buf, lexer_consume(lexer));
    bool period = false;

    while (lexer_peek(lexer, 0) != 0 && (isdigit(lexer_peek(lexer, 0)) || lexer_peek(lexer, 0) == '.' || lexer_peek(lexer, 0) == '_')) {
        if (lexer_peek(lexer, 0) == '_') {
            lexer_consume(lexer);
            continue;
        }

        if (lexer_peek(lexer, 0) == '.') {
            if (period) {
                fprintf(stderr, "ERROR: %s:%ld:%ld: Unexpected character: %c\n", lexer->file, lexer->line, lexer->column, lexer_peek(lexer, 0));
                exit(EXIT_FAILURE);
            }
            period = true;
        }

        sb_char_append(&buf, lexer_consume(lexer));
    }

    Token t;
    memset(&t, 0, sizeof(t));

    t.value = sb_export(buf);
    t.type = TOKEN_NUMBER;

    t.loc.file = lexer->file; 
    t.loc.line = lexer->line; 
    t.loc.column = lexer->column; 

    free(buf.items);
    return t;
}

Token lexer_parse_identifier(Lexer* lexer) {
    StringBuilder buf = {0};
    sb_char_append(&buf, lexer_consume(lexer));

    while (lexer_peek(lexer, 0) != 0 && (isalpha(lexer_peek(lexer, 0)) || isdigit(lexer_peek(lexer, 0)) || lexer_peek(lexer, 0) == '_')) {
        sb_char_append(&buf, lexer_consume(lexer));
    }

    Token t;
    memset(&t, 0, sizeof(t));

    t.value = sb_export(buf);
    // passing StringView.items instead of StringBuilder.items, as they are null terminated
    check_keywords(t.value.items, &t);

    t.loc.file = lexer->file; 
    t.loc.line = lexer->line; 
    t.loc.column = lexer->column; 

    free(buf.items);
    return t;
}

Tokens lexer_lex(Lexer* lexer) {
    Tokens tokens = {0};

    while (lexer_peek(lexer, 0) != 0) {
        char c = lexer_peek(lexer, 0);

        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
            lexer_consume(lexer);
            continue;
        }

        if (isspace(c)) {
            lexer_consume(lexer);
            continue;
        }

        if (isalpha(c) || c == '_') {
            Token t = lexer_parse_identifier(lexer);
            da_append(&tokens, t);
            continue;
        } 

        if (isdigit(c)) {
            Token t = lexer_parse_number(lexer);
            da_append(&tokens, t);
            continue;
        }

        // two character tokens
        if (lexer_try_parse_literal(lexer, "==", TOKEN_EQ, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "!=", TOKEN_NEQ, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, ">=", TOKEN_GTE, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "<=", TOKEN_LTE, &tokens)) continue;

        // single character tokens
        if (lexer_try_parse_literal(lexer, "<", TOKEN_LT, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, ">", TOKEN_GT, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "=", TOKEN_ASSIGN, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, ";", TOKEN_SEMI, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, ".", TOKEN_PERIOD, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, ",", TOKEN_COMMA, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "{", TOKEN_LEFTBRACE, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "}", TOKEN_RIGHTBRACE, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "(", TOKEN_LEFTPAREN, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, ")", TOKEN_RIGHTPAREN, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "[", TOKEN_LEFTBRACKET, &tokens)) continue;
        if (lexer_try_parse_literal(lexer, "]", TOKEN_RIGHTBRACKET, &tokens)) continue;

        fprintf(stderr, "ERROR: %s:%ld:%ld: Unexpected character: %c\n", lexer->file, lexer->line, lexer->column, c);
        exit(EXIT_FAILURE);
    }

    Token t;
    t.type = TOKEN_EOF;
    
    t.loc.file = lexer->file;
    t.loc.line = lexer->line;
    t.loc.column = lexer->column;

    da_append(&tokens, t);

    return tokens;
}

#endif // GENERIC_LEXER_IMPLEMENTATION
