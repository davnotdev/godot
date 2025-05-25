#include "combimgsamplersplitter.h"

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/pair.h"

const uint32_t SPV_HEADER_LENGTH = 5;
const uint32_t SPV_HEADER_MAGIC = 0x07230203;
const uint32_t SPV_HEADER_MAGIC_NUM_OFFSET = 0;
const uint32_t SPV_HEADER_INSTRUCTION_BOUND_OFFSET = 3;

const uint16_t SPV_INSTRUCTION_OP_NOP = 1;
const uint16_t SPV_INSTRUCTION_OP_TYPE_VOID = 19;
const uint16_t SPV_INSTRUCTION_OP_TYPE_IMAGE = 25;
const uint16_t SPV_INSTRUCTION_OP_TYPE_SAMPLER = 26;
const uint16_t SPV_INSTRUCTION_OP_TYPE_SAMPLED_IMAGE = 27;
const uint16_t SPV_INSTRUCTION_OP_TYPE_POINTER = 32;
const uint16_t SPV_INSTRUCTION_OP_TYPE_FUNCTION = 33;
const uint16_t SPV_INSTRUCTION_OP_FUNCTION_PARAMETER = 55;
const uint16_t SPV_INSTRUCTION_OP_FUNCTION_CALL = 57;
const uint16_t SPV_INSTRUCTION_OP_VARIABLE = 59;
const uint16_t SPV_INSTRUCTION_OP_LOAD = 61;
const uint16_t SPV_INSTRUCTION_OP_DECORATE = 71;
const uint16_t SPV_INSTRUCTION_OP_SAMPLED_IMAGE = 86;

uint32_t encode_word(uint16_t hiword, uint16_t loword) {
	return (hiword << 16) | loword;
}

const uint32_t SPV_STORAGE_CLASS_UNIFORM_CONSTANT = 0;
const uint32_t SPV_DECORATION_BINDING = 33;
const uint32_t SPV_DECORATION_DESCRIPTOR_SET = 34;

struct InstructionInsert {
	uint32_t previous_spv_idx;
	Vector<uint32_t> instruction;
};

struct WordInsert {
	uint32_t idx;
	uint32_t word;
	uint32_t head_idx;
};

template <typename T>
using Optional = Pair<T, bool>;

inline uint16_t hiword(uint32_t val) {
	return static_cast<uint16_t>(val >> 16);
}

inline uint16_t loword(uint32_t val) {
	return static_cast<uint16_t>(val & 0xFFFF);
}

Vector<uint32_t> combimgsampsplitter(const Vector<uint32_t> &in_spv) {
	uint32_t instruction_bound = in_spv[SPV_HEADER_INSTRUCTION_BOUND_OFFSET];
	uint32_t magic_number = in_spv[SPV_HEADER_MAGIC_NUM_OFFSET];

	Vector<uint32_t> spv_header;
	for (uint32_t i = 0; i < SPV_HEADER_LENGTH; i++) {
		spv_header.push_back(in_spv[i]);
	}

	Vector<uint32_t> spv;
	for (uint32_t i = SPV_HEADER_LENGTH; i < in_spv.size(); i++) {
		spv.push_back(in_spv[i]);
	}

	ERR_FAIL_COND_V(magic_number != SPV_HEADER_MAGIC, Vector<uint32_t>());

	Vector<InstructionInsert> instruction_inserts;
	Vector<WordInsert> word_inserts;
	Vector<uint32_t> new_spv = spv;

	Optional<uint32_t> op_type_sampler_idx;
	Optional<uint32_t> first_op_type_image_idx;
	Optional<uint32_t> first_op_decorate_idx;
	Optional<uint32_t> first_op_type_void_idx;

	Vector<uint32_t> op_type_sampled_image_idxs;
	Vector<uint32_t> op_type_pointer_idxs;
	Vector<uint32_t> op_variable_idxs;
	Vector<uint32_t> op_loads_idxs;
	Vector<uint32_t> op_decorate_idxs;
	Vector<uint32_t> op_type_function_idxs;
	Vector<uint32_t> op_function_parameter_idxs;
	Vector<uint32_t> op_function_call_idxs;

	// 1. Find locations of the instructions we need
	uint32_t spv_idx = 0;
	while (spv_idx < spv.size()) {
		uint32_t op = spv[spv_idx];
		uint16_t word_count = hiword(op);
		uint16_t instruction = loword(op);

		switch (instruction) {
			case SPV_INSTRUCTION_OP_TYPE_VOID: {
				first_op_type_void_idx.first = spv_idx;
				first_op_type_void_idx.second = true;
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_SAMPLER: {
				op_type_sampler_idx.first = spv_idx;
				op_type_sampler_idx.second = true;
				new_spv.write[spv_idx] = encode_word(word_count, SPV_INSTRUCTION_OP_NOP);
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_IMAGE: {
				if (!first_op_type_image_idx.second) {
					first_op_type_image_idx.first = spv_idx;
					first_op_type_image_idx.second = true;
				}
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_SAMPLED_IMAGE: {
				op_type_sampled_image_idxs.push_back(spv_idx);
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_POINTER: {
				if (spv[spv_idx + 2] == SPV_STORAGE_CLASS_UNIFORM_CONSTANT) {
					op_type_pointer_idxs.push_back(spv_idx);
				}
				break;
			}
			case SPV_INSTRUCTION_OP_VARIABLE: {
				op_variable_idxs.push_back(spv_idx);
				break;
			}
			case SPV_INSTRUCTION_OP_LOAD: {
				op_loads_idxs.push_back(spv_idx);
				break;
			}
			case SPV_INSTRUCTION_OP_DECORATE: {
				op_decorate_idxs.push_back(spv_idx);
				if (!first_op_decorate_idx.second) {
					first_op_decorate_idx.first = spv_idx;
					first_op_decorate_idx.second = true;
				}
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_FUNCTION: {
				op_type_function_idxs.push_back(spv_idx);
				break;
			}
			case SPV_INSTRUCTION_OP_FUNCTION_PARAMETER: {
				op_function_parameter_idxs.push_back(spv_idx);
				break;
			}
			case SPV_INSTRUCTION_OP_FUNCTION_CALL: {
				op_function_call_idxs.push_back(spv_idx);
				break;
			}
			default:
				break;
		}

		spv_idx += word_count;
	}

	// 2. Insert OpTypeSampler and respective OpTypePointer if necessary

	// - If there has been no OpTypeImage, there will be nothing to do
	if (!first_op_type_image_idx.second) {
		return in_spv;
	}

	// Let's avoid trouble and just insert after OpTypeVoid
	/* uint32_t op_type_image_idx = first_op_type_image_idx.first; */
	uint32_t op_type_void_idx = first_op_type_void_idx.first;

	uint32_t op_type_sampler_res_id;
	if (op_type_sampler_idx.second) {
		op_type_sampler_res_id = spv[op_type_sampler_idx.first + 1];
	} else {
		op_type_sampler_res_id = instruction_bound;
		instruction_bound += 1;
	}

	uint32_t op_type_pointer_sampler_res_id = instruction_bound;
	instruction_bound += 1;

	instruction_inserts.push_back(
			// Let's avoid trouble and just create the OpTypeSampler after OpTypeVoid
			/* { op_type_image_idx, */
			{ op_type_void_idx,
					{ encode_word(
							  2, SPV_INSTRUCTION_OP_TYPE_SAMPLER),
							op_type_sampler_res_id,
							encode_word(
									4, SPV_INSTRUCTION_OP_TYPE_POINTER),
							op_type_pointer_sampler_res_id, SPV_STORAGE_CLASS_UNIFORM_CONSTANT,
							op_type_sampler_res_id } });

	// 3. OpTypePointer
	Vector<Pair<uint32_t, uint32_t>> type_pointer_res_ids;

	for (uint32_t i = 0; i < op_type_pointer_idxs.size(); i++) {
		uint32_t tp_idx = op_type_pointer_idxs[i];

		for (uint32_t j = 0; j < op_type_sampled_image_idxs.size(); ++j) {
			uint32_t ts_idx = op_type_sampled_image_idxs[j];

			// - Find OpTypePointers that ref OpTypeSampledImage
			if (spv[tp_idx + 3] == spv[ts_idx + 1]) {
				uint32_t underlying_image_id = spv[ts_idx + 2];

				// - Change combined image sampler type to underlying image type
				new_spv.write[tp_idx + 3] = underlying_image_id;

				// - Save the OpTypePointer res id for later
				type_pointer_res_ids.push_back({ spv[tp_idx + 1], underlying_image_id });

				break;
			}
		}
	}

	// 4. OpVariable
	Vector<Pair<Pair<uint32_t, uint32_t>, uint32_t>>
			variable_res_ids;

	for (uint32_t i = 0; i < op_variable_idxs.size(); i++) {
		uint32_t v_idx = op_variable_idxs[i];

		for (uint32_t j = 0; j < type_pointer_res_ids.size(); ++j) {
			uint32_t tp_res_id = type_pointer_res_ids[j].first;
			uint32_t underlying_image_id = type_pointer_res_ids[j].second;

			// - Find all OpVariables that ref our typePointerResIds
			if (tp_res_id == spv[v_idx + 1]) {
				// - Inject OpVariable for new sampler
				uint32_t v_res_id = spv[v_idx + 2];
				uint32_t sampler_op_variable_res_id = instruction_bound;
				instruction_bound += 1;

				instruction_inserts.push_back(
						{ v_idx,
								{ encode_word(
										  4, SPV_INSTRUCTION_OP_VARIABLE),
										op_type_pointer_sampler_res_id, sampler_op_variable_res_id,
										SPV_STORAGE_CLASS_UNIFORM_CONSTANT } });

				// - Save the OpVariable res id for later
				variable_res_ids.push_back(Pair(
						Pair(v_res_id, sampler_op_variable_res_id), underlying_image_id));

				break;
			}
		}
	}

	// 5. OpDecorate
	HashMap<uint32_t,
			Pair<Optional<Pair<uint32_t, uint32_t>>,
					Optional<Pair<uint32_t, uint32_t>>>>
			sampler_id_to_decorations;
	HashSet<uint32_t> descriptor_sets_to_correct;

	// - Find the current binding and descriptor set pair for each combimgsamp
	for (uint32_t i = 0; i < op_decorate_idxs.size(); i++) {
		uint32_t d_idx = op_decorate_idxs[i];

		for (const auto &[v_res_id_and_sampler_v_res_id, _] : variable_res_ids) {
			uint32_t v_res_id = v_res_id_and_sampler_v_res_id.first;
			uint32_t sampler_v_res_id = v_res_id_and_sampler_v_res_id.second;
			if (v_res_id == spv[d_idx + 1]) {
				if (spv[d_idx + 2] == SPV_DECORATION_BINDING) {
					auto &decorations = sampler_id_to_decorations[sampler_v_res_id];
					decorations.first = Pair(Pair(d_idx, spv[d_idx + 3]), true);
				} else if (spv[d_idx + 2] == SPV_DECORATION_DESCRIPTOR_SET) {
					auto &decorations = sampler_id_to_decorations[sampler_v_res_id];
					decorations.second = Pair(Pair(d_idx, spv[d_idx + 3]), true);

					descriptor_sets_to_correct.insert(spv[d_idx + 3]);
				}
			}
		}
	}

	Vector<Pair<uint32_t,
			Pair<Optional<Pair<uint32_t, uint32_t>>,
					Optional<Pair<uint32_t, uint32_t>>>>>
			sampler_id_to_decorations_vec;

	for (const auto &[first, second] : sampler_id_to_decorations) {
		sampler_id_to_decorations_vec.push_back(Pair(first, second));
	}

	struct SamplerIdToDecorationsSort {
		bool operator()(const Pair<uint32_t, Pair<Optional<Pair<uint32_t, uint32_t>>, Optional<Pair<uint32_t, uint32_t>>>> &a, const Pair<uint32_t, Pair<Optional<Pair<uint32_t, uint32_t>>, Optional<Pair<uint32_t, uint32_t>>>> &b) const {
			return (a.second.first.second < b.second.first.second);
		}
	};

	sampler_id_to_decorations_vec.sort_custom<SamplerIdToDecorationsSort>();

	HashMap<uint32_t, Pair<Pair<uint32_t, uint32_t>, Pair<uint32_t, uint32_t>>>
			sampler_id_to_decorations_final;
	for (const auto &[sampler_id, maybe_binding_and_maybe_descriptor_set] :
			sampler_id_to_decorations_vec) {
		auto [maybe_binding, maybe_descriptor_set] =
				maybe_binding_and_maybe_descriptor_set;

		ERR_FAIL_COND_V(!maybe_binding.second, Vector<uint32_t>());
		ERR_FAIL_COND_V(!maybe_descriptor_set.second, Vector<uint32_t>());
		ERR_FAIL_COND_V(!first_op_decorate_idx.second, Vector<uint32_t>());

		auto [binding_idx, binding] = maybe_binding.first;
		auto [descriptor_set_idx, descriptor_set] = maybe_descriptor_set.first;

		sampler_id_to_decorations_final[sampler_id] = {
			{ binding_idx, binding }, { descriptor_set_idx, descriptor_set }
		};
	}

	// - Insert new descriptor set and binding for new sampler
	for (const auto &[sampler_v_res_id, binding_descriptor_set_pair] :
			sampler_id_to_decorations_final) {
		const auto &[binding_pair, descriptor_set_pair] =
				binding_descriptor_set_pair;
		auto [binding_idx, binding] = binding_pair;
		auto [descriptor_set_idx, descriptor_set] = descriptor_set_pair;

		// NOTE: If bindings are not ordered reasonably in spv, the original
		// implementation may fail.
		// Example:
		//      %u_other = (0, 1)
		//      %u_combined = (0, 0)
		//      %inserted_sampler = (0, 0)
		// becomes
		//      %u_other = (0, 1)
		//      %u_combined = (0, 0)
		//      %inserted_sampler = (0, 2)
		// previous_spv_idx: descriptor_set_idx.max(binding_idx),
		instruction_inserts.push_back(
				{ first_op_decorate_idx
								.first,
						{ encode_word(4, SPV_INSTRUCTION_OP_DECORATE), sampler_v_res_id,
								SPV_DECORATION_DESCRIPTOR_SET, descriptor_set,
								encode_word(4, SPV_INSTRUCTION_OP_DECORATE), sampler_v_res_id,
								SPV_DECORATION_BINDING,
								binding + 1 } });
	}

	// 6. OpTypeFunction
	for (uint32_t i = 0; i < op_type_function_idxs.size(); i++) {
		uint32_t tf_idx = op_type_function_idxs[i];

		// - Append a sampler OpTypePointer to OpTypeFunctions when an underlying image OpTypePointer
		// is found.
		for (const auto &[image_type_pointer, _] : type_pointer_res_ids) {
			uint16_t word_count = hiword(spv[tf_idx]);

			for (uint32_t j = 0; j < word_count - 3; ++j) {
				uint32_t ty_idx = tf_idx + 3 + j;

				if (spv[ty_idx] == image_type_pointer) {
					word_inserts.push_back({ ty_idx,
							op_type_pointer_sampler_res_id,
							tf_idx });
				}
			}
		}
	}

	// 7. OpFunctionParameter
	HashMap<uint32_t, Pair<uint32_t, uint32_t>> parameter_res_ids;

	for (uint32_t i = 0; i < op_function_parameter_idxs.size(); i++) {
		uint32_t fp_idx = op_function_parameter_idxs[i];

		for (const auto &[image_type_pointer, underlying_image_id] : type_pointer_res_ids) {
			// - Find all OpFunctionParameters that use an underlying image OpTypePointer
			if (spv[fp_idx + 1] == image_type_pointer) {
				uint32_t image_res_Id = spv[fp_idx + 2];

				uint32_t sampler_parameter_res_id = instruction_bound;
				instruction_bound += 1;

				instruction_inserts.push_back(
						{ fp_idx,
								{ encode_word(
										  3,
										  SPV_INSTRUCTION_OP_FUNCTION_PARAMETER),
										op_type_pointer_sampler_res_id, sampler_parameter_res_id } });

				parameter_res_ids[image_res_Id] = { sampler_parameter_res_id,
					underlying_image_id };
				break;
			}
		}
	}

	// 8. OpLoad
	for (uint32_t i = 0; i < op_loads_idxs.size(); i++) {
		uint32_t l_idx = op_loads_idxs[i];

		for (uint32_t j = 0; j < variable_res_ids.size(); ++j) {
			uint32_t v_res_id = variable_res_ids[j].first.first;
			uint32_t sampler_v_res_id = variable_res_ids[j].first.second;
			uint32_t underlying_image_id = variable_res_ids[j].second;

			// - Find all OpLoads that reference variableResIds
			if (v_res_id == spv[l_idx + 3]) {
				// - Insert OpLoads and OpSampledImage to replace combimgsamp
				uint32_t image_op_load_res_id = instruction_bound;
				instruction_bound += 1;

				uint32_t image_original_res_id = spv[l_idx + 2];
				uint32_t original_combined_res_id = new_spv[l_idx + 1];

				new_spv.write[l_idx + 1] = underlying_image_id;
				new_spv.write[l_idx + 2] = image_op_load_res_id;

				uint32_t sampler_op_load_res_id = instruction_bound;
				instruction_bound += 1;

				instruction_inserts.push_back(
						{ l_idx,
								{ encode_word(4, SPV_INSTRUCTION_OP_LOAD),
										op_type_sampler_res_id, sampler_op_load_res_id, sampler_v_res_id,

										encode_word(5, SPV_INSTRUCTION_OP_SAMPLED_IMAGE),
										original_combined_res_id, image_original_res_id,
										image_op_load_res_id, sampler_op_load_res_id } });

				break;
			}
		}
	}

	// TODO: This code is not very DRY!
	for (uint32_t i = 0; i < op_loads_idxs.size(); i++) {
		uint32_t l_idx = op_loads_idxs[i];

		for (const auto &[image_res_id, sampler_data] : parameter_res_ids) {
			uint32_t sampler_parameter_res_id = sampler_data.first;
			uint32_t underlying_image_id = sampler_data.second;

			if (spv[l_idx + 3] == image_res_id) {
				uint32_t image_op_load_res_id = instruction_bound;
				instruction_bound += 1;

				uint32_t image_original_res_id = spv[l_idx + 2];
				uint32_t original_combined_res_id = new_spv[l_idx + 1];

				new_spv.write[l_idx + 1] = underlying_image_id;
				new_spv.write[l_idx + 2] = image_op_load_res_id;

				uint32_t sampler_op_load_res_id = instruction_bound;
				instruction_bound += 1;

				instruction_inserts.push_back(
						{ l_idx,
								{ encode_word(4, SPV_INSTRUCTION_OP_LOAD),
										op_type_sampler_res_id, sampler_op_load_res_id,
										sampler_parameter_res_id,

										encode_word(5, SPV_INSTRUCTION_OP_SAMPLED_IMAGE),
										original_combined_res_id, image_original_res_id,
										image_op_load_res_id, sampler_op_load_res_id } });

				break;
			}
		}
	}

	// 9. OpFunctionCall
	for (uint32_t fc_idx : op_function_call_idxs) {
		Vector<Pair<uint32_t, uint32_t>> combined_res_ids;

		// - Handle use of nested function calls
		for (const auto &[image_res_id, pair] : parameter_res_ids) {
			combined_res_ids.push_back({ image_res_id, pair.first });
		}

		// - Handle use of uniform variables
		for (const auto &[image_id_and_sampler_id, _] : variable_res_ids) {
			combined_res_ids.push_back({ image_id_and_sampler_id.first, image_id_and_sampler_id.second });
		}

		for (const auto &[image_id, sampler_id] : combined_res_ids) {
			uint16_t word_count = hiword(spv[fc_idx]);

			for (uint32_t i = 0; i < word_count - 4; i++) {
				uint32_t param = spv[fc_idx + 4 + i];

				if (param == image_id) {
					word_inserts.push_back(WordInsert{
							.idx = fc_idx + 4 + i,
							.word = sampler_id,
							.head_idx = fc_idx });
				}
			}
		}
	}

	// 10. Insert New Instructions
	enum class InsertType { Word,
		Instruction };

	struct Insert {
		InsertType type;
		union {
			const WordInsert *word;
			const InstructionInsert *instruction;
		};
		const uint32_t *spv;
	};

	Vector<Insert> inserts;
	for (const auto &word_insert : word_inserts) {
		inserts.push_back((Insert){
				.type = InsertType::Word,
				.word = &word_insert,
				.spv = spv.ptr(),
		});
	}
	for (const auto &instruction_insert : instruction_inserts) {
		inserts.push_back((Insert){
				.type = InsertType::Instruction,
				.instruction = &instruction_insert,
				.spv = spv.ptr(),
		});
	}

	struct InsertsSort {
		bool operator()(const Insert &a, const Insert &b) const {
			if (a.type == InsertType::Word && b.type == InsertType::Word) {
				return a.word->idx < b.word->idx;
			} else if (a.type == InsertType::Instruction &&
					b.type == InsertType::Instruction) {
				return a.instruction->previous_spv_idx <
						b.instruction->previous_spv_idx;
			} else if (a.type == InsertType::Word) {
				return a.word->idx < b.instruction->previous_spv_idx;
			} else {
				return a.instruction->previous_spv_idx < b.word->idx;
			}
		}
	};

	inserts.sort_custom<InsertsSort>();
	inserts.reverse();

	for (const auto &it : inserts) {
		if (it.type == InsertType::Word) {
			const WordInsert &new_word = *it.word;
			new_spv.insert(new_word.idx + 1, new_word.word);

			uint16_t current_hiword = hiword(new_spv[new_word.head_idx]);
			new_spv.write[new_word.head_idx] =
					encode_word(current_hiword + 1, loword(new_spv[new_word.head_idx]));
		} else if (it.type == InsertType::Instruction) {
			const InstructionInsert &new_instruction = *it.instruction;
			uint16_t offset = hiword(spv[new_instruction.previous_spv_idx]);

			for (uint32_t i = 0; i < new_instruction.instruction.size(); i++) {
				new_spv.insert(new_instruction.previous_spv_idx +
								offset + i,
						new_instruction.instruction[i]);
			}
		}
	}

	// 11. Correct OpDecorate Bindings
	HashMap<uint32_t,
			Pair<Optional<uint32_t>,
					Optional<Pair<uint32_t, uint32_t>>>>
			candidates;

	uint32_t d_idx = 0;
	while (d_idx < new_spv.size()) {
		uint32_t op = new_spv[d_idx];
		uint16_t word_count = hiword(op);
		uint16_t instruction = loword(op);

		if (instruction == SPV_INSTRUCTION_OP_DECORATE) {
			switch (new_spv[d_idx + 2]) {
				case SPV_DECORATION_DESCRIPTOR_SET: {
					candidates[new_spv[d_idx + 1]].first.first = new_spv[d_idx + 3];
					candidates[new_spv[d_idx + 1]].first.second = true;
					break;
				}
				case SPV_DECORATION_BINDING: {
					candidates[new_spv[d_idx + 1]].second.first =
							Pair(d_idx, new_spv[d_idx + 3]);
					candidates[new_spv[d_idx + 1]].second.second =
							true;
					break;
				}
				default:
					break;
			}
		}
		d_idx += word_count;
	}

	for (const auto &descriptor_set : descriptor_sets_to_correct) {
		Vector<Pair<uint32_t, uint32_t>> bindings;

		for (const auto &[_, descriptor_data] : candidates) {
			auto &[maybe_descriptor_set, maybe_binding] = descriptor_data;
			if (maybe_descriptor_set.second && maybe_binding.second) {
				uint32_t this_descriptor_set = maybe_descriptor_set.first;
				auto [binding_idx, this_binding] = maybe_binding.first;

				if (this_descriptor_set == descriptor_set) {
					bindings.push_back(Pair(binding_idx, this_binding));
				}
			}
		}

		struct BindingsSort {
			bool operator()(const Pair<uint32_t, uint32_t> &a, const Pair<uint32_t, uint32_t> &b) const {
				return a.second == b.second ? a.first < b.second : a.second < b.second;
			}
		};

		bindings.sort_custom<BindingsSort>();

		int32_t prev_binding = -1;
		int32_t prev_id = -1;
		int32_t prev_d_idx = -1;
		int32_t increment = 0;

		for (auto &[d_idx, binding] : bindings) {
			uint32_t this_id = new_spv[d_idx + 1];

			if (binding == static_cast<uint32_t>(prev_binding)) {
				increment += 1;

				if (prev_id <= (int32_t)this_id) {
					new_spv[static_cast<uint32_t>(prev_d_idx) + 3];
					new_spv.write[d_idx + 3] -= 1;
				}
			}
			new_spv.write[d_idx + 3] += increment;
			prev_binding = static_cast<int32_t>(binding);
			prev_id = static_cast<int32_t>(this_id);
			prev_d_idx = static_cast<int32_t>(d_idx);
		}
	}

	// 12. Remove the original optypesampler

	uint32_t i_idx = 0;
	while (i_idx < new_spv.size()) {
		uint32_t op = new_spv[i_idx];
		uint16_t word_count = hiword(op);
		uint16_t instruction = loword(op);

		if (instruction == SPV_INSTRUCTION_OP_NOP) {
			for (uint32_t i = 0; i < word_count; i++) {
				new_spv.remove_at(i_idx);
			}
		} else {
			i_idx += word_count;
		}
	}

	// 13. Write New Header and New Code
	spv_header.write[SPV_HEADER_INSTRUCTION_BOUND_OFFSET] = instruction_bound;

	Vector<uint32_t> out_spv = spv_header;
	out_spv.append_array(new_spv);

	return out_spv;
}
