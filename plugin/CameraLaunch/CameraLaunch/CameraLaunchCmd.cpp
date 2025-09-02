#include "CameraLaunchCmd.h"

const char* CameraLaunchCmd::commandName = "cameraLaunch";

const char* CameraLaunchCmd::cameraFlag = "-c";
const char* CameraLaunchCmd::cameraFlagLong = "-camera";
const char* CameraLaunchCmd::velocityFlag = "-v";
const char* CameraLaunchCmd::velocityFlagLong = "-velocity";
const char* CameraLaunchCmd::gravityFlag = "-g";
const char* CameraLaunchCmd::gravityFlagLong = "-gravity";
const char* CameraLaunchCmd::startFrameFlag = "-s";
const char* CameraLaunchCmd::startFrameLongFlag = "-startFrame";

CameraLaunchCmd::CameraLaunchCmd()
{
	//CameraLaunchCmd::m_cameraPath is null
	CameraLaunchCmd::m_velocity = MVector(0, 0, 0);
	CameraLaunchCmd::m_gravity = -9.81;
	CameraLaunchCmd::m_startFrame = 0;
	CameraLaunchCmd::m_hasValidData = false;
}

CameraLaunchCmd::~CameraLaunchCmd()
{
	
}

void* CameraLaunchCmd::creator()
{
	return new CameraLaunchCmd();
}

MSyntax CameraLaunchCmd::newSyntax()
{
	MSyntax syntax;
	syntax.addFlag(velocityFlag, velocityFlagLong, MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
	syntax.addFlag(cameraFlag, cameraFlagLong, MSyntax::kString);
	syntax.addFlag(gravityFlag, gravityFlagLong, MSyntax::kDouble);
	syntax.addFlag(startFrameFlag, startFrameLongFlag, MSyntax::kDouble);
	return syntax;
}

MStatus CameraLaunchCmd::doIt(const MArgList& args)
{
	MStatus status = parseArguments(args);
	if (!status) return status;

	return redoIt();
}

MStatus CameraLaunchCmd::redoIt()
{
	if (!m_hasValidData) {
		return MS::kFailure;
	}

	return executeCommand();
}

MStatus CameraLaunchCmd::undoIt()
{
	if (!m_hasValidData) {
		return MS::kFailure;
	}

	// Restore keyframes
	//

	// Restore selection
	MGlobal::setActiveSelectionList(m_originalSelection);

	return MS::kSuccess;
}

bool CameraLaunchCmd::isUndoable() const
{
	return true;
}

MStatus CameraLaunchCmd::parseArguments(const MArgList& args) 
{
	MArgDatabase argData(newSyntax(), args);
	
	// Extract Camera
	if (argData.isFlagSet(cameraFlag)) {
		MString cameraName = argData.flagArgumentString(cameraFlag, 0);

		MSelectionList selList;
		selList.add(cameraName);

		MStatus status = selList.getDagPath(0, m_cameraPath);
		if (!status) {
			return MS::kFailure;
		}
		if (!m_cameraPath.hasFn(MFn::kCamera)) {
			return MS::kFailure;
		}
	}
	
	// Extract Velocity
	if (argData.isFlagSet(velocityFlag)) {
		double vx = argData.flagArgumentDouble(velocityFlag, 0);
		double vy = argData.flagArgumentDouble(velocityFlag, 1);
		double vz = argData.flagArgumentDouble(velocityFlag, 2);

		m_velocity = MVector(vx, vy, vz);
	}

	// Extract Gravity
	if (argData.isFlagSet(gravityFlag)) {
		double gravity = argData.flagArgumentDouble(gravityFlag, 0);

		m_gravity = gravity;
	}

	// Extract Start Frame
	if (argData.isFlagSet(startFrameFlag)) {
		int startFrame = argData.flagArgumentInt(startFrameFlag, 0);

		m_startFrame = startFrame;
	}

	m_hasValidData = true;
	return MS::kSuccess;
}

MStatus CameraLaunchCmd::executeCommand()
{
	// Generate keyframes
	MStatus keyframeStatus = generateKeyframes();
	if (!keyframeStatus) {
		return keyframeStatus;
	}

	// Store current selection
	MGlobal::getActiveSelectionList(m_originalSelection);

	// Clear selection and select camera that was launched
	MSelectionList newSel;
	newSel.add(m_cameraPath);
	MGlobal::setActiveSelectionList(newSel);

	return MS::kSuccess;
}

MStatus CameraLaunchCmd::generateKeyframes()
{
	std::vector<MVector> trajectoryPoints = calculateTrajectory();
	return setKeyframesOnCamera(trajectoryPoints);
}

std::vector<MVector> CameraLaunchCmd::calculateTrajectory()
{
	
}

MStatus CameraLaunchCmd::setKeyframesOnCamera(const std::vector<MVector>& points)
{

}

int CameraLaunchCmd::calculateFlightFrames()
{
	double flightTime = (2.0 * m_velocity.y) / m_gravity;

	// Get the current frame rate
	MTime::Unit timeUnit = MTime::uiUnit();
	MTime oneFrame(1.0, timeUnit);
	double secondsPerFrame = oneFrame.asUnits(MTime::kSeconds);
	double fps = 1.0 / secondsPerFrame;

	int flightFrames = (int)(flightTime * fps);

	return flightFrames;
}