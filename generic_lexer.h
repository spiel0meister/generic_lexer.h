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

    TOKEN_ASSIGN,
    TOKEN_EQ,

    TOKEN_SEMI,

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

Token lexer_parse_identifier(Lexer* lexer);

Tokens lexer_lex(Lexer* lexer);

#endif // GENERIC_LEXER_H

#ifdef GENERIC_LEXER_IMPLEMENTATION

StringView sb_export(StringBuilder sb) {
    StringView view = {0};
    view.items = (char*)malloc(sizeof(char) * (sb.size + 1));
    memcpy(view.items, sb.items, sb.size);
    view.items[sb.size] = 0;
    view.size = sb.size;
    return view;
}

void check_keywords(char* buf, Token* t) {
    if (strcmp(buf, "if") == 0) {
        t->type = TOKEN_IF;
    } else if (strcmp(buf, "else") == 0) {
        t->type = TOKEN_ELSE;
    } else if (strcmp(buf, "return") == 0) {
        t->type = TOKEN_RETURN;
    } else {
        t->type = TOKEN_IDENT;
    }
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

#define token_literal(lexer, tokens, c, clit, ttype) \
    if (c == clit) { \
        Token t; \
        t.type = ttype; \
        StringBuilder buf = {0}; \
        sb_char_append(&buf, c); \
        t.value = sb_export(buf); \
        t.loc.file = (lexer)->file; \
        t.loc.line = (lexer)->line; \
        t.loc.column = (lexer)->column; \
        da_append(tokens, t); \
        free(buf.items); \
        lexer_consume(lexer); \
        continue; \
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

        if (c == '=') {
            Token t; 

            if (lexer_peek(lexer, 1) == '=') {
                t.type = TOKEN_EQ;

                StringBuilder buf = {0}; 
                sb_cstr_append(&buf, "=="); 
                t.value = sb_export(buf); 
                free(buf.items); 

                lexer_consume(lexer);
            } else {
                t.type = TOKEN_ASSIGN; 

                StringBuilder buf = {0}; 
                sb_char_append(&buf, c); 
                t.value = sb_export(buf); 
                free(buf.items); 
            }

            t.loc.file = (lexer)->file; 
            t.loc.line = (lexer)->line; 
            t.loc.column = (lexer)->column; 
            da_append(&tokens, t); 
            lexer_consume(lexer); 
            continue; 
        }

        token_literal(lexer, &tokens, c, ';', TOKEN_SEMI);
        token_literal(lexer, &tokens, c, '{', TOKEN_LEFTBRACE);
        token_literal(lexer, &tokens, c, '}', TOKEN_RIGHTBRACE);
        token_literal(lexer, &tokens, c, '(', TOKEN_LEFTPAREN);
        token_literal(lexer, &tokens, c, ')', TOKEN_RIGHTPAREN);
        token_literal(lexer, &tokens, c, '[', TOKEN_LEFTBRACKET);
        token_literal(lexer, &tokens, c, ']', TOKEN_RIGHTBRACKET);

        fprintf(stderr, "ERROR: %s:%ld:%ld: Unexpected character: %c\n", lexer->file, lexer->line, lexer->column, c);
        exit(EXIT_FAILURE);
    }

    return tokens;
}

#endif // GENERIC_LEXER_IMPLEMENTATION
