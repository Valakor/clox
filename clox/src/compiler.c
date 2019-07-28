//
//  compiler.c
//  clox
//
//  Created by Matthew Pohlmann on 5/18/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "debug.h"
#include "object.h"
#include "array.h"

#include <stdio.h>
#include <stdlib.h>



typedef struct sParser
{
	Token current;
	Token previous;

	bool hadError;
	bool panicMode;
} Parser;

typedef enum ePrecedence
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

typedef struct sParseRule
{
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

typedef struct sLocal
{
	Token name;
	int depth;
} Local;

typedef enum eFunctionType
{
	TYPE_FUNCTION,
	TYPE_SCRIPT,
} FunctionType;

typedef struct sCompiler
{
	struct sCompiler * enclosing;
	Scanner * scanner;
	Parser * parser;

	ObjFunction * function;
	FunctionType type;

	// TODO: Enhancement. Allow more local variables
	// TODO: Optimization. Make lookup faster (currently requires linear search through array)
	// TODO: Enhancement. Add concept of variables that don't allow re-assignment ('let'?)
	Local locals[UINT8_COUNT];
	int localCount;
	int scopeDepth;
} Compiler;

Compiler * current = NULL;



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
static void initCompiler(Compiler * compiler, Scanner * scanner, Parser * parser, FunctionType type);
static ObjFunction * endCompiler(void);
static void emitReturn(void);
static void expression(void);
static void statement(void);
static void declaration(void);
static void funDeclaration(void);
static void varDeclaration(void);
static void printStatement(void);
static void returnStatement(void);
static void whileStatement(void);
static void expressionStatement(void);
static void forStatement(void);
static void ifStatement(void);
static void parsePrecedence(Precedence precendece);
static uint32_t identifierConstant(Token * name);
static bool resolveLocal(Compiler * compiler, Token * name, uint32_t * localIndex);
static void declareVariable(void);
static uint8_t argumentList(void);
static const ParseRule * getRule(TokenType type);

ObjFunction * compile(const char * source)
{
	Scanner scanner;
	initScanner(&scanner, source);

	Parser parser;
	memset(&parser, 0, sizeof(parser));

	Compiler compiler;
	initCompiler(&compiler, &scanner, &parser, TYPE_SCRIPT);

	advance();

	while (!match(TOKEN_EOF))
	{
		declaration();
	}

	ObjFunction * function = endCompiler();

	return parser.hadError ? NULL : function;
}

static void advance(void)
{
	current->parser->previous = current->parser->current;

	for (;;)
	{
		current->parser->current = scanToken(current->scanner);

		if (current->parser->current.type != TOKEN_ERROR)
			break;

		errorAtCurrent(current->parser->current.start);
	}
}

static void errorAtCurrent(const char * message)
{
	errorAt(&current->parser->current, message);
}

static void error(const char * message)
{
	errorAt(&current->parser->previous, message);
}

static void errorAt(Token * token, const char * message)
{
	if (current->parser->panicMode)
		return;

	current->parser->panicMode = true;

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

	current->parser->hadError = true;
}

static void consume(TokenType type, const char * message)
{
	if (current->parser->current.type == type)
	{
		advance();
		return;
	}

	errorAtCurrent(message);
}

static bool check(TokenType type)
{
	return current->parser->current.type == type;
}

static bool match(TokenType type)
{
	if (!check(type)) return false;
	advance();
	return true;
}

static Chunk * currentChunk(void)
{
	return &current->function->chunk;
}

static void synchronize(void)
{
	current->parser->panicMode = false;

	while (current->parser->current.type != TOKEN_EOF)
	{
		if (current->parser->previous.type == TOKEN_SEMICOLON)
			return;

		switch (current->parser->current.type)
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
	writeChunk(currentChunk(), byte, current->parser->previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2)
{
	emitByte(byte1);
	emitByte(byte2);
}

static void emitLoop(uint32_t loopStart)
{
	emitByte(OP_LOOP);

	uint32_t offset = ARY_LEN(currentChunk()->aryB) - loopStart + 2;
	if (offset > UINT16_MAX)
	{
		error("Loop body too large.");
	}

	emitByte((offset >> 8) & 0xff);
	emitByte(offset & 0xff);
}

static uint32_t emitJump(uint8_t instruction)
{
	emitByte(instruction);
	emitByte(0xff);
	emitByte(0xff);
	return ARY_LEN(currentChunk()->aryB) - 2;
}

static void emitReturn(void)
{
	emitByte(OP_NIL);
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

static void patchJump(uint32_t offset)
{
	// -2 to adjust for the bytecode for the jump offset itself

	uint32_t jump = ARY_LEN(currentChunk()->aryB) - offset - 2;

	if (jump > UINT16_MAX)
	{
		error("Too much code to jump over.");
	}

	currentChunk()->aryB[offset] = (jump >> 8) & 0xff;
	currentChunk()->aryB[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler * compiler, Scanner * scanner, Parser * parser, FunctionType type)
{
	compiler->enclosing = current;
	compiler->scanner = scanner;
	compiler->parser = parser;
	compiler->function = NULL;
	compiler->type = type;
	compiler->localCount = 0;
	compiler->scopeDepth = 0;
	compiler->function = newFunction();
	current = compiler;

	if (type != TYPE_SCRIPT)
	{
		current->function->name = copyString(current->parser->previous.start, current->parser->previous.length);
	}

	Local * local = &current->locals[current->localCount++];
	local->depth = 0;
	local->name.start = "";
	local->name.length = 0;
}

static ObjFunction * endCompiler(void)
{
	emitReturn();
	ObjFunction * function = current->function;

#if DEBUG_PRINT_CODE
	if (!current->parser->hadError)
	{
		disassembleChunk(
			currentChunk(),
			function->name != NULL ? function->name->aChars : "<script>");
	}
#endif

	current = current->enclosing;

	return function;
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

	TokenType operatorType = current->parser->previous.type;

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

static void call(bool canAssign)
{
	UNUSED(canAssign);

	uint8_t argCount = argumentList();
	emitBytes(OP_CALL, argCount);
}

static void literal(bool canAssign)
{
	UNUSED(canAssign);

	switch (current->parser->previous.type)
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

	double value = strtod(current->parser->previous.start, NULL);
	emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign)
{
	UNUSED(canAssign);

	emitConstant(OBJ_VAL(copyString(current->parser->previous.start + 1, current->parser->previous.length - 2)));
}

static void and_(bool canAssign)
{
	UNUSED(canAssign);

	uint32_t endJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);
	parsePrecedence(PREC_AND);

	patchJump(endJump);
}

static void or_(bool canAssign)
{
	// TODO (matthewp) Not as efficient as just having an OP_JUMP_IF_TRUE

	UNUSED(canAssign);

	uint32_t elseJump = emitJump(OP_JUMP_IF_FALSE);
	uint32_t endJump = emitJump(OP_JUMP);

	patchJump(elseJump);
	emitByte(OP_POP);

	parsePrecedence(PREC_OR);
	patchJump(endJump);
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
	namedVariable(current->parser->previous, canAssign);
}

static void unary(bool canAssign)
{
	UNUSED(canAssign);

	TokenType operatorType = current->parser->previous.type;

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
	{ grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
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
	{ NULL,     and_,    PREC_AND },        // TOKEN_AND
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
	{ literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
	{ NULL,     NULL,    PREC_NONE },       // TOKEN_IF
	{ literal,  NULL,    PREC_NONE },       // TOKEN_NIL
	{ NULL,     or_,     PREC_OR },         // TOKEN_OR
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

	ParseFn prefixFn = getRule(current->parser->previous.type)->prefix;

	if (prefixFn == NULL)
	{
		error("Expect expression.");
		return;
	}

	bool canAssign = (precedence <= PREC_ASSIGNMENT);
	prefixFn(canAssign);

	while (precedence <= getRule(current->parser->current.type)->precedence)
	{
		advance();
		ParseFn infixFn = getRule(current->parser->previous.type)->infix;
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

	Token * name = &current->parser->previous;

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

	return identifierConstant(&current->parser->previous);
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

static uint8_t argumentList(void)
{
	uint8_t argCount = 0;

	if (!check(TOKEN_RIGHT_PAREN))
	{
		do
		{
			expression();

			if (argCount == 255)
			{
				error("Cannot have more than 255 arguments.");
			}

			argCount++;
		}
		while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");

	return argCount;
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

static void function(FunctionType type)
{
	Compiler compiler;
	initCompiler(&compiler, current->scanner, current->parser, type);
	beginScope();

	// Compile the parameter list

	consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

	if (!check(TOKEN_RIGHT_PAREN))
	{
		do
		{
			uint32_t paramConstant = parseVariable("Expect parameter name.");
			defineVariable(paramConstant);

			current->function->arity++;

			if (current->function->arity > 255)
			{
				error("Cannot have more than 255 parameters.");
			}
		}
		while (match(TOKEN_COMMA));
	}

	consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

	// The body

	consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
	block();

	// Create the function object

	ObjFunction * function = endCompiler();
	emitConstant(OBJ_VAL(function));
}

static void declaration(void)
{
	if (match(TOKEN_FUN))
	{
		funDeclaration();
	}
	else if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else
	{
		statement();
	}

	if (current->parser->panicMode)
	{
		synchronize();
	}
}

static void funDeclaration(void)
{
	uint32_t global = parseVariable("Expect function name.");
	markInitialized();
	function(TYPE_FUNCTION);
	defineVariable(global);
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
	else if (match(TOKEN_FOR))
	{
		forStatement();
	}
	else if (match(TOKEN_IF))
	{
		ifStatement();
	}
	else if (match(TOKEN_RETURN))
	{
		returnStatement();
	}
	else if (match(TOKEN_WHILE))
	{
		whileStatement();
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

static void returnStatement(void)
{
	if (current->type == TYPE_SCRIPT)
	{
		error("Cannot return from top-level code.");
	}

	if (match(TOKEN_SEMICOLON))
	{
		emitReturn();
	}
	else
	{
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
		emitByte(OP_RETURN);
	}
}

static void whileStatement(void)
{
	// TODO (matthewp) Add support for 'continue' statement

	uint32_t loopStart = ARY_LEN(currentChunk()->aryB);

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	uint32_t exitJump = emitJump(OP_JUMP_IF_FALSE);

	emitByte(OP_POP);
	statement();

	emitLoop(loopStart);

	patchJump(exitJump);
	emitByte(OP_POP);
}

static void expressionStatement(void)
{
	expression();
	consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
	emitByte(OP_POP);
}

static void forStatement(void)
{
	// TODO (matthewp) Add support for 'continue' statement

	beginScope();

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	if (match(TOKEN_VAR))
	{
		varDeclaration();
	}
	else if (match(TOKEN_SEMICOLON))
	{
		// No initializer
	}
	else
	{
		expressionStatement();
	}

	uint32_t loopStart = ARY_LEN(currentChunk()->aryB);

	bool hasJump = false;
	uint32_t exitJump = 0;

	if (!match(TOKEN_SEMICOLON))
	{
		expression();
		consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

		// Jump out of the loop if the condition is false

		exitJump = emitJump(OP_JUMP_IF_FALSE);
		emitByte(OP_POP); // Condition
		hasJump = true;
	}

	if (!match(TOKEN_RIGHT_PAREN))
	{
		// TODO (matthewp) This is pretty weird an adds additional jumps

		uint32_t bodyJump = emitJump(OP_JUMP);

		uint32_t incrementStart = ARY_LEN(currentChunk()->aryB);
		expression();
		emitByte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

		emitLoop(loopStart);
		loopStart = incrementStart;
		patchJump(bodyJump);
	}

	statement();

	emitLoop(loopStart);

	if (hasJump)
	{
		patchJump(exitJump);
		emitByte(OP_POP); // Condition
	}

	endScope();
}

static void ifStatement(void)
{
	// TODO (matthewp) Support 'switch' statements

	consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
	expression();
	consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

	uint32_t thenJump = emitJump(OP_JUMP_IF_FALSE);
	emitByte(OP_POP);
	statement();

	uint32_t elseJump = emitJump(OP_JUMP);

	patchJump(thenJump);
	emitByte(OP_POP);

	if (match(TOKEN_ELSE))
	{
		statement();
	}

	patchJump(elseJump);
}
