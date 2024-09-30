#include "combimgsamplersplitter.h"

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/pair.h"

const uint32_t SPV_HEADER_LENGTH = 5;
const uint32_t SPV_HEADER_MAGIC = 0x07230203;
const uint32_t SPV_HEADER_MAGIC_NUM_OFFSET = 0;
const uint32_t SPV_HEADER_INSTRUCTION_BOUND_OFFSET = 3;

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

uint32_t encodeWord(uint16_t hiword, uint16_t loword) {
	return (hiword << 16) | loword;
}

const uint32_t SPV_NOP_WORD = encodeWord(1, 0);

const uint32_t SPV_STORAGE_CLASS_UNIFORM_CONSTANT = 0;
const uint32_t SPV_DECORATION_BINDING = 33;
const uint32_t SPV_DECORATION_DESCRIPTOR_SET = 34;

struct InstructionInsert {
	uint32_t previousSpvIdx;
	Vector<uint32_t> instruction;
};

struct WordInsert {
	uint32_t idx;
	uint32_t word;
	uint32_t headIdx;
};

template <typename T>
using Optional = Pair<T, bool>;

inline uint16_t hiword(uint32_t val) {
	return static_cast<uint16_t>(val >> 16);
}

inline uint16_t loword(uint32_t val) {
	return static_cast<uint16_t>(val & 0xFFFF);
}

Vector<uint32_t> combimgsampsplitter(const Vector<uint32_t> &inSpv) {
	uint32_t instructionBound = inSpv[SPV_HEADER_INSTRUCTION_BOUND_OFFSET];
	uint32_t magicNumber = inSpv[SPV_HEADER_MAGIC_NUM_OFFSET];

	Vector<uint32_t> spvHeader;
	for (int i = 0; i < SPV_HEADER_LENGTH; i++) {
		spvHeader.push_back(inSpv[i]);
	}

	Vector<uint32_t> spv;
	for (int i = SPV_HEADER_LENGTH; i < inSpv.size(); i++) {
		spv.push_back(inSpv[i]);
	}

	ERR_FAIL_COND_V(magicNumber != SPV_HEADER_MAGIC, Vector<uint32_t>());

	Vector<InstructionInsert> instructionInserts;
	Vector<WordInsert> wordInserts;
	Vector<uint32_t> newSpv = spv;

	Optional<uint32_t> opTypeSamplerIdx;
	Optional<uint32_t> firstOpTypeImageIdx;
	Optional<uint32_t> firstOpDecorateIdx;

	Vector<uint32_t> opTypeSampledImageIdxs;
	Vector<uint32_t> opTypePointerIdxs;
	Vector<uint32_t> opVariableIdxs;
	Vector<uint32_t> opLoadIdxs;
	Vector<uint32_t> opDecorateIdxs;
	Vector<uint32_t> opTypeFunctionIdxs;
	Vector<uint32_t> opFunctionParamterIdxs;
	Vector<uint32_t> opFunctionCallIdxs;

	// 1. Find locations of the instructions we need
	uint32_t spvIdx = 0;
	while (spvIdx < spv.size()) {
		uint32_t op = spv[spvIdx];
		uint16_t wordCound = hiword(op);
		uint16_t instruction = loword(op);

		switch (instruction) {
			case SPV_INSTRUCTION_OP_TYPE_SAMPLER: {
				opTypeSamplerIdx.first = spvIdx;
				opTypeSamplerIdx.second = true;
				for (int i = spvIdx; i < spvIdx + wordCound; i++) {
					newSpv.set(i, SPV_NOP_WORD);
				}
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_IMAGE: {
				if (!firstOpTypeImageIdx.second) {
					firstOpTypeImageIdx.first = spvIdx;
					firstOpTypeImageIdx.second = true;
				}
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_SAMPLED_IMAGE: {
				opTypeSampledImageIdxs.push_back(spvIdx);
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_POINTER: {
				if (spv[spvIdx + 2] == SPV_STORAGE_CLASS_UNIFORM_CONSTANT) {
					opTypePointerIdxs.push_back(spvIdx);
				}
				break;
			}
			case SPV_INSTRUCTION_OP_VARIABLE: {
				opVariableIdxs.push_back(spvIdx);
				break;
			}
			case SPV_INSTRUCTION_OP_LOAD: {
				opLoadIdxs.push_back(spvIdx);
				break;
			}
			case SPV_INSTRUCTION_OP_DECORATE: {
				opDecorateIdxs.push_back(spvIdx);
				if (!firstOpDecorateIdx.second) {
					firstOpDecorateIdx.first = spvIdx;
					firstOpDecorateIdx.second = true;
				}
				break;
			}
			case SPV_INSTRUCTION_OP_TYPE_FUNCTION: {
				opTypeFunctionIdxs.push_back(spvIdx);
				break;
			}
			case SPV_INSTRUCTION_OP_FUNCTION_PARAMETER: {
				opFunctionParamterIdxs.push_back(spvIdx);
				break;
			}
			case SPV_INSTRUCTION_OP_FUNCTION_CALL: {
				opFunctionCallIdxs.push_back(spvIdx);
				break;
			}
			default:
				break;
		}

		spvIdx += wordCound;
	}

	// 2. Insert OpTypeSampler and respective OpTypePointer if necessary

	ERR_FAIL_COND_V(!firstOpTypeImageIdx.second, Vector<uint32_t>());
	uint32_t opTypeImageIdx = firstOpTypeImageIdx.first;

	uint32_t opTypeSamplerResId;
	if (opTypeSamplerIdx.second) {
		opTypeSamplerResId = spv[opTypeSamplerIdx.first + 1];
	} else {
		opTypeSamplerResId = instructionBound;
		instructionBound += 1;
	}

	uint32_t opTypePointerSamplerResId = instructionBound;
	instructionBound += 1;

	instructionInserts.push_back(
			{ opTypeImageIdx,
					{ encodeWord(
							  2, SPV_INSTRUCTION_OP_TYPE_SAMPLER),
							opTypeSamplerResId,
							encodeWord(
									4, SPV_INSTRUCTION_OP_TYPE_POINTER),
							opTypePointerSamplerResId, SPV_STORAGE_CLASS_UNIFORM_CONSTANT,
							opTypeSamplerResId } });

	// 3. OpTypePointer
	Vector<Pair<uint32_t, uint32_t>> typePointerResIds;

	for (int i = 0; i < opTypePointerIdxs.size(); i++) {
		uint32_t tpIdx = opTypePointerIdxs[i];

		for (int j = 0; j < opTypeSampledImageIdxs.size(); ++j) {
			uint32_t tsIdx = opTypeSampledImageIdxs[j];

			// - Find OpTypePointers that ref OpTypeSampledImage
			if (spv[tpIdx + 3] == spv[tsIdx + 1]) {
				// - Inject OpTypePointer for sampler pair

				uint32_t underlyingImageId = spv[tsIdx + 2];
				uint32_t opTypePointerResId = instructionBound;
				instructionBound += 1;
				instructionInserts.push_back(
						{ tpIdx,
								{ encodeWord(
										  4,
										  SPV_INSTRUCTION_OP_TYPE_POINTER),
										opTypePointerResId, SPV_STORAGE_CLASS_UNIFORM_CONSTANT,
										underlyingImageId } });

				// - Change combined image sampler type to underlying image type
				newSpv.set(tpIdx + 3, underlyingImageId);

				// - Save the OpTypePointer res id for later
				typePointerResIds.push_back({ spv[tpIdx + 1], underlyingImageId });

				break;
			}
		}
	}

	// 4. OpVariable
	Vector<Pair<Pair<uint32_t, uint32_t>, uint32_t>>
			variableResIds;

	for (int i = 0; i < opVariableIdxs.size(); i++) {
		uint32_t vIdx = opVariableIdxs[i];

		for (int j = 0; j < typePointerResIds.size(); ++j) {
			uint32_t tpResId = typePointerResIds[j].first;
			uint32_t underlyingImageId = typePointerResIds[j].second;

			// - Find all OpVariables that ref our typePointerResIds
			if (tpResId == spv[vIdx + 1]) {
				// - Inject OpVariable for new sampler
				uint32_t vResId = spv[vIdx + 2];
				uint32_t samplerOpVariableResId = instructionBound;
				instructionBound += 1;

				instructionInserts.push_back(
						{ vIdx,
								{ encodeWord(
										  4, SPV_INSTRUCTION_OP_VARIABLE),
										opTypePointerSamplerResId, samplerOpVariableResId,
										SPV_STORAGE_CLASS_UNIFORM_CONSTANT } });

				// - Save the OpVariable res id for later
				variableResIds.push_back(Pair(
						Pair(vResId, samplerOpVariableResId), underlyingImageId));

				break;
			}
		}
	}

	// 5. OpLoad
	for (int i = 0; i < opLoadIdxs.size(); i++) {
		uint32_t lIdx = opLoadIdxs[i];

		for (int j = 0; j < variableResIds.size(); ++j) {
			uint32_t vResId = variableResIds[j].first.first;
			uint32_t samplerVResId = variableResIds[j].first.second;
			uint32_t underlyingImageId = variableResIds[j].second;

			// - Find all OpLoads that reference variableResIds
			if (vResId == spv[lIdx + 3]) {
				// - Insert OpLoads and OpSampledImage to replace combimgsamp
				uint32_t imageOpLoadResId = instructionBound;
				instructionBound += 1;

				uint32_t imageOriginalResId = spv[lIdx + 2];
				uint32_t originalCombinedResId = newSpv[lIdx + 1];

				newSpv.set(lIdx + 1, underlyingImageId);
				newSpv.set(lIdx + 2, imageOpLoadResId);

				uint32_t samplerOpLoadResId = instructionBound;
				instructionBound += 1;

				instructionInserts.push_back(
						{ lIdx,
								{ encodeWord(4, SPV_INSTRUCTION_OP_LOAD),
										opTypeSamplerResId, samplerOpLoadResId, samplerVResId,

										encodeWord(5, SPV_INSTRUCTION_OP_SAMPLED_IMAGE),
										originalCombinedResId, imageOriginalResId,
										imageOpLoadResId, samplerOpLoadResId } });

				break;
			}
		}
	}

	// 6. OpDecorate
	HashMap<uint32_t,
			Pair<Optional<Pair<uint32_t, uint32_t>>,
					Optional<Pair<uint32_t, uint32_t>>>>
			samplerIdToDecorations;
	HashSet<uint32_t> descriptorSetsToCorrect;

	// - Find the current binding and descriptor set pair for each combimgsamp
	for (int i = 0; i < opDecorateIdxs.size(); i++) {
		uint32_t dIdx = opDecorateIdxs[i];

		for (const auto &[vResIdAndSamplerVResId, _] : variableResIds) {
			uint32_t vResId = vResIdAndSamplerVResId.first;
			uint32_t samplerVResId = vResIdAndSamplerVResId.second;
			if (vResId == spv[dIdx + 1]) {
				if (spv[dIdx + 2] == SPV_DECORATION_BINDING) {
					descriptorSetsToCorrect.insert(spv[dIdx + 3]);

					auto &decorations = samplerIdToDecorations[samplerVResId];
					decorations.first = Pair(Pair(dIdx, spv[dIdx + 3]), true);
				} else if (spv[dIdx + 2] == SPV_DECORATION_DESCRIPTOR_SET) {
					auto &decorations = samplerIdToDecorations[samplerVResId];
					decorations.second = Pair(Pair(dIdx, spv[dIdx + 3]), true);
				}
			}
		}
	}

	Vector<Pair<uint32_t,
			Pair<Optional<Pair<uint32_t, uint32_t>>,
					Optional<Pair<uint32_t, uint32_t>>>>>
			samplerIdToDecorationsVec;

	for (const auto &[first, second] : samplerIdToDecorations) {
		samplerIdToDecorationsVec.push_back(Pair(first, second));
	}

	struct SamplerIdToDecorationsSort {
		bool operator()(const Pair<uint32_t, Pair<Optional<Pair<uint32_t, uint32_t>>, Optional<Pair<uint32_t, uint32_t>>>> &a, const Pair<uint32_t, Pair<Optional<Pair<uint32_t, uint32_t>>, Optional<Pair<uint32_t, uint32_t>>>> &b) const {
			return (a.second.first.second < b.second.first.second);
		}
	};

	samplerIdToDecorationsVec.sort_custom<SamplerIdToDecorationsSort>();

	HashMap<uint32_t, Pair<Pair<uint32_t, uint32_t>, Pair<uint32_t, uint32_t>>>
			samplerIdToDecorationsFinal;
	for (const auto &[samplerId, maybeBindingAndMaybeDescriptorSet] :
			samplerIdToDecorationsVec) {
		auto [maybeBinding, maybeDescriptorSet] =
				maybeBindingAndMaybeDescriptorSet;

		ERR_FAIL_COND_V(!maybeBinding.second, Vector<uint32_t>());
		ERR_FAIL_COND_V(!maybeDescriptorSet.second, Vector<uint32_t>());
		ERR_FAIL_COND_V(!firstOpDecorateIdx.second, Vector<uint32_t>());

		auto [bindingIdx, binding] = maybeBinding.first;
		auto [descriptorSetIdx, descriptorSet] = maybeDescriptorSet.first;

		samplerIdToDecorationsFinal[samplerId] = {
			{ bindingIdx, binding }, { descriptorSetIdx, descriptorSet }
		};
	}

	// - Insert new descriptor set and binding for new sampler
	for (const auto &[samplerVResId, bindingDescriptorSetPair] :
			samplerIdToDecorationsFinal) {
		const auto &[bindingPair, descriptorSetPair] =
				bindingDescriptorSetPair;
		auto [bindingIdx, binding] = bindingPair;
		auto [descriptorSetIdx, descriptorSet] = descriptorSetPair;

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
		instructionInserts.push_back(
				{ firstOpDecorateIdx
								.first,
						{ encodeWord(4, SPV_INSTRUCTION_OP_DECORATE), samplerVResId,
								SPV_DECORATION_DESCRIPTOR_SET, descriptorSet,
								encodeWord(4, SPV_INSTRUCTION_OP_DECORATE), samplerVResId,
								SPV_DECORATION_BINDING,
								binding + 1 } });
	}

	// 7. OpTypeFunction
	for (int i = 0; i < opTypeFunctionIdxs.size(); i++) {
		uint32_t tfIdx = opTypeFunctionIdxs[i];

		// - Append a sampler OpTypePointer to OpTypeFunctions when an underlying image OpTypePointer
		// is found.
		for (const auto &[imageTypePointer, _] : typePointerResIds) {
			uint16_t wordCount = hiword(spv[tfIdx]);

			for (int j = 0; j < wordCount - 3; ++j) {
				uint32_t tyIdx = tfIdx + 3 + j;

				if (spv[tyIdx] == imageTypePointer) {
					wordInserts.push_back({ tyIdx,
							opTypePointerSamplerResId,
							tfIdx });
				}
			}
		}
	}

	// 8. OpFunctionParameter
	HashMap<uint32_t, Pair<uint32_t, uint32_t>> parameterResIds;

	for (int i = 0; i < opFunctionParamterIdxs.size(); i++) {
		uint32_t fpIdx = opFunctionParamterIdxs[i];

		for (const auto &[imageTypePointer, underlyingImageId] : typePointerResIds) {
			// - Find all OpFunctionParameters that use an underlying image OpTypePointer
			if (spv[fpIdx + 1] == imageTypePointer) {
				uint32_t imageResId = spv[fpIdx + 2];

				uint32_t samplerParamterResId = instructionBound;
				instructionBound += 1;

				instructionInserts.push_back(
						{ fpIdx,
								{ encodeWord(
										  3,
										  SPV_INSTRUCTION_OP_FUNCTION_PARAMETER),
										opTypePointerSamplerResId, samplerParamterResId } });

				parameterResIds[imageResId] = { samplerParamterResId,
					underlyingImageId };
				break;
			}
		}
	}

	for (int i = 0; i < opLoadIdxs.size(); i++) {
		uint32_t lIdx = opLoadIdxs[i];

		for (const auto &[image_res_id, sampler_data] : parameterResIds) {
			uint32_t sampler_parameter_res_id = sampler_data.first;
			uint32_t underlying_image_id = sampler_data.second;

			if (spv[lIdx + 3] == image_res_id) {
				uint32_t image_op_load_res_id = instructionBound;
				instructionBound += 1;

				uint32_t image_original_res_id = spv[lIdx + 2];
				uint32_t original_combined_res_id = newSpv[lIdx + 1];

				newSpv.set(lIdx + 1, underlying_image_id);
				newSpv.set(lIdx + 2, image_op_load_res_id);

				uint32_t sampler_op_load_res_id = instructionBound;
				instructionBound += 1;

				instructionInserts.push_back(
						{ lIdx,
								{ encodeWord(4, SPV_INSTRUCTION_OP_LOAD),
										opTypeSamplerResId, sampler_op_load_res_id,
										sampler_parameter_res_id,

										encodeWord(5, SPV_INSTRUCTION_OP_SAMPLED_IMAGE),
										original_combined_res_id, image_original_res_id,
										image_op_load_res_id, sampler_op_load_res_id } });

				break;
			}
		}
	}

	// 9. OpFunctionCall
	for (uint32_t fcIdx : opFunctionCallIdxs) {
		Vector<Pair<uint32_t, uint32_t>> combinedResIds;

		// - Handle use of nested function calls
		for (const auto &[imageResId, pair] : parameterResIds) {
			combinedResIds.push_back({ imageResId, pair.first });
		}

		// - Handle use of uniform variables
		for (const auto &[imageIdAndSamplerId, _] : variableResIds) {
			combinedResIds.push_back({ imageIdAndSamplerId.first, imageIdAndSamplerId.second });
		}

		for (const auto &[imageId, samplerId] : combinedResIds) {
			uint16_t wordCount = hiword(spv[fcIdx]);

			for (uint32_t i = 0; i < wordCount - 4; i++) {
				uint32_t param = spv[fcIdx + 4 + i];

				if (param == imageId) {
					wordInserts.push_back(WordInsert{
							.idx = fcIdx + 4 + i,
							.word = samplerId,
							.headIdx = fcIdx });
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
	for (const auto &wordInsert : wordInserts) {
		inserts.push_back((Insert){
				.type = InsertType::Word,
				.word = &wordInsert,
				.spv = spv.ptr(),
		});
	}
	for (const auto &instructionInsert : instructionInserts) {
		inserts.push_back((Insert){
				.type = InsertType::Instruction,
				.instruction = &instructionInsert,
				.spv = spv.ptr(),
		});
	}

	struct InsertsSort {
		bool operator()(const Insert &a, const Insert &b) const {
			if (a.type == InsertType::Word && b.type == InsertType::Word) {
				return a.word->idx < b.word->idx;
			} else if (a.type == InsertType::Instruction &&
					b.type == InsertType::Instruction) {
				return a.instruction->previousSpvIdx <
						b.instruction->previousSpvIdx;
			} else if (a.type == InsertType::Word) {
				return a.word->idx < b.instruction->previousSpvIdx;
			} else {
				return a.instruction->previousSpvIdx < b.word->idx;
			}
		}
	};

	inserts.sort_custom<InsertsSort>();
	inserts.reverse();

	for (const auto &it : inserts) {
		if (it.type == InsertType::Word) {
			const WordInsert &newWord = *it.word;
			newSpv.insert(newWord.idx + 1, newWord.word);

			uint16_t currentHiword = hiword(newSpv[newWord.headIdx]);
			newSpv.set(newWord.headIdx,
					encodeWord(currentHiword + 1, loword(newSpv[newWord.headIdx])));
		} else if (it.type == InsertType::Instruction) {
			const InstructionInsert &newInstruction = *it.instruction;
			uint16_t offset = hiword(spv[newInstruction.previousSpvIdx]);

			for (int i = 0; i < newInstruction.instruction.size(); i++) {
				newSpv.insert(newInstruction.previousSpvIdx +
								offset + i,
						newInstruction.instruction[i]);
			}
		}
	}

	// 11. Correct OpDecorate Bindings
	HashMap<uint32_t,
			Pair<Optional<uint32_t>,
					Optional<Pair<uint32_t, uint32_t>>>>
			candidates;

	uint32_t dIdx = 0;
	while (dIdx < newSpv.size()) {
		uint32_t op = newSpv[dIdx];
		uint16_t wordCount = hiword(op);
		uint16_t instruction = loword(op);

		if (instruction == SPV_INSTRUCTION_OP_DECORATE) {
			switch (newSpv[dIdx + 2]) {
				case SPV_DECORATION_DESCRIPTOR_SET: {
					candidates[newSpv[dIdx + 1]].first.first = newSpv[dIdx + 3];
					candidates[newSpv[dIdx + 1]].first.second = true;
					break;
				}
				case SPV_DECORATION_BINDING: {
					candidates[newSpv[dIdx + 1]].second.first =
							Pair(dIdx, newSpv[dIdx + 3]);
					candidates[newSpv[dIdx + 1]].second.second =
							true;
					break;
				}
				default:
					break;
			}
		}
		dIdx += wordCount;
	}

	for (const auto &descriptorSet : descriptorSetsToCorrect) {
		Vector<Pair<uint32_t, uint32_t>> bindings;

		for (const auto &[_, descriptorData] : candidates) {
			auto &[maybeDescriptorSet, maybeBinding] = descriptorData;
			if (maybeDescriptorSet.second && maybeBinding.second) {
				uint32_t thisDescriptorSet = maybeDescriptorSet.first;
				auto [bindingIdx, thisBinding] = maybeBinding.first;

				if (thisDescriptorSet == descriptorSet) {
					bindings.push_back(Pair(bindingIdx, thisBinding));
				}
			}
		}

		struct BindingsSort {
			bool operator()(const Pair<uint32_t, uint32_t> &a, const Pair<uint32_t, uint32_t> &b) const {
				return a.second < b.second;
			}
		};

		bindings.sort_custom<BindingsSort>();

		int32_t prevBinding = -1;
		int32_t increment = 0;

		for (auto &[dIdx, binding] : bindings) {
			if (binding == static_cast<uint32_t>(prevBinding)) {
				increment += 1;
			}
			newSpv.set(dIdx + 3, newSpv[dIdx + 3] + increment);
			prevBinding = static_cast<int32_t>(binding);
		}
	}

	// 12. Write New Header and New Code
	spvHeader.set(SPV_HEADER_INSTRUCTION_BOUND_OFFSET, instructionBound);

	Vector<uint32_t> outSpv = spvHeader;

	for (int i = 0; i < newSpv.size(); i++) {
		if (newSpv[i] != SPV_NOP_WORD) {
			outSpv.push_back(newSpv[i]);
		}
	}

	return outSpv;
}
