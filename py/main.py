import maya.cmds as cmds

cmds.cameraLaunch(camera="camera", velocity=(10.0, 10.0, 3.0), gravity=-9.81, startFrame=1)