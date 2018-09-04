""" @package amici
The AMICI Python module (in doxygen this will also contain documentation about the C++ library)

The AMICI Python module provides functionality for importing SBML models and turning them into C++ Python extensions.

Getting started:
```
# creating a extension module for an SBML model:
import amici
amiSbml = amici.SbmlImporter('mymodel.sbml')
amiSbml.sbml2amici('modelName', 'outputDirectory')

# using the created module (set python path)
import modelName
help(modelName)
```

Attributes:
    dirname: absolute path to parent directory of this file
    amici_path: absolute root path of the amici repository
    amiciSwigPath: absolute path of the amici swig directory
    amiciSrcPath: absolute path of the amici source directory
    amiciModulePath: absolute root path of the amici module
    hdf5_enabled: boolean indicating if amici was compiled with hdf5 support
"""

import os

# If this file is inside the amici package, import swig interface,
# otherwise we are inside the git repository, then don't
dirname = os.path.dirname(__file__)
hdf5_enabled = False
if os.path.isfile(os.path.join(dirname, 'amici.py')):
    try:
        from . import amici
        from .amici import *
        hdf5_enabled = True
    except AttributeError:
        from . import amici_without_hdf5 as amici
        from .amici_without_hdf5 import *

# determine package installation path, or, if used directly from git
# repository, get repository root
if os.path.exists(os.path.join(os.path.dirname(__file__), '..', '..', '.git')):
    amici_path = os.path.abspath(os.path.join(
        os.path.dirname(__file__), '..', '..'))
else:
    amici_path = os.path.dirname(__file__)

amiciSwigPath = os.path.join(amici_path, 'swig')
amiciSrcPath = os.path.join(amici_path, 'src')
amiciModulePath = os.path.dirname(__file__)

from .sbml_import import SbmlImporter, assignmentRules2observables, \
    constantSpeciesToParameters
from .numpy import rdataToNumPyArrays, edataToNumPyArrays
from .pandas import constructEdataFromDataFrame, \
    getDataObservablesAsDataFrame, getSimulationObservablesAsDataFrame, \
    getSimulationStatesAsDataFrame, getResidualsAsDataFrame


def runAmiciSimulation(model, solver, edata=None):
    """ Convenience wrapper around amici.runAmiciSimulation (generated by swig)

    Arguments:
        model: Model instance
        solver: Solver instance, must be generated from Model.getSolver()
        edata: ExpData instance (optional)

    Returns:
        ReturnData object with simulation results

    Raises:
        
    """
    if edata and edata.__class__.__name__ == 'ExpDataPtr':
        edata = edata.get()

    rdata = amici.runAmiciSimulation(solver.get(), edata, model.get())
    return rdataToNumPyArrays(rdata)

def ExpData(rdata, sigma_y, sigma_z):
    """ Convenience wrapper for ExpData constructor

    Arguments:
        rdata: rdataToNumPyArrays output
        sigma_y: standard deviation for ObservableData
        sigma_z: standard deviation for EventData

    Returns:
        ExpData Instance

    Raises:

    """
    return amici.ExpData(rdata['ptr'].get(), sigma_y, sigma_z)


def runAmiciSimulations(model, solver, edata_list):
    """ Convenience wrapper for loops of amici.runAmiciSimulation

    Arguments:
        model: Model instance
        solver: Solver instance, must be generated from Model.getSolver()
        edata_list: list of ExpData instances

    Returns:
        list of ReturnData objects with simulation results

    Raises:

    """
    rdata_list = []
    for edata in edata_list:
        rdata = runAmiciSimulation(
            model,
            solver,
            edata,
        )
        rdata_list.append(rdata)

    return rdata_list