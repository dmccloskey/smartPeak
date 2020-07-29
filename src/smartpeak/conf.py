# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os, subprocess

project     = 'SmartPeak'
copyright   = '2020, SmartPeak Team'
author      = 'SmartPeak Team'

extensions              = [ "breathe", 
                            "exhale",
                            "sphinx.ext.todo" ]

todo_include_todos      = True
todo_link_only          = True

breathe_default_project = "SmartPeak"

exclude_patterns        = ['_build', 'Thumbs.db', '.DS_Store', '*.csv']
master_doc              = 'index'
html_theme              = 'sphinx_rtd_theme'

def configureDoxyfile(input_dir, output_dir):
    with open('Doxyfile.in', 'r') as file :
        filedata = file.read()

    filedata = filedata.replace('@DOXYGEN_INPUT_DIR@', input_dir)
    filedata = filedata.replace('@DOXYGEN_OUTPUT_DIR@', output_dir)

    with open('Doxyfile', 'w') as file:
        file.write(filedata)


docs_build_on_RtD = os.environ.get('READTHEDOCS', None) == 'True'

breathe_projects = {"SmartPeak" : "docs/xml"}

if docs_build_on_RtD:
    input_dir = '../smartpeak'
    output_dir = 'build'
    configureDoxyfile(input_dir, output_dir)
    subprocess.call('doxygen', shell=True)
    breathe_projects['SmartPeak'] = output_dir + '/xml'


exhale_args = {
    # These arguments are required
    "containmentFolder":     "./api",
    "rootFileName":          "library_root.rst",
    "rootFileTitle":         "Library API",
    "doxygenStripFromPath":  "..",
    # Suggested optional arguments
    "createTreeView":        True,
    # TIP: if using the sphinx-bootstrap-theme, you need
    # "treeViewIsBootstrap": True,
    "exhaleExecutesDoxygen": True,
    "exhaleDoxygenStdin":    "INPUT = include/SmartPeak"
}


primary_domain      = 'cpp'
highlight_language  = 'cpp'