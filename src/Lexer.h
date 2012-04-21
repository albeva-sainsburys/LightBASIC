//
//  Lexer.h
//  LightBASIC
//
//  Created by Albert Varaksin on 25/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//
#pragma once

#include "Token.h"

// llvm
namespace llvm {
    class MemoryBuffer;
}

// lbc
namespace lbc {

// forward reference
class Token;

/**
 * This class deals with lexing the input into tokens
 */
class Lexer : NonCopyable
{
public:
    
    // create new lexer instance
    Lexer(const llvm::MemoryBuffer * buffer);
    
    // get next token
    Token * next();
    
private:
    
    /// make token
    Token * MakeToken(TokenType type, int len = 0);
    Token * MakeToken(TokenType type, const std::string & lexeme, int len = -1);
    
    void multilineComment();
    Token * identifier();
    Token * number();
    Token * string();
    
    // the source
    const llvm::MemoryBuffer * m_src;
    const char * m_input, * m_tokenStart;
    
    // state info
    unsigned int m_line;
    unsigned short m_col;
    bool m_hasStmt;
};

} // ~lbc namespace
