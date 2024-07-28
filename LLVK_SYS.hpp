//
// Created by liuya on 7/28/2024.
//

#pragma once



#define LLVK_NAMESPACE_BEGIN namespace LLVK{
#define LLVK_NAMESPACE_END }

LLVK_NAMESPACE_BEGIN
constexpr bool ALWAYS_TRUE = true;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;


// concept
namespace Concept {
    template<typename T>
    concept is_range = requires(T var){
        var.size();
        var.data();
    };

    template<typename T>
    concept has_cleanFn = requires(T var){
        var.cleanup();
    };



}

LLVK_NAMESPACE_END