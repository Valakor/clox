//
//  debug.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "chunk.h"

void disassembleChunk(Chunk * chunk, const char * name);
unsigned disassembleInstruction(Chunk * chunk, unsigned i);
