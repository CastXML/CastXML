/*
  Copyright Kitware, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "RunClang.h"
#include "Options.h"
#include "Output.h"
#include "Utils.h"

#include <cxsys/SystemTools.hxx>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>

//----------------------------------------------------------------------------
class ASTConsumer: public clang::ASTConsumer
{
  clang::CompilerInstance& CI;
  llvm::raw_ostream& OS;
  Options const& Opts;
public:
  ASTConsumer(clang::CompilerInstance& ci, llvm::raw_ostream& os,
              Options const& opts):
    CI(ci), OS(os), Opts(opts) {}

  void AddImplicitMembers(clang::CXXRecordDecl* rd) {
    clang::Sema& sema = this->CI.getSema();
    sema.ForceDeclarationOfImplicitMembers(rd);

#   define DEFINE_IMPLICIT(name, decl) do {                           \
      clang::Sema::SFINAETrap trap(sema, /*AccessChecking=*/ true);   \
      sema.DefineImplicit##name(clang::SourceLocation(), (decl));     \
      if (trap.hasErrorOccurred()) {                                  \
        (decl)->setInvalidDecl();                                     \
      }                                                               \
    } while(0)

    for(clang::DeclContext::decl_iterator i = rd->decls_begin(),
          e = rd->decls_end(); i != e; ++i) {
      clang::CXXMethodDecl* m = clang::dyn_cast<clang::CXXMethodDecl>(*i);
      if(m && m->isImplicit() && !m->isDeleted() &&
         !m->doesThisDeclarationHaveABody()) {
        if (clang::CXXConstructorDecl* c =
            clang::dyn_cast<clang::CXXConstructorDecl>(m)) {
          if (c->isDefaultConstructor()) {
            DEFINE_IMPLICIT(DefaultConstructor, c);
          } else if (c->isCopyConstructor()) {
            DEFINE_IMPLICIT(CopyConstructor, c);
          } else if (c->isMoveConstructor()) {
            DEFINE_IMPLICIT(MoveConstructor, c);
          }
        } else if (clang::CXXDestructorDecl* d =
                   clang::dyn_cast<clang::CXXDestructorDecl>(m)) {
          DEFINE_IMPLICIT(Destructor, d);
        } else if (m->isCopyAssignmentOperator()) {
          DEFINE_IMPLICIT(CopyAssignment, m);
        } else if (m->isMoveAssignmentOperator()) {
          DEFINE_IMPLICIT(MoveAssignment, m);
        }
      }
    }

#   undef DEFINE_IMPLICIT
  }

  void HandleTagDeclDefinition(clang::TagDecl* d) {
    if(clang::CXXRecordDecl* rd = clang::dyn_cast<clang::CXXRecordDecl>(d)) {
      if(!rd->isDependentContext()) {
        this->AddImplicitMembers(rd);
      }
    }
  }

  void HandleTranslationUnit(clang::ASTContext& ctx) {
    outputXML(this->CI, ctx, this->OS, this->Opts);
  }
};

//----------------------------------------------------------------------------
template <class T>
class CastXMLPredefines: public T
{
protected:
  Options const& Opts;

  CastXMLPredefines(Options const& opts): Opts(opts) {}
  std::string UpdatePredefines(std::string const& predefines) {
    // Clang's InitializeStandardPredefinedMacros forces some
    // predefines even when -undef is given.  Filter them out.
    // Also substitute our chosen predefines prior to those that came
    // from the command line.
    char const predef_start[] = "# 1 \"<built-in>\" 3\n";
    char const predef_end[] = "# 1 \"<command line>\" 1\n";
    std::string::size_type start = predefines.find(predef_start);
    std::string::size_type end = predefines.find(predef_end);
    if(start != std::string::npos && end != std::string::npos) {
      return (predefines.substr(0, start+sizeof(predef_start)-1) +
              this->Opts.Predefines +
              predefines.substr(end));
    } else {
      return predefines + this->Opts.Predefines;
    }
  }

  bool BeginSourceFileAction(clang::CompilerInstance& CI,
                             llvm::StringRef /*Filename*/) {
    if(this->Opts.HaveCC) {
      CI.getPreprocessor().setPredefines(
      this->UpdatePredefines(CI.getPreprocessor().getPredefines()));
    }
    return true;
  }
};

//----------------------------------------------------------------------------
class CastXMLPrintPreprocessedAction:
  public CastXMLPredefines<clang::PrintPreprocessedAction>
{
public:
  CastXMLPrintPreprocessedAction(Options const& opts):
    CastXMLPredefines(opts) {}
};

//----------------------------------------------------------------------------
class CastXMLSyntaxOnlyAction:
  public CastXMLPredefines<clang::SyntaxOnlyAction>
{
  clang::ASTConsumer* CreateASTConsumer(clang::CompilerInstance &CI,
                                        llvm::StringRef InFile) {
    using llvm::sys::path::filename;
    if(!this->Opts.GccXml) {
      return clang::SyntaxOnlyAction::CreateASTConsumer(CI, InFile);
    } else if(llvm::raw_ostream* OS =
              CI.createDefaultOutputFile(false, filename(InFile), "xml")) {
      return new ASTConsumer(CI, *OS, this->Opts);
    } else {
      return 0;
    }
  }
public:
  CastXMLSyntaxOnlyAction(Options const& opts):
    CastXMLPredefines(opts) {}
};

//----------------------------------------------------------------------------
static clang::FrontendAction*
CreateFrontendAction(clang::CompilerInstance* CI, Options const& opts)
{
  clang::frontend::ActionKind action =
    CI->getInvocation().getFrontendOpts().ProgramAction;
  switch(action) {
  case clang::frontend::PrintPreprocessedInput:
    return new CastXMLPrintPreprocessedAction(opts);
  case clang::frontend::ParseSyntaxOnly:
    return new CastXMLSyntaxOnlyAction(opts);
  default:
    std::cerr << "error: unsupported action: " << int(action) << "\n";
    return 0;
  }
}

//----------------------------------------------------------------------------
static bool runClangCI(clang::CompilerInstance* CI, Options const& opts)
{
  // Create a diagnostics engine for this compiler instance.
  CI->createDiagnostics();
  if(!CI->hasDiagnostics()) {
    return false;
  }

  // Set frontend options we captured directly.
  CI->getFrontendOpts().OutputFile = opts.OutputFile;

  if(opts.GccXml) {
#   define MSG(x) "error: '--castxml-gccxml' does not work with " x "\n"
    if(CI->getLangOpts().ObjC1 || CI->getLangOpts().ObjC2) {
      std::cerr << MSG("Objective C");
      return false;
    }
    if(CI->getLangOpts().CPlusPlus1y) {
      std::cerr << MSG("c++1y");
      return false;
    }
    if(CI->getLangOpts().C11) {
      std::cerr << MSG("c11");
      return false;
    }
    if(CI->getLangOpts().C99) {
      std::cerr << MSG("c99");
      return false;
    }
#   undef MSG
  }

  // Construct our Clang front-end action.  This dispatches
  // handling of each input file with an action based on the
  // flags provided (e.g. -E to preprocess-only).
  llvm::OwningPtr<clang::FrontendAction>
    action(CreateFrontendAction(CI, opts));
  if(action) {
    return CI->ExecuteAction(*action);
  } else {
    return false;
  }
}

//----------------------------------------------------------------------------
static llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>
runClangCreateDiagnostics(const char* const* argBeg, const char* const* argEnd)
{
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>
    diagOpts(new clang::DiagnosticOptions);
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>
    diagID(new clang::DiagnosticIDs());
  llvm::OwningPtr<llvm::opt::OptTable>
    opts(clang::driver::createDriverOptTable());
  unsigned missingArgIndex, missingArgCount;
  llvm::OwningPtr<llvm::opt::InputArgList>
    args(opts->ParseArgs(argBeg, argEnd, missingArgIndex, missingArgCount));
  clang::ParseDiagnosticArgs(*diagOpts, *args);
  clang::TextDiagnosticPrinter* diagClient =
    new clang::TextDiagnosticPrinter(llvm::errs(), &*diagOpts);
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>
    diags(new clang::DiagnosticsEngine(diagID, &*diagOpts, diagClient));
  clang::ProcessWarningOptions(*diags, *diagOpts, /*ReportDiags=*/false);
  return diags;
}

//----------------------------------------------------------------------------
static int runClangImpl(const char* const* argBeg,
                        const char* const* argEnd,
                        Options const& opts)
{
  // Construct a diagnostics engine for use while processing driver options.
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags =
    runClangCreateDiagnostics(argBeg, argEnd);

  // Use the approach in clang::createInvocationFromCommandLine to
  // get system compiler setting arguments from the Driver.
  clang::driver::Driver d("clang", llvm::sys::getDefaultTargetTriple(),
                          "dummy.out", *diags);
  if(!cxsys::SystemTools::FileIsFullPath(d.ResourceDir.c_str()) ||
     !cxsys::SystemTools::FileIsDirectory(d.ResourceDir.c_str())) {
    d.ResourceDir = getClangResourceDir();
  }
  llvm::SmallVector<const char *, 16> cArgs;
  cArgs.push_back("<clang>");
  cArgs.insert(cArgs.end(), argBeg, argEnd);

  // Tell the driver not to generate any commands past syntax parsing.
  if(opts.PPOnly) {
    cArgs.push_back("-E");
  } else {
    cArgs.push_back("-fsyntax-only");
  }

  // Ask the driver to build the compiler commands for us.
  llvm::OwningPtr<clang::driver::Compilation> c(d.BuildCompilation(cArgs));

  // For '-###' just print the jobs and exit early.
  if(c->getArgs().hasArg(clang::driver::options::OPT__HASH_HASH_HASH)) {
    c->getJobs().Print(llvm::errs(), "\n", true);
    return 0;
  }

  // Reject '-o' with multiple inputs.
  if(!opts.OutputFile.empty() && c->getJobs().size() > 1) {
    diags->Report(clang::diag::err_drv_output_argument_with_multiple_files);
    return 1;
  }

  // Run Clang for each compilation computed by the driver.
  // This should be once per input source file.
  bool result = true;
  for(clang::driver::JobList::const_iterator i = c->getJobs().begin(),
        e = c->getJobs().end(); i != e; ++i) {
    clang::driver::Command* cmd = llvm::dyn_cast<clang::driver::Command>(*i);
    if(cmd && strcmp(cmd->getCreator().getName(), "clang") == 0) {
      // Invoke Clang with this set of arguments.
      llvm::OwningPtr<clang::CompilerInstance>
        CI(new clang::CompilerInstance());
      const char* const* cmdArgBeg = cmd->getArguments().data();
      const char* const* cmdArgEnd = cmdArgBeg + cmd->getArguments().size();
      if (clang::CompilerInvocation::CreateFromArgs
          (CI->getInvocation(), cmdArgBeg, cmdArgEnd, *diags)) {
        result = runClangCI(CI.get(), opts) && result;
      } else {
        result = false;
      }
    } else {
      // Skip this unexpected job.
      llvm::SmallString<128> buf;
      llvm::raw_svector_ostream msg(buf);
      (*i)->Print(msg, "\n", true);
      diags->Report(clang::diag::err_fe_expected_clang_command);
      diags->Report(clang::diag::err_fe_expected_compiler_job)
        << msg.str();
      result = false;
    }
  }
  return result? 0:1;
}

//----------------------------------------------------------------------------
int runClang(const char* const* argBeg,
             const char* const* argEnd,
             Options const& opts)
{
  llvm::SmallVector<const char*, 32> args(argBeg, argEnd);
  std::string fmsc_version = "-fmsc-version=";

  if(opts.HaveCC) {
    // Configure target to match that of given compiler.
    if(!opts.Triple.empty()) {
      args.push_back("-target");
      args.push_back(opts.Triple.c_str());
    }

    // Tell Clang driver not to add its header search paths.
    args.push_back("-nostdinc");

    // Add header search paths detected from given compiler.
    for(std::vector<Options::Include>::const_iterator
          i = opts.Includes.begin(), e = opts.Includes.end();
        i != e; ++i) {
      if(i->Framework) {
        args.push_back("-iframework");
      } else {
        args.push_back("-isystem");
      }
      args.push_back(i->Directory.c_str());
    }

    // Tell Clang not to add its predefines.
    args.push_back("-undef");

    // Configure language options to match given compiler.
    const char* pd = opts.Predefines.c_str();
    if(strstr(pd, "#define _MSC_EXTENSIONS ")) {
      args.push_back("-fms-extensions");
    }
    if(const char* d = strstr(pd, "#define _MSC_VER ")) {
      args.push_back("-fms-compatibility");
      // Extract the _MSC_VER value to give to -fmsc-version=.
      d += 17;
      if(const char* e = strchr(d, '\n')) {
        if(*(e - 1) == '\r') {
          --e;
        }
        fmsc_version.append(d, e-d);
        args.push_back(fmsc_version.c_str());
      }
    }
  }

  return runClangImpl(args.data(), args.data() + args.size(), opts);
}
