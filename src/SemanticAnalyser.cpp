//
//  SemanticAnalyser.cpp
//  LightBASIC
//
//  Created by Albert Varaksin on 26/02/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//

#include "SemanticAnalyser.h"
#include "Ast.h"
#include "Token.h"
#include "Type.h"
#include "Symbol.h"
#include "SymbolTable.h"


#define LOG_VAR(var)    std::cout << #var << " = " << (var) << " : " << __FUNCTION__ << "(" << __LINE__ << ")\n";
#define LOG(msg)        std::cout << msg << " : " << __FUNCTION__ << "(" << __LINE__ << ")\n";

using namespace lbc;

/**
 * Simple helper. Basically ensured that original
 * value of a variable provided is restored apon
 * exiting the scope
 */
template<typename T> struct ScopeGuard : NonCopyable{
    
    // create
    ScopeGuard(T & value) : target(value), value(value)
    {
    }
    
    // restore
    ~ScopeGuard() {
        target = value;
    }
    
    // members
    T & target;
    T   value;
};

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)
#define SCOPED_GUARD(V) ScopeGuard<decltype(V)> MAKE_UNIQUE(_tmp_guard) (V);


//
// create new one
SemanticAnalyser::SemanticAnalyser()
:   RecursiveAstVisitor(true),
    m_table(nullptr),
    m_symbol(nullptr),
    m_type(nullptr),
    m_coerceType(nullptr)
{
}


//
// AstProgram
void SemanticAnalyser::visit(AstProgram * ast)
{
    // reset the state
    m_table = nullptr;
    m_symbol = nullptr;
    m_type = nullptr;
    m_coerceType = nullptr;
    
    // create new scope
    ast->symbolTable = make_shared<SymbolTable>(m_table);
    m_table = ast->symbolTable.get();
    // visit all declarations
    for (auto & decl : ast->decls) decl->accept(this);
    // restore the symbol table
    m_table = m_table->parent();
}


//
// AstFunctionDecl
void SemanticAnalyser::visit(AstFunctionDecl * ast)
{
    // function id
    auto const & id = ast->signature->id->token->lexeme();
    
    // check if already exists
    if (m_table->exists(id)) {
        throw Exception(string("Duplicate symbol: ") + id);
    }
    
    // analyse the signature. This will create a type
    ast->signature->accept(this);
    
    // create the symbol
    m_symbol = new Symbol(id, m_type, ast, nullptr);
    
    // attributes
    if (ast->attribs) ast->attribs->accept(this);
    
    // store in the ast. Should Ast own the symbol or the symbol table?
    ast->symbol = m_symbol;
    
    // store in the symbol table
    m_table->add(id, m_symbol);
}


//
// AstFuncSignature
void SemanticAnalyser::visit(AstFuncSignature * ast)
{
    // create new type
    auto funcType = new FunctionType();
    // process params
    m_type = funcType;
    if (ast->params) ast->params->accept(this);
    // process result type
    if (ast->typeExpr) {
        ast->typeExpr->accept(this);
        funcType->result(m_type);
    }
    // var arg?
    funcType->vararg = ast->vararg;
    //
    m_type = funcType;
}


//
// AstFuncParam
void SemanticAnalyser::visit(AstFuncParam * ast)
{
    auto funcType = static_cast<FunctionType *>(m_type);
    ast->typeExpr->accept(this);
    funcType->params.push_back(m_type);
    m_type = funcType;
}


//
// AstTypeExpr
void SemanticAnalyser::visit(AstTypeExpr * ast)
{
    // get the kind
    auto kind = ast->token->type();
    // basic type
    m_type = PrimitiveType::get(kind);
    if (!m_type) {
        throw Exception(string("Invalid type: ") + ast->token->name());
    }
    // is it a pointer?
    if (ast->level) m_type = PtrType::get(m_type, ast->level);
}


//
// AstFunctionStmt
void SemanticAnalyser::visit(AstFunctionStmt * ast)
{
    // function id
    const string & id = ast->signature->id->token->lexeme();
    
    // look up if declared
    m_symbol= m_table->get(id, true);
    
    // already implemented?
    if (m_symbol && m_symbol->impl()) {
        throw Exception(string("Type '") + id + "' is already implemented");
    }
    
    // process the signature
    ast->signature->accept(this);
    
    // m_type contains the type of the signature. if symbol was found
    // then check that types match.
    if (m_symbol) {
        if (!m_symbol->type()->compare(m_type)) {
            throw Exception(string("Type mismatch between '") + id + "' and '" + m_symbol->id());
        }
        m_symbol->impl(ast);
        if (ast->attribs) {
            throw Exception(string("Duplicate attribute declarations"));
        }
    } else {
        m_symbol = new Symbol(id, m_type, ast, ast);
        ast->symbol = m_symbol;
        m_table->add(id, m_symbol);
    }
    
    // attributes
    if (ast->attribs) ast->attribs->accept(this);
    
    // create new symbol table
    m_table = new SymbolTable(m_table);
    ast->stmts->symbolTable = m_table;
    
    // function type
    auto funcType = static_cast<FunctionType *>(m_type);
    
    // fill the table with parameters
    if (ast->signature->params) {
        int i = 0;
        for(auto & param : ast->signature->params->params) {
            const string & paramId = param->id->token->lexeme();
            if (m_table->exists(paramId)) {
                throw Exception(string("Duplicate defintion of ") + paramId);
            }
            auto sym = new Symbol(paramId, funcType->params[i++], param.get(), nullptr);
            m_table->add(paramId, sym);
            param->symbol = sym;
        }
    }
    
    // process function body
    ast->stmts->accept(this);
    
    // restore symbol table
    m_table = m_table->parent();
}


//
// Variable declaration
void SemanticAnalyser::visit(AstVarDecl * ast)
{
    // backup type
    auto tmp = m_type;
    
    // id
    const string & id = ast->id->token->lexeme();
    
    // exists?
    if (m_table->exists(id)) {
        throw Exception(string("Duplicate definition of ") + id);
    }
    
    // get the type
    ast->typeExpr->accept(this);
    
    // can this type be instantiated?
    if (!m_type->isInstantiable()) {
        throw Exception(string("Cannot declare a variable with type ") + ast->typeExpr->token->lexeme());
    }
    
    // create new symbol
    m_symbol= new Symbol(id, m_type, ast, ast);
    ast->symbol = m_symbol;
    
    // attributes
    if (ast->attribs) ast->attribs->accept(this);
    
    // add to the symbol table
    m_table->add(id, m_symbol);
    
    // restore type
    m_type = tmp;
}


//
// AstAssignStmt
void SemanticAnalyser::visit(AstAssignStmt * ast)
{
    // id
    const string & id = ast->id->token->lexeme();
    
    // get the symbol
    auto symbol = m_table->get(id);
    
    // check
    if (!symbol) {
        throw Exception(string("Use of undeclared identifier '") + id + "'");
    } else if (!symbol->type()->isInstantiable()) {
        throw Exception(string("Cannot assign to identifier '") + id + "'");
    }
    
    // m_type will hold the result of the expression
    SCOPED_GUARD(m_coerceType);
    m_coerceType = symbol->type();
    expression(ast->expr);
}


//
// AstCallStmt
void SemanticAnalyser::visit(AstCallStmt * ast)
{
    ast->expr->accept(this);
}


//
// Process the expression. If type coercion is required
// then insert AstCastExpr node in front of current ast expression
void SemanticAnalyser::expression(unique_ptr<AstExpression> & ast)
{
    // process the expression
    ast->accept(this);
    
    if (m_coerceType) coerce(ast, m_coerceType);
}


//
// coerce expression to a type if needed
void SemanticAnalyser::coerce(unique_ptr<AstExpression> & ast, Type * type)
{
    // deal with type coercion. If can create implicit cast node
    if (!type->compare(ast->type)) {
        // check if this is signed -> unsigned or vice versa
        if (ast->type->isIntegral() && type->isIntegral()) {
            if (ast->type->getSizeInBits() == type->getSizeInBits()) {
                return;
            }
        }
        // prefix expression with AstCastExpr
        auto expr = ast.release();
        ast.reset(new AstCastExpr(expr));
        ast->type = type;
    }
}


//
// AstCallExpr
void SemanticAnalyser::visit(AstCallExpr * ast)
{
    // the id
    const string & id = ast->id->token->lexeme();
    
    // find
    auto sym = m_table->get(id);
    if (!sym) {
        throw Exception(string("Use of undeclared identifier '") + id + "'");
    }
    
    // not a callable function?
    if (sym->type()->kind() != Type::Function) {
        throw Exception(string("Called identifier '") + id + "' is not a function");
    }
    
    auto type = static_cast<FunctionType *>(sym->type());
    
    // check the parameter types against the argument types
    if (ast->args) {
        if (type->vararg && ast->args->args.size() < type->params.size()) {
            throw Exception("Argument count mismatch");
        }
        else if (!type->vararg && type->params.size() != ast->args->args.size()) {
            throw Exception("Argument count mismatch");
        }
        int i = 0;
        SCOPED_GUARD(m_coerceType);
        for(auto & arg : ast->args->args) {
            m_coerceType = type->params.size() < i + 1 ? type->params[i++] : nullptr;
            expression(arg);
            if (!m_coerceType) {
                // cast vararg params to ints if less than 32bit
                if (arg->type->isIntegral() && arg->type->getSizeInBits() < 32) {
                    coerce(arg, PrimitiveType::get(TokenType::Integer));
                }
            }
        }
    } else if (type->params.size() != 0) {
        throw Exception("Argument count mismatch");
    }
    
    // set the expression type
    ast->type = type->result();
}


//
// AstLiteralExpr
void SemanticAnalyser::visit(AstLiteralExpr * ast)
{
    if (ast->token->type() == TokenType::StringLiteral) {
        ast->type = PtrType::get(PrimitiveType::get(TokenType::Byte), 1);
    } else if (ast->token->type() == TokenType::NumericLiteral) {
        if (m_coerceType) ast->type = m_coerceType;
        else ast->type = PrimitiveType::get(TokenType::Integer);
    } else {
        throw Exception("Invalid type");
    }
}


//
// AstIdentExpr
void SemanticAnalyser::visit(AstIdentExpr * ast)
{
    // the id
    const string & id = ast->token->lexeme();
    
    // get from the table
    auto sym = m_table->get(id);
    
    // not found?
    if (!sym) {
        throw Exception(string("Use of undeclared identifier '") + id + "'");
    }
    
    // set type
    ast->type = sym->type();
}


//
// AstReturnStmt
void SemanticAnalyser::visit(AstReturnStmt * ast)
{
    assert(m_type->isFunction());
    auto funcType = static_cast<FunctionType *>(m_type);
    
    if (ast->expr) {
        SCOPED_GUARD(m_coerceType);
        m_coerceType = funcType->result();
        expression(ast->expr);
    }
}


//
// AstAttribute
void SemanticAnalyser::visit(AstAttribute * ast)
{
    const string & id = ast->id->token->lexeme();
    if (id == "ALIAS") {
        if (!ast->params) 
            throw Exception("Alias expects a string value");
        if (ast->params->params.size() != 1)
            throw Exception("Alias expects one string value");
        if (ast->params->params[0]->token->type() != TokenType::StringLiteral)
            throw Exception("Incorrect Alias value. String expected");
        if (m_symbol) {
            m_symbol->alias(ast->params->params[0]->token->lexeme());
        }
    }
}













