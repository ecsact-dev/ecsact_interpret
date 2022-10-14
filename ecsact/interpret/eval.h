#ifndef ECSACT_INTERPRET_EVAL_H
#define ECSACT_INTERPRET_EVAL_H

#include <stdint.h>
#include "ecsact/parse.h"

typedef enum ecsact_eval_error_code {
	/// No error
	ECSACT_EVAL_OK = 0,
	/// There was an evaluation attempted multiple times on a statement with the
	/// same ID. Statement IDs must be unique. 
	ECSACT_EVAL_ERR_DUPLICANT_STATEMENT_ID = 1,
	/// The given statement was not expected in the given context.
	ECSACT_EVAL_ERR_UNEXPECTED_STATEMENT = 2,
} ecsact_eval_error_code;

typedef struct ecsact_eval_error {
	ecsact_eval_error_code code;
	ecsact_statement_sv relevant_content;
} ecsact_eval_error;

/**
 * Invokes the appropriate ecsact runtime dynamic module methods for the given
 * statement typically from an `ecsact_parse` call.
 * @param context_statement the contextual statement for the `statement` param
 * @param statement the statment being evaluated
 * @returns error details
 */
ecsact_eval_error ecsact_eval_statement
	( const ecsact_statement*  context_statement
	, const ecsact_statement*  statement
	);

/**
 * Clears memory of all previous successfully evaluated statements.
 */
void ecsact_eval_reset();

#endif//ECSACT_INTERPRET_EVAL_H
