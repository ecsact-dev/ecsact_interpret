#ifndef ECSACT_PARSE_RUNTIME_INTEROP_H
#define ECSACT_PARSE_RUNTIME_INTEROP_H

#include "ecsact/parse.h"

/**
 * Parses the ecsact files with `ecsact_parse` and invokes ecsact runtime
 * dynamic module methods resulting in components, systems, and actions being
 * registered.
 */
void ecsact_parse_runtime_interop
	( const char**  file_paths
	, int           file_paths_count
	);

#endif//ECSACT_PARSE_RUNTIME_INTEROP_H
