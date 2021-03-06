//
//  ParseVAR.cpp
//  LightBASIC
//
//  Created by Albert on 24/06/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//

#include "ParserShared.h"
using namespace lbc;

/**
 * VAR = "VAR" id "=" Expression
 */
std::unique_ptr<AstVarDecl> Parser::kwVar()
{
    // "DIM"
    EXPECT(TokenType::Var);
    
    auto decl = make_unique<AstVarDecl>();
    
    // id
    decl->id = identifier();
    if (hasError()) return nullptr;
    
    // "="
    EXPECT(TokenType::Assign);
    
    // Expression
    decl->expr = expression();
    if (hasError()) return nullptr;
    
    // done
    return decl;
}
