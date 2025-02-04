# -*- coding: utf-8 -*-

"""
***************************************************************************
    ModelerAlgorithm.py
    ---------------------
    Date                 : August 2012
    Copyright            : (C) 2012 by Victor Olaya
    Email                : volayaf at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""


__author__ = 'Victor Olaya'
__date__ = 'August 2012'
__copyright__ = '(C) 2012, Victor Olaya'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os.path
import sys
import copy
import time
import json
from PyQt4 import QtCore, QtGui
from qgis.core import *
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.modeler.WrongModelException import WrongModelException
from processing.core.GeoAlgorithmExecutionException import \
        GeoAlgorithmExecutionException
from processing.gui.Help2Html import  getHtmlFromHelpFile
from processing.modeler.ModelerUtils import ModelerUtils
from processing.parameters.ParameterRaster import ParameterRaster
from processing.parameters.ParameterDataObject import ParameterDataObject
from processing.parameters.ParameterExtent import ParameterExtent
from processing.parameters.ParameterMultipleInput import ParameterMultipleInput
from processing.parameters.ParameterVector import ParameterVector
from processing.tools import dataobjects


class Input():

    def __init__(self, param=None, pos=None):
        self.param = param
        self.pos = pos

    def todict(self):
        return self.__dict__

    @staticmethod
    def fromdict(d):
        return Input(d["param"], d["pos"])


class Output():

    def __init__(self, description=""):
        self.description = description
        self.pos = None

    def todict(self):
        return self.__dict__




class Algorithm():

    def __init__(self, consoleName=""):

        self.name = None
        self.description = ""

        #The type of the algorithm, indicated as a string, which corresponds
        #to the string used to refer to it in the python console
        self.consoleName = consoleName

        self._algInstance = None

        #A dict of Input object. keys are param names
        self.params = {}

        #A dict of Output with final output descriptions. Keys are output names.
        #Outputs not final are not stored in this dict
        self.outputs = {}

        self.pos = None

        self.dependencies = []

        self.paramsFolded = True
        self.outputsFolded = True
        self.active = True


    def todict(self):
        return {k:v for k,v in self.__dict__.iteritems() if  not k.startswith("_")}

    @property
    def algorithm(self):
        if self._algInstance is None:
            self._algInstance = ModelerUtils.getAlgorithm(self.consoleName).getCopy();
        return self._algInstance

    def setName(self, model):
        if self.name is None:
            i = 1
            name = self.consoleName + "_" + str(i)
            while name in model.algs:
                i += 1
                name = self.consoleName + "_" + str(i)
            self.name = name

class ValueFromInput():

    def __init__(self, name=""):
        self.name = name

    def todict(self):
        return self.__dict__

    def __str__(self):
        return self.name

    def __eq__(self, other):
        try:
            return self.name == other.name
        except:
            return False

class ValueFromOutput():

    def __init__(self, alg="", output=""):
        self.alg = alg
        self.output = output

    def todict(self):
        return self.__dict__

    def __eq__(self, other):
        try:
            return self.alg == other.alg and self.output == other.output
        except:
            return False

    def __str__(self):
        return self.alg + "," + self.output

class ModelerAlgorithm(GeoAlgorithm):

    CANVAS_SIZE = 4000

    def getCopy(self):
        newone = ModelerAlgorithm()
        newone.provider = self.provider
        newone.algs = copy.deepcopy(self.algs)
        newone.inputs = copy.deepcopy(self.inputs)
        newone.defineCharacteristics()
        newone.name = self.name
        newone.group = self.group
        newone.descriptionFile = self.descriptionFile
        return newone

    def __init__(self):
        self.name = "Model"
        # The dialog where this model is being edited
        self.modelerdialog = None
        self.descriptionFile = None
        self.helpContent = {}

        # Geoalgorithms in this model. A dict of Algorithm objects, with names as keys
        self.algs = {}

        #Input parameters. A dict of Input objects, with names as keys
        self.inputs = {}
        GeoAlgorithm.__init__(self)

    def getIcon(self):
        return QtGui.QIcon(os.path.dirname(__file__) + '/../images/model.png')

    def defineCharacteristics(self):
        self.parameters = [inp.param for inp in self.inputs.values()]
        self.outputs = []
        for alg in self.algs.values():
            if alg.active:
                for out in alg.outputs:
                    modelOutput = copy.deepcopy(alg.algorithm.getOutputFromName(out))
                    modelOutput.name = self.getSafeNameForOutput(alg.name, out)
                    modelOutput.description = alg.outputs[out].description
                    self.outputs.append(modelOutput)

    def addParameter(self, param):
        self.inputs[param.param.name] = param

    def updateParameter(self, param):
        self.inputs[param.name].param = param

    def addAlgorithm(self, alg):
        name = self.getNameForAlgorithm(alg)
        alg.name = name
        self.algs[name] = alg

    def getNameForAlgorithm(self, alg):
        i = 1
        while alg.consoleName.upper().replace(":", "") + "_" + str(i) in self.algs.keys():
            i += 1
        return alg.consoleName.upper().replace(":", "") + "_" + str(i)

    def updateAlgorithm(self, alg):
        alg.pos = self.algs[alg.name].pos
        self.algs[alg.name] = alg

        from processing.modeler.ModelerGraphicItem import ModelerGraphicItem
        for i, out in enumerate(alg.outputs):
            alg.outputs[out].pos = (alg.outputs[out].pos or
                    alg.pos + QtCore.QPointF(ModelerGraphicItem.BOX_WIDTH,
                                             (i + 1.5) * ModelerGraphicItem.BOX_HEIGHT))

    def removeAlgorithm(self, name):
        """Returns True if the algorithm could be removed, False if
        others depend on it and could not be removed.
        """
        if self.hasDependencies(name):
            return False
        del self.algs[name]
        self.modelerdialog.hasChanged = True
        return True

    def removeParameter(self, name):
        """Returns True if the parameter could be removed, False if
        others depend on it and could not be removed.
        """
        if self.hasDependencies(name):
            return False
        del self.inputs[name]
        self.modelerdialog.hasChanged = True
        return True

    def hasDependencies(self, name):
        """This method returns True if some other element depends on
        the passed one.
        """
        for alg in self.algs.values():
            for value in alg.params.values():
                if value is None:
                    continue
                if isinstance(value, list):
                    for v in value:
                        if isinstance(v, ValueFromInput):
                            if v.name == name:
                                return True
                        elif isinstance(v, ValueFromOutput):
                            if v.alg == name:
                                return True
                if isinstance(value, ValueFromInput):
                    if value.name == name:
                        return True
                elif isinstance(value, ValueFromOutput):
                    if value.alg == name:
                        return True
        return False


    def getDependsOnAlgorithms(self, name):
        """This method returns a list with names of algorithms
        a given one depends on.
        """
        alg = self.algs[name]
        algs = set()
        algs.update(set(alg.dependencies))
        for value in alg.params.values():
            if value is None:
                continue
            if isinstance(value, list):
                for v in value:
                    if isinstance(v, ValueFromOutput):
                        algs.add(v.alg)
                        algs.update(self.getDependsOnAlgorithms(v.alg))
            elif isinstance(value, ValueFromOutput):
                algs.add(value.alg)
                algs.update(self.getDependsOnAlgorithms(value.alg))


        return algs

    def getDependentAlgorithms(self, name):
        """This method returns a list with the names of algorithms
        depending on a given one. It includes the algorithm itself
        """
        algs = set()
        algs.add(name)
        for alg in self.algs.values():
            for value in alg.params.values():
                if value is None:
                    continue
                if isinstance(value, list):
                    for v in value:
                        if isinstance(v, ValueFromOutput) and v.alg == name:
                            algs.update(self.getDependentAlgorithms(alg.name))
                elif isinstance(value, ValueFromOutput) and value.alg == name:
                            algs.update(self.getDependentAlgorithms(alg.name))

        return algs

    def setPositions(self, paramPos, algPos, outputsPos):
        for param, pos in paramPos.iteritems():
            self.inputs[param].pos = pos
        for alg, pos in algPos.iteritems():
            self.algs[alg].pos = pos
        for alg, positions in outputsPos.iteritems():
            for output, pos in positions.iteritems():
                self.algs[alg].outputs[output].pos = pos

    def prepareAlgorithm(self, alg):
        algInstance = alg.algorithm
        for param in algInstance.parameters:
            if not param.hidden:
                value = self.resolveValue(alg.params[param.name])
                if value is None and isinstance(param, ParameterExtent):
                    value = self.getMinCoveringExtent()
                # We allow unexistent filepaths, since that allows
                # algorithms to skip some conversion routines
                if not param.setValue(value) and not isinstance(param,
                        ParameterDataObject):
                    raise GeoAlgorithmExecutionException('Wrong value: '
                            + str(value))
        for out in algInstance.outputs:
            if not out.hidden:
                if out.name in alg.outputs:
                    name = self.getSafeNameForOutput(alg.name, out.name)
                    modelOut = self.getOutputFromName(name)
                    if modelOut:
                        out.value = modelOut.value
                else:
                    out.value = None

        return algInstance

    def deactivateAlgorithm(self, algName):
        dependent = self.getDependentAlgorithms(algName)
        for alg in dependent:
            self.algs[alg].active = False

    def activateAlgorithm(self, algName):
        parents = self.getDependsOnAlgorithms(algName)
        for alg in parents:
            if not self.algs[alg].active:
                return False
        self.algs[algName].active = True
        return True

    def getSafeNameForOutput(self, algName, outName):
        return outName + '_ALG' + algName

    def resolveValue(self, value):
        if value is None:
            return None
        if isinstance(value, list):
            return ";".join([self.resolveValue(v) for v in value])
        if isinstance(value, ValueFromInput):
            return self.getParameterFromName(value.name).value
        elif isinstance(value, ValueFromOutput):
            return self.algs[value.alg].algorithm.getOutputFromName(value.output).value
        else:
            return value

    def getMinCoveringExtent(self):
        first = True
        found = False
        for param in self.parameters:
            if param.value:
                if isinstance(param, (ParameterRaster, ParameterVector)):
                    found = True
                    if isinstance(param.value, (QgsRasterLayer, QgsVectorLayer)):
                        layer = param.value
                    else:
                        layer = dataobjects.getObjectFromUri(param.value)
                    self.addToRegion(layer, first)
                    first = False
                elif isinstance(param, ParameterMultipleInput):
                    found = True
                    layers = param.value.split(';')
                    for layername in layers:
                        layer = dataobjects.getObjectFromUri(layername)
                        self.addToRegion(layer, first)
                        first = False
        if found:
            return ','.join([str(v) for v in [self.xmin, self.xmax, self.ymin, self.ymax]])
        else:
            return None

    def addToRegion(self, layer, first):
        if first:
            self.xmin = layer.extent().xMinimum()
            self.xmax = layer.extent().xMaximum()
            self.ymin = layer.extent().yMinimum()
            self.ymax = layer.extent().yMaximum()
        else:
            self.xmin = min(self.xmin, layer.extent().xMinimum())
            self.xmax = max(self.xmax, layer.extent().xMaximum())
            self.ymin = min(self.ymin, layer.extent().yMinimum())
            self.ymax = max(self.ymax, layer.extent().yMaximum())


    def processAlgorithm(self, progress):
        executed = []
        toExecute = [alg for alg in self.algs.values() if alg.active]
        while len(executed) < len(toExecute):
            for alg in toExecute:
                if alg.name not in executed:
                    canExecute = True
                    required = self.getDependsOnAlgorithms(alg.name)
                    for requiredAlg in required:
                        if requiredAlg != alg.name and requiredAlg not in executed:
                            canExecute = False
                            break
                    if canExecute:
                        try:
                            progress.setDebugInfo('Prepare algorithm: ' + alg.name)
                            self.prepareAlgorithm(alg)
                            progress.setText('Running %s [%i/%i]' % ( alg.description, len(executed) + 1 ,len(toExecute)))
                            progress.setDebugInfo('Parameters: ' + ', '.join([unicode(p).strip()
                                                + '=' + unicode(p.value) for p in alg.algorithm.parameters]))
                            t0 = time.time()
                            alg.algorithm.execute(progress, self)
                            dt = time.time() - t0
                            executed.append(alg.name)
                            progress.setDebugInfo(
                                    'OK. Execution took %0.3f ms (%i outputs).'
                                    % (dt, len(alg.algorithm.outputs)))
                        except GeoAlgorithmExecutionException, e:
                            progress.setDebugInfo('Failed')
                            raise GeoAlgorithmExecutionException(
                                    'Error executing algorithm %s\n%s' % (alg.description, e.msg))

        progress.setDebugInfo(
                'Model processed ok. Executed %i algorithms total' % len(executed))


    def getAsCommand(self):
        if self.descriptionFile:
            return GeoAlgorithm.getAsCommand(self)
        else:
            return None

    def commandLineName(self):
        if self.descriptionFile is None:
            return ''
        else:
            return 'modeler:' + os.path.basename(self.descriptionFile)[:-6].lower()

    def setModelerView(self, dialog):
        self.modelerdialog = dialog

    def updateModelerView(self):
        if self.modelerdialog:
            self.modelerdialog.repaintModel()

    def help(self):
        try:
            helpfile = self.descriptionFile + '.help'
            return True, getHtmlFromHelpFile(self, helpfile)
        except:
            return False, None

    def todict(self):
        keys = ["inputs", "group", "name", "algs"]
        return {k:v for k,v in self.__dict__.iteritems() if k in keys}

    def toJson(self):
        def todict(o):
            if isinstance(o, QtCore.QPointF):
                return {"class": "point", "values": {"x": o.x(), "y": o.y()}}
            try:
                d = o.todict()
                return {"class": o.__class__.__module__ + "." + o.__class__.__name__, "values": d}
            except Exception, e:
                pass
        return json.dumps(self, default=todict, indent=4)

    @staticmethod
    def fromJson(s):
        def fromdict(d):
            try:
                fullClassName = d["class"]
                tokens = fullClassName.split(".")
                className = tokens[-1]
                moduleName = ".".join(tokens[:-1])
                values = d["values"]
                if className == "point":
                    return QtCore.QPointF(values["x"], values["y"])
                def _import(name):
                    __import__(name)
                    return sys.modules[name]
                module = _import(moduleName)
                clazz = getattr(module, className)
                instance = clazz()
                for k,v in values.iteritems():
                    instance.__dict__[k] = v
                return instance
            except KeyError:
                return d
            except Exception, e:
                raise e
        try:
            model = json.loads(s, object_hook = fromdict)
        except Exception, e:
            raise WrongModelException(e.args[0])
        return model


    @staticmethod
    def fromJsonFile(filename):
        with open(filename) as f:
            s = f.read()
        alg = ModelerAlgorithm.fromJson(s)
        alg.descriptionFile = filename
        return alg

