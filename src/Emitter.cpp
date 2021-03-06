//
//  Emitter.cpp
//  LightBASIC
//
//  Created by Albert Varaksin on 08/03/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//
#include "Emitter.h"
#include "Context.h"

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/ReaderWriter.h>

#include <iostream>

using namespace lbc;


/**
 * Create new Emitter instance
 */
Emitter::Emitter(Context & ctx) : m_ctx(ctx)
{}


/**
 * clean up
 */
Emitter::~Emitter()
{}


/**
 * add module to the emitter's list
 */
void Emitter::add(llvm::Module * module)
{
    m_modules.push_back(std::unique_ptr<llvm::Module>(module));
}


/**
 * generate the results
 */
void Emitter::generate()
{
    switch (m_ctx.emit()) {
        case EmitType::Executable:
            emitExecutable();
            break;
        case EmitType::Library:
            emitLibrary();
            break;
        case EmitType::Object:
        case EmitType::Asm:
            emitObjOrAsm();
            break;
        case EmitType::Llvm:
            emitLlvm();
            break;
    }
}


/**
 * emit executable binary
 */
void Emitter::emitExecutable()
{
    // generate .obj files
    emitObjOrAsm();
    std::stringstream link_cmd, rm_cmd;
    std::stringstream arhc, lib_paths, libs, objs, output; 
    
    // rm
    rm_cmd << "rm";
    
    // linker flags
    link_cmd << getTool(Tool::Ld);
    
    // library include paths
    for (auto & path : m_ctx.get(ResourceType::LibraryPath)) {
        lib_paths << " -L" << path;
    }
    
    // libraries
    for (auto & lib : m_ctx.get(ResourceType::Library)) {
       libs <<  " -l" << lib;
    }
    
    // the output
    output << " -o " << m_ctx.output();
    
    // add object files
    for (auto & module : m_modules) {
        // get path
        Path path(module->getModuleIdentifier());
        path.extension("o");
        objs << " " << path;
        rm_cmd << " " << path;
    }
    
    // platform specific
    auto & triple = m_ctx.triple();
    if (triple.getOS() == llvm::Triple::Linux) {
        // architecture
        Path sys_path;
        if (triple.isArch32Bit()) {
            link_cmd << " -m elf_i386"
                     << " -dynamic-linker /lib/ld-linux.so.2"
                     ;
            sys_path = "/usr/libr32";
            if (!sys_path.isDirectory()) {
                sys_path = "/usr/lib/i386-linux-gnu";
                if (!sys_path.isDirectory()) {
                    // raise error: omething wrong;
                    return;
                }
            }
        } else if (triple.isArch64Bit()) {
            link_cmd << " -m elf_x86_64"
                     << " -dynamic-linker /lib64/ld-linux-x86-64.so.2"
                     ;
            sys_path = "/usr/lib/x86_64-linux-gnu";
        }
        link_cmd << output.str()
                 << " -L /usr/lib"
                 << " -L" << sys_path
                 << " " << sys_path << "/crt1.o"
                 << " " << sys_path << "/crti.o"
                 << objs.str()
                 << " --no-as-needed -lc"
                 << " " << sys_path << "/crtn.o"
                 ;
    } else if (triple.isMacOSX()) {
        link_cmd << " -arch " << m_ctx.triple().getArchName().str();
        link_cmd << " -macosx_version_min 10.6.0"
                 << " -L/usr/lib"
                 << lib_paths.str()
                 << " -lSystem -lcrt1.10.6.o"
                 << libs.str()
                 << output.str()
                 << objs.str()
                 ;
    } else if (triple.isOSWindows()) {
        // TODO
    }
    
    // do the thing
    if (m_ctx.verbose()) std::cout << link_cmd.str() << '\n';
    if (::system(link_cmd.str().c_str())) return;
    if (::system(rm_cmd.str().c_str())) return;
}


/**
 * Emit library (static or shared (dll on windows))
 */
void Emitter::emitLibrary()
{
    // Emitting library is not implemented
    return ;
}


/**
 * emit asm files
 */
void Emitter::emitObjOrAsm()
{
    // show verbose?
    bool verbose = m_ctx.verbose();
    
    // the command
    std::stringstream llc_cmd;
    llc_cmd << getTool(Tool::LlvmLlc);
    
    // output type
    if (m_ctx.emit() == EmitType::Asm) {
        llc_cmd << " -filetype=asm";
    } else {
        llc_cmd << " -filetype=obj";
    }
    
    // optimization
    OptimizationLevel lvl = m_ctx.opt();
    llc_cmd << " " << getOptimizationLevel(lvl);
    
    // architecture
    auto & triple = m_ctx.triple();
    if (triple.getArch() == llvm::Triple::x86) {
        llc_cmd << " -march=" << "x86";
    } else if (triple.getArch() == llvm::Triple::x86_64) {
        llc_cmd << " -march=" << "x86-64";
    } else {
        llc_cmd << " -march=" << triple.getArchName().str();
    }
    
    for (auto & module : m_modules) {
        // get path
        Path path(module->getModuleIdentifier());
        path.extension("bc");
        
        // write .bc files
        {
            std::string errors;
            llvm::raw_fd_ostream stream(path.c_str(), errors);
            if (errors.length()) {
                // raise error: errors
                return;
            }
            llvm::WriteBitcodeToFile(module.get(), stream);
        }
        
        // assemble
        {
            std::stringstream s;
            auto output = path;
            if (m_ctx.emit() == EmitType::Asm) {
                output.extension("s");
            } else {
                output.extension("o");
            }
            s << llc_cmd.str() << " -o " << output << " " << path;
            if (verbose) std::cout << s.str() << '\n';
            if (::system(s.str().c_str())) return;
        }
        
        // delete tmp file
        {
            std::stringstream s;
            s << "rm " << path;
            if (::system(s.str().c_str())) return;
        }
    }
}


/**
 * emit llvm ir files
 */
void Emitter::emitLlvm()
{
    bool verbose = m_ctx.verbose();
    OptimizationLevel lvl = m_ctx.opt();
    std::string optimize = "";
    if (lvl != OptimizationLevel::O0) {
        optimize = getTool(Tool::LlvmOpt) + " -S " + getOptimizationLevel(lvl) + " ";
    }
    
    for (auto & module : m_modules) {
        // get path
        Path path(module->getModuleIdentifier());
        path.extension("ll");
        
        // write .ll file
        {
            std::string errors;
            llvm::raw_fd_ostream stream(path.c_str(), errors);
            if (errors.length()) {
                // TODO raise error: errors
                return;
            }
            module->print(stream, nullptr);
        }
        
        // optimize
        if (optimize.length()) {
            std::stringstream s;
            s << optimize << path << " -o " << path;
            if (::system(s.str().c_str())) return;
        }
        
        // info
        if (verbose) std::cout << "Generated: " << path << '\n';
    }
}


/**
 * get tool path
 */
std::string Emitter::getTool(Tool tool)
{
    switch (tool) {
        case Tool::LlvmOpt:
            return "/usr/local/bin/opt";
        case Tool::LlvmLlc:
            return "/usr/local/bin/llc";
        case Tool::Ld:
            return "/usr/bin/ld";
    }
}

