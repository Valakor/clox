//
//  scanner.h
//  clox
//
//  Created by Matthew Pohlmann on 5/18/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once



typedef enum eTokenType
{
	// Single-character tokens
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

	// One or two character tokens
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,

	// Literals
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

	// Keywords
	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

	TOKEN_ERROR,
	TOKEN_EOF
} TokenType;



typedef struct sToken
{
	TokenType type;
	const char * start;
	int length;
	int line;
} Token;

typedef struct sScanner
{
	const char * start;
	const char * current;
	int line;
} Scanner;



void initScanner(Scanner * scanner, const char* source);
Token scanToken(Scanner * scanner);
