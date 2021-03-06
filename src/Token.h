//
//  Token.h
//  LightBASIC
//
//  Created by Albert Varaksin on 25/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//
#pragma once
#include "Token.def.h"
//#include "SourceLocation.h"
#include <llvm/Support/SMLoc.h>

namespace lbc {
    
    /**
     * Declare token types enum
     */
    enum class TokenType : int {
        #define IMPL_TOKENS(id, ...) id,
        ALL_TOKENS(IMPL_TOKENS)
        #undef IMPL_TOKENS
        _LAST
    };
    
    /**
     * This class represents a single
     * parsed token return by the lexer
     */
    class Token : NonCopyable
    {
    public:
        
        // get token name based on its type
        static const std::string & getTokenName(TokenType type);
        
        // get token type
        static TokenType getTokenType(const std::string & id, TokenType def = TokenType::Identifier);
        
        // create token type and lexeme
        Token(TokenType type,  const llvm::SMLoc & loc, const std::string & lexeme = "")
            :  m_loc(loc), m_lexeme(lexeme), m_type(type) {}
        
        // get lexeme
        const std::string & lexeme() const { return m_lexeme; }
        
        // get token type name
        const std::string & name() const { return getTokenName(m_type); }
        
        // get location
        const llvm::SMLoc & location() const { return m_loc; }
        
        // get type
        TokenType type() const { return m_type; }
        
        // set new type
        void type(TokenType typ) { m_type = typ; }
        
        // allocate objects from the pool
        void * operator new(size_t);
        void operator delete(void *);
        
        private:
        llvm::SMLoc m_loc;
        std::string m_lexeme;
        TokenType m_type;
    };
    
}
