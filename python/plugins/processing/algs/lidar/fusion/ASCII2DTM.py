# -*- coding: utf-8 -*-

"""
***************************************************************************
    ASCII2DTM.py
    ---------------------
    Date                 : May 2014
    Copyright            : (C) 2014 by Niccolo' Marchi
    Email                : sciurusurbanus at hotmail dot it
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""

__author__ = "Niccolo' Marchi"
__date__ = 'May 2014'
__copyright__ = "(C) 2014 by Niccolo' Marchi"

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import os
import subprocess
from processing.parameters.ParameterFile import ParameterFile
from processing.parameters.ParameterSelection import ParameterSelection
from processing.parameters.ParameterNumber import ParameterNumber
from processing.outputs.OutputFile import OutputFile
from FusionAlgorithm import FusionAlgorithm
from FusionUtils import FusionUtils


class ASCII2DTM(FusionAlgorithm):

    INPUT = 'INPUT'
    OUTPUT = 'OUTPUT'
    COORDSYS = 'COORDSYS'
    XYUNITS = 'XYUNITS'
    ZUNITS = 'ZUNITS'
    UNITS = ['Meter', 'Feet']
    ZONE = 'ZONE'

    def defineCharacteristics(self):
        self.name = 'ASCII to DTM'
        self.group = 'Conversion'
        self.addParameter(ParameterFile(self.INPUT, 'Input ESRI ASCII layer'))
        self.addParameter(ParameterSelection(self.XYUNITS, 'XY Units',
                          self.UNITS))
        self.addParameter(ParameterSelection(self.ZUNITS, 'Z Units',
                          self.UNITS))
        self.addParameter(ParameterSelection(self.COORDSYS, 'Coordinate system',
                          ['unknown', 'UTM', 'state plane']))
        self.addParameter(ParameterNumber(self.ZONE, "Coordinate system zone ('0' for unknown)", 0, None,
                          0))
        self.addOutput(OutputFile(self.OUTPUT, 'Output surface', 'dtm'))
        self.addAdvancedModifiers()

    def processAlgorithm(self, progress):
        commands = [os.path.join(FusionUtils.FusionPath(), 'ASCII2DTM.exe')]
        commands.append('/verbose')
        self.addAdvancedModifiersToCommand(commands)
        outFile = self.getOutputValue(self.OUTPUT)
        commands.append(outFile)
        commands.append(self.UNITS[self.getParameterValue(self.XYUNITS)][0])
        commands.append(self.UNITS[self.getParameterValue(self.ZUNITS)][0])
        commands.append(str(self.getParameterValue(self.COORDSYS)))
        commands.append(str(self.getParameterValue(self.ZONE)))
        commands.append('0')
        commands.append('0')
        files = self.getParameterValue(self.INPUT).split(';')
        if len(files) == 1:
            commands.append(self.getParameterValue(self.INPUT))
        else:
            FusionUtils.createFileList(files)
            commands.append(FusionUtils.tempFileListFilepath())
        FusionUtils.runFusion(commands, progress)
