//
//  Ast.h
//  LightBASIC
//
//  Created by Albert Varaksin on 25/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//
#pragma once
#include "Ast.def.h"

namespace lbc {
    
    // forward declare Ast classes
    AST_DECLARE_CLASSES();
    
    // forward declare the visitor
    class AstVisitor;
    class Token;
    class Symbol;
    class SymbolTable;
    class Type;
    
    //
    // declare Ast node. define visitor method, mempory pooling and virtual destructor
    #define DECLARE_AST(C) \
        virtual void accept(AstVisitor * visitor); \
        void * operator new(size_t); \
        void operator delete(void *); \
        virtual ~C();
    
    
    //--------------------------------------------------------------------------
    // Basic Nodes
    //--------------------------------------------------------------------------
    
    /**
     * This is the base node for the AST tree
     */
    struct Ast : NonCopyable
    {
        // visitor pattern
        virtual void accept(AstVisitor * visitor) = 0;
        
        // virtual destructor
        virtual ~Ast() = default;
    };
    
    
    /**
     * base node for statements
     */
    struct AstStatement : Ast {};
    
    
    /**
     * program root
     */
    struct AstProgram : Ast {
        // create
        AstProgram(const string & name);
        // program name
        string name;
        // list of declaration lists
        vector<AstDeclaration *> decls;
        // symbol table
        shared_ptr<SymbolTable> symbolTable;
        // content node
        DECLARE_AST(AstProgram);
    };


    //--------------------------------------------------------------------------
    // Declarations
    //--------------------------------------------------------------------------
    
    
    /**
     * Declaration
     */
    struct AstDeclaration : AstStatement
    {
        // create
        AstDeclaration(AstAttributeList * attribs = nullptr);
        // attribs
        AstAttributeList * attribs;
        // declaration symbol. Does not own the symbol!
        Symbol * symbol;
        // content node
        DECLARE_AST(AstDeclaration);
    };
    
    
    /**
     * attribute list
     */
    struct AstAttributeList : Ast
    {
        // create
        AstAttributeList();
        // hold list of attributes
        vector<AstAttribute *> attribs;
        // content node
        DECLARE_AST(AstAttributeList);
    };
    
    
    /**
     * attribute node
     */
    struct AstAttribute : Ast
    {
        // create
        AstAttribute(AstIdentExpr * id = nullptr, AstAttribParamList * params = nullptr);
        // attribute id
        AstIdentExpr * id;
        // attribute params
        AstAttribParamList * params;
        // content node
        DECLARE_AST(AstAttribute);
    };
    
    
    /**
     * list of attribute parameters
     */
    struct AstAttribParamList : Ast
    {
        // create
        AstAttribParamList();
        // attribute params
        vector<AstLiteralExpr *> params;
        // content node
        DECLARE_AST(AstAttribParamList);
    };
    
    
    /**
     * variable declaration
     */
    struct AstVarDecl : AstDeclaration
    {
        // create
        AstVarDecl(AstIdentExpr * id = nullptr, AstTypeExpr * typeExpr = nullptr);
        // variable id
        AstIdentExpr * id;
        // variable type
        AstTypeExpr * typeExpr;
        // content node
        DECLARE_AST(AstVarDecl);
    };
    
    
    /**
     * function declaration
     */
    struct AstFunctionDecl : AstDeclaration
    {
        // create
        AstFunctionDecl(AstFuncSignature * signature = nullptr);
        // function signature
        AstFuncSignature * signature;
        // content node
        DECLARE_AST(AstFunctionDecl);
    };
    
    
    /**
     * function signature part
     */
    struct AstFuncSignature : Ast
    {
        // create
        AstFuncSignature(AstIdentExpr * id = nullptr, AstFuncParamList * args = nullptr, AstTypeExpr * typeExpr = nullptr, bool vararg = false);
        // function id
        AstIdentExpr * id;
        // function parameters
        AstFuncParamList * params;
        // function type
        AstTypeExpr * typeExpr;
        // flags
        int vararg:1;
        // content node
        DECLARE_AST(AstFuncSignature);
    };
    
    
    /**
     * function parameter list
     */
    struct AstFuncParamList : Ast
    {
        // create
        AstFuncParamList();
        // list of function parameters
        vector<AstFuncParam *> params;
        // content node
        DECLARE_AST(AstFuncParamList);
    };
    
    
    /**
     * Function parameter
     */
    struct AstFuncParam : AstDeclaration
    {
        // create
        AstFuncParam(AstIdentExpr * id = nullptr, AstTypeExpr * typeExpr = nullptr);
        // variable id
        AstIdentExpr * id;
        // variable type
        AstTypeExpr * typeExpr;
        // content node
        DECLARE_AST(AstFuncParam);
    };
    
    
    /**
     * function implementation
     */
    struct AstFunctionStmt : AstDeclaration
    {
        // create
        AstFunctionStmt(AstFuncSignature * signature = nullptr, AstStmtList * stmts = nullptr);
        // function signature
        AstFuncSignature * signature;
        // function body
        AstStmtList * stmts;
        // content node
        DECLARE_AST(AstFunctionStmt);
    };
    
    
    //--------------------------------------------------------------------------
    // Statements
    //--------------------------------------------------------------------------
    
    
    /**
     * Root node representing whole program
     */
    struct AstStmtList : Ast
    {
        // create
        AstStmtList();
        // list of statements
        vector<AstStatement *> stmts;
        // associated symbol table. Does not own!
        SymbolTable * symbolTable;
        // content node
        DECLARE_AST(AstStmtList);
    };
    
    
    /**
     * assigment statement
     */
    struct AstAssignStmt : AstStatement
    {
        // create 
        AstAssignStmt(AstIdentExpr * id = nullptr, AstExpression * expr = nullptr);
        // assignee id
        AstIdentExpr * id;
        // the expression
        AstExpression * expr;
        // content node
        DECLARE_AST(AstAssignStmt);
    };
    
    
    /**
     * RETURN statement
     */
    struct AstReturnStmt : AstStatement
    {
        // create
        AstReturnStmt(AstExpression * expr = nullptr);
        // the expression
        AstExpression * expr;
        // content node
        DECLARE_AST(AstReturnStmt);
    };
    
    
    /**
     * call statement. wrap the call expression
     */
    struct AstCallStmt : AstStatement
    {
        // create
        AstCallStmt(AstCallExpr * expr = nullptr);
        // the call expression
        AstCallExpr * expr;
        // content node
        DECLARE_AST(AstCallStmt);
    };
    
    //--------------------------------------------------------------------------
    // Expressions
    //--------------------------------------------------------------------------
    
    /**
     * base node for expressions
     */
    struct AstExpression : Ast
    {
        // create
        AstExpression();
        // expression type. Does not own!
        Type * type;
        // content node
        DECLARE_AST(AstExpression);
    };
    
    
    /**
     * cast expression. Target type is help in the Expression
     */
    struct AstCastExpr : AstExpression
    {
        // create
        AstCastExpr(AstExpression * expr = nullptr, AstTypeExpr * typeExpr = nullptr);
        // sub expression
        AstExpression * expr;
        // type expession
        AstTypeExpr * typeExpr;
        // content node
        DECLARE_AST(AstCastExpr);
    };
    
    
    /**
     * identifier node
     */
    struct AstIdentExpr : AstExpression
    {
        // create
        AstIdentExpr(Token * token = nullptr);
        // the id token
        Token * token;
        // content node
        DECLARE_AST(AstIdentExpr);
    };
    
    
    /**
     * base for literal expressions (string, number)
     */
    struct AstLiteralExpr : AstExpression
    {
        // create
        AstLiteralExpr(Token * token = nullptr);
        // the value token
        Token * token;
        // content node
        DECLARE_AST(AstLiteralExpr);
    };
    
    
    /**
     * call expression
     */
    struct AstCallExpr : AstExpression
    {
        // create
        AstCallExpr(AstIdentExpr * id = nullptr, AstFuncArgList * args = nullptr);
        // callee id
        AstIdentExpr * id;
        // parameters
        AstFuncArgList * args;
        // content node
        DECLARE_AST(AstCallExpr);
    };
    
    
    /**
     * function arguments
     */
    struct AstFuncArgList : Ast
    {
        // create
        AstFuncArgList();
        // list of arguments
        vector<AstExpression *> args;
        // content node
        DECLARE_AST(AstFuncArgList);
    };
    
    
    //--------------------------------------------------------------------------
    // Type
    //--------------------------------------------------------------------------
    
    /**
     * type declarator
     */
    struct AstTypeExpr : Ast
    {
        // create
        AstTypeExpr(Token * token = nullptr, int level = 0);
        // type id
        Token * token;
        // dereference level
        int level;
        // content node
        DECLARE_AST(AstTypeExpr);
    };
    
}
