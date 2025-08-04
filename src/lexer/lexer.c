#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

static void ReadChar(Lexer* l) {
  if ((size_t)l->read_position >= l->input_len) {
    l->ch = 0;
  } else {
    l->ch = l->input[l->read_position];
  }
  l->position = l->read_position;
  l->read_position += 1;
}

Lexer* NewLexer(const char* input) {
  Lexer* l = (Lexer*)malloc(sizeof(Lexer));
  l->input = input;
  l->input_len = strlen(input);
  l->position = 0;
  l->read_position = 0;
  l->ch = 0;
  ReadChar(l);
  return l;
}

static void SkipWhiteSpace(Lexer* l) {
  while (isspace((unsigned char)l->ch)) {
    ReadChar(l);
  }
}

static char* ReadIdentifier(Lexer* l) {
  int start = l->position;
  ReadChar(l);
  while (isalnum((unsigned char)l->ch) || l->ch == '_') {
    ReadChar(l);
  }
  int length = l->position - start;
  char* ident = (char*)malloc((size_t)length + 1);
  memcpy(ident, &l->input[start], (size_t)length);
  ident[length] = '\0';
  return ident;
}

static char* ReadNumber(Lexer* l) {
  int start = l->position;
  while (isdigit((unsigned char)l->ch)) {
    ReadChar(l);
  }
  int length = l->position - start;
  char* num = (char*)malloc((size_t)length + 1);
  memcpy(num, &l->input[start], (size_t)length);
  num[length] = '\0';
  return num;
}

static TokenType LookUpIdent(const char* ident) {
  if (strcmp(ident, "let") == 0) {
    return TOKEN_LET;
  }
  if (strcmp(ident, "if") == 0) {
    return TOKEN_IF;
  }
  if (strcmp(ident, "else") == 0) {
    return TOKEN_ELSE;
  }
  if (strcmp(ident, "while") == 0) {
    return TOKEN_WHILE;
  }
  if (strcmp(ident, "out") == 0) {
    return TOKEN_OUT;
  }
  if (strcmp(ident, "in") == 0) {
    return TOKEN_IN;
  }
  if (strcmp(ident, "ns") == 0) {
    return TOKEN_NS;
  }
  if (strcmp(ident, "fn") == 0) {
    return TOKEN_FN;
  }
  if (strcmp(ident, "return") == 0) {
    return TOKEN_RETURN;
  }
  return TOKEN_IDENT;
}

static char PeekChar(Lexer* l) {
  if ((size_t)l->read_position >= l->input_len) {
    return 0;
  } else {
    return l->input[l->read_position];
  }
}

static Token NewToken(TokenType type, char* literal) {
  Token tok;
  tok.type = type;
  tok.literal = literal;
  return tok;
}

Token NextToken(Lexer* l) {
  Token tok;

  SkipWhiteSpace(l);

  switch (l->ch) {
    case '=':
      if (PeekChar(l) == '=') {
        ReadChar(l);
        tok = NewToken(TOKEN_EQUAL, "==");
      } else {
        tok = NewToken(TOKEN_ASSIGN, "=");
      }
      break;

    case ';':
      tok = NewToken(TOKEN_SEMICOLON, ";");
      break;
    case ',':
      tok = NewToken(TOKEN_COMMA, ",");
      break;
    case '.':
      tok = NewToken(TOKEN_DOT, ".");
      break;

    case '(' :
      tok = NewToken(TOKEN_LPAREN, "(");
      break;
    case ')' :
      tok = NewToken(TOKEN_RPAREN, ")");
      break;
    case '{' :
      tok = NewToken(TOKEN_LBRACE, "{");
      break;
    case '}' :
      tok = NewToken(TOKEN_RBRACE, "}");
      break;

    case '+':
      tok = NewToken(TOKEN_PLUS, "+");
      break;
    case '*':
      tok = NewToken(TOKEN_ASTERISK, "*");
      break;

    case '-':
      if (PeekChar(l) == '>') {
        ReadChar(l);
        tok = NewToken(TOKEN_ARROW, "->");
      } else {
        tok = NewToken(TOKEN_MINUS, "-");
      }
      break;

    case '/':
      if (PeekChar(l) == '/') {
        while (l->ch != '\n' && l->ch != 0) {
          ReadChar(l);
        }
        return NextToken(l);
      } else {
        tok = NewToken(TOKEN_SLASH, "/");
      }
      break;

    case '<':
      if (PeekChar(l) == '=') {
        ReadChar(l);
        tok = NewToken(TOKEN_LESS_EQUAL, "<=");
      } else {
        tok = NewToken(TOKEN_LESS, "<");
      }
      break;

    case '>':
      if (PeekChar(l) == '=') {
        ReadChar(l);
        tok = NewToken(TOKEN_GREATER_EQUAL, ">=");
      } else {
        tok = NewToken(TOKEN_GREATER, ">");
      }
      break;

    case '!':
      if (PeekChar(l) == '=') {
        ReadChar(l);
        tok = NewToken(TOKEN_NOT_EQUAL, "!=");
      } else {
        char* illegal = (char*)malloc(2);
        illegal[0] = '!';
        illegal[1] = '\0';
        tok = NewToken(TOKEN_ILLEGAL, illegal);
      }
      break;

    case 0:
      tok = NewToken(TOKEN_EOF, "");
      break;

    default:
      if (isalpha((unsigned char)l->ch) || l->ch == '_') {
        char* literal = ReadIdentifier(l);
        TokenType type = LookUpIdent(literal);
        tok.type = type;
        tok.literal = literal;
        return tok;
      } else if (isdigit((unsigned char)l->ch)) {
        char* literal = ReadNumber(l);
        tok.type = TOKEN_INT;
        tok.literal = literal;
        return tok;
      } else {
        char* illegal_char_str = (char*)malloc(2);
        illegal_char_str[0] = l->ch;
        illegal_char_str[1] = '\0';
        tok = NewToken(TOKEN_ILLEGAL, illegal_char_str);
      }
      break;
  }

  ReadChar(l);
  return tok;
}

