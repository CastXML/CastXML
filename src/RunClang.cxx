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

#include "llvm/Config/llvm-config.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Version.h"
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
#include "llvm/ADT/STLExtras.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <queue>

#if LLVM_VERSION_MAJOR > 3 ||                                                 \
  LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 9
#define CASTXML_OWNS_OSTREAM
#endif

class ASTConsumer : public clang::ASTConsumer
{
  clang::CompilerInstance& CI;
#ifdef CASTXML_OWNS_OSTREAM
  std::unique_ptr<llvm::raw_ostream> OwnOS;
#endif
  llvm::raw_ostream& OS;
  Options const& Opts;
  struct Class
  {
    clang::CXXRecordDecl* RD;
    int Depth;
    Class(clang::CXXRecordDecl* rd, int depth)
      : RD(rd)
      , Depth(depth)
    {
    }
  };
  std::queue<Class> Classes;
  int ClassImplicitMemberDepth = 0;

public:
#ifdef CASTXML_OWNS_OSTREAM
  ASTConsumer(clang::CompilerInstance& ci,
              std::unique_ptr<llvm::raw_ostream> os, Options const& opts)
    : CI(ci)
    , OwnOS(std::move(os))
    , OS(*OwnOS)
    , Opts(opts)
  {
  }
#else
  ASTConsumer(clang::CompilerInstance& ci, llvm::raw_ostream& os,
              Options const& opts)
    : CI(ci)
    , OS(os)
    , Opts(opts)
  {
  }
#endif

  void AddImplicitMembers(Class const& c)
  {
    clang::CXXRecordDecl* rd = c.RD;
    this->ClassImplicitMemberDepth = c.Depth + 1;

    clang::Sema& sema = this->CI.getSema();
    sema.ForceDeclarationOfImplicitMembers(rd);

    for (clang::DeclContext::decl_iterator i = rd->decls_begin(),
                                           e = rd->decls_end();
         i != e; ++i) {
      clang::CXXMethodDecl* m = clang::dyn_cast<clang::CXXMethodDecl>(*i);
      if (m && !m->isDeleted() && !m->isInvalidDecl()) {
        bool mark = false;
        clang::CXXConstructorDecl* c =
          clang::dyn_cast<clang::CXXConstructorDecl>(m);
        if (c) {
          mark = (c->isDefaultConstructor() || c->isCopyConstructor() ||
                  c->isMoveConstructor());
        } else if (clang::dyn_cast<clang::CXXDestructorDecl>(m)) {
          mark = true;
        } else {
          mark =
            (m->isCopyAssignmentOperator() || m->isMoveAssignmentOperator());
        }
        if (mark) {
          /* Ensure the member is defined.  */
          sema.MarkFunctionReferenced(clang::SourceLocation(), m);
          if (c && c->isDefaulted() && c->isDefaultConstructor() &&
              c->isTrivial() && !c->isUsed(false) &&
              !c->hasAttr<clang::DLLExportAttr>()) {
            /* Clang does not build the definition of trivial constructors
               until they are used.  Force semantic checking.  */
            sema.DefineImplicitDefaultConstructor(clang::SourceLocation(), c);
          }
          /* Finish implicitly instantiated member.  */
          sema.PerformPendingInstantiations();
        }
      }
    }
  }

  void HandleTagDeclDefinition(clang::TagDecl* d)
  {
    if (clang::CXXRecordDecl* rd = clang::dyn_cast<clang::CXXRecordDecl>(d)) {
      if (!rd->isDependentContext()) {
        if (this->ClassImplicitMemberDepth < 16) {
          this->Classes.push(Class(rd, this->ClassImplicitMemberDepth));
        }
      }
    }
  }

  void HandleTranslationUnit(clang::ASTContext& ctx)
  {
    clang::Sema& sema = this->CI.getSema();

    // Perform instantiations needed by the original translation unit.
    sema.PerformPendingInstantiations();

    if (!sema.getDiagnostics().hasErrorOccurred()) {
      // Suppress diagnostics from below extensions to the translation unit.
      sema.getDiagnostics().setSuppressAllDiagnostics(true);

      // Add implicit members to classes.
      while (!this->Classes.empty()) {
        Class c = this->Classes.front();
        this->Classes.pop();
        this->AddImplicitMembers(c);
      }
    }

    // Tell Clang to finish the translation unit and tear down the parser.
    sema.ActOnEndOfTranslationUnit();

    // Process the AST.
    outputXML(this->CI, ctx, this->OS, this->Opts);
  }
};

template <class T>
class CastXMLPredefines : public T
{
protected:
  Options const& Opts;

  CastXMLPredefines(Options const& opts)
    : Opts(opts)
  {
  }
  std::string UpdatePredefines(clang::CompilerInstance const& CI)
  {
    std::string const& predefines = CI.getPreprocessor().getPredefines();

    // Identify the portion of the predefines string corresponding to
    // built-in predefined macros.
    char const predef_start[] = "# 1 \"<built-in>\" 3\n";
    char const predef_end[] = "# 1 \"<command line>\" 1\n";
    std::string::size_type start = predefines.find(predef_start);
    std::string::size_type end = std::string::npos;
    if (start != std::string::npos) {
      start += sizeof(predef_start) - 1;
      end = predefines.find(predef_end, start);
      if (end == std::string::npos) {
        end = predefines.size();
      }
    }

    std::string builtins;

    // Add a builtin to identify CastXML itself.
    char castxml_version[64];
    sprintf(castxml_version, "#define __castxml__ %u\n", getVersionValue());
    builtins += castxml_version;

    // Add builtins to identify the internal Clang compiler.
    builtins +=
#define STR(x) STR_(x)
#define STR_(x) #x
      "#define __castxml_clang_major__ " STR(
        CLANG_VERSION_MAJOR) "\n"
                             "#define __castxml_clang_minor__ " STR(
                               CLANG_VERSION_MINOR) "\n"
                                                    "#define "
                                                    "__castxml_clang_"
                                                    "patchlevel__ "
#ifdef CLANG_VERSION_PATCHLEVEL
      STR(CLANG_VERSION_PATCHLEVEL)
#else
                                                    "0"
#endif
        "\n"
#undef STR
#undef STR_
      ;

    // If we detected predefines from another compiler, substitute them.
    if (this->Opts.HaveCC) {
      builtins += this->Opts.Predefines;

      // Provide __builtin_va_arg_pack if simulating the actual GNU compiler.
      if (this->NeedBuiltinVarArgPack(this->Opts.Predefines)) {
        // Clang does not support this builtin, so fake it to tolerate
        // uses in function bodies while parsing.
        builtins += "\n"
                    "#define __builtin_va_arg_pack() 0\n"
                    "#define __builtin_va_arg_pack_len() 1\n";
      }

      // Provide __float128 if simulating the actual GNU compiler.
      if (!this->HaveFloat128(CI) &&
          this->NeedFloat128(this->Opts.Predefines)) {
        // Clang provides its own (fake) builtin in gnu++11 mode but issues
        // diagnostics when it is used in some contexts.  Provide our own
        // approximation of the builtin instead.
        builtins += "\n"
                    "typedef struct __castxml__float128_s { "
                    "  char x[16] __attribute__((aligned(16))); "
                    "} __castxml__float128;\n"
                    "#define __float128 __castxml__float128\n";
      }

      // Provide __is_assignable builtin if simulating MSVC.
      // When a future Clang version supports the builtin then
      // we can skip this when built against such a Clang.
      if (CI.getLangOpts().MSCompatibilityVersion >= 190000000 &&
          CI.getLangOpts().CPlusPlus11) {
        builtins +=
          "\n"
          "template <typename T> T&& __castxml__declval() noexcept;\n"
          "template <typename To, typename Fr, typename =\n"
          "  decltype(__castxml__declval<To>() = __castxml__declval<Fr>())>\n"
          "  static char (&__castxml__is_assignable_check(int))[1];\n"
          "template <typename, typename>\n"
          "  static char (&__castxml__is_assignable_check(...))[2];\n"
          "#define __is_assignable(_To,_Fr) \\\n"
          "  (sizeof(__castxml__is_assignable_check<_To,_Fr>(0)) == \\\n"
          "   sizeof(char(&)[1]))\n";
      }

#if LLVM_VERSION_MAJOR < 3 || LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 8
      // Clang 3.8 and above provide a __make_integer_seq builtin needed
      // in C++14 mode.  Provide it ourselves for older Clang versions.
      if (CI.getLangOpts().CPlusPlus14) {
        builtins +=
          "\n"
          "template <typename _T, _T> struct __castxml__integral_constant;\n"
          "template <template<typename _U, _U...> class _S,\n"
          "          typename, typename, bool>\n"
          "  struct __castxml__make_integer_seq_impl;\n"
          "template <template<typename _U, _U...> class _S,\n"
          "          class _T, _T... __v>\n"
          "  struct __castxml__make_integer_seq_impl<_S,\n"
          "       __castxml__integral_constant<_T, 0>,\n"
          "       _S<_T, __v...>, true> {\n"
          "     typedef _S<_T, __v...> type;\n"
          "  };\n"
          "template <template<typename _U, _U...> class _S,\n"
          "          class _T, _T __i, _T... __v>\n"
          "  struct __castxml__make_integer_seq_impl<_S,\n"
          "       __castxml__integral_constant<_T, __i>,\n"
          "       _S<_T, __v...>, true>\n"
          "    : __castxml__make_integer_seq_impl<_S,\n"
          "       __castxml__integral_constant<_T, __i - 1>,\n"
          "       _S<_T, __i - 1, __v...>, __i >= 1 > {};\n"
          "template <template<typename _U, _U...> class _S,\n"
          "          typename _T, _T _Sz>\n"
          "using __castxml__make_integer_seq = typename\n"
          "  __castxml__make_integer_seq_impl<_S,\n"
          "      __castxml__integral_constant<_T, _Sz>,\n"
          "     _S<_T>, (_Sz>=0)>::type;\n"
          "#define __make_integer_seq __castxml__make_integer_seq\n";
      }
#endif

      // Prevent glibc use of a GNU extension not implemented by Clang.
      if (this->NeedNoMathInlines(this->Opts.Predefines)) {
        builtins += "\n"
                    "#define __NO_MATH_INLINES 1\n";
      }

    } else {
      builtins += predefines.substr(start, end - start);
    }
    return predefines.substr(0, start) + builtins + predefines.substr(end);
  }

  bool IsActualGNU(std::string const& pd) const
  {
    return (pd.find("#define __GNUC__ ") != pd.npos &&
            pd.find("#define __clang__ ") == pd.npos &&
            pd.find("#define __INTEL_COMPILER ") == pd.npos &&
            pd.find("#define __CUDACC__ ") == pd.npos &&
            pd.find("#define __PGI ") == pd.npos);
  }

  bool NeedBuiltinVarArgPack(std::string const& pd)
  {
    return this->IsActualGNU(pd);
  }

  bool NeedFloat128(std::string const& pd) const
  {
    return (this->IsActualGNU(pd) &&
            (pd.find("#define __i386__ ") != pd.npos ||
             pd.find("#define __x86_64__ ") != pd.npos ||
             pd.find("#define __ia64__ ") != pd.npos));
  }

  bool HaveFloat128(clang::CompilerInstance const& CI) const
  {
#if LLVM_VERSION_MAJOR > 3 || LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 8
    return CI.getTarget().hasFloat128Type();
#else
    static_cast<void>(CI);
    return false;
#endif
  }

  bool NeedNoMathInlines(std::string const& pd) const
  {
    return (this->IsActualGNU(pd) &&
            (pd.find("#define __i386__ ") != pd.npos &&
             pd.find("#define __OPTIMIZE__ ") != pd.npos &&
             pd.find("#define __NO_MATH_INLINES ") == pd.npos));
  }

  bool BeginSourceFileAction(clang::CompilerInstance& CI,
                             llvm::StringRef /*Filename*/)
  {
    CI.getPreprocessor().setPredefines(this->UpdatePredefines(CI));

    // Tell Clang not to tear down the parser at EOF.
    CI.getPreprocessor().enableIncrementalProcessing();

    return true;
  }
};

class CastXMLPrintPreprocessedAction
  : public CastXMLPredefines<clang::PrintPreprocessedAction>
{
public:
  CastXMLPrintPreprocessedAction(Options const& opts)
    : CastXMLPredefines(opts)
  {
  }
};

class CastXMLSyntaxOnlyAction
  : public CastXMLPredefines<clang::SyntaxOnlyAction>
{
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance& CI, llvm::StringRef InFile) override
  {
    using llvm::sys::path::filename;
    if (!this->Opts.GccXml && !this->Opts.CastXml) {
      return clang::SyntaxOnlyAction::CreateASTConsumer(CI, InFile);
#ifdef CASTXML_OWNS_OSTREAM
    } else if (std::unique_ptr<llvm::raw_ostream> OS =
                 CI.createDefaultOutputFile(false, filename(InFile), "xml")) {
      return llvm::make_unique<ASTConsumer>(CI, std::move(OS), this->Opts);
#else
    } else if (llvm::raw_ostream* OS =
                 CI.createDefaultOutputFile(false, filename(InFile), "xml")) {
      return llvm::make_unique<ASTConsumer>(CI, *OS, this->Opts);
#endif
    } else {
      return nullptr;
    }
  }

public:
  CastXMLSyntaxOnlyAction(Options const& opts)
    : CastXMLPredefines(opts)
  {
  }
};

static clang::FrontendAction* CreateFrontendAction(clang::CompilerInstance* CI,
                                                   Options const& opts)
{
  clang::frontend::ActionKind action =
    CI->getInvocation().getFrontendOpts().ProgramAction;
  switch (action) {
    case clang::frontend::PrintPreprocessedInput:
      return new CastXMLPrintPreprocessedAction(opts);
    case clang::frontend::ParseSyntaxOnly:
      return new CastXMLSyntaxOnlyAction(opts);
    default:
      std::cerr << "error: unsupported action: " << int(action) << "\n";
      return nullptr;
  }
}

static bool runClangCI(clang::CompilerInstance* CI, Options const& opts)
{
  // Create a diagnostics engine for this compiler instance.
  CI->createDiagnostics();
  if (!CI->hasDiagnostics()) {
    return false;
  }

  // Set frontend options we captured directly.
  CI->getFrontendOpts().OutputFile = opts.OutputFile;

  if (opts.GccXml) {
#define MSG(x) "error: '--castxml-gccxml' does not work with " x "\n"
    if (CI->getLangOpts().ObjC1 || CI->getLangOpts().ObjC2) {
      std::cerr << MSG("Objective C");
      return false;
    }
#undef MSG
  }

  if (opts.CastXml) {
#define MSG(x) "error: '--castxml-output=<v>' does not work with " x "\n"
    if (CI->getLangOpts().ObjC1 || CI->getLangOpts().ObjC2) {
      std::cerr << MSG("Objective C");
      return false;
    }
#undef MSG
  }

  // Construct our Clang front-end action.  This dispatches
  // handling of each input file with an action based on the
  // flags provided (e.g. -E to preprocess-only).
  std::unique_ptr<clang::FrontendAction> action(
    CreateFrontendAction(CI, opts));
  if (action) {
    return CI->ExecuteAction(*action);
  } else {
    return false;
  }
}

static llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>
runClangCreateDiagnostics(const char* const* argBeg, const char* const* argEnd)
{
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> diagOpts(
    new clang::DiagnosticOptions);
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID(
    new clang::DiagnosticIDs());
  std::unique_ptr<llvm::opt::OptTable> opts(
    clang::driver::createDriverOptTable());
  unsigned missingArgIndex, missingArgCount;
#if LLVM_VERSION_MAJOR > 3 ||                                                 \
  LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7
  llvm::opt::InputArgList args(opts->ParseArgs(
    llvm::makeArrayRef(argBeg, argEnd), missingArgIndex, missingArgCount));
  clang::ParseDiagnosticArgs(*diagOpts, args);
#else
  std::unique_ptr<llvm::opt::InputArgList> args(
    opts->ParseArgs(argBeg, argEnd, missingArgIndex, missingArgCount));
  clang::ParseDiagnosticArgs(*diagOpts, *args);
#endif
  clang::TextDiagnosticPrinter* diagClient =
    new clang::TextDiagnosticPrinter(llvm::errs(), &*diagOpts);
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags(
    new clang::DiagnosticsEngine(diagID, &*diagOpts, diagClient));
  clang::ProcessWarningOptions(*diags, *diagOpts, /*ReportDiags=*/false);
  return diags;
}

static int runClangImpl(const char* const* argBeg, const char* const* argEnd,
                        Options const& opts)
{
  // Construct a diagnostics engine for use while processing driver options.
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags =
    runClangCreateDiagnostics(argBeg, argEnd);

  // Use the approach in clang::createInvocationFromCommandLine to
  // get system compiler setting arguments from the Driver.
  clang::driver::Driver d("clang", llvm::sys::getDefaultTargetTriple(),
                          *diags);
  if (!llvm::sys::path::is_absolute(d.ResourceDir) ||
      !llvm::sys::fs::is_directory(d.ResourceDir)) {
    d.ResourceDir = getClangResourceDir();
  }
  llvm::SmallVector<const char*, 16> cArgs;
  cArgs.push_back("<clang>");
  cArgs.insert(cArgs.end(), argBeg, argEnd);

  // Tell the driver not to generate any commands past syntax parsing.
  if (opts.PPOnly) {
    cArgs.push_back("-E");
  } else {
    cArgs.push_back("-fsyntax-only");
  }

  // Ask the driver to build the compiler commands for us.
  std::unique_ptr<clang::driver::Compilation> c(d.BuildCompilation(cArgs));
  if (diags->hasErrorOccurred()) {
    return 1;
  }

  // For '-###' just print the jobs and exit early.
  if (c->getArgs().hasArg(clang::driver::options::OPT__HASH_HASH_HASH)) {
    c->getJobs().Print(llvm::errs(), "\n", true);
    return 0;
  }

  // Reject '-o' with multiple inputs.
  if (!opts.OutputFile.empty() && c->getJobs().size() > 1) {
    diags->Report(clang::diag::err_drv_output_argument_with_multiple_files);
    return 1;
  }

  // Run Clang for each compilation computed by the driver.
  // This should be once per input source file.
  bool result = true;
  for (auto const& job : c->getJobs()) {
    clang::driver::Command const* cmd =
      llvm::dyn_cast<clang::driver::Command>(&job);
    if (cmd && strcmp(cmd->getCreator().getName(), "clang") == 0) {
      // Invoke Clang with this set of arguments.
      std::unique_ptr<clang::CompilerInstance> CI(
        new clang::CompilerInstance());
      const char* const* cmdArgBeg = cmd->getArguments().data();
      const char* const* cmdArgEnd = cmdArgBeg + cmd->getArguments().size();
      if (clang::CompilerInvocation::CreateFromArgs(
            CI->getInvocation(), cmdArgBeg, cmdArgEnd, *diags)) {
        if (diags->hasErrorOccurred()) {
          return 1;
        }
        result = runClangCI(CI.get(), opts) && result;
      } else {
        result = false;
      }
    } else {
      // Skip this unexpected job.
      llvm::SmallString<128> buf;
      llvm::raw_svector_ostream msg(buf);
      job.Print(msg, "\n", true);
      diags->Report(clang::diag::err_fe_expected_clang_command);
      diags->Report(clang::diag::err_fe_expected_compiler_job) << msg.str();
      result = false;
    }
  }
  return result ? 0 : 1;
}

int runClang(const char* const* argBeg, const char* const* argEnd,
             Options const& opts)
{
  llvm::SmallVector<const char*, 32> args(argBeg, argEnd);
  std::string fmsc_version = "-fmsc-version=";
  std::string std_flag = "-std=";

  if (opts.HaveCC) {
    // Configure target to match that of given compiler.
    if (!opts.HaveTarget && !opts.Triple.empty()) {
      args.push_back("-target");
      args.push_back(opts.Triple.c_str());
    }

    // Tell Clang driver not to add its header search paths.
    args.push_back("-nobuiltininc");
    args.push_back("-nostdlibinc");

    // Add header search paths detected from given compiler.
    for (std::vector<Options::Include>::const_iterator
           i = opts.Includes.begin(),
           e = opts.Includes.end();
         i != e; ++i) {
      if (i->Framework) {
        args.push_back("-iframework");
      } else {
        args.push_back("-isystem");
      }
      args.push_back(i->Directory.c_str());
    }

    // Tell Clang not to add its predefines.
    args.push_back("-undef");

    // Configure language options to match given compiler.
    std::string const& pd = opts.Predefines;
    if (pd.find("#define _MSC_EXTENSIONS ") != pd.npos) {
      args.push_back("-fms-extensions");
    }
    if (const char* d = strstr(pd.c_str(), "#define _MSC_VER ")) {
      args.push_back("-fms-compatibility");
      // Extract the _MSC_VER value to give to -fmsc-version=.
      d += 17;
      if (const char* e = strchr(d, '\n')) {
        if (*(e - 1) == '\r') {
          --e;
        }
        std::string const msc_ver_str(d, e - d);
        fmsc_version += msc_ver_str;
        args.push_back(fmsc_version.c_str());

        if (!opts.HaveStd) {
          if (pd.find("#define __cplusplus ") != pd.npos) {
            // Extract the C++ level from _MSC_VER to give to -std=.
            // Note that Clang also does this but old versions of Clang
            // do not know about new versions of MSVC.
            errno = 0;
            long msc_ver = std::strtol(msc_ver_str.c_str(), nullptr, 10);
            if (errno != 0) {
              msc_ver = 1600;
            }
            if (msc_ver >= 1900) {
              args.push_back("-std=c++14");
            } else if (msc_ver >= 1600) {
              args.push_back("-std=c++11");
            } else {
              args.push_back("-std=c++98");
            }
          } else {
            args.push_back("-std=c89");
          }
        }
      }
    } else if (!opts.HaveStd) {
      // Check for GNU extensions.
      if (pd.find("#define __GNUC__ ") != pd.npos &&
          pd.find("#define __STRICT_ANSI__ ") == pd.npos) {
        std_flag += "gnu";
      } else {
        std_flag += "c";
      }

      if (const char* d = strstr(pd.c_str(), "#define __cplusplus ")) {
        // Extract the C++ level to give to -std=.  We do this above for
        // MSVC because it does not set __cplusplus to standard values.
        d += 20;
        if (const char* e = strchr(d, '\n')) {
          if (*(e - 1) == '\r') {
            --e;
          }

          // Add the standard year.
          std::string const std_date_str(d, e - d);
          errno = 0;
          long std_date = std::strtol(std_date_str.c_str(), nullptr, 10);
          if (errno != 0) {
            std_date = 0;
          }
          std_flag += "++";
          if (std_date >= 201406L) {
            std_flag += "1z";
          } else if (std_date >= 201402L) {
            std_flag += "14";
          } else if (std_date >= 201103L) {
            std_flag += "11";
          } else {
            std_flag += "98";
          }
          args.push_back(std_flag.c_str());
        }
      } else if (const char* d =
                   strstr(pd.c_str(), "#define __STDC_VERSION__ ")) {
        // Extract the C standard level.
        d += 25;
        if (const char* e = strchr(d, '\n')) {
          if (*(e - 1) == '\r') {
            --e;
          }
          std::string const std_date_str(d, e - d);
          errno = 0;
          long std_date = std::strtol(std_date_str.c_str(), nullptr, 10);
          if (errno != 0) {
            std_date = 0;
          }
          if (std_date >= 201112L) {
            std_flag += "11";
          } else if (std_date >= 199901L) {
            std_flag += "99";
          } else {
            std_flag += "89";
          }
          args.push_back(std_flag.c_str());
        }
      } else {
        // Assume C 89.
        std_flag += "89";
        args.push_back(std_flag.c_str());
      }
    }
  }

  return runClangImpl(args.data(), args.data() + args.size(), opts);
}
