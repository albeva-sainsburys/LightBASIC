//
//  Symbol.h
//  LightBASIC
//
//  Created by Albert Varaksin on 29/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//
#pragma once

namespace llvm {
    class Value;
}

namespace lbc {
    
    // forward declaration
    class Type;
    class AstDeclaration;
    
    /**
     * Symbol represents a single instance of a type (variables, functions, ...)
     */
    struct Symbol : NonCopyable
    {
        // create new symbol
        Symbol(const string & id, Type * type = nullptr, AstDeclaration * decl = nullptr, AstDeclaration * impl = nullptr);
        
        // clean up
        ~Symbol();
        
        // get id
        const string & id() const { return m_id; }
        
        // get alias
        const string & alias() const { return m_alias.length() ? m_alias : m_id; }
        
        // set alias
        void alias(const string & alias) { m_alias = alias; }
        
        // get type
        Type * type() const { return m_type; }
        
        // set type
        void type(Type * type) { m_type = type; }
        
        // get declaration ast node
        AstDeclaration * decl() const { return m_decl; }
        
        // set declaration ast node
        void decl(AstDeclaration * decl) { m_decl = decl; }
        
        // get implementation ast node
        AstDeclaration * impl() const { return m_impl; }
        
        // set implementation ast node
        void impl(AstDeclaration * impl) { m_impl = impl; }
        
        // allocate objects from the pool
        void * operator new(size_t);
        void operator delete(void *);
        
        // llvm argument
        llvm::Value * value;
        
    private:
        // id
        string m_id, m_alias;
        // symbol type
        Type * m_type;
        // symbol declaration (ast node)
        AstDeclaration * m_decl;
        // implementation ( functions may have seperate declaration )
        AstDeclaration * m_impl;
    };
    
}