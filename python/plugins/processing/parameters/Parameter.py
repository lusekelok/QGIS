# -*- coding: utf-8 -*-

"""
***************************************************************************
    Parameter.py
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


class Parameter:
    """Base class for all parameters that a geoalgorithm might
    take as input.
    """

    def __init__(self, name='', description=''):
        self.name = name
        self.description = description
        self.value = None

        self.isAdvanced = False

        # A hidden parameter can be used to set a hard-coded value.
        # It can be used as any other parameter, but it will not be
        # shown to the user
        self.hidden = False

    def setValue(self, obj):
        """Sets the value of the parameter.

        Returns true if the value passed is correct for the type
        of parameter.
        """
        self.value = str(obj)
        return True

    def __str__(self):
        return self.name + ' <' + self.__module__.split('.')[-1] + '>'

    def serialize(self):
        return self.__module__.split('.')[-1] + '|' + self.name + '|' \
            + self.description

    def getValueAsCommandLineParameter(self):
        """Returns the value of this parameter as it should have been
        entered in the console if calling an algorithm using the
        Processing.runalg() method.
        """
        return str(self.value)

    def parameterName(self):
        return self.__module__.split('.')[-1]

    def todict(self):
        return self.__dict__


