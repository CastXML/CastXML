.. castxml-manual-description: C-family Abstract Syntax Tree XML Output

castxml(1)
**********

Synopsis
========

::

  castxml ( <castxml-opt> | <clang-opt> | <src> )...

Description
===========

Parse C-family source files and optionally write a subset of the
Abstract Syntax Tree (AST) to a representation in XML.

Source files are parsed as complete translation units using an
internal `Clang`_ compiler.  XML output is enabled by the
``--castxml-gccxml`` option and produces a format close to
that of `gccxml`_.  Future versions of ``castxml`` may support
alternative output formats.

.. _`Clang`: http://clang.llvm.org/
.. _`gccxml`: http://gccxml.org

Options
=======

The following command-line options are interpreted by ``castxml``.
Remaining options are given to the internal Clang compiler.

``--castxml-cc-<id> <cc>``, ``--castxml-cc-<id> "(" <cc> <cc-opt>... ")"``
  Configure the internal Clang preprocessor and target platform to
  match that of the given compiler command.  The ``<id>`` names
  a reference compiler with which the given command is compatible.
  It must be one of:

  * ``gnu``: GNU Compiler Collection C++ (g++)
  * ``gnu-c``: GNU Compiler Collection C (gcc)
  * ``msvc``: Microsoft Visual C++ (cl)
  * ``msvc-c``: Microsoft Visual C (cl)

  ``<cc>`` names a compiler (e.g. ``/usr/bin/gcc`` or ``cl``) and
  ``<cc-opt>...`` specifies options that may affect its target
  (e.g. ``-m32``).
  The target platform detected from the given compiler may be
  overridden by a separate Clang ``-target`` option.
  The language standard level detected from the given compiler
  may be overridden by a separate Clang ``-std=`` option.

``--castxml-gccxml``
  Generate XML output in a format close to that of `gccxml`_.
  Write output to ``<src>.xml`` or file named by ``-o``.
  The gccxml format does not support Clang language modes other than
  ``-std=c++98`` or ``-std=c89``.  This output format may be used with
  language modes ``-std=c++11`` and ``-std=c++14`` but the output will
  not contain implicit move constructors or move assignment operators,
  and may contain ``<Unimplemented/>`` elements on non-c++98 constructs.

``--castxml-start <name>[,<name>]...``
  Start AST traversal at declaration(s) with the given qualified name(s).
  Multiple names may be specified as a comma-separated list or by repeating
  the option.

``-help``, ``--help``
  Print ``castxml`` and internal Clang compiler usage information.

``-o <file>``
  Write output to ``<file>``.  At most one ``<src>`` file may
  be specified as input.

``--version``
  Print ``castxml`` and internal Clang compiler version information.

Preprocessing
=============

CastXML preprocesses source files using an internal Clang compiler
using its own predefined macros for the target platform by default.
The ``--castxml-cc-<id>`` option switches the predefined macros
to match those detected from the given compiler command.  In either
case, CastXML always adds the following predefined macros:

``__castxml__``
  Defined to an integer encoding the CastXML version number as
  ``printf("%d%03d%03d",major,minor,patch)``.

``__castxml_clang_major__``
  Defined to the value of  ``__clang_major__`` from the internal Clang.

``__castxml_clang_minor__``
  Defined to the value of  ``__clang_minor__`` from the internal Clang.

``__castxml_clang_patchlevel__``
  Defined to the value of  ``__clang_patchlevel__`` from the internal Clang.

Source files may use these to identify the tool that is actually doing the
preprocessing even when ``--castxml-cc-<id>`` changes the predefined macros.
