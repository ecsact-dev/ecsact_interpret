#include "ecsact/interpret/eval.h"

#include <concepts>
#include <unordered_map>
#include <string_view>
#include <vector>
#include <string>
#include <string_view>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <span>
#include "magic_enum.hpp"
#include "ecsact/parse.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.hh"

using namespace std::string_literals;

std::optional<ecsact_field_id> find_field_by_name(
	ecsact_composite_id compo_id,
	std::string_view    target_field_name
) {
	std::vector<ecsact_field_id> field_ids;
	field_ids.resize(ecsact_meta_count_fields(compo_id));
	ecsact_meta_get_field_ids(
		compo_id,
		static_cast<int32_t>(field_ids.size()),
		field_ids.data(),
		nullptr
	);
	for(auto& field_id : field_ids) {
		std::string field_name = ecsact_meta_field_name(compo_id, field_id);
		if(field_name == target_field_name) {
			return field_id;
		}
	}

	return {};
}

std::optional<ecsact_enum_id> find_enum_by_name(
	ecsact_package_id package_id,
	std::string_view  target_enum_name
) {
	using ecsact::meta::get_enum_ids;

	for(auto& enum_id : get_enum_ids(package_id)) {
		std::string enum_name = ecsact_meta_enum_name(enum_id);
		if(enum_name == target_enum_name) {
			return enum_id;
		}
	}

	return {};
}

std::optional<ecsact_field_type> find_user_field_type_by_name(
	ecsact_package_id package_id,
	std::string_view  user_type_name,
	int32_t           length
) {
	auto enum_id = find_enum_by_name(package_id, user_type_name);
	if(enum_id) {
		return ecsact_field_type{
			.kind = ECSACT_TYPE_KIND_ENUM,
			.type{.enum_id = *enum_id},
			.length = length,
		};
	}

	return {};
}

template<typename R, typename T>
static std::optional<R> cast_optional_id(std::optional<T> opt_id) {
	if(opt_id) {
		return ecsact_id_cast<R>(*opt_id);
	}

	return {};
}

template<typename T>
std::optional<T> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
);

template<typename T>
std::optional<T> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
);

template<>
std::optional<ecsact_component_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_component_ids(package_id)) {
		if(name == ecsact::meta::component_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_transient_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_transient_ids(package_id)) {
		if(name == ecsact::meta::transient_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_system_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_system_ids(package_id)) {
		if(name == ecsact::meta::system_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_action_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_action_ids(package_id)) {
		if(name == ecsact::meta::action_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_enum_id> find_by_name(
	ecsact_package_id  package_id,
	const std::string& name
) {
	for(auto id : ecsact::meta::get_enum_ids(package_id)) {
		if(name == ecsact::meta::enum_name(id)) {
			return id;
		}
	}

	return {};
}

template<>
std::optional<ecsact_composite_id> find_by_name(
	ecsact_package_id  pkg_id,
	const std::string& name
) {
	if(auto id = find_by_name<ecsact_component_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_composite_id>(*id);
	}

	if(auto id = find_by_name<ecsact_transient_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_composite_id>(*id);
	}

	if(auto id = find_by_name<ecsact_action_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_composite_id>(*id);
	}

	return {};
}

template<>
std::optional<ecsact_decl_id> find_by_name(
	ecsact_package_id  pkg_id,
	const std::string& name
) {
	if(auto id = find_by_name<ecsact_component_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	if(auto id = find_by_name<ecsact_transient_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	if(auto id = find_by_name<ecsact_system_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	if(auto id = find_by_name<ecsact_action_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_decl_id>(*id);
	}

	return {};
}

template<>
std::optional<ecsact_component_like_id> find_by_name(
	ecsact_package_id  pkg_id,
	const std::string& name
) {
	if(auto id = find_by_name<ecsact_component_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_component_like_id>(*id);
	}

	if(auto id = find_by_name<ecsact_transient_id>(pkg_id, name)) {
		return ecsact_id_cast<ecsact_component_like_id>(*id);
	}

	return {};
}

template<>
std::optional<ecsact_composite_id> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
) {
	switch(statement.type) {
		case ECSACT_STATEMENT_COMPONENT:
			return cast_optional_id<ecsact_composite_id>(
				find_by_name<ecsact_component_id>(
					package_id,
					std::string(
						statement.data.component_statement.component_name.data,
						statement.data.component_statement.component_name.length
					)
				)
			);
		case ECSACT_STATEMENT_TRANSIENT:
			return cast_optional_id<ecsact_composite_id>(
				find_by_name<ecsact_transient_id>(
					package_id,
					std::string(
						statement.data.transient_statement.transient_name.data,
						statement.data.transient_statement.transient_name.length
					)
				)
			);
		case ECSACT_STATEMENT_ACTION:
			return cast_optional_id<ecsact_composite_id>(
				find_by_name<ecsact_action_id>(
					package_id,
					std::string(
						statement.data.action_statement.action_name.data,
						statement.data.action_statement.action_name.length
					)
				)
			);
		default:
			break;
	}

	return {};
}

template<>
std::optional<ecsact_component_like_id> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
) {
	switch(statement.type) {
		case ECSACT_STATEMENT_COMPONENT:
			return cast_optional_id<ecsact_component_like_id>(
				find_by_name<ecsact_component_id>(
					package_id,
					std::string(
						statement.data.component_statement.component_name.data,
						statement.data.component_statement.component_name.length
					)
				)
			);
		case ECSACT_STATEMENT_TRANSIENT:
			return cast_optional_id<ecsact_component_like_id>(
				find_by_name<ecsact_transient_id>(
					package_id,
					std::string(
						statement.data.transient_statement.transient_name.data,
						statement.data.transient_statement.transient_name.length
					)
				)
			);
		default:
			break;
	}

	return {};
}

template<>
std::optional<ecsact_system_like_id> find_by_statement(
	ecsact_package_id       package_id,
	const ecsact_statement& statement
) {
	switch(statement.type) {
		case ECSACT_STATEMENT_SYSTEM:
			return cast_optional_id<ecsact_system_like_id>(
				find_by_name<ecsact_system_id>(
					package_id,
					std::string(
						statement.data.system_statement.system_name.data,
						statement.data.system_statement.system_name.length
					)
				)
			);
		case ECSACT_STATEMENT_ACTION:
			return cast_optional_id<ecsact_system_like_id>(
				find_by_name<ecsact_action_id>(
					package_id,
					std::string(
						statement.data.action_statement.action_name.data,
						statement.data.action_statement.action_name.length
					)
				)
			);
		default:
			break;
	}

	return {};
}

static ecsact_eval_error
eval_none_statement(ecsact_package_id, std::span<const ecsact_statement>&, const ecsact_statement&) {
	return {};
}

static ecsact_eval_error
eval_unknown_statement(ecsact_package_id, std::span<const ecsact_statement>&, const ecsact_statement&) {
	return {};
}

static ecsact_eval_error eval_import_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto&       data = statement.data.import_statement;
	std::string import_name(
		data.import_package_name.data,
		data.import_package_name.length
	);
	for(auto dep_pkg_id : ecsact::meta::get_package_ids()) {
		if(dep_pkg_id == package_id) {
			continue;
		}
		if(ecsact::meta::package_name(dep_pkg_id) == import_name) {
			// TODO(zaucy): Import support
			// ecsact_add_dependency(package_id, dep_pkg_id);
			return {};
		}
	}

	return ecsact_eval_error{
		.code = ECSACT_EVAL_ERR_UNKNOWN_IMPORT,
		.relevant_content = data.import_package_name,
	};
}

static ecsact_eval_error eval_component_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(!context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.component_statement;
	auto name = std::string(data.component_name.data, data.component_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.component_name,
		};
	}

	ecsact_create_component(
		package_id,
		data.component_name.data,
		data.component_name.length
	);

	return {};
}

static ecsact_eval_error eval_transient_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(!context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.transient_statement;
	auto name = std::string(data.transient_name.data, data.transient_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.transient_name,
		};
	}

	ecsact_create_transient(
		package_id,
		data.transient_name.data,
		data.transient_name.length
	);

	return {};
}

static ecsact_eval_error eval_system_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	std::optional<ecsact_system_like_id> parent_sys_like_id{};
	if(!context_stack.empty()) {
		parent_sys_like_id = find_by_statement<ecsact_system_like_id>(
			package_id,
			context_stack.back()
		);
		if(!parent_sys_like_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
				.relevant_content = {},
			};
		}
	}

	auto& data = statement.data.system_statement;
	auto  name = std::string(data.system_name.data, data.system_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.system_name,
		};
	}

	auto sys_id = ecsact_create_system(
		package_id,
		data.system_name.data,
		data.system_name.length
	);

	if(parent_sys_like_id) {
		ecsact_add_child_system(*parent_sys_like_id, sys_id);
	}

	return {};
}

static ecsact_eval_error eval_action_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(!context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.action_statement;
	auto  name = std::string(data.action_name.data, data.action_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.action_name,
		};
	}

	ecsact_create_action(
		package_id,
		data.action_name.data,
		data.action_name.length
	);

	return {};
}

static ecsact_eval_error eval_enum_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(!context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.enum_statement;
	auto  name = std::string(data.enum_name.data, data.enum_name.length);

	auto existing_decl = find_by_name<ecsact_decl_id>(package_id, name);
	if(existing_decl) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_DECLARATION_NAME_TAKEN,
			.relevant_content = data.enum_name,
		};
	}

	ecsact_create_enum(package_id, data.enum_name.data, data.enum_name.length);

	return {};
}

static ecsact_eval_error eval_enum_value_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.enum_value_statement;

	if(context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = data.name,
		};
	}

	if(context_stack.back().type != ECSACT_STATEMENT_ENUM) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = data.name,
		};
	}

	auto& context_data = context_stack.back().data.enum_statement;
	auto  enum_name =
		std::string(context_data.enum_name.data, context_data.enum_name.length);

	auto enum_id = find_by_name<ecsact_enum_id>(package_id, enum_name);
	if(!enum_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = context_data.enum_name,
		};
	}

	ecsact_add_enum_value(*enum_id, data.value, data.name.data, data.name.length);

	return {};
}

static ecsact_eval_error eval_builtin_type_field_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.field_statement;

	if(context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto compo_id =
		find_by_statement<ecsact_composite_id>(package_id, context_stack.back());
	if(!compo_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	std::string field_name(data.field_name.data, data.field_name.length);

	for(auto field_id : ecsact::meta::get_field_ids(*compo_id)) {
		std::string other_field_name = ecsact_meta_field_name(*compo_id, field_id);
		if(field_name == other_field_name) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,
				.relevant_content = data.field_name,
			};
		}
	}

	ecsact_add_field(
		*compo_id,
		ecsact_field_type{
			.kind = ECSACT_TYPE_KIND_BUILTIN,
			.type{.builtin = data.field_type},
			.length = data.length,
		},
		data.field_name.data,
		data.field_name.length
	);

	return {};
}

static ecsact_eval_error eval_user_type_field_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	auto& data = statement.data.user_type_field_statement;

	if(context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto compo_id =
		find_by_statement<ecsact_composite_id>(package_id, context_stack.back());
	if(!compo_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	std::string field_name(data.field_name.data, data.field_name.length);

	for(auto field_id : ecsact::meta::get_field_ids(*compo_id)) {
		std::string other_field_name = ecsact_meta_field_name(*compo_id, field_id);
		if(field_name == other_field_name) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,
				.relevant_content = data.field_name,
			};
		}
	}

	auto user_type_name =
		std::string_view(data.user_type_name.data, data.user_type_name.length);
	auto user_field_type =
		find_user_field_type_by_name(package_id, user_type_name, data.length);
	if(!user_field_type) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_FIELD_TYPE,
			.relevant_content = data.user_type_name,
		};
	}

	ecsact_add_field(
		*compo_id,
		*user_field_type,
		data.field_name.data,
		data.field_name.length
	);

	return {};
}

static ecsact_eval_error eval_entity_field_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	return eval_builtin_type_field_statement(
		package_id,
		context_stack,
		statement
	);
}

static ecsact_eval_error eval_system_component_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	std::optional<ecsact_system_like_id>    sys_like_id{};
	std::optional<ecsact_component_like_id> assoc_comp{};
	std::optional<ecsact_field_id>          assoc_comp_field{};

	auto& context = context_stack.back();
	switch(context.type) {
		case ECSACT_STATEMENT_SYSTEM:
		case ECSACT_STATEMENT_ACTION:
			sys_like_id =
				find_by_statement<ecsact_system_like_id>(package_id, context);
			break;
		case ECSACT_STATEMENT_SYSTEM_COMPONENT: {
			auto& data = context.data.system_component_statement;

			if(context_stack.size() < 2) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}
			sys_like_id = find_by_statement<ecsact_system_like_id>(
				package_id,
				context_stack[context_stack.size() - 2]
			);

			std::string comp_like_name(
				data.component_name.data,
				data.component_name.length
			);

			assoc_comp =
				find_by_name<ecsact_component_like_id>(package_id, comp_like_name);
			if(!assoc_comp) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = {},
				};
			}

			if(data.with_entity_field_name.length == 0) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = data.component_name,
				};
			}
			assoc_comp_field = find_field_by_name(
				ecsact_id_cast<ecsact_composite_id>(*assoc_comp),
				std::string_view(
					data.with_entity_field_name.data,
					data.with_entity_field_name.length
				)
			);

			if(!assoc_comp_field) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
					.relevant_content = data.with_entity_field_name,
				};
			}
			break;
		}
		default:
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
				.relevant_content = {},
			};
	}

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.system_component_statement;

	std::string comp_like_name(
		data.component_name.data,
		data.component_name.length
	);

	auto comp_like_id =
		find_by_name<ecsact_component_like_id>(package_id, comp_like_name);

	if(!comp_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE,
			.relevant_content = data.component_name,
		};
	}

	if(assoc_comp) {
		auto assoc_caps = ecsact::meta::system_association_capabilities(
			*sys_like_id,
			*assoc_comp,
			*assoc_comp_field
		);
		for(auto& entry : assoc_caps) {
			if(entry.first == *comp_like_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_MULTIPLE_CAPABILITIES_SAME_COMPONENT_LIKE,
					.relevant_content = data.component_name,
				};
			}
		}
	} else {
		for(auto& entry : ecsact::meta::system_capabilities(*sys_like_id)) {
			if(entry.first == *comp_like_id) {
				return ecsact_eval_error{
					.code = ECSACT_EVAL_ERR_MULTIPLE_CAPABILITIES_SAME_COMPONENT_LIKE,
					.relevant_content = data.component_name,
				};
			}
		}
	}

	std::optional<ecsact_field_id> entity_field_id{};
	if(data.with_entity_field_name.length > 0) {
		std::string entity_field_name(
			data.with_entity_field_name.data,
			data.with_entity_field_name.length
		);

		for(auto field_id : ecsact::meta::get_field_ids(*comp_like_id)) {
			std::string field_name = ecsact_meta_field_name(
				ecsact_id_cast<ecsact_composite_id>(*comp_like_id),
				field_id
			);
			if(entity_field_name == field_name) {
				entity_field_id = field_id;
				break;
			}
		}

		if(!entity_field_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,
				.relevant_content = data.with_entity_field_name,
			};
		}
	}

	if(assoc_comp) {
		ecsact_set_system_association_capability(
			*sys_like_id,
			*assoc_comp,
			*assoc_comp_field,
			*comp_like_id,
			data.capability
		);
	} else {
		ecsact_set_system_capability(*sys_like_id, *comp_like_id, data.capability);
	}

	return {};
}

static ecsact_eval_error eval_system_generates_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(context_stack.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto sys_like_id =
		find_by_statement<ecsact_system_like_id>(package_id, context_stack.back());

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto gen_ids = ecsact::meta::get_system_generates_ids(*sys_like_id);
	if(!gen_ids.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_ONLY_ONE_GENERATES_BLOCK_ALLOWED,
			.relevant_content = {},
		};
	}

	ecsact_add_system_generates(*sys_like_id);

	return {};
}

static ecsact_eval_error eval_system_with_entity_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(context_stack.size() < 2) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& sys_comp_statement = context_stack.back();
	if(sys_comp_statement.type != ECSACT_STATEMENT_SYSTEM_COMPONENT) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& sys_like_statement = context_stack[context_stack.size() - 2];
	auto  sys_like_id =
		find_by_statement<ecsact_system_like_id>(package_id, sys_like_statement);

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto comp_name = std::string(
		sys_comp_statement.data.system_component_statement.component_name.data,
		sys_comp_statement.data.system_component_statement.component_name.length
	);

	auto comp_like_id =
		find_by_name<ecsact_component_like_id>(package_id, comp_name);

	if(!comp_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_LIKE_TYPE,
			.relevant_content =
				sys_comp_statement.data.system_component_statement.component_name,
		};
	}

	auto& data = statement.data.system_with_entity_statement;

	std::string entity_field_name(
		data.with_entity_field_name.data,
		data.with_entity_field_name.length
	);

	std::optional<ecsact_field_id> entity_field_id{};
	for(auto field_id : ecsact::meta::get_field_ids(*comp_like_id)) {
		std::string field_name = ecsact_meta_field_name(
			ecsact_id_cast<ecsact_composite_id>(*comp_like_id),
			field_id
		);
		if(entity_field_name == field_name) {
			entity_field_id = field_id;
			break;
		}
	}

	if(!entity_field_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_FIELD_NAME_ALREADY_EXISTS,
			.relevant_content = data.with_entity_field_name,
		};
	}

	return {};
}

static ecsact_eval_error eval_entity_constraint_statement(
	ecsact_package_id                  package_id,
	std::span<const ecsact_statement>& context_stack,
	const ecsact_statement&            statement
) {
	if(context_stack.size() < 2) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& generates_statement = context_stack[context_stack.size() - 1];
	auto& sys_like_statement = context_stack[context_stack.size() - 2];

	if(generates_statement.type != ECSACT_STATEMENT_SYSTEM_GENERATES) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto sys_like_id =
		find_by_statement<ecsact_system_like_id>(package_id, sys_like_statement);

	if(!sys_like_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto& data = statement.data.entity_constraint_statement;

	std::string comp_name(
		data.constraint_component_name.data,
		data.constraint_component_name.length
	);

	auto comp_id = find_by_name<ecsact_component_id>(package_id, comp_name);
	if(!comp_id) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_UNKNOWN_COMPONENT_TYPE,
			.relevant_content = data.constraint_component_name,
		};
	}

	auto sys_gen_ids = ecsact::meta::get_system_generates_ids(*sys_like_id);
	if(sys_gen_ids.empty()) {
		return ecsact_eval_error{
			.code = ECSACT_EVAL_ERR_INVALID_CONTEXT,
			.relevant_content = {},
		};
	}

	auto gen_comps =
		ecsact::meta::get_system_generates_components(*sys_like_id, sys_gen_ids[0]);

	for(auto& entry : gen_comps) {
		if(entry.first == *comp_id) {
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_GENERATES_DUPLICATE_COMPONENT_CONSTRAINTS,
				.relevant_content = data.constraint_component_name,
			};
		}
	}

	ecsact_system_generates_set_component(
		*sys_like_id,
		sys_gen_ids[0],
		*comp_id,
		data.optional ? ECSACT_SYS_GEN_OPTIONAL : ECSACT_SYS_GEN_REQUIRED
	);

	return {};
}

ecsact_eval_error ecsact_eval_statement(
	ecsact_package_id       package_id,
	int32_t                 statement_stack_size,
	const ecsact_statement* statement_stack
) {
	if(statement_stack_size == 0) {
		return {};
	}

	auto&     statement = statement_stack[statement_stack_size - 1];
	std::span context_statements(statement_stack, statement_stack_size - 1);

	switch(statement.type) {
		case ECSACT_STATEMENT_NONE:
			return eval_none_statement(package_id, context_statements, statement);
		case ECSACT_STATEMENT_UNKNOWN:
			return eval_unknown_statement(package_id, context_statements, statement);
		case ECSACT_STATEMENT_PACKAGE:
			return ecsact_eval_error{
				.code = ECSACT_EVAL_ERR_UNEXPECTED_STATEMENT,
				.relevant_content = {},
			};
		case ECSACT_STATEMENT_IMPORT:
			return eval_import_statement(package_id, context_statements, statement);
		case ECSACT_STATEMENT_COMPONENT:
			return eval_component_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_TRANSIENT:
			return eval_transient_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_SYSTEM:
			return eval_system_statement(package_id, context_statements, statement);
		case ECSACT_STATEMENT_ACTION:
			return eval_action_statement(package_id, context_statements, statement);
		case ECSACT_STATEMENT_ENUM:
			return eval_enum_statement(package_id, context_statements, statement);
		case ECSACT_STATEMENT_ENUM_VALUE:
			return eval_enum_value_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_BUILTIN_TYPE_FIELD:
			return eval_builtin_type_field_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_USER_TYPE_FIELD:
			return eval_user_type_field_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_ENTITY_FIELD:
			return eval_entity_field_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_SYSTEM_COMPONENT:
			return eval_system_component_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_SYSTEM_GENERATES:
			return eval_system_generates_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_SYSTEM_WITH_ENTITY:
			return eval_system_with_entity_statement(
				package_id,
				context_statements,
				statement
			);
		case ECSACT_STATEMENT_ENTITY_CONSTRAINT:
			return eval_entity_constraint_statement(
				package_id,
				context_statements,
				statement
			);
	}

	throw std::runtime_error("Unhandled statement type");
}

ecsact_package_id ecsact_eval_package_statement(
	const ecsact_package_statement* package_statement
) {
	return ecsact_create_package(
		package_statement->main,
		package_statement->package_name.data,
		package_statement->package_name.length
	);
}

void ecsact_eval_reset() {
	// eval_pkgs.clear();
}
