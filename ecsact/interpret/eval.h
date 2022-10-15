#ifndef ECSACT_INTERPRET_EVAL_H
#define ECSACT_INTERPRET_EVAL_H

#include <stdint.h>
#include "ecsact/parse.h"

typedef enum ecsact_eval_error_code {
	/// No error
	ECSACT_EVAL_OK,

	/// There was an evaluation attempted multiple times on a statement with the
	/// same ID. Statement IDs must be unique. 
	ECSACT_EVAL_ERR_DUPLICANT_STATEMENT_ID,

	/// A declaration with the name given already exists.
	ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
	
	/// Statement cannot be evaluated in context provided.
	ECSACT_EVAL_ERR_INVALID_CONTEXT,

	/// Composite already has field with name provided.
	ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,

	/// Field type could not be found by name given.
	ECSACT_EVAL_ERR_UNKNOWN_FIELD_TYPE,

	/// Cannot find component or transient with name given.
	ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE,

	/// Cannot find component with name given.
	ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_TYPE,

	/// Only 1 generates block is allowed per system-like.
	ECSACT_EVAL_ERR_ONLY_ONE_GENERATES_BLOCK_ALLOWED,

	/// Found multiple capabilities for the same component-like. Only one
	/// capability entry is allowed per component-like per system-like.
	ECSACT_EVAL_ERR_MULTIPLE_CAPABILITIES_SAME_COMPONENT_LIKE,

	/// Found multiple generates component constraints for the same component in
	/// the same generates block.
	ECSACT_EVAL_ERR_GENERATES_DUPLICATE_COMPONENT_CONSTRAINTS,

	/// An import statement was found after a declaration was already made.
	ECSACT_EVAL_ERR_LATE_IMPORT,

	/// The given statement was not expected in the given context.
	ECSACT_EVAL_ERR_UNEXPECTED_STATEMENT,
} ecsact_eval_error_code;

typedef struct ecsact_eval_error {
	ecsact_eval_error_code code;
	ecsact_statement_sv relevant_content;
} ecsact_eval_error;

ecsact_package_id ecsact_eval_package_statement
	( const ecsact_package_statement* package_statement
	);

/**
 * Invokes the appropriate ecsact runtime dynamic module methods for the given
 * statement typically from an `ecsact_parse` call.
 * @param package_id the package the statement should be evaluated in
 * @param statement_stack_size size of `statement_stack` parameter
 * @param statement_stack sequential list of size `statement_stack`. The list is
 *        treated like a stack where the last element is the 'top' element. The
 *        'top' element is the statement that is evaluated.
 * @returns error details
 */
ecsact_eval_error ecsact_eval_statement
	( ecsact_package_id        package_id
	, int32_t                  statement_stack_size
	, const ecsact_statement*  statement_stack
	);

/**
 * Clears memory of all previous successfully evaluated statements.
 */
void ecsact_eval_reset();

#endif//ECSACT_INTERPRET_EVAL_H
