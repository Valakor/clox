//
//  compiler.h
//  clox
//
//  Created by Matthew Pohlmann on 5/18/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#pragma once

#include "common.h"
#include "chunk.h"



bool compile(const char * source, Chunk * chunk);
