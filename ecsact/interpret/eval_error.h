#ifndef ECSACT_EVAL_ERROR_H
#define ECSACT_EVAL_ERROR_H

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

	/// When evaluating a file the first statement was not a package statement.
	ECSACT_EVAL_ERR_EXPECTED_PACKAGE_STATEMENT,

	/// An import statement was found after a declaration was already made.
	ECSACT_EVAL_ERR_LATE_IMPORT,

	/// Could not find package with name given to import.
	ECSACT_EVAL_ERR_UNKNOWN_IMPORT,

	/// The given statement was not expected in the given context.
	ECSACT_EVAL_ERR_UNEXPECTED_STATEMENT,

	/// The given field name was not found
	ECSACT_EVAL_ERR_UNKNOWN_FIELD_NAME,

	/// Not an error code. Start of file only errors.
	/// File error codes only applies when parsing complete Ecsact files or
	/// buffers. Individual evaluations do not have conceptual endings of
	/// statements.
	ECSACT_EVAL_BEGIN_ERR_FILE_ONLY = 10000,

	/// System or action has no capabilities.
	ECSACT_EVAL_ERR_NO_CAPABILITIES,

	/// Not an error code. End of file only errors.
	/// SEE: ECSACT_EVAL_BEGIN_ERR_FILE_ONLY,
	ECSACT_EVAL_END_ERR_FILE_ONLY = 20000,
} ecsact_eval_error_code;

#endif // ECSACT_EVAL_ERROR_H
