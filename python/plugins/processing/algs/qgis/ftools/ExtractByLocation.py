# -*- coding: utf-8 -*-

"""
***************************************************************************
    ExtractByLocation.py
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

from PyQt4.QtCore import *
from qgis.core import *
from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.parameters.ParameterVector import ParameterVector
from processing.parameters.ParameterBoolean import ParameterBoolean
from processing.outputs.OutputVector import OutputVector
from processing.tools import dataobjects, vector


class ExtractByLocation(GeoAlgorithm):

    INPUT = 'INPUT'
    INTERSECT = 'INTERSECT'
    TOUCHES = 'TOUCHES'
    OVERLAPS = 'OVERLAPS'
    WITHIN = 'WITHIN'
    OUTPUT = 'OUTPUT'

    METHODS = ['creating new selection', 'adding to current selection',
               'removing from current selection']
    opFlags = 0
    operators = {'TOUCHES':1,'OVERLAPS':2,'WITHIN':4}

    def defineCharacteristics(self):
        self.name = 'Extract by location'
        self.group = 'Vector selection tools'
        self.addParameter(ParameterVector(self.INPUT, 'Layer to select from',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterVector(self.INTERSECT,
                          'Additional layer (intersection layer)',
                          [ParameterVector.VECTOR_TYPE_ANY]))
        self.addParameter(ParameterBoolean(self.TOUCHES,
                          'Include input features that touch the selection features',
                          [True]))
        self.addParameter(ParameterBoolean(self.OVERLAPS,
                          'Include input features that overlap/cross the selection features',
                          [True]))
        self.addParameter(ParameterBoolean(self.WITHIN,
                          'Include input features completely within the selection features',
                          [True]))
        self.addOutput(OutputVector(self.OUTPUT, 'Selection'))

    def processAlgorithm(self, progress):
        filename = self.getParameterValue(self.INPUT)
        layer = dataobjects.getObjectFromUri(filename)
        filename = self.getParameterValue(self.INTERSECT)
        selectLayer = dataobjects.getObjectFromUri(filename)
        index = vector.spatialindex(layer)

        def _points_op(geomA,geomB):
            return geomA.intersects(geomB)

        def _poly_lines_op(geomA,geomB):
            if geomA.disjoint(geomB):
                return False
            intersects = False
            if self.opFlags & self.operators['TOUCHES']:
                intersects |= geomA.touches(geomB)
            if not intersects and (self.opFlags & self.operators['OVERLAPS']):
                if geomB.type() == QGis.Line or geomA.type() == QGis.Line:
                    intersects |= geomA.crosses(geomB)
                else:
                    intersects |= geomA.overlaps(geomB)
            if not intersects and (self.opFlags & self.operators['WITHIN']):
                intersects |= geomA.contains(geomB)
            return intersects

        def _sp_operator():
            if layer.geometryType() == QGis.Point:
                return _points_op
            else:
                return _poly_lines_op

        self.opFlags = 0
        if self.getParameterValue(self.TOUCHES):
            self.opFlags |= self.operators['TOUCHES']
        if self.getParameterValue(self.OVERLAPS):
            self.opFlags |= self.operators['OVERLAPS']
        if self.getParameterValue(self.WITHIN):
            self.opFlags |= self.operators['WITHIN']

        sp_operator = _sp_operator()

        output = self.getOutputFromName(self.OUTPUT)
        writer = output.getVectorWriter(layer.pendingFields(),
                layer.dataProvider().geometryType(), layer.crs())

        geom = QgsGeometry()
        selectedSet = []
        current = 0
        features = vector.features(selectLayer)
        featureCount = len(features)
        total = 100.0 / float(len(features))
        for current,f in enumerate(features):
            geom = QgsGeometry(f.geometry())
            intersects = index.intersects(geom.boundingBox())
            for i in intersects:
                request = QgsFeatureRequest().setFilterFid(i)
                feat = layer.getFeatures(request).next()
                tmpGeom = QgsGeometry(feat.geometry())
                if sp_operator(geom,tmpGeom):
                    selectedSet.append(feat.id())
            progress.setPercentage(int(current * total))

        for i, f in enumerate(vector.features(layer)):
            if f.id() in selectedSet:
                writer.addFeature(f)
            progress.setPercentage(100 * i / float(featureCount))
        del writer
