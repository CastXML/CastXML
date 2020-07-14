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
``--castxml-output=<v>`` or ``--castxml-gccxml`` option.

.. _`Clang`: https://clang.llvm.org/
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

``--castxml-output=<v>``
  Write XML output to to ``<src>.xml`` or file named by ``-o``.
  The ``<v>`` specifies the "epic" format version number to generate,
  and must be ``1``.

``--castxml-gccxml``
  Generate XML output in a format close to that of `gccxml`_.
  Write output to ``<src>.xml`` or file named by ``-o``.
  The gccxml format does not support Clang language modes other than
  ``-std=c++98`` or ``-std=c89``.  This output format may be used with
  language modes ``-std=c++11``, ``-std=c++14``, ``-std=c99``, and
  ``-std=c11`` but the output will not contain implicit move constructors
  or move assignment operators, and may contain ``<Unimplemented/>``
  elements on non-c++98 constructs.

``--castxml-start <name>[,<name>]...``
  Start AST traversal at declaration(s) with the given qualified name(s).
  Multiple names may be specified as a comma-separated list or by repeating
  the option.

``-help``, ``--help``
  Print ``castxml`` and internal Clang compiler usage information.

``-o <file>``
  If output is generated (e.g. via ``--castxml-output=<v>``), write
  the output to ``<file>``.  At most one ``<src>`` file may be specified
  as input.

``--version``
  Print ``castxml`` and internal Clang compiler version information.

  Release versions of CastXML use the format::

    <major>.<minor>.<patch>[-rc<n>][-<id>]

  where the ``<patch>`` component is less than ``20000000``, ``<n>``
  is an optional release candidate number, and ``<id>`` may contain
  arbitrary text (in case of development between patch versions).

  Development versions of CastXML use the format::

    <major>.<minor>.<date>[-<id>]

  where the ``<date>`` component is of format ``CCYYMMDD`` and ``<id>``
  may contain arbitrary text.  This represents development as of a
  particular date following the ``<major>.<minor>`` feature release.

Output Format Versions
======================

With ``--castxml-output=<v>``
-----------------------------

The XML root element tag will be of the form:

.. code-block:: xml

  <CastXML format="1.2.0">

* The first component is the ``epic`` format version number given to the
  ``--castxml-output=<v>`` flag, and currently must always be ``1``.
* The second component is the ``major`` format version number and increments
  when a new XML element is added or for other major changes.
  Clients will need updating.
* The third component is the ``minor`` format version number and increments
  whenever a new XML attribute is added to an existing element or a minor
  bug is fixed in the XML output of an existing element or attribute
  (clients should work unchanged unless they want the new info).

With ``--castxml-gccxml``
-------------------------

The XML root element tag will be of the form:

.. code-block:: xml

  <GCC_XML version="0.9.0" cvs_revision="1.139">

The ``version`` number corresponds to the last `gccxml`_ version that was
ever released (for backward compatibility).  The ``cvs_revision`` number is
a running number that is incremented for each minor change in the xml format.


Preprocessing
=============

CastXML preprocesses source files using an internal Clang compiler
using its own predefined macros for the target platform by default.
The ``--castxml-cc-<id>`` option switches the predefined macros
to match those detected from the given compiler command.  In either
case, CastXML always adds the following predefined macros:

``__castxml_major__``
  Defined to the CastXML major version number in decimal.

``__castxml_minor__``
  Defined to the CastXML minor version number in decimal.

``__castxml_patch__``
  Defined to the CastXML patch version number in decimal.

``__castxml_check(major,minor,patch)``
  Defined to a constant expression encoding the three version components for
  comparison with ``__castxml__``.  The actual encoding is unspecified.

``__castxml__``
  Defined to a constant expression encoding the CastXML version components::

    __castxml_check(__castxml_major__,__castxml_minor__,__castxml_patch__)

``__castxml_clang_major__``
  Defined to the value of  ``__clang_major__`` from the internal Clang.

``__castxml_clang_minor__``
  Defined to the value of  ``__clang_minor__`` from the internal Clang.

``__castxml_clang_patchlevel__``
  Defined to the value of  ``__clang_patchlevel__`` from the internal Clang.

Source files may use these to identify the tool that is actually doing the
preprocessing even when ``--castxml-cc-<id>`` changes the predefined macros.

FAQ
===

Why are C++ function bodies not dumped in XML?
----------------------------------------------

This feature has not been implemented because the driving project for which
CastXML was written had no need for function bodies.

Is there a DTD specifying the XML format dumped?
------------------------------------------------

No.

Why don't I see templates in the output?
----------------------------------------

This feature has not been implemented because the driving project for which
CastXML was written had no need for uninstantiated templates.
Template instantiations will still be dumped, though. For example:

.. code-block:: c++

  template <class T> struct foo {};
  typedef foo<int>::foo foo_int;

will instantiate ``foo<int>``, which will be included in the output.
However, there will be no place that explicitly lists the set of types used
for the instantiation other than in the name. This is because the proper way to
do it is to dump the templates too and reference them from the instantiations
with the template arguments listed. Since the features will be linked they
should be implemented together.
