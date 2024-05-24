#include "ecsact/runtime/meta.h"

#include <type_traits>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <utility>

static auto gen_next_assoc_id() -> ecsact_system_assoc_id {
	static ecsact_system_assoc_id next_assoc_id = {};
	ecsact_system_assoc_id        result = next_assoc_id;

	reinterpret_cast<std::underlying_type_t<decltype(next_assoc_id)>&>(
		next_assoc_id
	) += 1;

	return result;
}

struct assoc_info {
	using cap_comp_list_t =
		std::vector<std::pair<ecsact_component_like_id, ecsact_system_capability>>;

	ecsact_system_assoc_id                  id;
	std::optional<ecsact_component_like_id> comp_id;
	std::vector<ecsact_field_id>            assoc_fields;

	cap_comp_list_t caps;
};

using system_assoc_map_info_t =
	std::unordered_map<ecsact_system_like_id, std::vector<assoc_info>>;

static system_assoc_map_info_t system_assoc_info{};

static auto get_system_assoc_list( //
	ecsact_system_like_id system_id
) -> std::vector<assoc_info>* {
	auto itr = system_assoc_info.find(system_id);
	if(itr != system_assoc_info.end()) {
		return &itr->second;
	}

	return nullptr;
}

static auto get_assoc_info( //
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) -> assoc_info* {
	auto list = get_system_assoc_list(system_id);
	if(!list) {
		return nullptr;
	}

	auto itr = std::find_if( //
		list->begin(),
		list->end(),
		[=](auto item) -> bool { return item.id == assoc_id; }
	);

	if(itr == list->end()) {
		return nullptr;
	}

	return &(*itr);
}

ecsact_system_assoc_id ecsact_add_system_assoc( //
	ecsact_system_like_id system_id
) {
	auto itr = system_assoc_info.find(system_id);
	if(itr == system_assoc_info.end()) {
		itr = system_assoc_info.insert(system_assoc_info.end(), {system_id, {}});
	}

	auto& info = itr->second.emplace_back(assoc_info{});
	info.id = gen_next_assoc_id();
	return info.id;
}

void ecsact_remove_system_assoc( //
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto list = get_system_assoc_list(system_id);
	if(!list) {
		// No associations. Why did you try to remove? User error.
		return;
	}

	auto itr = std::find_if( //
		list->begin(),
		list->end(),
		[=](auto item) -> bool { return item.id == assoc_id; }
	);

	if(itr == list->end()) {
		// Unknown association id. User error.
		return;
	}

	list->erase(itr);
}

int32_t ecsact_meta_system_assoc_count( //
	ecsact_system_like_id system_id
) {
	auto list = get_system_assoc_list(system_id);
	if(list) {
		return list->size();
	}

	return 0;
}

int32_t ecsact_meta_system_assoc_ids(
	ecsact_system_like_id   system_id,
	int32_t                 max_assoc_count,
	ecsact_system_assoc_id* out_assoc_ids,
	int32_t*                out_assoc_count
) {
	auto list = get_system_assoc_list(system_id);
	if(!list) {
		if(out_assoc_count) {
			*out_assoc_count = 0;
		}
		return 0;
	}

	if(out_assoc_count) {
		*out_assoc_count = list->size();
	}

	for(int32_t i = 0; max_assoc_count > i; ++i) {
		if(i >= list->size()) {
			break;
		}
		auto& assoc = list->at(i);
		out_assoc_ids[i] = assoc.id;
	}

	return 0;
}

int32_t ecsact_meta_system_assoc_fields_count(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return 0;
	}

	return static_cast<int>(info->assoc_fields.size());
}

int32_t ecsact_meta_system_assoc_fields(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id,
	int32_t                max_fields_count,
	ecsact_field_id*       out_fields,
	int32_t*               out_fields_count
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		if(out_fields_count) {
			*out_fields_count = 0;
		}
		return 0;
	}

	if(out_fields_count) {
		*out_fields_count = info->assoc_fields.size();
	}

	for(int32_t i = 0; max_fields_count > i; ++i) {
		if(i >= info->assoc_fields.size()) {
			break;
		}
		out_fields[i] = info->assoc_fields[i];
	}

	return 0;
}

int32_t ecsact_meta_system_assoc_capabilities_count(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return 0;
	}

	return info->caps.size();
}

void ecsact_meta_system_assoc_capabilities(
	ecsact_system_like_id     system_id,
	ecsact_system_assoc_id    assoc_id,
	int32_t                   max_capabilities_count,
	ecsact_component_like_id* out_capability_component_ids,
	ecsact_system_capability* out_capabilities,
	int32_t*                  out_capabilities_count
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		if(out_capabilities_count) {
			*out_capabilities_count = 0;
		}
		return;
	}

	for(int32_t i = 0; max_capabilities_count > i; ++i) {
		if(i >= info->caps.size()) {
			break;
		}

		out_capability_component_ids[i] = info->caps[i].first;
		out_capabilities[i] = info->caps[i].second;
	}
}
