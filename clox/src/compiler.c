//
//  compiler.c
//  clox
//
//  Created by Matthew Pohlmann on 5/18/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "compiler.h"
#include "scanner.h"
#include "debug.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>


typedef struct
{
	Token current;
	Token previous;

	bool hadError;
	bool panicMode;
} Parser;

typedef enum
{
	PREC_NONE,
	PREC_ASSIGNMENT,	// =
	PREC_OR,			// or
	PREC_AND,			// and
	PREC_EQUALITY,		// == !=
	PREC_COMPARISON,	// < > <= >=
	PREC_TERM,			// + -
	PREC_FACTOR,		// * /
	PREC_UNARY,			// ! -
	PREC_CALL,			// . () []
	PREC_PRIMARY,
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

typedef struct
{
	Token name;
	int depth;
} Local;

typedef struct Compiler
{
	// TODO: Enhancement. Allow more local variables
	// TODO: Optimization. Make lookup faster (currently requires linear search through array)
	// TODO: Enhancement. Add concept of variables that don't allow re-assignment ('let'?)
	Local locals[UINT8_COUNT];
	int localCount;
	int scopeDepth;
} Compiler;

Parser parser;
Compiler * current = NULL;
Chunk * compilingChunk;


static void advance(void);
static bool check(TokenType type);
static bool match(TokenType type);
static void errorAtCurrent(const char * message);
static void error(const char * message);
static void errorAt(Token * token, const char * message);
static void consume(TokenType type, const char * message);
static void emitByte(uint8_t byte);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static Chunk * currentChunk(void);
static void initCompiler(Compiler * compiler);
static void endCompiler(void);
static void emitReturn(void);
static void expression(void);
static void statement(void);
static void declaration(void);
static void varDeclaration(void);
static void printStatement(void);
static void expressionStatement(void);
static void parsePrecedence(Precedence precendece);
static uint32_t identifierConstant(Token * name);
static bool resolveLocal(Compiler * compiler, Token * name, uint32_t * localIndex);
static void declareVariable(void);
static const ParseRule * getRule(TokenType type);

bool compile(const char * source, Chunk * chunk)
{
	initScanner(source);

	Compiler compiler;
	initCompiler(&compiler);

	compilingChunk = chunk;

	memset(&parser, 0, sizeof(parser));

	advance();

	while (!match(TOKEN_EOF))
	{
		declaration();
	}

	endCompiler();

	return !parser.hadError;
}

static void advance(void)
{
	parser.previous = parser.current;

	for (;;)
	{
		parser.current = scanToken();

		if (parser.current.type != TOKEN_ERROR)
			break;

		errorAtCurrent(parser.current.start);
	}
}

static void errorAtCurrent(const char * message)
{
	errorAt(&parser.current, message);
}

static void error(const char * message)
{
	errorAt(&parser.previous, message);
}

static void errorAt(Token * token, const char * message)
{
	if (parser.panicMode)
		return;

	parser.panicMode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF)
	{
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR)
	{
		// Nothing.
	}
	else
	{
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);

	parser.hadError = true;
}

static void consume(TokenType type, const char * message)
{
	if (parser.current.type == type)
	{
		advance();
		return;
	}

	errorAtCurrent(message);
}

static bool check(TokenType type)
{
	return parser.current.type == type;
}

static bool match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
}

static Chunk * currentChunk(void)
{
	return compilingChunk;
}

static void synchronize(void)
{
	parser.panicMode = false;

	while (parser.current.type != TOKEN_EOF)
	{
		if (parser.previous.type == TOKEN_SEMICOLON)
			return;

		switch (parser.current.type)
		{
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;

		default:
			break;
		}

		advance();
	}
}

static void emitByte(uint8_t byte)
{
	writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void emitReturn(void)
{
	emitByte(OP_RETURN);
}

static uint32_t makeConstant(Value value)
{
	// TODO: De-duplicate equivalent constants added to the chunk

	uint32_t constant = addConstant(currentChunk(), value);

	if (constant > 16777215) // UINT24_MAX
	{
		error("Too many constants in one chunk.");
		return 0;
	}

	return constant;
}

static void emitConstantHelper(uint32_t constant, OpCode opShort, OpCode opLong)
{
	if (constant <= UINT8_MAX)
	{
		// Use more optimal 1-byte constant op

		emitBytes(opShort, (uint8_t)constant);
	}
	else if (constant <= UINT24_MAX)
	{
		// Use 3-byte constant op

		emitByte(opLong);

		for (int i = 0; i < 3; i++)
		{
			uint8_t b = (constant >> (8 * (3 - i - 1))) & UINT8_MAX;
			emitByte(b);
		}
	}
	else
	{
		ASSERT(false);
	}
}

static void emitConstant(Value value)
{
	uint32_t constant = makeConstant(value);
	emitConstantHelper(constant, OP_CONSTANT, OP_CONSTANT_LONG);
}

static void initCompiler(Compiler * compiler)
{
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	current = compiler;
}

static void endCompiler(void)
{
	emitReturn();

#if DEBUG_PRINT_CODE
	if (!parser.hadError)
	{
		disassembleChunk(currentChunk(), "code");
	}
#endif
}

static void beginScope(void)
{
	current->scopeDepth++;
}

static void endScope(void)
{
	current->scopeDepth--;

	unsigned numLocals = 0;

	while (current->localCount > 0 &&
		   current->locals[current->localCount - 1].depth > current->scopeDepth)
	{
		numLocals++;
		current->localCount--;
	}

	if (numLocals == 0)
		return;

	if (numLocals == 1)
	{
		emitByte(OP_POP);
	}
	else
	{
		// Assumption: Only 256 locals possible
		// NOTE: POPN stores N - 2

		ASSERT(numLocals <= UINT8_COUNT);

		uint8_t arg = (uint8_t)(numLocals - 2);

		emitBytes(OP_POPN, arg);
	}
}

static void binary(bool canAssign)
{
	UNUSED(canAssign);

	// Remember the operator

	TokenType operatorType = parser.previous.type;

	// Compile the right operand

	const ParseRule * rule = getRule(operatorType);
	parsePrecedence((Precedence)(rule->precedence + 1));

	// Emit the operator instruction

	switch (operatorType)
	{
		case TOKEN_BANG_EQUAL:		emitBytes(OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL:		emitByte(OP_EQUAL); break;
		case TOKEN_GREATER:			emitByte(OP_GREATER); break;
		case TOKEN_GREATER_EQUAL:	emitBytes(OP_LESS, OP_NOT); break;
		case TOKEN_LESS:			emitByte(OP_LESS); break;
		case TOKEN_LESS_EQUAL:		emitBytes(OP_GREATER, OP_NOT); break;
		case TOKEN_PLUS:			emitByte(OP_ADD); break;
		case TOKEN_MINUS:			emitByte(OP_SUBTRACT); break;
		case TOKEN_STAR:			emitByte(OP_MULTIPLY); break;
		case TOKEN_SLASH:			emitByte(OP_DIVIDE); break;
		default:
			return; // Unreachable
	}
}

static void literal(bool canAssign)
{
	UNUSED(canAssign);

	switch (parser.previous.type)
	{
		case TOKEN_FALSE: emitByte(OP_FALSE); break;
		case TOKEN_NIL: emitByte(OP_NIL); break;
		case TOKEN_TRUE: emitByte(OP_TRUE); break;
		default:
			return; // Unreachable
	}
}

static void grouping(bool canAssign)
{
	UNUSED(canAssign);

	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign)
{
	UNUSED(canAssign);

	double value = strtod(parser.previous.start, NULL);
	emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign)
{
	UNUSED(canAssign);

	emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign)
{
	uint8_t getOp, setOp;
	uint32_t arg;

	if (resolveLocal(current, &name, &arg))
	{
		getOp = OP_GET_LOCAL;
		setOp = OP_SET_LOCAL;

		ASSERT(arg < UINT8_COUNT);
	}
	else
	{
		arg = identifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		setOp = OP_SET_GLOBAL;

		// TODO: Support OP_DEFINE_GLOBAL_LONG opcode

		if (arg >= UINT8_COUNT)
		{
			error("Too many global variables defined.");
			return;
		}
	}

	if (canAssign && match(TOKEN_EQUAL))
	{
		expression();
		emitBytes(setOp, (uint8_t)arg);
	}
	else
	{
		emitBytes(getOp, (uint8_t)arg);
	}
}

static void variable(bool canAssign)
{
	namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign)
{
	UNUSED(canAssign);

	TokenType operatorType = parser.previous.type;

	// Compile the operand

	parsePrecedence(PREC_UNARY);

	// Emit the operator instruction

	switch (operatorType)
	{
		case TOKEN_BANG: emitByte(OP_NOT); break;
		case TOKEN_MINUS: emitByte(OP_NEGATE); break;
		default:
			return; // Unreachable
	}
}

static const ParseRule rules[] =
{
	{ grouping, NULL,    PREC_CALL },       // TOKEN_LEFT_PAREN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
	{ NULL,     NULL,    PREC_CALL },       // TOKEN_DOT
	{ unary,    binary,  PREC_TERM },       // TOKEN_MINUS
	{ NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
	{ NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
	{ NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
	{ unary,    NULL,    PREC_NONE },       // TOKEN_BANG
	{ NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
	{ NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
	{ NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
	{ variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
	{ string,   NULL,    PREC_NONE },       // TOKEN_STRING
	{ number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
	{ NULL,     NULL,    PREC_AND },        // TOKEN_AND
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
	{ literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_IF
	{ literal,  NULL,    PREC_NONE },       // TOKEN_NIL
	{ NULL,     NULL,    PREC_OR },         // TOKEN_OR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
	{ literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static void parsePrecedence(Precedence precedence)
{
	advance();

	ParseFn prefixFn = getRule(parser.previous.type)->prefix;

	if (prefixFn == NULL)
	{
		error("Expect expression.");
		return;
	}

	bool canAssign = (precedence <= PREC_ASSIGNMENT);
	prefixFn(canAssign);

	while (precedence <= getRule(parser.current.type)->precedence)
	{
		advance();
		ParseFn infixFn = getRule(parser.previous.type)->infix;
		infixFn(canAssign);
	}

	if (canAssign && match(TOKEN_EQUAL))
	{
		error("Invalid assignment target.");
		expression();
	}
}

static uint32_t identifierConstant(Token * name)
{
	return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token * a, Token * b)
{
	if (a->length != b->length)
		return false;

	return memcmp(a->start, b->start, a->length) == 0;
}

static bool resolveLocal(Compiler * compiler, Token * name, uint32_t * localIndex)
{
	for (int i = compiler->localCount - 1; i >= 0; i--)
	{
		Local * local = &compiler->locals[i];

		if (identifiersEqual(name, &local->name))
		{
			if (local->depth == -1)
			{
				error("Cannot read local variable in its own initializer.");
			}

			*localIndex = i;
			return true;
		}
	}

	return false;
}

static void addLocal(Token name)
{
	if (current->localCount == UINT8_COUNT)
	{
		error("Too many local variables in function.");
		return;
	}

	Local * local = &current->locals[current->localCount++];
	local->name = name;
	local->depth = -1;
}

static void declareVariable(void)
{
	// Global variables are implicitly declared

	if (current->scopeDepth == 0)
		return;

	Token * name = &parser.previous;

	for (int i = current->localCount - 1; i >= 0; i--)
	{
		Local * local = &current->locals[i];

		if (local->depth != -1 && local->depth < current->scopeDepth)
			break;

		if (identifiersEqual(name, &local->name))
		{
			error("Variable with this name already declared in this scope.");
		}
	}

	addLocal(*name);
}

static uint32_t parseVariable(const char * errorMessage)
{
	consume(TOKEN_IDENTIFIER, errorMessage);

	declareVariable();

	if (current->scopeDepth > 0)
		return 0;

	return identifierConstant(&parser.previous);
}

static void markInitialized(void)
{
	if (current->scopeDepth == 0)
		return;

	current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint32_t global)
{
	if (current->scopeDepth > 0)
	{
		markInitialized();
		return;
	}

	// TODO: Support OP_DEFINE_GLOBAL_LONG opcode

	if (global >= UINT8_COUNT)
	{
		error("Too many global variables defined.");
		return;
	}

	emitBytes(OP_DEFINE_GLOBAL, (uint8_t)global);
}

static const ParseRule * getRule(TokenType type)
{
	return &rules[type];
}

static void expression(void)
{
	parsePrecedence(PREC_ASSIGNMENT);
}

static void block(void)
{
	while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
	{
		declaration();
	}

	consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void declaration(void)
{
	if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}

	if (parser.panicMode)
	{
		synchronize();
	}
}

static void varDeclaration(void)
{
	uint32_t global = parseVariable("Expect variable name.");

	if (match(TOKEN_EQUAL))
	{
		expression();
	}
	else
	{
		emitByte(OP_NIL);
	}

	consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

	defineVariable(global);
}

static void statement(void)
{
	if (match(TOKEN_PRINT))
	{
		printStatement();
	}
	else if (match(TOKEN_LEFT_BRACE))
	{
		beginScope();
		block();
		endScope();
	}
	else
	{
		expressionStatement();
	}
}

static void printStatement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after value.");
	emitByte(OP_PRINT);
}

static void expressionStatement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}
