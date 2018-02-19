//
//  debug.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#ifndef debug_h
#define debug_h

#include "common.h"
#include "chunk.h"

void disassembleChunk(Chunk * chunk, const char * name);
int disassembleInstruction(Chunk * chunk, int i);

#endif /* debug_h */
