//
//  common.h
//  clox
//
//  Created by Matthew Pohlmann on 2/19/18.
//  Copyright © 2018 Matthew Pohlmann. All rights reserved.
//

#ifndef common_h
#define common_h

#include <stdbool.h>
#include <stdint.h>

#include <assert.h>

#if DEBUG
	#define ASSERT(f) assert(f)
	#define ASSERTMSG(f, msg) assert(f && msg)
#else
	#define ASSERT(...) (void)0
	#define ASSERTMSG(...) (void)0
#endif

#endif /* common_h */
