#pragma once

#include "ecsact/parse/statements.h"

namespace ecsact::detail {
	template<typename Callback>
	auto parse_statement_data_visit
		( const ecsact_statement&  statement
		, Callback&&               callback
		)
	{
		switch(statement.type) {
			case ECSACT_STATEMENT_NONE:
				return callback(ecsact_none_statement{});
			case ECSACT_STATEMENT_UNKNOWN:
				return callback(ecsact_unknown_statement{});
			case ECSACT_STATEMENT_PACKAGE:
				return callback(statement.data.package_statement);
			case ECSACT_STATEMENT_IMPORT:
				return callback(statement.data.import_statement);
			case ECSACT_STATEMENT_COMPONENT:
				return callback(statement.data.component_statement);
			case ECSACT_STATEMENT_SYSTEM:
				return callback(statement.data.system_statement);
			case ECSACT_STATEMENT_ACTION:
				return callback(statement.data.action_statement);
			case ECSACT_STATEMENT_ENUM:
				return callback(statement.data.enum_statement);
			case ECSACT_STATEMENT_I8_FIELD:
			case ECSACT_STATEMENT_U8_FIELD:
			case ECSACT_STATEMENT_I16_FIELD:
			case ECSACT_STATEMENT_U16_FIELD:
			case ECSACT_STATEMENT_I32_FIELD:
			case ECSACT_STATEMENT_U32_FIELD:
			case ECSACT_STATEMENT_F32_FIELD:
			case ECSACT_STATEMENT_ENTITY_FIELD:
				return callback(statement.data.field_statement);
			case ECSACT_STATEMENT_USER_TYPE_FIELD:
				return callback(statement.data.user_type_field_statement);
			case ECSACT_STATEMENT_SYSTEM_COMPONENT:
				return callback(statement.data.system_component_statement);
			// case ECSACT_STATEMENT_SYSTEM_GENERATES:
			// 	return callback(statement.data.system_generates_statement);
			case ECSACT_STATEMENT_SYSTEM_WITH_ENTITY:
				return callback(statement.data.system_with_entity_statement);
			case ECSACT_STATEMENT_ENTITY_CONSTRAINT:
				return callback(statement.data.entity_constraint_statement);
		}
	}
}
