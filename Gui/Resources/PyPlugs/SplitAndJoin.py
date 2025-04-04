# -*- coding: utf-8 -*-
#Split and Join Toolset PyPlug
import NatronEngine

#To access selection in the NodeGraph
from NatronGui import *
import sys

#Specify that this is a toolset and not a regular PyPlug
def getIsToolset():
    return True

def getPluginID():
    return "fr.inria.SplitAndJoin"

def getLabel():
    return "Split and Join"

def getVersion():
    return 1

def getGrouping():
    return "Views"

def getPluginDescription():
    return "Автоматически создает по одному виду для каждого вида в проекте после выбранного узла и объединяет их."

def createInstance(app,group):
    #Since this is a toolset, group will be set to None
    
    views = app.getViewNames();
    nbviews = len(views)
    if nbviews <= 1:
        natron.warningDialog("Split and Join","Проект должен содержать как минимум 2 вида")
        return
    
    selectedNodes = app.getSelectedNodes();
    if len(selectedNodes) != 1:
        natron.warningDialog("Split and Join","Вы должны выбрать только один узел")
        return

    selNode = selectedNodes[0]
    if selNode.isOutputNode():
        natron.warningDialog("Split and Join","Выбранный узел не должен быть узлом просмотра или вывода")
        return

    selectedNodePosition = selNode.getPosition()
    selectedNodeSize = selNode.getSize()


    totalWidth = nbviews * selectedNodeSize[0] + ((nbviews - 1) * selectedNodeSize[0] / 2)

    yposition = selectedNodePosition[1] + selectedNodeSize[1] * 3.
    xposition = selectedNodePosition[0] + selectedNodeSize[0] / 2. - totalWidth / 2.

    joinViewsNode = app.createNode("fr.inria.built-in.JoinViews")
    joinViewsNode.setPosition(selectedNodePosition[0], yposition + selectedNodeSize[1] * 3.)

    for i, v in enumerate(views):
        oneViewNode = app.createNode("fr.inria.built-in.OneView")
        oneViewNode.setLabel(v)
        oneViewNode.getParam("view").set(i)
        oneViewNode.setPosition(xposition,yposition)
        oneViewNode.connectInput(0,selNode)
        joinViewsNode.connectInput(nbviews - i - 1, oneViewNode)
        xposition += selectedNodeSize[0] * 1.5



