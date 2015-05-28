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

  * ``gnu``: GNU Compiler Collection (gcc)
  * ``msvc``: Microsoft Visual C++ (cl)

  ``<cc>`` names a compiler (e.g. ``/usr/bin/gcc`` or ``cl``) and
  ``<cc-opt>...`` specifies options that may affect its target
  (e.g. ``-m32``).
  The target platform detected from the given compiler may be
  overridden by a separate Clang ``-target`` option.

``--castxml-gccxml``
  Generate XML output in a format close to that of `gccxml`_.
  Write output to ``<src>.xml`` or file named by ``-o``.
  The gccxml format does not support Clang language modes other than
  ``-std=c++98`` or ``-std=c89``.  This output format may be used with
  language mode ``-std=c++11`` but the output will not contain implicit
  move constructors or move assignment operators, and may contain
  ``<Unimplemented/>`` elements on non-c++98 constructs.

``--castxml-start <name>``
  Start AST traversal at the declaration(s) with the given
  qualified name.

``-help``, ``--help``
  Print ``castxml`` and internal Clang compiler usage information.

``-o <file>``
  Write output to ``<file>``.  At most one ``<src>`` file may
  be specified as input.

``--version``
  Print ``castxml`` and internal Clang compiler version information.
