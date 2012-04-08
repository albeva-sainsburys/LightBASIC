//
//  LightBASIC.h.pch
//  LightBASIC
//
//  Created by Albert Varaksin on 25/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//


// use c++11 standard library for shared_ptr, hash maps, etc...
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <cctype>
#include <string>
#include <inttypes.h>
#include <unordered_map>
// boost libraries
#include <boost/filesystem.hpp>
#include <boost/pool/pool.hpp>
#include <boost/algorithm/string/join.hpp>

// llvm
#include "stdint.h"
#include <llvm/Config/config.h>
#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/ReaderWriter.h>


// some helpers
namespace lbc {
    
    // namespace shortcuts
    using namespace std;
    namespace FS = boost::filesystem;

    /**
     * Base class for exceptions
     */
    struct Exception : public runtime_error
    {
        // Create new instance of Exception
        explicit Exception(const string & message) : runtime_error(message) {}
    };
    
    #define STRINGIFY_IMPL(v) #v
    #define STRINGIFY(v) STRINGIFY_IMPL(v)
    #define THROW_EXCEPTION(_msg) throw Exception(std::string(_msg) + ". \n" + __FILE__ + "(" + STRINGIFY(__LINE__) + ")\n" + __PRETTY_FUNCTION__);

    /**
     * NonCopyable c++11 style
     */
    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable & operator = (const NonCopyable &) = delete;
    };

    /**
     * Simple helper. Basically ensured that original
     * value of a variable provided is restored apon
     * exiting the scope
     */
    template<typename T> struct ScopeGuard : NonCopyable{

        // create
        ScopeGuard(T & value) : m_target(value), m_value(value)
        {
        }

        // restore
        ~ScopeGuard() {
            m_target = m_value;
        }

        // members
        T & m_target;
        T   m_value;
    };

    #define CONCATENATE_DETAIL(x, y) x##y
    #define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
    #define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)
    #define SCOPED_GUARD(V) ScopeGuard<decltype(V)> MAKE_UNIQUE(_tmp_guard) (V);
}