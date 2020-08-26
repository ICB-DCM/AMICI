# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/stable/config

import os
import sys
import re
import subprocess
import mock

from sphinx.transforms.post_transforms import ReferencesResolver


def install_amici_deps_rtd():
    """Install AMICI dependencies and set up environment for use on RTD"""

    # cblas -- manually install ubuntu deb package
    cblas_root = os.path.join(amici_dir, 'ThirdParty', 'libatlas-base-dev',
                              'usr')

    if os.path.isdir(cblas_root):
        # If this exists, it means this has been run before. On RTD, sphinx is
        #  being run several times and we don't want to reinstall dependencies
        #  every time.
        return

    cblas_inc_dir = os.path.join(cblas_root, "include", "x86_64-linux-gnu")
    cblas_lib_dir = os.path.join(cblas_root, "lib", "x86_64-linux-gnu")
    cmd = (f"cd '{os.path.join(amici_dir, 'ThirdParty')}' "
           "&& apt download libatlas-base-dev && mkdir libatlas-base-dev "
           "&& cd libatlas-base-dev "
           "&& ar x ../libatlas-base-dev_3.10.3-5_amd64.deb "
           "&& tar -xJf data.tar.xz "
           f"&& ln -s {cblas_inc_dir}/cblas-atlas.h {cblas_inc_dir}/cblas.h "
           )
    subprocess.run(cmd, shell=True, check=True)
    os.environ['BLAS_CFLAGS'] = f'-I{cblas_inc_dir}'
    os.environ['BLAS_LIBS'] = (f'-L{cblas_lib_dir}/atlas -L{cblas_lib_dir} '
                               '-lcblas -latlas -lblas -lm')

    # build swig4.0
    subprocess.run(os.path.join(amici_dir, 'scripts',
                                'downloadAndBuildSwig.sh'), check=True)

    # add swig to path
    swig_dir = os.path.join(amici_dir, 'ThirdParty', 'swig-4.0.1', 'install',
                            'bin')
    os.environ['SWIG'] = os.path.join(swig_dir, 'swig')


# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#

amici_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# -- RTD custom build --------------------------------------------------------

# only execute those commands when running from RTD
if 'READTHEDOCS' in os.environ and os.environ['READTHEDOCS']:
    install_amici_deps_rtd()

# Install AMICI if not already present
try:
    import amici
except ModuleNotFoundError:
    subprocess.run([
        'python', '-m', 'pip', 'install', '--verbose', '-e',
        os.path.join(amici_dir, 'python', 'sdist')
    ], check=True)

    from importlib import invalidate_caches
    invalidate_caches()

    sys.path.insert(0, amici_dir)
    sys.path.insert(0, os.path.join(amici_dir, 'python', 'sdist'))

    import amici

# -- Project information -----------------------------------------------------
# The short X.Y version
version = amici.__version__
# The full version, including alpha/beta/rc tags
release = version

project = 'AMICI'
copyright = '2020, The AMICI developers'
author = 'The AMICI developers'
title = 'AMICI Documentation'

# -- Mock out some problematic modules-------------------------------------

# Note that for sub-modules, all parent modules must be listed explicitly.
autodoc_mock_imports = ['_amici', 'amici._amici']
for mod_name in autodoc_mock_imports:
    sys.modules[mod_name] = mock.MagicMock()

# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'readthedocs_ext.readthedocs',
    'sphinx.ext.autodoc',
    'sphinx.ext.doctest',
    'sphinx.ext.coverage',
    'sphinx.ext.intersphinx',
    'sphinx.ext.autosummary',
    'sphinx.ext.viewcode',
    'nbsphinx',
    'recommonmark',
    'sphinx_autodoc_typehints',
    'hoverxref.extension',
    'breathe',
    'exhale',
]

intersphinx_mapping = {
    'pysb': ('https://pysb.readthedocs.io/en/stable/', None),
    'petab': ('https://petab.readthedocs.io/en/stable/', None),
    'pandas': ('https://pandas.pydata.org/docs/', None),
    'numpy': ('https://numpy.org/devdocs/', None),
    'sympy': ('https://docs.sympy.org/latest/', None),
    'python': ('https://docs.python.org/3', None),
}

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = ['.rst', '.md']

# The master toctree document.
master_doc = 'index'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path .
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store',
                    '**.ipynb_checkpoints', 'numpy.py', 'MATLAB.md', 'gfx']

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = False

# sphinx-autodoc-typehints
typehints_fully_qualified = True
typehints_document_rtype = True
set_type_checking_flag = True

# hoverxref
hoverxref_auto_ref = True
hoverxref_roles = ['term']
hoverxref_domains = ['py']

# breathe settings
breathe_projects = {
    "AMICI": "./_doxyoutput/xml",
}

breathe_default_project = "AMICI"

# exhale settings

exhale_args = {
    # These arguments are required
    "containmentFolder": "./_exhale_cpp_api",
    "rootFileName": "library_root.rst",
    "rootFileTitle": "AMICI C++ API",
    "doxygenStripFromPath": "..",
    # Suggested optional arguments
    "createTreeView": True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin":    "INPUT = ../include",
    "afterTitleDescription":
        "AMICI C++ library functions",
    "verboseBuild": False,
}

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
# html_theme_options = {}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path = ['_static']
html_favicon = "gfx/logo.png"

# Custom sidebar templates, must be a dictionary that maps document names
# to template names.
#
# The default sidebars (for documents that don't match any pattern) are
# defined by theme itself.  Builtin themes are using these templates by
# default: ``['localtoc.html', 'relations.html', 'sourcelink.html',
# 'searchbox.html']``.
#
# html_sidebars = {}


# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'AMICIdoc'


# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',

    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'AMICI.tex', title,
     author, 'manual'),
]


# -- Options for manual page output ------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (master_doc, 'amici', title,
     [author], 1)
]


# -- Options for Texinfo output ----------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (master_doc, 'AMICI', title,
     author, 'AMICI', 'Advanced Multilanguage Interface for CVODES and IDAS.',
     'Miscellaneous'),
]

# Custom processing routines for docstrings and signatures

typemaps = {
    'std::vector< amici::realtype,std::allocator< amici::realtype > >':
        'DoubleVector',
    'std::vector< double,std::allocator< double > >':
        'DoubleVector',
    'std::vector< int,std::allocator< int > >':
        'IntVector',
    'std::vector< amici::ParameterScaling,std::allocator< '
    'amici::ParameterScaling >': 'ParameterScalingVector',
    'std::vector< std::string,std::allocator< std::string > >':
        'StringVector',
    'std::vector< bool,std::allocator< bool > >':
        'BoolVector',
    'std::map< std::string,amici::realtype,std::less< std::string >,'
    'std::allocator< std::pair< std::string const,amici::realtype > > >':
        'StringDoubleMap',
    'std::vector< amici::ExpData *,std::allocator< amici::ExpData * > >':
        'ExpDataPtrVector',
    'std::vector< std::unique_ptr< amici::ReturnData >,std::allocator< '
    'std::unique_ptr< amici::ReturnData > > >':
        'Iterable[ReturnData]',
    'std::unique_ptr< amici::ExpData >':
        'ExpData',
    'std::unique_ptr< amici::ReturnData >':
        'ReturnData',
    'std::unique_ptr< amici::Solver >':
        'Solver',
    'amici::realtype':
        'float',
}

vector_types = {
    'IntVector': ':class:`int`',
    'BoolVector': ':class:`bool`',
    'DoubleVector': ':class:`float`',
    'StringVector': ':class:`str`',
    'ExpDataPtrVector': ':class:`amici.amici.ExpData`',
}


def process_docstring(app, what, name, obj, options, lines):
    # only apply in the amici.amici module
    if len(name.split('.')) < 2 or name.split('.')[1] != 'amici':
        return

    # add custom doc to swig generated classes
    if len(name.split('.')) == 3 and name.split('.')[2] in \
            ['IntVector', 'BoolVector', 'DoubleVector', 'StringVector',
             'ExpDataPtrVector']:
        cname = name.split('.')[2]
        lines.append(
            f'Swig-Generated class templating common python '
            f'types including :class:`Iterable` '
            f'[{vector_types[cname]}] '
            f'and '
            f':class:`numpy.array` [{vector_types[cname]}] to facilitate'
            ' interfacing with C++ bindings.'
        )
        return

    if name == 'amici.amici.StringDoubleMap':
        lines.append(
            'Swig-Generated class templating :class:`Dict` '
            '[:class:`str`, :class:`float`] to  facilitate'
            ' interfacing with C++ bindings.'
        )
        return

    if name == 'amici.amici.ParameterScalingVector':
        lines.append(
            'Swig-Generated class, which, in contrast to other Vector '
            'classes, does not allow for simple interoperability with common '
            'python types, but must be created using '
            ':func:`amici.amici.parameterScalingFromIntVector`'
        )
        return

    if len(name.split('.')) == 3 and name.split('.')[2] in \
            ['ExpDataPtr', 'ReturnDataPtr', 'ModelPtr', 'SolverPtr']:
        cname = name.split('.')[2]
        lines.append(
            f'Swig-Generated class that implements smart pointers to '
            f'{cname.replace("Ptr","")} as objects.'
        )
        return

    # add linebreaks before argument/return definitions
    lines_clean = []

    while len(lines):
        line = lines.pop(0)

        if re.match(r':(type|rtype|param|return)', line) and \
                len(lines_clean) and lines_clean[-1] != '':
            lines_clean.append('')

        lines_clean.append(line)
    lines.extend(lines_clean)

    for i in range(len(lines)):
        # fix types
        for old, new in typemaps.items():
            lines[i] = lines[i].replace(old, new)
        lines[i] = re.sub(
            r'amici::(Model|Solver|ExpData) ',
            r':class:`amici\.amici\.\1\`',
            lines[i]
        )
        lines[i] = re.sub(
            r'amici::(runAmiciSimulation[s]?)',
            r':func:`amici\.amici\.\1`',
            lines[i]
        )


def fix_typehints(sig: str) -> str:
    # cleanup types
    if not isinstance(sig, str):
        return sig

    for old, new in typemaps.items():
        sig = sig.replace(old, new)
    sig = sig.replace('void', 'None')
    sig = sig.replace('amici::realtype', 'float')
    sig = sig.replace('std::string', 'str')
    sig = sig.replace('double', 'float')
    sig = sig.replace('long', 'int')
    sig = sig.replace('char const *', 'str')
    sig = sig.replace('amici::', '')
    sig = sig.replace('sunindextype', 'int')
    sig = sig.replace('H5::H5File', 'object')

    # remove const
    sig = sig.replace(' const ', r' ')
    sig = re.sub(r' const$', r'', sig)

    # remove pass by reference
    sig = re.sub(r' &(,|\))', r'\1', sig)
    sig = re.sub(r' &$', r'', sig)

    # turn gsl_spans and pointers int Iterables
    sig = re.sub(r'([\w\.]+) \*', r'Iterable[\1]', sig)
    sig = re.sub(r'gsl::span< ([\w\.]+) >', r'Iterable[\1]', sig)

    # fix garbled output
    sig = sig.replace(' >', '')
    return sig


def process_signature(app, what: str, name: str, obj, options, signature,
                      return_annotation):

    if signature is None:
        return

    # only apply in the amici.amici module
    if name.split('.')[1] != 'amici':
        return

    signature = fix_typehints(signature)
    if hasattr(obj, '__annotations__'):
        for ann in obj.__annotations__:
            obj.__annotations__[ann] = fix_typehints(obj.__annotations__[ann])

    return signature, return_annotation


# this code fixes references in symlinked md files in documentation folder
# link replacements must be in env.domains['std'].labels
doclinks = {
    'documentation/development': '/development.md',
    'documentation/CI': '/ci.md',
    'documentation/code_review_guide': '/code_review_guide.md',
}


def process_missing_ref(app, env, node, contnode):
    if not any(link in node['reftarget'] for link in doclinks):
        return  # speedup futile processing

    for old, new in doclinks.items():
        node['reftarget'] = node['reftarget'].replace(old, new)
    cnode = node[0]
    if 'refuri' in cnode:
        for old, new in doclinks.items():
            cnode['refuri'] = cnode['refuri'].replace(old, new)

    refdoc = node.get('refdoc', env.docname)
    resolver = ReferencesResolver(env.get_doctree(refdoc))
    result = resolver.resolve_anyref(refdoc, node, cnode)
    return result


def skip_member(app, what, name, obj, skip, options):
    ignored = ['AbstractModel', 'CVodeSolver', 'IDASolver', 'Model_ODE',
               'Model_DAE', 'ConditionContext', 'checkSigmaPositivity',
               'createGroup', 'createGroup', 'equals', 'printErrMsgIdAndTxt',
               'wrapErrHandlerFn', 'printWarnMsgIdAndTxt',
               'AmiciApplication', 'writeReturnData',
               'writeReturnDataDiagnosis', 'attributeExists', 'locationExists',
               'createAndWriteDouble1DDataset',
               'createAndWriteDouble2DDataset',
               'createAndWriteDouble3DDataset',
               'createAndWriteInt1DDataset', 'createAndWriteInt2DDataset',
               'createAndWriteInt3DDataset', 'getDoubleDataset1D',
               'getDoubleDataset2D', 'getDoubleDataset3D', 'getIntDataset1D',
               'getIntScalarAttribute', 'getDoubleScalarAttribute',
               'stdVec2ndarray', 'SwigPyIterator', 'thisown']

    if name in ignored:
        return True

    if name.startswith('_') and name != '__init__':
        return True

    # ignore various functions for std::vector<> types
    if re.match(r'^<function [\w]+Vector\.', str(obj)):
        return True

    # ignore various functions for smart pointer types
    if re.match(r'^<function [\w]+Ptr\.', str(obj)):
        return True

    # ignore various functions for StringDoubleMap
    if str(obj).startswith('<function StringDoubleMap'):
        return True

    return None


def setup(app: 'sphinx.application.Sphinx'):
    app.connect('autodoc-process-docstring', process_docstring, priority=0)
    app.connect('autodoc-process-signature', process_signature, priority=0)
    app.connect('missing-reference', process_missing_ref, priority=0)
    app.connect('autodoc-skip-member', skip_member, priority=0)
    app.config.intersphinx_mapping = intersphinx_mapping
    app.config.autosummary_generate = True
    app.config.autodoc_mock_imports = autodoc_mock_imports
