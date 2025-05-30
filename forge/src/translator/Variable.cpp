﻿/**
 * Variable.cpp:
 *      @Author         :   Margoo(qiuzhengyu@acm.org)
 *      @Date           :   5/4/2025
 */

#include <include/translator/Variable.h>

#include <include/TemplateManager.h>

#include <inja/inja.hpp>

std::string VariableTranslator::GlobalVariableTranslate(llvm::GlobalVariable &Variable,
                                                        const std::string &Initializer,
                                                        Context &Contxt) {
    inja::json data;

    auto name = Variable.getName().str();
    if (!Variable.hasName()) {
        name = LLVMAnonyExtractor::Extract(&Variable, Contxt);
    }

    auto initializer = Initializer;
    // Process the array type, we do not
    if (auto *arrType = llvm::dyn_cast<llvm::ArrayType>(Variable.getValueType())) {
        do {
            // We just skip the string initializer or the array that already has initializer
            if (Initializer[0] == '\"' || !initializer.empty()) {
                break;
            }

            auto array = ArrayExpand(arrType);
            initializer = ArrayZeroInitExpand(array.first);
        } while (false);
    }

    name = "v_" + LLVMNameLegalizer::Legalize(name);

    data["name"] = name;
    data["hasInitializer"] = !initializer.empty();
    data["initializer"] = initializer;
    data["isGlobal"] = true;

    return TemplateManager::Instance().RenderFile(TemplateManager::Variable, data);
}

std::pair<std::vector<int>, llvm::Type *> VariableTranslator::ArrayExpand(
    llvm::ArrayType *ArrType, std::vector<int> Index) {
    auto elem = ArrType->getElementType();
    Index.emplace_back(ArrType->getNumElements());
    if (auto *arrType = llvm::dyn_cast<llvm::ArrayType>(elem)) {
        return ArrayExpand(arrType, Index);
    }

    return {Index, elem};
}

std::string VariableTranslator::ArrayZeroInitExpand(std::vector<int> Index) {
    if (Index.empty()) {
        return "0";
    }

    inja::json arrayData;
    auto size = Index[0];
    arrayData["num"] = size;

    Index.pop_back();
    for (auto count = 0; count < size; ++count) {
        arrayData["elements"].push_back(ArrayZeroInitExpand(Index));
    }

    return TemplateManager::Instance().RenderFile(TemplateManager::Array, arrayData);
}
