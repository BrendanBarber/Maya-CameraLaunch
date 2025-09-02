#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "CameraLaunchCmd.h"

MStatus initializePlugin(MObject obj)
{
	const char* pluginVendor = "Brendan Barber";
	const char* pluginVersion = "0.1";

	MFnPlugin fnPlugin(obj, pluginVendor, pluginVersion);

	fnPlugin.registerCommand(CameraLaunchCmd::commandName, CameraLaunchCmd::creator, CameraLaunchCmd::newSyntax);

	MGlobal::displayInfo("Plugin has been initialized!");

	return (MS::kSuccess);
}

MStatus uninitializePlugin(MObject obj)
{

	MFnPlugin fnPlugin(obj);
	
	fnPlugin.deregisterCommand(CameraLaunchCmd::commandName);

	MGlobal::displayInfo("Plugin has been uninitialized!");

	return (MS::kSuccess);
}