//
// Created by lp on 2024/7/16.
//

//
// Created by star on 2/13/2022.
//

#pragma once

#include <random>
#include <iostream>
#include <numeric>
#include <numbers>
#include <limits>
#include <cstdlib>

namespace LLVK{
    constexpr double pi = std::numbers::pi;
    constexpr double infinity = std::numeric_limits<double>::infinity();

    constexpr inline double degrees_to_radians(double degrees) {
        return degrees * std::numbers::pi / 180.0;
    }

    constexpr inline double clamp(double x, double min, double max) {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    template<typename T>
    class Singleton
    {
    public:
        Singleton(Singleton const &) = delete;
        Singleton & operator = (Singleton const &)= delete;
        static T& instance(){
            static thread_local T t;
            return t;
        }

    protected:
        Singleton()= default;

    };


    class RandEngine: public Singleton<RandEngine>{
    public:
        std::random_device rd{};
        std::mt19937 generator{rd()};
    };



    class RandIntEngine: public Singleton<RandEngine>{
    public:
        std::random_device rd{};
        std::mt19937 generator{rd()};
    };



    // default generate 0-1 numbers
    inline double random_double(double low= 0, double high = 1) {
        thread_local auto &instance = RandEngine::instance();
        std::uniform_real_distribution<double> distribution(low, high);
        return distribution(instance.generator);
    }


    // random generate 0-n int numbers
    inline int random_int(int low, int high){
        thread_local auto &instance = RandIntEngine::instance();
        std::uniform_int_distribution<int> distribution(low, high);
        return distribution(instance.generator);
    }


    // Below code from https://stackoverflow.com/questions/21237905/how-do-i-generate-thread-safe-uniform-random-numbers
    /*
    inline double random_double(const double & min = 0 , const double & max = 1) {
        static thread_local std::random_device rd{};
        static thread_local std::mt19937 generator{rd()};
        std::uniform_real_distribution<double> distribution(min,max);
        return distribution(generator);
    }

    inline double random_int(const int & min, const int & max) {
        static thread_local std::random_device rd{};
        static thread_local std::mt19937 generator{rd()};
        std::uniform_int_distribution<int> distribution(min,max);
        return distribution(generator);
    }

    */


    template<typename RGB_t, typename Color_T>
    constexpr inline RGB_t vec_to_rgb( const Color_T & pixel_color){
        return {static_cast<int>(255.999 * pixel_color.x()) ,
                static_cast<int>(255.999 * pixel_color.y()),
                static_cast<int>(255.999 * pixel_color.z()) };
    }


}

