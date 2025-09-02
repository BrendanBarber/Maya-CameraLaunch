#pragma once

#include <vector>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>

class CameraLaunchCmd : public MPxCommand
{
public:
	static const char* commandName;

	CameraLaunchCmd();
	virtual ~CameraLaunchCmd();

	virtual MStatus doIt(const MArgList& args) override;
	virtual MStatus redoIt() override;
	virtual MStatus undoIt() override;
	virtual bool isUndoable() const override;

	static void* creator();

	static MSyntax newSyntax();

private:
	static const char* cameraFlag;
	static const char* cameraFlagLong;
	static const char* velocityFlag;
	static const char* velocityFlagLong;
	static const char* gravityFlag;
	static const char* gravityFlagLong;
	static const char* startFrameFlag;
	static const char* startFrameLongFlag;

	MDagPath m_cameraPath;
	MVector m_velocity;
	double m_gravity;
	int m_startFrame;

	bool m_hasValidData;
	MSelectionList m_originalSelection;

	std::vector<MVector> calculateTrajectory();
	MStatus setKeyframesOnCamera(const std::vector<MVector>& points);
	int calculateFlightFrames();


	MStatus parseArguments(const MArgList& args);
	MStatus executeCommand();
	MStatus generateKeyframes();
};