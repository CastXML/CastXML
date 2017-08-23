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

#include "Output.h"
#include "Options.h"
#include "Utils.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclFriend.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclOpenMP.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/RecordLayout.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <vector>

class ASTVisitorBase
{
protected:
  clang::CompilerInstance& CI;
  clang::ASTContext const& CTX;
  llvm::raw_ostream& OS;

  ASTVisitorBase(clang::CompilerInstance& ci, clang::ASTContext const& ctx,
                 llvm::raw_ostream& os)
    : CI(ci)
    , CTX(ctx)
    , OS(os)
  {
  }

  // Represent cv qualifier state of one dump node.
  struct DumpQual
  {
  private:
    typedef void (DumpQual::*bool_type)() const;
    void bool_true() const {}
  public:
    bool IsConst;
    bool IsVolatile;
    bool IsRestrict;
    DumpQual()
      : IsConst(false)
      , IsVolatile(false)
      , IsRestrict(false)
    {
    }
    operator bool_type() const
    {
      return (this->IsConst || this->IsVolatile || this->IsRestrict)
        ? &DumpQual::bool_true
        : nullptr;
    }
    friend bool operator<(DumpQual const& l, DumpQual const& r)
    {
      if (!l.IsConst && r.IsConst) {
        return true;
      } else if (l.IsConst && !r.IsConst) {
        return false;
      } else if (!l.IsVolatile && r.IsVolatile) {
        return true;
      } else if (l.IsVolatile && !r.IsVolatile) {
        return false;
      } else if (!l.IsRestrict && r.IsRestrict) {
        return true;
      } else if (l.IsRestrict && !r.IsRestrict) {
        return false;
      } else {
        return false;
      }
    }
    friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                         DumpQual const& dq)
    {
      return os << (dq.IsConst ? "c" : "") << (dq.IsVolatile ? "v" : "")
                << (dq.IsRestrict ? "r" : "");
    }
  };

  // Represent id of one dump node.
  struct DumpId
  {
  private:
    typedef void (DumpId::*bool_type)() const;
    void bool_true() const {}
  public:
    unsigned int Id;
    DumpQual Qual;
    DumpId()
      : Id(0)
      , Qual()
    {
    }
    DumpId(unsigned int id, DumpQual dq)
      : Id(id)
      , Qual(dq)
    {
    }
    operator bool_type() const
    {
      return this->Id != 0 ? &DumpId::bool_true : nullptr;
    }
    friend bool operator<(DumpId const& l, DumpId const& r)
    {
      if (l.Id < r.Id) {
        return true;
      } else if (l.Id > r.Id) {
        return false;
      } else {
        return l.Qual < r.Qual;
      }
    }
    friend llvm::raw_ostream& operator<<(llvm::raw_ostream& os,
                                         DumpId const& id)
    {
      return os << id.Id << id.Qual;
    }
  };

  // Record status of one AST node to be dumped.
  struct DumpNode
  {
    DumpNode()
      : Index()
      , Complete(false)
    {
    }

    // Index in nodes ordered by first encounter.
    DumpId Index;

    // Whether the node is to be traversed completely.
    bool Complete;
  };

// Report all decl nodes as unimplemented until overridden.
#define ABSTRACT_DECL(DECL)
#define DECL(CLASS, BASE)                                                     \
  void Output##CLASS##Decl(clang::CLASS##Decl const* d, DumpNode const* dn)   \
  {                                                                           \
    this->OutputUnimplementedDecl(d, dn);                                     \
  }
#include "clang/AST/DeclNodes.inc"

  void OutputUnimplementedDecl(clang::Decl const* d, DumpNode const* dn)
  {
    /* clang-format off */
    this->OS << "  <Unimplemented id=\"_" << dn->Index
             << "\" kind=\"" << encodeXML(d->getDeclKindName()) << "\"/>\n";
    /* clang-format on */
  }

// Report all type nodes as unimplemented until overridden.
#define ABSTRACT_TYPE(CLASS, BASE)
#define TYPE(CLASS, BASE)                                                     \
  void Output##CLASS##Type(clang::CLASS##Type const* t, DumpNode const* dn)   \
  {                                                                           \
    this->OutputUnimplementedType(t, dn);                                     \
  }
#include "clang/AST/TypeNodes.def"

  void OutputUnimplementedType(clang::Type const* t, DumpNode const* dn)
  {
    /* clang-format off */
    this->OS << "  <Unimplemented id=\"_" << dn->Index
             << "\" type_class=\"" << encodeXML(t->getTypeClassName())
             << "\"/>\n";
    /* clang-format on */
  }
};

class ASTVisitor : public ASTVisitorBase
{
  // Store a type to be visited, possibly as a record member.
  struct DumpType
  {
    DumpType()
      : Type()
      , Class(0)
    {
    }
    DumpType(clang::QualType t, clang::Type const* c = 0)
      : Type(t)
      , Class(c)
    {
    }
    friend bool operator<(DumpType const& l, DumpType const& r)
    {
      // Order by pointer value without discarding low-order
      // bits used to encode qualifiers.
      void const* lpp = &l.Type;
      void const* rpp = &r.Type;
      void const* lpv = *static_cast<void const* const*>(lpp);
      void const* rpv = *static_cast<void const* const*>(rpp);
      if (lpv < rpv) {
        return true;
      } else if (lpv > rpv) {
        return false;
      } else {
        return l.Class < r.Class;
      }
    }

    clang::QualType Type;
    clang::Type const* Class;
  };

  // Store an entry in the node traversal queue.
  struct QueueEntry
  {
    // Available node kinds.
    enum Kinds
    {
      KindQual,
      KindDecl,
      KindType
    };

    QueueEntry(DumpNode const* dn)
      : Kind(KindQual)
      , Decl(nullptr)
      , Type()
      , DN(dn)
    {
    }
    QueueEntry(clang::Decl const* d, DumpNode const* dn)
      : Kind(KindDecl)
      , Decl(d)
      , Type()
      , DN(dn)
    {
    }
    QueueEntry(DumpType t, DumpNode const* dn)
      : Kind(KindType)
      , Decl(nullptr)
      , Type(t)
      , DN(dn)
    {
    }

    // Kind of node at this entry.
    Kinds Kind;

    // The declaration when Kind == KindDecl.
    clang::Decl const* Decl;

    // The type when Kind == KindType.
    DumpType Type;

    // The dump status for this node.
    DumpNode const* DN;

    friend bool operator<(QueueEntry const& l, QueueEntry const& r)
    {
      return l.DN->Index < r.DN->Index;
    }
  };

  class PrinterHelper : public clang::PrinterHelper
  {
    ASTVisitor& Visitor;

  public:
    PrinterHelper(ASTVisitor& v)
      : Visitor(v)
    {
    }
    bool handledStmt(clang::Stmt* s, llvm::raw_ostream& os) override
    {
      return this->Visitor.PrintHelpStmt(s, os);
    }
  };

  /** Get the dump status node for a Clang declaration.  */
  DumpNode* GetDumpNode(clang::Decl const* d) { return &this->DeclNodes[d]; }

  /** Get the dump status node for a Clang type.  */
  DumpNode* GetDumpNode(DumpType t) { return &this->TypeNodes[t]; }

  /** Get the dump status node for a qualified DumpId.  */
  DumpNode* GetDumpNode(DumpId id)
  {
    assert(id.Qual);
    return &this->QualNodes[id];
  }

  /** Allocate a dump node for a Clang declaration.  */
  DumpId AddDeclDumpNode(clang::Decl const* d, bool complete,
                         bool forType = false);
  DumpId AddDeclDumpNodeForType(clang::Decl const* d, bool complete,
                                DumpQual dq);

  /** Allocate a dump node for a Clang type.  */
  DumpId AddTypeDumpNode(DumpType dt, bool complete, DumpQual dq = DumpQual());

  /** Allocate a dump node for a qualified DumpId.  */
  DumpId AddQualDumpNode(DumpId id);

  /** Helper common to AddDeclDumpNode and AddTypeDumpNode.  */
  template <typename K>
  DumpId AddDumpNodeImpl(K k, bool complete);

  /** Allocate a dump node for a source file entry.  */
  unsigned int AddDumpFile(clang::FileEntry const* f);

  /** Add class template specializations and instantiations for output.  */
  void AddClassTemplateDecl(clang::ClassTemplateDecl const* d,
                            std::set<DumpId>* emitted = 0);

  /** Add function template specializations and instantiations for output.  */
  void AddFunctionTemplateDecl(clang::FunctionTemplateDecl const* d,
                               std::set<DumpId>* emitted = 0);

  /** Add declaration context members for output.  */
  void AddDeclContextMembers(clang::DeclContext const* dc,
                             std::set<DumpId>& emitted);

  /** Add a starting declaration for output.  */
  void AddStartDecl(clang::Decl const* d);

  /** Queue leftover nodes that do not need complete output.  */
  void QueueIncompleteDumpNodes();

  /** Traverse AST nodes until the queue is empty.  */
  void ProcessQueue();
  void ProcessFileQueue();

  /** Output start tags on top of xml file. */
  void OutputStartXMLTags();

  /** Output end tags. */
  void OutputEndXMLTags();

  /** Dispatch output of a declaration.  */
  void OutputDecl(clang::Decl const* d, DumpNode const* dn);

  /** Dispatch output of a qualified or unqualified type.  */
  void OutputType(DumpType dt, DumpNode const* dn);

  /** Output a qualified type.  */
  void OutputCvQualifiedType(DumpNode const* dn);

  /** Get the XML IDREF for the element defining the given
      declaration context (namespace, class, etc.).  */
  DumpId GetContextIdRef(clang::DeclContext const* dc);

  /** Return the unqualified name of the declaration context
      (class, struct, union) of the given method.  */
  std::string GetContextName(clang::CXXMethodDecl const* d);

  /** Print the XML IDREF value referencing the given type.
      If the type has top-level cv-qualifiers, they are
      appended to the numeric id as single characters (c=const,
      v=volatile, r=restrict) to reference the XML ID of
      a CvQualifiedType element describing the qualifiers
      and referencing the unqualified type.  */
  void PrintTypeIdRef(clang::QualType t, bool complete);

  /** Print an id="_<n>" XML unique ID attribute.  */
  void PrintIdAttribute(DumpNode const* dn);

  /** Print a name="..." attribute.  */
  void PrintNameAttribute(std::string const& name);

  /** Print a mangled="..." attribute.  */
  void PrintMangledAttribute(clang::NamedDecl const* d);

  /** Print an offset="..." attribute. */
  void PrintOffsetAttribute(unsigned int const& offset);

  /** Print size="..." and align="..." attributes. */
  void PrintABIAttributes(clang::TypeInfo const& t);
  void PrintABIAttributes(clang::TypeDecl const* d);

  /** Print a basetype="..." attribute with the XML IDREF for
      the given type.  Also queues the given type for later output.  */
  void PrintBaseTypeAttribute(clang::Type const* c, bool complete);

  /** Print a type="..." attribute with the XML IDREF for
      the given (possibly cv-qualified) type.  Also queues
      the given type for later output.  */
  void PrintTypeAttribute(clang::QualType t, bool complete);

  /** Print a returns="..." attribute with the XML IDREF for
      the given (possibly cv-qualified) type.  Also queue
      the given type for later output.  */
  void PrintReturnsAttribute(clang::QualType t, bool complete);

  /** Print the XML attributes location="fid:line" file="fid" line="line"
      for the given decl.  */
  void PrintLocationAttribute(clang::Decl const* d);

  /** Print a members="..." attribute listing the XML IDREFs for
      members of the given declaration context.  Also queues the
      context members for later output.  */
  void PrintMembersAttribute(clang::DeclContext const* dc);
  void PrintMembersAttribute(std::set<DumpId> const& emitted);

  /** Print a bases="..." attribute listing the XML IDREFs for
      bases of the given class type.  Also queues the base classes
      for later output.  */
  void PrintBasesAttribute(clang::CXXRecordDecl const* dx);

  /** Print an attributes="..." attribute listing the given attributes.  */
  void PrintAttributesAttribute(std::vector<std::string> const& attrs);

  /** Print an attributes="..." attribute listing the given
      declaration's attributes.  */
  void PrintAttributesAttribute(clang::Decl const* d);

  /** Get the attributes of the given function type.  */
  void GetFunctionTypeAttributes(clang::FunctionProtoType const* t,
                                 std::vector<std::string>& attrs);

  /** Get the attributes of the given declaration.  */
  void GetDeclAttributes(clang::Decl const* d,
                         std::vector<std::string>& attrs);

  /** Print a throw="..." attribute listing the XML IDREFs for
      the types that the given function prototype declares in
      the throw() specification.  */
  void PrintThrowsAttribute(clang::FunctionProtoType const* fpt,
                            bool complete);

  /** Print a befriending="..." attribute listing the XML IDREFs for
      friends of the given class.  Also queues the friends for later
      output.  */
  void PrintBefriendingAttribute(clang::CXXRecordDecl const* dx);

  /** Flags used by function output methods to pass information
      to the OutputFunctionHelper method.  */
  enum FunctionHelperFlags
  {
    FH_Returns = (1 << 0),
    FH_Static = (1 << 1),
    FH_Explicit = (1 << 2),
    FH_Const = (1 << 3),
    FH_Virtual = (1 << 4),
    FH_Pure = (1 << 5),
    FH__Last
  };

  /** Output a function element using the name and flags given by
      the caller.  This encompasses functionality common to all the
      function declaration output methods.  */
  void OutputFunctionHelper(clang::FunctionDecl const* d, DumpNode const* dn,
                            const char* tag, std::string const& name,
                            unsigned int flags);

  /** Output a function type element using the tag given by the caller.
      This encompasses functionality common to all the function type
      output methods.  */
  void OutputFunctionTypeHelper(clang::FunctionProtoType const* t,
                                DumpNode const* dn, const char* tag,
                                clang::Type const* c);

  /** Output an <Argument/> element inside a function element.  */
  void OutputFunctionArgument(clang::ParmVarDecl const* a, bool complete,
                              clang::Expr const* def);

  /** Print some statements (expressions) in a custom form.  */
  bool PrintHelpStmt(clang::Stmt const* s, llvm::raw_ostream& os);

  /** Print an access="..." attribute.  */
  void PrintAccessAttribute(clang::AccessSpecifier as);

  /** Print a context="..." attribute with the XML IDREF for
      the containing declaration context (namespace, class, etc.).
      Also prints access="..." attribute for class members to
      indicate public, protected, or private membership.  */
  void PrintContextAttribute(clang::Decl const* d,
                             clang::AccessSpecifier alt = clang::AS_none);

  void PrintFloat128Type(DumpNode const* dn);

  // Decl node output methods.
  void OutputTranslationUnitDecl(clang::TranslationUnitDecl const* d,
                                 DumpNode const* dn);
  void OutputNamespaceDecl(clang::NamespaceDecl const* d, DumpNode const* dn);
  void OutputRecordDecl(clang::RecordDecl const* d, DumpNode const* dn);
  void OutputCXXRecordDecl(clang::CXXRecordDecl const* d, DumpNode const* dn);
  void OutputClassTemplateSpecializationDecl(
    clang::ClassTemplateSpecializationDecl const* d, DumpNode const* dn);
  void OutputTypedefDecl(clang::TypedefDecl const* d, DumpNode const* dn);
  void OutputEnumDecl(clang::EnumDecl const* d, DumpNode const* dn);
  void OutputFieldDecl(clang::FieldDecl const* d, DumpNode const* dn);
  void OutputVarDecl(clang::VarDecl const* d, DumpNode const* dn);

  void OutputFunctionDecl(clang::FunctionDecl const* d, DumpNode const* dn);
  void OutputCXXMethodDecl(clang::CXXMethodDecl const* d, DumpNode const* dn);
  void OutputCXXConversionDecl(clang::CXXConversionDecl const* d,
                               DumpNode const* dn);
  void OutputCXXConstructorDecl(clang::CXXConstructorDecl const* d,
                                DumpNode const* dn);
  void OutputCXXDestructorDecl(clang::CXXDestructorDecl const* d,
                               DumpNode const* dn);

  // Type node output methods.
  void OutputBuiltinType(clang::BuiltinType const* t, DumpNode const* dn);
  void OutputConstantArrayType(clang::ConstantArrayType const* t,
                               DumpNode const* dn);
  void OutputIncompleteArrayType(clang::IncompleteArrayType const* t,
                                 DumpNode const* dn);
  void OutputFunctionProtoType(clang::FunctionProtoType const* t,
                               DumpNode const* dn);
  void OutputLValueReferenceType(clang::LValueReferenceType const* t,
                                 DumpNode const* dn);
  void OutputMemberPointerType(clang::MemberPointerType const* t,
                               DumpNode const* dn);
  void OutputMethodType(clang::FunctionProtoType const* t,
                        clang::Type const* c, DumpNode const* dn);
  void OutputOffsetType(clang::QualType t, clang::Type const* c,
                        DumpNode const* dn);
  void OutputPointerType(clang::PointerType const* t, DumpNode const* dn);
  void OutputElaboratedType(clang::ElaboratedType const* t,
                            DumpNode const* dn);

  /** Queue declarations matching given qualified name in given context.  */
  void LookupStart(clang::DeclContext const* dc, std::string const& name);

private:
  // List of starting declaration names.
  Options const& Opts;

  // Total number of nodes to be dumped.
  unsigned int NodeCount;

  // Total number of source files to be referenced.
  unsigned int FileCount;

  // Whether we need a File element for compiler builtins.
  bool FileBuiltin;

  // Whether we are in the complete or incomplete output step.
  bool RequireComplete;

  // Mangling context for target ABI.
  std::unique_ptr<clang::MangleContext> MangleContext;

  // Control declaration and type printing.
  clang::PrintingPolicy PrintingPolicy;

  // Map from clang AST declaration node to our dump status node.
  typedef std::map<clang::Decl const*, DumpNode> DeclNodesMap;
  DeclNodesMap DeclNodes;

  // Map from clang AST type node to our dump status node.
  typedef std::map<DumpType, DumpNode> TypeNodesMap;
  TypeNodesMap TypeNodes;

  // Map from qualified DumpId to our dump status node.
  typedef std::map<DumpId, DumpNode> QualNodesMap;
  QualNodesMap QualNodes;

  // Map from clang file entry to our source file index.
  typedef std::map<clang::FileEntry const*, unsigned int> FileNodesMap;
  FileNodesMap FileNodes;

  // Node traversal queue.
  std::set<QueueEntry> Queue;

  // File traversal queue.
  std::queue<clang::FileEntry const*> FileQueue;

public:
  ASTVisitor(clang::CompilerInstance& ci, clang::ASTContext& ctx,
             llvm::raw_ostream& os, Options const& opts)
    : ASTVisitorBase(ci, ctx, os)
    , Opts(opts)
    , NodeCount(0)
    , FileCount(0)
    , FileBuiltin(false)
    , RequireComplete(true)
    , MangleContext(ctx.createMangleContext())
    , PrintingPolicy(ctx.getPrintingPolicy())
  {
    this->PrintingPolicy.SuppressUnwrittenScope = true;
  }

  /** Visit declarations in the given translation unit.
      This is the main entry point.  */
  void HandleTranslationUnit(clang::TranslationUnitDecl const* tu);
};

ASTVisitor::DumpId ASTVisitor::AddDeclDumpNode(clang::Decl const* d,
                                               bool complete, bool forType)
{
  // Select the definition or canonical declaration.
  d = d->getCanonicalDecl();
  if (clang::RecordDecl const* rd = clang::dyn_cast<clang::RecordDecl>(d)) {
    if (clang::RecordDecl const* rdd = rd->getDefinition()) {
      d = rdd;
    }
  }

  // Replace some decls with those they reference.
  switch (d->getKind()) {
    case clang::Decl::UsingShadow:
      return this->AddDeclDumpNode(
        static_cast<clang::UsingShadowDecl const*>(d)->getTargetDecl(),
        complete, forType);
    case clang::Decl::LinkageSpec: {
      clang::DeclContext const* dc =
        static_cast<clang::LinkageSpecDecl const*>(d)->getDeclContext();
      return this->AddDeclDumpNode(clang::Decl::castFromDeclContext(dc),
                                   complete, forType);
    } break;
    default:
      break;
  }

  // Skip invalid declarations that are not needed for a type element.
  if (d->isInvalidDecl() && !forType) {
    return DumpId();
  }

  // Skip C++11 declarations gccxml does not support.
  if (this->Opts.GccXml || this->Opts.CastXml) {
    if (clang::FunctionDecl const* fd =
          clang::dyn_cast<clang::FunctionDecl>(d)) {
      if (fd->isDeleted()) {
        return DumpId();
      }

      if (fd->getLiteralIdentifier()) {
        return DumpId();
      }

      if (clang::FunctionProtoType const* fpt =
            fd->getType()->getAs<clang::FunctionProtoType>()) {
        if (fpt->getReturnType()->isRValueReferenceType()) {
          return DumpId();
        }
        for (clang::FunctionProtoType::param_type_iterator
               i = fpt->param_type_begin(),
               e = fpt->param_type_end();
             i != e; ++i) {
          if ((*i)->isRValueReferenceType()) {
            return DumpId();
          }
        }
      }
    }

    if (clang::dyn_cast<clang::TypeAliasDecl>(d)) {
      return DumpId();
    }

    if (clang::dyn_cast<clang::TypeAliasTemplateDecl>(d)) {
      return DumpId();
    }

    if (clang::TypedefDecl const* td =
          clang::dyn_cast<clang::TypedefDecl>(d)) {
      if (td->getUnderlyingType()->isRValueReferenceType()) {
        return DumpId();
      }
    }
  }

  return this->AddDumpNodeImpl(d, complete);
}

ASTVisitor::DumpId ASTVisitor::AddDeclDumpNodeForType(clang::Decl const* d,
                                                      bool complete,
                                                      DumpQual dq)
{
  // Get the id for the canonical decl.
  DumpId id = this->AddDeclDumpNode(d, complete, true);

  // If any qualifiers were collected through layers of desugaring
  // then get the id of the qualified type referencing this decl.
  if (id && dq) {
    id = this->AddQualDumpNode(DumpId(id.Id, dq));
  }

  return id;
}

ASTVisitor::DumpId ASTVisitor::AddTypeDumpNode(DumpType dt, bool complete,
                                               DumpQual dq)
{
  clang::QualType t = dt.Type;
  clang::Type const* c = dt.Class;

  // Extract local qualifiers and recurse with locally unqualified type.
  if (t.hasLocalQualifiers()) {
    dq.IsConst = dq.IsConst || t.isLocalConstQualified();
    dq.IsVolatile = dq.IsVolatile || t.isLocalVolatileQualified();
    dq.IsRestrict = dq.IsRestrict || t.isLocalRestrictQualified();
    return this->AddTypeDumpNode(DumpType(t.getLocalUnqualifiedType(), c),
                                 complete, dq);
  }

  // Replace some types with their decls.
  switch (t->getTypeClass()) {
    case clang::Type::Adjusted:
      return this->AddTypeDumpNode(
        DumpType(t->getAs<clang::AdjustedType>()->getAdjustedType(), c),
        complete, dq);
    case clang::Type::Attributed:
      return this->AddTypeDumpNode(
        DumpType(t->getAs<clang::AttributedType>()->getEquivalentType(), c),
        complete, dq);
    case clang::Type::Decayed:
      return this->AddTypeDumpNode(
        DumpType(t->getAs<clang::DecayedType>()->getDecayedType(), c),
        complete, dq);
    case clang::Type::Elaborated:
      if (this->Opts.GccXml || !t->isElaboratedTypeSpecifier()) {
        return this->AddTypeDumpNode(
          DumpType(t->getAs<clang::ElaboratedType>()->getNamedType(), c),
          complete, dq);
      }
      break;
    case clang::Type::Enum:
      return this->AddDeclDumpNodeForType(
        t->getAs<clang::EnumType>()->getDecl(), complete, dq);
    case clang::Type::Paren:
      return this->AddTypeDumpNode(
        DumpType(t->getAs<clang::ParenType>()->getInnerType(), c), complete,
        dq);
    case clang::Type::Record:
      return this->AddDeclDumpNodeForType(
        t->getAs<clang::RecordType>()->getDecl(), complete, dq);
    case clang::Type::SubstTemplateTypeParm:
      return this->AddTypeDumpNode(
        DumpType(
          t->getAs<clang::SubstTemplateTypeParmType>()->getReplacementType(),
          c),
        complete, dq);
    case clang::Type::TemplateSpecialization: {
      clang::TemplateSpecializationType const* tst =
        t->getAs<clang::TemplateSpecializationType>();
      if (tst->isSugared()) {
        return this->AddTypeDumpNode(DumpType(tst->desugar(), c), complete,
                                     dq);
      }
    } break;
    case clang::Type::Typedef: {
      clang::TypedefType const* tdt = t->getAs<clang::TypedefType>();
      if (!tdt->isInstantiationDependentType() && tdt->isSugared()) {
        // Make sure all containing contexts are not templates.
        clang::Decl const* d = tdt->getDecl();
        while (clang::DeclContext const* tdc = d->getDeclContext()) {
          if (clang::CXXRecordDecl const* tdx =
                clang::dyn_cast<clang::CXXRecordDecl>(tdc)) {
            d = tdx;
            if (tdx->getDescribedClassTemplate() ||
                clang::isa<clang::ClassTemplatePartialSpecializationDecl>(
                  tdx)) {
              // This TypedefType refers to a non-dependent
              // TypedefDecl member of a class template.  Since gccxml
              // format does not include uninstantiated templates we
              // must use the desugared type so that we do not end up
              // referencing a class template as context.
              return this->AddTypeDumpNode(tdt->desugar(), complete, dq);
            }
          } else {
            break;
          }
        }
      }
      return this->AddDeclDumpNodeForType(tdt->getDecl(), complete, dq);
    } break;
    default:
      break;
  }

  // Get the id for the fully desugared, unqualified type.
  DumpId id = this->AddDumpNodeImpl(dt, complete);

  // If any qualifiers were collected through layers of desugaring
  // then get the id of the qualified type.
  if (id && dq) {
    id = this->AddQualDumpNode(DumpId(id.Id, dq));
  }

  return id;
}

ASTVisitor::DumpId ASTVisitor::AddQualDumpNode(DumpId id)
{
  DumpNode* dn = this->GetDumpNode(id);
  if (!dn->Index) {
    dn->Index = id;
    // Always treat CvQualifiedType nodes as complete.
    dn->Complete = true;
    this->Queue.insert(QueueEntry(dn));
  }
  return dn->Index;
}

template <typename K>
ASTVisitor::DumpId ASTVisitor::AddDumpNodeImpl(K k, bool complete)
{
  // Update an existing node or add one.
  DumpNode* dn = this->GetDumpNode(k);
  if (dn->Index) {
    // Node was already encountered.  See if it is now complete.
    if (complete && !dn->Complete) {
      // Node is now complete, but wasn't before.  Queue it.
      dn->Complete = true;
      this->Queue.insert(QueueEntry(k, dn));
    }
  } else {
    // This is a new node.  Assign it an index.
    dn->Index.Id = ++this->NodeCount;
    dn->Complete = complete;
    if (complete || !this->RequireComplete) {
      // Node is complete.  Queue it.
      this->Queue.insert(QueueEntry(k, dn));
    }
  }
  // Return node's index.
  return dn->Index;
}

unsigned int ASTVisitor::AddDumpFile(clang::FileEntry const* f)
{
  unsigned int& index = this->FileNodes[f];
  if (index == 0) {
    index = ++this->FileCount;
    this->FileQueue.push(f);
  }
  return index;
}

void ASTVisitor::AddClassTemplateDecl(clang::ClassTemplateDecl const* d,
                                      std::set<DumpId>* emitted)
{
  // Queue all the instantiations of this class template.
  for (clang::ClassTemplateDecl::spec_iterator i = d->spec_begin(),
                                               e = d->spec_end();
       i != e; ++i) {
    clang::CXXRecordDecl const* rd = *i;
    DumpId id = this->AddDeclDumpNode(rd, true);
    if (id && emitted) {
      emitted->insert(id);
    }
  }
}

void ASTVisitor::AddFunctionTemplateDecl(clang::FunctionTemplateDecl const* d,
                                         std::set<DumpId>* emitted)
{
  // Queue all the instantiations of this function template.
  for (clang::FunctionTemplateDecl::spec_iterator i = d->spec_begin(),
                                                  e = d->spec_end();
       i != e; ++i) {
    clang::FunctionDecl const* fd = *i;
    DumpId id = this->AddDeclDumpNode(fd, true);
    if (id && emitted) {
      emitted->insert(id);
    }
  }
}

void ASTVisitor::AddDeclContextMembers(clang::DeclContext const* dc,
                                       std::set<DumpId>& emitted)
{
  bool const isTranslationUnit = clang::isa<clang::TranslationUnitDecl>(dc);

  for (clang::DeclContext::decl_iterator i = dc->decls_begin(),
                                         e = dc->decls_end();
       i != e; ++i) {
    clang::Decl const* d = *i;

    // Skip declarations that are not really members of this context.
    if (d->getDeclContext() != dc) {
      continue;
    }

    // Skip declarations that we use internally as builtins.
    if (isTranslationUnit) {
      if (clang::NamedDecl const* nd = clang::dyn_cast<clang::NamedDecl>(d)) {
        if (clang::IdentifierInfo const* ii = nd->getIdentifier()) {
          if (ii->getName().find("__castxml") != std::string::npos) {
            continue;
          }
        }
      }
    }

    // Ignore certain members.
    switch (d->getKind()) {
      case clang::Decl::CXXRecord: {
        clang::CXXRecordDecl const* rd =
          static_cast<clang::CXXRecordDecl const*>(d);
        if (rd->isInjectedClassName()) {
          continue;
        }
      } break;
      case clang::Decl::AccessSpec: {
        continue;
      } break;
      case clang::Decl::ClassTemplate: {
        this->AddClassTemplateDecl(
          static_cast<clang::ClassTemplateDecl const*>(d), &emitted);
        continue;
      } break;
      case clang::Decl::ClassTemplatePartialSpecialization: {
        continue;
      } break;
      case clang::Decl::Empty: {
        continue;
      } break;
      case clang::Decl::Friend: {
        continue;
      } break;
      case clang::Decl::FunctionTemplate: {
        this->AddFunctionTemplateDecl(
          static_cast<clang::FunctionTemplateDecl const*>(d), &emitted);
        continue;
      } break;
      case clang::Decl::LinkageSpec: {
        this->AddDeclContextMembers(
          static_cast<clang::LinkageSpecDecl const*>(d), emitted);
        continue;
      } break;
      case clang::Decl::Namespace: {
        clang::NamespaceDecl const* nd =
          static_cast<clang::NamespaceDecl const*>(d);
        if (nd->isInline()) {
          this->AddDeclContextMembers(nd, emitted);
          continue;
        }
      } break;
      case clang::Decl::Using: {
        continue;
      } break;
      case clang::Decl::UsingDirective: {
        continue;
      } break;
      default:
        break;
    }

    // Queue this decl and print its id.
    if (DumpId id = this->AddDeclDumpNode(d, true)) {
      emitted.insert(id);
    }
  }
}

void ASTVisitor::AddStartDecl(clang::Decl const* d)
{
  switch (d->getKind()) {
    case clang::Decl::ClassTemplate:
      this->AddClassTemplateDecl(
        static_cast<clang::ClassTemplateDecl const*>(d));
      break;
    case clang::Decl::FunctionTemplate:
      this->AddFunctionTemplateDecl(
        static_cast<clang::FunctionTemplateDecl const*>(d));
      break;
    case clang::Decl::Namespace: {
      if (!static_cast<clang::NamespaceDecl const*>(d)->isInline()) {
        this->AddDeclDumpNode(d, true);
      }
    } break;
    case clang::Decl::Using: {
      clang::UsingDecl const* ud = static_cast<clang::UsingDecl const*>(d);
      for (clang::UsingDecl::shadow_iterator i = ud->shadow_begin(),
                                             e = ud->shadow_end();
           i != e; ++i) {
        this->AddDeclDumpNode(*i, true);
      }
    } break;
    default:
      this->AddDeclDumpNode(d, true);
      break;
  }
}

void ASTVisitor::QueueIncompleteDumpNodes()
{
  // Queue declaration nodes that do not need complete output.
  for (DeclNodesMap::const_iterator i = this->DeclNodes.begin(),
                                    e = this->DeclNodes.end();
       i != e; ++i) {
    if (!i->second.Complete) {
      this->Queue.insert(QueueEntry(i->first, &i->second));
    }
  }

  // Queue type nodes that do not need complete output.
  for (TypeNodesMap::const_iterator i = this->TypeNodes.begin(),
                                    e = this->TypeNodes.end();
       i != e; ++i) {
    if (!i->second.Complete) {
      this->Queue.insert(QueueEntry(i->first, &i->second));
    }
  }
}

void ASTVisitor::ProcessQueue()
{
  // Dispatch each entry in the queue based on its node kind.
  while (!this->Queue.empty()) {
    QueueEntry qe = *this->Queue.begin();
    this->Queue.erase(this->Queue.begin());
    switch (qe.Kind) {
      case QueueEntry::KindQual:
        this->OutputCvQualifiedType(qe.DN);
        break;
      case QueueEntry::KindDecl:
        this->OutputDecl(qe.Decl, qe.DN);
        break;
      case QueueEntry::KindType:
        this->OutputType(qe.Type, qe.DN);
        break;
    }
  }
}

void ASTVisitor::ProcessFileQueue()
{
  if (this->FileBuiltin) {
    /* clang-format off */
    this->OS <<
      "  <File id=\"f0\" name=\"" << encodeXML("<builtin>") << "\"/>\n"
      ;
    /* clang-format on */
  }
  while (!this->FileQueue.empty()) {
    clang::FileEntry const* f = this->FileQueue.front();
    this->FileQueue.pop();
    /* clang-format off */
    this->OS <<
      "  <File"
      " id=\"f" << this->FileNodes[f] << "\""
      " name=\"" << encodeXML(f->getName()) << "\""
      "/>\n"
      ;
    /* clang-format on */
  }
}

void ASTVisitor::OutputDecl(clang::Decl const* d, DumpNode const* dn)
{
  // Dispatch output of the declaration.
  switch (d->getKind()) {
#define ABSTRACT_DECL(DECL)
#define DECL(CLASS, BASE)                                                     \
  case clang::Decl::CLASS:                                                    \
    this->Output##CLASS##Decl(static_cast<clang::CLASS##Decl const*>(d), dn); \
    break;
#include "clang/AST/DeclNodes.inc"
  }
}

void ASTVisitor::OutputType(DumpType dt, DumpNode const* dn)
{
  clang::QualType t = dt.Type;
  clang::Type const* c = dt.Class;

  if (c) {
    // Output the method type.
    this->OutputMethodType(t->getAs<clang::FunctionProtoType>(), c, dn);
  } else {
    // Dispatch output of the unqualified type.
    switch (t->getTypeClass()) {
#define ABSTRACT_TYPE(CLASS, BASE)
#define TYPE(CLASS, BASE)                                                     \
  case clang::Type::CLASS:                                                    \
    this->Output##CLASS##Type(                                                \
      static_cast<clang::CLASS##Type const*>(t.getTypePtr()), dn);            \
    break;
#include "clang/AST/TypeNodes.def"
    }
  }
}

void ASTVisitor::OutputCvQualifiedType(DumpNode const* dn)
{
  DumpId id = dn->Index;

  // Create a special CvQualifiedType element to hold top-level
  // cv-qualifiers for a real type node.
  this->OS << "  <CvQualifiedType id=\"_" << id << "\"";

  // Refer to the unqualified type.
  this->OS << " type=\"_" << id.Id << "\"";

  // Add the cv-qualification attributes.
  if (id.Qual.IsConst) {
    this->OS << " const=\"1\"";
  }
  if (id.Qual.IsVolatile) {
    this->OS << " volatile=\"1\"";
  }
  if (id.Qual.IsRestrict) {
    this->OS << " restrict=\"1\"";
  }
  this->OS << "/>\n";
}

ASTVisitor::DumpId ASTVisitor::GetContextIdRef(clang::DeclContext const* dc)
{
  while (dc->isInlineNamespace()) {
    dc = dc->getParent();
  }

  if (clang::Decl const* d = clang::dyn_cast<clang::Decl>(dc)) {
    return this->AddDeclDumpNode(d, false);
  } else {
    return DumpId();
  }
}

std::string ASTVisitor::GetContextName(clang::CXXMethodDecl const* d)
{
  clang::DeclContext const* dc = d->getDeclContext();
  if (clang::RecordDecl const* rd = clang::dyn_cast<clang::RecordDecl>(dc)) {
    return rd->getName().str();
  }
  return "";
}

void ASTVisitor::PrintTypeIdRef(clang::QualType t, bool complete)
{
  // Add the type node.
  DumpId id = this->AddTypeDumpNode(t, complete);

  // Print the reference.
  this->OS << "_" << id;
}

void ASTVisitor::PrintIdAttribute(DumpNode const* dn)
{
  this->OS << " id=\"_" << dn->Index << "\"";
}

void ASTVisitor::PrintNameAttribute(std::string const& name)
{
  std::string n = stringReplace(name, "__castxml__float128_s", "__float128");
  this->OS << " name=\"" << encodeXML(n) << "\"";
}

void ASTVisitor::PrintMangledAttribute(clang::NamedDecl const* d)
{
  // Compute the mangled name.
  std::string s;
  {
    llvm::raw_string_ostream rso(s);
    this->MangleContext->mangleName(d, rso);
  }

  // We cannot mangle __float128 correctly because Clang does not have
  // it as an internal type, so skip mangled attributes involving it.
  if (s.find("__float128") != s.npos) {
    s = "";
  }

  // Strip a leading 1 byte in MS mangling.
  if (!s.empty() && s[0] == '\1') {
    s = s.substr(1);
  }

  this->OS << " mangled=\"" << encodeXML(s) << "\"";
}

void ASTVisitor::PrintOffsetAttribute(unsigned int const& offset)
{
  this->OS << " offset=\"" << offset << "\"";
}

void ASTVisitor::PrintABIAttributes(clang::TypeDecl const* d)
{
  if (clang::TypeDecl const* td = clang::dyn_cast<clang::TypeDecl>(d)) {
    clang::Type const* ty = td->getTypeForDecl();
    if (!ty->isIncompleteType()) {
      this->PrintABIAttributes(this->CTX.getTypeInfo(ty));
    }
  }
}

void ASTVisitor::PrintABIAttributes(clang::TypeInfo const& t)
{
  this->OS << " size=\"" << t.Width << "\"";
  this->OS << " align=\"" << t.Align << "\"";
}

void ASTVisitor::PrintBaseTypeAttribute(clang::Type const* c, bool complete)
{
  this->OS << " basetype=\"";
  this->PrintTypeIdRef(clang::QualType(c, 0), complete);
  this->OS << "\"";
}

void ASTVisitor::PrintTypeAttribute(clang::QualType t, bool complete)
{
  this->OS << " type=\"";
  this->PrintTypeIdRef(t, complete);
  this->OS << "\"";
}

void ASTVisitor::PrintReturnsAttribute(clang::QualType t, bool complete)
{
  this->OS << " returns=\"";
  this->PrintTypeIdRef(t, complete);
  this->OS << "\"";
}

void ASTVisitor::PrintLocationAttribute(clang::Decl const* d)
{
  clang::SourceLocation sl = d->getLocation();
  if (sl.isValid()) {
    clang::FullSourceLoc fsl = this->CTX.getFullLoc(sl).getExpansionLoc();
    if (clang::FileEntry const* f =
          this->CI.getSourceManager().getFileEntryForID(fsl.getFileID())) {
      unsigned int id = this->AddDumpFile(f);
      unsigned int line = fsl.getExpansionLineNumber();
      /* clang-format off */
      this->OS <<
        " location=\"f" << id << ":" << line << "\""
        " file=\"f" << id << "\""
        " line=\"" << line << "\"";
      /* clang-format on */
      return;
    }
  }
  if (d->isImplicit()) {
    this->FileBuiltin = true;
    this->OS << " location=\"f0:0\" file=\"f0\" line=\"0\"";
  }
}

bool ASTVisitor::PrintHelpStmt(clang::Stmt const* s, llvm::raw_ostream& os)
{
  switch (s->getStmtClass()) {
    case clang::Stmt::CStyleCastExprClass: {
      // Duplicate clang::StmtPrinter::VisitCStyleCastExpr
      // but with canonical type so we do not print an unqualified name.
      clang::CStyleCastExpr const* e =
        static_cast<clang::CStyleCastExpr const*>(s);
      os << "(";
      e->getTypeAsWritten().getCanonicalType().print(os, this->PrintingPolicy);
      os << ")";
      PrinterHelper ph(*this);
      e->getSubExpr()->printPretty(os, &ph, this->PrintingPolicy);
      return true;
    } break;
    case clang::Stmt::CXXConstCastExprClass:       // fallthrough
    case clang::Stmt::CXXDynamicCastExprClass:     // fallthrough
    case clang::Stmt::CXXReinterpretCastExprClass: // fallthrough
    case clang::Stmt::CXXStaticCastExprClass: {
      // Duplicate clang::StmtPrinter::VisitCXXNamedCastExpr
      // but with canonical type so we do not print an unqualified name.
      clang::CXXNamedCastExpr const* e =
        static_cast<clang::CXXNamedCastExpr const*>(s);
      os << e->getCastName() << '<';
      e->getTypeAsWritten().getCanonicalType().print(os, this->PrintingPolicy);
      os << ">(";
      PrinterHelper ph(*this);
      e->getSubExpr()->printPretty(os, &ph, this->PrintingPolicy);
      os << ")";
      return true;
    } break;
    case clang::Stmt::DeclRefExprClass: {
      // Print the fully qualified name of the referenced declaration.
      clang::DeclRefExpr const* e = static_cast<clang::DeclRefExpr const*>(s);
      if (clang::NamedDecl const* d =
            clang::dyn_cast<clang::NamedDecl>(e->getDecl())) {
        std::string s;
        {
          llvm::raw_string_ostream rso(s);
          d->printQualifiedName(rso, this->PrintingPolicy);
          rso.str();
        }
        if (clang::isa<clang::EnumConstantDecl>(d)) {
          // Clang does not exclude the "::" after an unnamed enum type.
          std::string::size_type pos = s.find("::::");
          if (pos != s.npos) {
            s.erase(pos, 2);
          }
        }
        os << s;
        return true;
      }
    } break;
    default:
      break;
  }
  return false;
}

void ASTVisitor::PrintAccessAttribute(clang::AccessSpecifier as)
{
  switch (as) {
    case clang::AS_private:
      this->OS << " access=\"private\"";
      break;
    case clang::AS_protected:
      this->OS << " access=\"protected\"";
      break;
    case clang::AS_public:
      this->OS << " access=\"public\"";
      break;
    case clang::AS_none:
      break;
  }
}

void ASTVisitor::PrintContextAttribute(clang::Decl const* d,
                                       clang::AccessSpecifier alt)
{
  clang::DeclContext const* dc = d->getDeclContext();
  if (DumpId id = this->GetContextIdRef(dc)) {
    this->OS << " context=\"_" << id << "\"";
    if (dc->isRecord()) {
      clang::AccessSpecifier as = d->getAccess();
      this->PrintAccessAttribute(as != clang::AS_none ? as : alt);
    }
  }
}

void ASTVisitor::PrintMembersAttribute(clang::DeclContext const* dc)
{
  std::set<DumpId> emitted;
  this->AddDeclContextMembers(dc, emitted);
  this->PrintMembersAttribute(emitted);
}

void ASTVisitor::PrintMembersAttribute(std::set<DumpId> const& emitted)
{
  if (!emitted.empty()) {
    this->OS << " members=\"";
    const char* sep = "";
    for (std::set<DumpId>::const_iterator i = emitted.begin(),
                                          e = emitted.end();
         i != e; ++i) {
      this->OS << sep << "_" << *i;
      sep = " ";
    }
    this->OS << "\"";
  }
}

void ASTVisitor::PrintBasesAttribute(clang::CXXRecordDecl const* dx)
{
  this->OS << " bases=\"";
  const char* sep = "";
  for (clang::CXXRecordDecl::base_class_const_iterator i = dx->bases_begin(),
                                                       e = dx->bases_end();
       i != e; ++i) {
    this->OS << sep;
    sep = " ";
    switch (i->getAccessSpecifier()) {
      case clang::AS_private:
        this->OS << "private:";
        break;
      case clang::AS_protected:
        this->OS << "protected:";
        break;
      default:
        break;
    }
    this->PrintTypeIdRef(i->getType().getCanonicalType(), true);
  }
  this->OS << "\"";
}

void ASTVisitor::PrintAttributesAttribute(
  std::vector<std::string> const& attrs)
{
  if (attrs.empty()) {
    return;
  }
  this->OS << " attributes=\"";
  const char* sep = "";
  for (std::string const& a : attrs) {
    this->OS << sep << encodeXML(a);
    sep = " ";
  }
  this->OS << "\"";
}

void ASTVisitor::PrintAttributesAttribute(clang::Decl const* d)
{
  std::vector<std::string> attributes;
  this->GetDeclAttributes(d, attributes);
  this->PrintAttributesAttribute(attributes);
}

void ASTVisitor::GetFunctionTypeAttributes(clang::FunctionProtoType const* t,
                                           std::vector<std::string>& attrs)
{
  switch (t->getExtInfo().getCC()) {
    case clang::CallingConv::CC_C:
      break;
    case clang::CallingConv::CC_X86StdCall:
      attrs.push_back("__stdcall__");
      break;
    case clang::CallingConv::CC_X86FastCall:
      attrs.push_back("__fastcall__");
      break;
    case clang::CallingConv::CC_X86ThisCall:
      attrs.push_back("__thiscall__");
      break;
    default:
      break;
  }
}

void ASTVisitor::GetDeclAttributes(clang::Decl const* d,
                                   std::vector<std::string>& attrs)
{
  for (auto const* a : d->specific_attrs<clang::AnnotateAttr>()) {
    attrs.push_back("annotate(" + a->getAnnotation().str() + ")");
  }
}

void ASTVisitor::PrintThrowsAttribute(clang::FunctionProtoType const* fpt,
                                      bool complete)
{
  if (fpt && fpt->hasDynamicExceptionSpec()) {
    clang::FunctionProtoType::exception_iterator i = fpt->exception_begin();
    clang::FunctionProtoType::exception_iterator e = fpt->exception_end();
    this->OS << " throw=\"";
    const char* sep = "";
    for (; i != e; ++i) {
      this->OS << sep;
      this->PrintTypeIdRef(*i, complete);
      sep = " ";
    }
    this->OS << "\"";
  }
}

void ASTVisitor::PrintBefriendingAttribute(clang::CXXRecordDecl const* dx)
{
  if (dx && dx->hasFriends()) {
    this->OS << " befriending=\"";
    const char* sep = "";
    for (clang::CXXRecordDecl::friend_iterator i = dx->friend_begin(),
                                               e = dx->friend_end();
         i != e; ++i) {
      clang::FriendDecl const* fd = *i;
      if (clang::NamedDecl const* nd = fd->getFriendDecl()) {
        if (nd->isTemplateDecl()) {
          // gccxml output format does not have uninstantiated templates
          continue;
        }

        if (DumpId id = this->AddDeclDumpNode(nd, false)) {
          this->OS << sep << "_" << id;
          sep = " ";
        }
      } else if (clang::TypeSourceInfo const* tsi = fd->getFriendType()) {
        this->OS << sep;
        this->PrintTypeIdRef(tsi->getType(), false);
        sep = " ";
      }
    }
    this->OS << "\"";
  }
}

void ASTVisitor::PrintFloat128Type(DumpNode const* dn)
{
  this->OS << "  <FundamentalType";
  this->PrintIdAttribute(dn);
  this->OS << " name=\"__float128\" size=\"128\" align=\"128\"/>\n";
}

void ASTVisitor::OutputFunctionHelper(clang::FunctionDecl const* d,
                                      DumpNode const* dn, const char* tag,
                                      std::string const& name,
                                      unsigned int flags)
{
  this->OS << "  <" << tag;
  this->PrintIdAttribute(dn);
  if (!name.empty()) {
    this->PrintNameAttribute(name);
  }
  if (flags & FH_Returns) {
    this->PrintReturnsAttribute(d->getReturnType(), dn->Complete);
  }
  this->PrintContextAttribute(d);
  this->PrintLocationAttribute(d);

  if (flags & FH_Static) {
    this->OS << " static=\"1\"";
  }
  if (flags & FH_Explicit) {
    this->OS << " explicit=\"1\"";
  }
  if (flags & FH_Const) {
    this->OS << " const=\"1\"";
  }
  if (flags & FH_Virtual) {
    this->OS << " virtual=\"1\"";
  }
  if (flags & FH_Pure) {
    this->OS << " pure_virtual=\"1\"";
  }
  if (d->isInlined()) {
    this->OS << " inline=\"1\"";
  }
  if (d->getStorageClass() == clang::SC_Extern) {
    this->OS << " extern=\"1\"";
  }
  if (d->isImplicit()) {
    this->OS << " artificial=\"1\"";
  }

  if (clang::CXXMethodDecl const* md =
        clang::dyn_cast<clang::CXXMethodDecl>(d)) {
    if (md->size_overridden_methods() > 0) {
      this->OS << " overrides=\"";
      const char* sep = "";
      for (clang::CXXMethodDecl::method_iterator
             i = md->begin_overridden_methods(),
             e = md->end_overridden_methods();
           i != e; ++i) {
        if (DumpId id = this->AddDeclDumpNode(*i, false)) {
          this->OS << sep << "_" << id;
          sep = " ";
        }
      }
      this->OS << "\"";
    }
  }

  std::vector<std::string> attributes;

  if (clang::FunctionProtoType const* fpt =
        d->getType()->getAs<clang::FunctionProtoType>()) {
    this->PrintThrowsAttribute(fpt, dn->Complete);
    if (!clang::isa<clang::CXXConstructorDecl>(d) &&
        !clang::isa<clang::CXXDestructorDecl>(d)) {
      this->PrintMangledAttribute(d);
    }
    this->GetFunctionTypeAttributes(fpt, attributes);
  }

  this->GetDeclAttributes(d, attributes);
  this->PrintAttributesAttribute(attributes);

  if (unsigned np = d->getNumParams()) {
    this->OS << ">\n";
    for (unsigned i = 0; i < np; ++i) {
      // Use the default argument from the most recent declaration.
      // Clang accumulates the defaults and only the last one has
      // them all.
      clang::ParmVarDecl const* pd = d->getMostRecentDecl()->getParamDecl(i);
      clang::Expr const* def = pd->getInit();
      if (!def && pd->hasUninstantiatedDefaultArg()) {
        def = pd->getUninstantiatedDefaultArg();
      }

      // Use the parameter located in the canonical declaration.
      this->OutputFunctionArgument(d->getParamDecl(i), dn->Complete, def);
    }
    if (d->isVariadic()) {
      this->OS << "    <Ellipsis/>\n";
    }
    this->OS << "  </" << tag << ">\n";
  } else {
    this->OS << "/>\n";
  }
}

void ASTVisitor::OutputFunctionTypeHelper(clang::FunctionProtoType const* t,
                                          DumpNode const* dn, const char* tag,
                                          clang::Type const* c)
{
  this->OS << "  <" << tag;
  this->PrintIdAttribute(dn);
  if (c) {
    this->PrintBaseTypeAttribute(c, dn->Complete);
  }
  this->PrintReturnsAttribute(t->getReturnType(), dn->Complete);
  if (t->isConst()) {
    this->OS << " const=\"1\"";
  }
  if (t->isVolatile()) {
    this->OS << " volatile=\"1\"";
  }
  if (t->isRestrict()) {
    this->OS << " restrict=\"1\"";
  }
  std::vector<std::string> attributes;
  this->GetFunctionTypeAttributes(t, attributes);
  this->PrintAttributesAttribute(attributes);
  if (t->param_type_begin() != t->param_type_end()) {
    this->OS << ">\n";
    for (clang::FunctionProtoType::param_type_iterator
           i = t->param_type_begin(),
           e = t->param_type_end();
         i != e; ++i) {
      this->OS << "    <Argument";
      this->PrintTypeAttribute(*i, dn->Complete);
      this->OS << "/>\n";
    }
    if (t->isVariadic()) {
      this->OS << "    <Ellipsis/>\n";
    }
    this->OS << "  </" << tag << ">\n";
  } else {
    this->OS << "/>\n";
  }
}

void ASTVisitor::OutputFunctionArgument(clang::ParmVarDecl const* a,
                                        bool complete, clang::Expr const* def)
{
  this->OS << "    <Argument";
  std::string name = a->getName().str();
  if (!name.empty()) {
    this->PrintNameAttribute(name);
  }
  this->PrintTypeAttribute(a->getType(), complete);
  this->PrintLocationAttribute(a);
  if (def) {
    this->OS << " default=\"";
    std::string s;
    llvm::raw_string_ostream rso(s);
    PrinterHelper ph(*this);
    def->printPretty(rso, &ph, this->PrintingPolicy);
    this->OS << encodeXML(rso.str());
    this->OS << "\"";
  }
  this->PrintAttributesAttribute(a);
  this->OS << "/>\n";
}

void ASTVisitor::OutputTranslationUnitDecl(clang::TranslationUnitDecl const* d,
                                           DumpNode const* dn)
{
  this->OS << "  <Namespace";
  this->PrintIdAttribute(dn);
  this->PrintNameAttribute("::");
  if (dn->Complete) {
    this->PrintMembersAttribute(d);
  }
  this->OS << "/>\n";
}

void ASTVisitor::OutputNamespaceDecl(clang::NamespaceDecl const* d,
                                     DumpNode const* dn)
{
  this->OS << "  <Namespace";
  this->PrintIdAttribute(dn);
  std::string name = d->getName().str();
  if (!name.empty()) {
    this->PrintNameAttribute(name);
  }
  this->PrintContextAttribute(d);
  if (dn->Complete) {
    std::set<DumpId> emitted;
    for (clang::NamespaceDecl const* r : d->redecls()) {
      this->AddDeclContextMembers(r, emitted);
    }
    this->PrintMembersAttribute(emitted);
  }
  this->OS << "/>\n";
}

void ASTVisitor::OutputRecordDecl(clang::RecordDecl const* d,
                                  DumpNode const* dn)
{
  const char* tag;
  switch (d->getTagKind()) {
    case clang::TTK_Class:
      tag = "Class";
      break;
    case clang::TTK_Union:
      tag = "Union";
      break;
    case clang::TTK_Struct:
      tag = "Struct";
      break;
    case clang::TTK_Interface:
      return;
    case clang::TTK_Enum:
      return;
  }
  clang::CXXRecordDecl const* dx = clang::dyn_cast<clang::CXXRecordDecl>(d);
  bool doBases = false;

  this->OS << "  <" << tag;
  this->PrintIdAttribute(dn);
  if (!d->isAnonymousStructOrUnion()) {
    std::string s;
    llvm::raw_string_ostream rso(s);
    d->getNameForDiagnostic(rso, this->PrintingPolicy, false);
    this->PrintNameAttribute(rso.str());
  }
  clang::AccessSpecifier access = clang::AS_none;
  if (clang::ClassTemplateSpecializationDecl const* dxts =
        clang::dyn_cast<clang::ClassTemplateSpecializationDecl>(d)) {
    // This is a template instantiation so get the access of the original
    // template.  Access of the instantiation itself has no meaning.
    if (clang::ClassTemplateDecl const* dxt = dxts->getSpecializedTemplate()) {
      access = dxt->getAccess();
    }
  }
  this->PrintContextAttribute(d, access);
  this->PrintLocationAttribute(d);
  if (d->getDefinition()) {
    if (dx && dx->isAbstract()) {
      this->OS << " abstract=\"1\"";
    }
    if (dn->Complete && !d->isInvalidDecl()) {
      this->PrintMembersAttribute(d);
      doBases = dx && dx->getNumBases();
      if (doBases) {
        this->PrintBasesAttribute(dx);
      }
      this->PrintBefriendingAttribute(dx);
    }
  } else {
    this->OS << " incomplete=\"1\"";
  }
  this->PrintABIAttributes(d);
  this->PrintAttributesAttribute(d);
  if (doBases) {
    this->OS << ">\n";
    clang::ASTRecordLayout const& layout = this->CTX.getASTRecordLayout(dx);
    for (clang::CXXRecordDecl::base_class_const_iterator i = dx->bases_begin(),
                                                         e = dx->bases_end();
         i != e; ++i) {
      clang::QualType bt = i->getType().getCanonicalType();
      clang::CXXRecordDecl const* bd = clang::dyn_cast<clang::CXXRecordDecl>(
        bt->getAs<clang::RecordType>()->getDecl());
      this->OS << "    <Base";
      this->PrintTypeAttribute(bt, true);
      this->PrintAccessAttribute(i->getAccessSpecifier());
      this->OS << " virtual=\"" << (i->isVirtual() ? 1 : 0) << "\"";
      if (bd && !i->isVirtual()) {
        this->OS << " offset=\"" << layout.getBaseClassOffset(bd).getQuantity()
                 << "\"";
      }
      this->OS << "/>\n";
    }
    this->OS << "  </" << tag << ">\n";
  } else {
    this->OS << "/>\n";
  }
}

void ASTVisitor::OutputCXXRecordDecl(clang::CXXRecordDecl const* d,
                                     DumpNode const* dn)
{
  if (d->getDescribedClassTemplate()) {
    // We do not implement class template output yet.
    this->ASTVisitorBase::OutputCXXRecordDecl(d, dn);
    return;
  }

  this->OutputRecordDecl(d, dn);
}

void ASTVisitor::OutputClassTemplateSpecializationDecl(
  clang::ClassTemplateSpecializationDecl const* d, DumpNode const* dn)
{
  this->OutputCXXRecordDecl(d, dn);
}

void ASTVisitor::OutputTypedefDecl(clang::TypedefDecl const* d,
                                   DumpNode const* dn)
{
  // As a special case, replace our compatibility Typedef for __float128
  // with a FundamentalType so we generate the same thing gccxml did.
  if (d->getName() == "__castxml__float128" &&
      clang::isa<clang::TranslationUnitDecl>(d->getDeclContext())) {
    clang::SourceLocation sl = d->getLocation();
    if (sl.isValid()) {
      clang::FullSourceLoc fsl = this->CTX.getFullLoc(sl).getExpansionLoc();
      if (!this->CI.getSourceManager().getFileEntryForID(fsl.getFileID())) {
        this->PrintFloat128Type(dn);
        return;
      }
    }
  }

  this->OS << "  <Typedef";
  this->PrintIdAttribute(dn);
  this->PrintNameAttribute(d->getName().str());
  this->PrintTypeAttribute(d->getUnderlyingType(), dn->Complete);
  this->PrintContextAttribute(d);
  this->PrintLocationAttribute(d);
  this->PrintAttributesAttribute(d);
  this->OS << "/>\n";
}

void ASTVisitor::OutputEnumDecl(clang::EnumDecl const* d, DumpNode const* dn)
{
  this->OS << "  <Enumeration";
  this->PrintIdAttribute(dn);
  std::string name = d->getName().str();
  if (name.empty()) {
    if (clang::TypedefNameDecl const* td = d->getTypedefNameForAnonDecl()) {
      name = td->getName().str();
    }
  }
  this->PrintNameAttribute(name);
  this->PrintContextAttribute(d);
  this->PrintLocationAttribute(d);
  this->PrintABIAttributes(d);
  this->PrintAttributesAttribute(d);
  clang::EnumDecl::enumerator_iterator enum_begin = d->enumerator_begin();
  clang::EnumDecl::enumerator_iterator enum_end = d->enumerator_end();
  if (enum_begin != enum_end) {
    this->OS << ">\n";
    for (clang::EnumDecl::enumerator_iterator i = enum_begin; i != enum_end;
         ++i) {
      clang::EnumConstantDecl const* ecd = *i;
      this->OS << "    <EnumValue";
      this->PrintNameAttribute(ecd->getName());
      this->OS << " init=\"" << ecd->getInitVal() << "\"";
      this->PrintAttributesAttribute(ecd);
      this->OS << "/>\n";
    }
    this->OS << "  </Enumeration>\n";
  } else {
    this->OS << "/>\n";
  }
}

void ASTVisitor::OutputFieldDecl(clang::FieldDecl const* d, DumpNode const* dn)
{
  this->OS << "  <Field";
  this->PrintIdAttribute(dn);
  this->PrintNameAttribute(d->getName().str());
  this->PrintTypeAttribute(d->getType(), dn->Complete);
  if (d->isBitField()) {
    unsigned bits = d->getBitWidthValue(this->CTX);
    this->OS << " bits=\"" << bits << "\"";
  }
  this->PrintContextAttribute(d);
  this->PrintLocationAttribute(d);
  this->PrintOffsetAttribute(this->CTX.getFieldOffset(d));
  if (d->isMutable()) {
    this->OS << " mutable=\"1\"";
  }
  this->PrintAttributesAttribute(d);

  this->OS << "/>\n";
}

void ASTVisitor::OutputVarDecl(clang::VarDecl const* d, DumpNode const* dn)
{
  this->OS << "  <Variable";
  this->PrintIdAttribute(dn);
  this->PrintNameAttribute(d->getName().str());
  this->PrintTypeAttribute(d->getType(), dn->Complete);
  if (clang::Expr const* init = d->getInit()) {
    this->OS << " init=\"";
    std::string s;
    llvm::raw_string_ostream rso(s);
    PrinterHelper ph(*this);
    init->printPretty(rso, &ph, this->PrintingPolicy);
    this->OS << encodeXML(rso.str());
    this->OS << "\"";
  }
  this->PrintContextAttribute(d);
  this->PrintLocationAttribute(d);
  if (d->getStorageClass() == clang::SC_Static) {
    this->OS << " static=\"1\"";
  }
  if (d->getStorageClass() == clang::SC_Extern) {
    this->OS << " extern=\"1\"";
  }
  this->PrintMangledAttribute(d);
  this->PrintAttributesAttribute(d);

  this->OS << "/>\n";
}

void ASTVisitor::OutputFunctionDecl(clang::FunctionDecl const* d,
                                    DumpNode const* dn)
{
  if (d->getDescribedFunctionTemplate()) {
    // We do not implement function template output yet.
    this->ASTVisitorBase::OutputFunctionDecl(d, dn);
    return;
  }

  unsigned int flags = FH_Returns;
  if (d->getStorageClass() == clang::SC_Static) {
    flags |= FH_Static;
  }
  if (d->isOverloadedOperator()) {
    this->OutputFunctionHelper(
      d, dn, "OperatorFunction",
      clang::getOperatorSpelling(d->getOverloadedOperator()), flags);
  } else if (clang::IdentifierInfo const* ii = d->getIdentifier()) {
    this->OutputFunctionHelper(d, dn, "Function", ii->getName().str(), flags);
  } else {
    this->OutputUnimplementedDecl(d, dn);
  }
}

void ASTVisitor::OutputCXXMethodDecl(clang::CXXMethodDecl const* d,
                                     DumpNode const* dn)
{
  if (d->getDescribedFunctionTemplate()) {
    // We do not implement function template output yet.
    this->ASTVisitorBase::OutputCXXMethodDecl(d, dn);
    return;
  }

  unsigned int flags = FH_Returns;
  if (d->isStatic()) {
    flags |= FH_Static;
  }
  if (d->isConst()) {
    flags |= FH_Const;
  }
  if (d->isVirtual()) {
    flags |= FH_Virtual;
  }
  if (d->isPure()) {
    flags |= FH_Pure;
  }
  if (d->isOverloadedOperator()) {
    this->OutputFunctionHelper(
      d, dn, "OperatorMethod",
      clang::getOperatorSpelling(d->getOverloadedOperator()), flags);
  } else if (clang::IdentifierInfo const* ii = d->getIdentifier()) {
    this->OutputFunctionHelper(d, dn, "Method", ii->getName().str(), flags);
  } else {
    this->OutputUnimplementedDecl(d, dn);
  }
}

void ASTVisitor::OutputCXXConversionDecl(clang::CXXConversionDecl const* d,
                                         DumpNode const* dn)
{
  if (d->getDescribedFunctionTemplate()) {
    // We do not implement function template output yet.
    this->ASTVisitorBase::OutputCXXConversionDecl(d, dn);
    return;
  }

  unsigned int flags = FH_Returns;
  if (d->isConst()) {
    flags |= FH_Const;
  }
  if (d->isVirtual()) {
    flags |= FH_Virtual;
  }
  if (d->isPure()) {
    flags |= FH_Pure;
  }
  this->OutputFunctionHelper(d, dn, "Converter", "", flags);
}

void ASTVisitor::OutputCXXConstructorDecl(clang::CXXConstructorDecl const* d,
                                          DumpNode const* dn)
{
  if (d->getDescribedFunctionTemplate()) {
    // We do not implement function template output yet.
    this->ASTVisitorBase::OutputCXXConstructorDecl(d, dn);
    return;
  }

  unsigned int flags = 0;
  if (d->isExplicit()) {
    flags |= FH_Explicit;
  }
  this->OutputFunctionHelper(d, dn, "Constructor", this->GetContextName(d),
                             flags);
}

void ASTVisitor::OutputCXXDestructorDecl(clang::CXXDestructorDecl const* d,
                                         DumpNode const* dn)
{
  if (d->getDescribedFunctionTemplate()) {
    // We do not implement function template output yet.
    this->ASTVisitorBase::OutputCXXDestructorDecl(d, dn);
    return;
  }

  unsigned int flags = 0;
  if (d->isVirtual()) {
    flags |= FH_Virtual;
  }
  if (d->isPure()) {
    flags |= FH_Pure;
  }
  this->OutputFunctionHelper(d, dn, "Destructor", this->GetContextName(d),
                             flags);
}

void ASTVisitor::OutputBuiltinType(clang::BuiltinType const* t,
                                   DumpNode const* dn)
{
  this->OS << "  <FundamentalType";
  this->PrintIdAttribute(dn);

  // gccxml used different name variants than Clang for some types
  std::string name;
  switch (t->getKind()) {
    case clang::BuiltinType::Short:
      name = "short int";
      break;
    case clang::BuiltinType::UShort:
      name = "short unsigned int";
      break;
    case clang::BuiltinType::Long:
      name = "long int";
      break;
    case clang::BuiltinType::ULong:
      name = "long unsigned int";
      break;
    case clang::BuiltinType::LongLong:
      name = "long long int";
      break;
    case clang::BuiltinType::ULongLong:
      name = "long long unsigned int";
      break;
    default:
      name = t->getName(this->PrintingPolicy).str();
      break;
  };
  this->PrintNameAttribute(name);
  this->PrintABIAttributes(this->CTX.getTypeInfo(t));

  this->OS << "/>\n";
}

void ASTVisitor::OutputConstantArrayType(clang::ConstantArrayType const* t,
                                         DumpNode const* dn)
{
  this->OS << "  <ArrayType";
  this->PrintIdAttribute(dn);
  this->OS << " min=\"0\" max=\"" << (t->getSize() - 1) << "\"";
  this->PrintTypeAttribute(t->getElementType(), dn->Complete);
  this->OS << "/>\n";
}

void ASTVisitor::OutputIncompleteArrayType(clang::IncompleteArrayType const* t,
                                           DumpNode const* dn)
{
  this->OS << "  <ArrayType";
  this->PrintIdAttribute(dn);
  this->OS << " min=\"0\" max=\"\"";
  this->PrintTypeAttribute(t->getElementType(), dn->Complete);
  this->OS << "/>\n";
}

void ASTVisitor::OutputFunctionProtoType(clang::FunctionProtoType const* t,
                                         DumpNode const* dn)
{
  this->OutputFunctionTypeHelper(t, dn, "FunctionType", 0);
}

void ASTVisitor::OutputLValueReferenceType(clang::LValueReferenceType const* t,
                                           DumpNode const* dn)
{
  this->OS << "  <ReferenceType";
  this->PrintIdAttribute(dn);
  this->PrintTypeAttribute(t->getPointeeType(), false);
  this->OS << "/>\n";
}

void ASTVisitor::OutputMemberPointerType(clang::MemberPointerType const* t,
                                         DumpNode const* dn)
{
  if (t->isMemberDataPointerType()) {
    this->OutputOffsetType(t->getPointeeType(), t->getClass(), dn);
  } else {
    this->OS << "  <PointerType";
    this->PrintIdAttribute(dn);
    DumpId id = this->AddTypeDumpNode(
      DumpType(t->getPointeeType(), t->getClass()), false);
    this->OS << " type=\"_" << id << "\"";
    this->OS << "/>\n";
  }
}

void ASTVisitor::OutputMethodType(clang::FunctionProtoType const* t,
                                  clang::Type const* c, DumpNode const* dn)
{
  this->OutputFunctionTypeHelper(t, dn, "MethodType", c);
}

void ASTVisitor::OutputOffsetType(clang::QualType t, clang::Type const* c,
                                  DumpNode const* dn)
{
  this->OS << "  <OffsetType";
  this->PrintIdAttribute(dn);
  this->PrintBaseTypeAttribute(c, dn->Complete);
  this->PrintTypeAttribute(t, dn->Complete);
  this->OS << "/>\n";
}

void ASTVisitor::OutputPointerType(clang::PointerType const* t,
                                   DumpNode const* dn)
{
  this->OS << "  <PointerType";
  this->PrintIdAttribute(dn);
  this->PrintTypeAttribute(t->getPointeeType(), false);
  this->PrintABIAttributes(this->CTX.getTypeInfo(t));
  this->OS << "/>\n";
}

void ASTVisitor::OutputElaboratedType(clang::ElaboratedType const* t,
                                      DumpNode const* dn)
{
  this->OS << "  <ElaboratedType";
  this->PrintIdAttribute(dn);
  this->PrintTypeAttribute(t->getNamedType(), false);
  this->OS << "/>\n";
}

void ASTVisitor::OutputStartXMLTags()
{
  /* clang-format off */
  this->OS <<
    "<?xml version=\"1.0\"?>\n"
    ;
  /* clang-format on */
  if (this->Opts.CastXml) {
    // Start dump with castxml-compatible format.
    /* clang-format off */
    this->OS <<
      "<CastXML format=\"" << Opts.CastXmlEpicFormatVersion << ".1.0\">\n"
      ;
    /* clang-format on */
  } else if (this->Opts.GccXml) {
    // Start dump with gccxml-compatible format (legacy).
    /* clang-format off */
    this->OS <<
      "<GCC_XML version=\"0.9.0\" cvs_revision=\"1.140\">\n"
      ;
    /* clang-format on */
  }
}

void ASTVisitor::OutputEndXMLTags()
{
  // Finish dump.
  if (this->Opts.CastXml) {
    /* clang-format off */
    this->OS <<
      "</CastXML>\n"
      ;
    /* clang-format on */
  } else if (this->Opts.GccXml) {
    /* clang-format off */
    this->OS <<
      "</GCC_XML>\n"
      ;
    /* clang-format on */
  }
}

void ASTVisitor::LookupStart(clang::DeclContext const* dc,
                             std::string const& name)
{
  std::string::size_type pos = name.find("::");
  std::string cur = name.substr(0, pos);

  clang::IdentifierTable& ids = CI.getPreprocessor().getIdentifierTable();
  auto const& result = dc->lookup(clang::DeclarationName(&ids.get(cur)));
  if (pos == name.npos) {
    for (clang::NamedDecl const* n : result) {
      this->AddStartDecl(n);
    }
  } else {
    std::string rest = name.substr(pos + 2);
    for (clang::NamedDecl const* n : result) {
      if (clang::DeclContext const* idc =
            clang::dyn_cast<clang::DeclContext const>(n)) {
        this->LookupStart(idc, rest);
      }
    }
  }

  for (clang::UsingDirectiveDecl const* i : dc->using_directives()) {
    this->LookupStart(i->getNominatedNamespace(), name);
  }
}

void ASTVisitor::HandleTranslationUnit(clang::TranslationUnitDecl const* tu)
{
  // Add the starting nodes for the dump.
  if (!this->Opts.StartNames.empty()) {
    // Use the specified starting locations.
    for (std::vector<std::string>::const_iterator
           i = this->Opts.StartNames.begin(),
           e = this->Opts.StartNames.end();
         i != e; ++i) {
      this->LookupStart(tu, *i);
    }
  } else {
    // No start specified.  Use whole translation unit.
    this->AddStartDecl(tu);
  }

  // Dump opening tags.
  this->OutputStartXMLTags();

  // Dump the complete nodes.
  this->ProcessQueue();

  // Queue all the incomplete nodes.
  this->RequireComplete = false;
  this->QueueIncompleteDumpNodes();

  // Dump the incomplete nodes.
  this->ProcessQueue();

  // Dump the filename queue.
  this->ProcessFileQueue();

  // Dump end tags.
  this->OutputEndXMLTags();
}

void outputXML(clang::CompilerInstance& ci, clang::ASTContext& ctx,
               llvm::raw_ostream& os, Options const& opts)
{
  ASTVisitor v(ci, ctx, os, opts);
  v.HandleTranslationUnit(ctx.getTranslationUnitDecl());
}
