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
	std::vector<MVector> keyframes;

	MVector startPos = m_cameraPath.inclusiveMatrix() * MPoint::origin;
	keyframes.push_back(startPos);

	double timeToApex = -m_velocity.y / m_gravity;

	double apexX = startPos.x + m_velocity.x * timeToApex;
	double apexY = startPos.y + m_velocity.y * timeToApex + 0.5 * m_gravity * timeToApex * timeToApex;
	double apexZ = startPos.z + m_velocity.z * timeToApex;
	keyframes.push_back(MVector(apexX, apexY, apexZ));

	int totalFrames = calculateFlightFrames();
	MTime::Unit timeUnit = MTime::uiUnit();
	MTime oneFrame(1.0, timeUnit);
	double secondsPerFrame = oneFrame.asUnits(MTime::kSeconds);
	double totalTime = totalFrames * secondsPerFrame;

	double endX = startPos.x + m_velocity.x * totalTime;
	double endY = startPos.y + m_velocity.y * totalTime + 0.5 * m_gravity * totalTime * totalTime;
	double endZ = startPos.z + m_velocity.z * totalTime;
	keyframes.push_back(MVector(endX, endY, endZ));

	return keyframes;
}

MStatus CameraLaunchCmd::setKeyframesOnCamera(const std::vector<MVector>& points)
{
	MStatus status = MS::kSuccess;

	// Use the stored camera path instead of getting from selection
	MObject cameraTransform = m_cameraPath.transform(&status);
	if (status != MS::kSuccess) {
		MGlobal::displayError("Failed to get camera transform");
		return status;
	}

	if (!cameraTransform.hasFn(MFn::kTransform)) {
		MGlobal::displayError("Camera object is not a transform node");
		return MS::kFailure;
	}

	MFnTransform transformFn(cameraTransform);

	MPlug translateXPlug = transformFn.findPlug("translateX", false, &status);
	MPlug translateYPlug = transformFn.findPlug("translateY", false, &status);
	MPlug translateZPlug = transformFn.findPlug("translateZ", false, &status);

	if (status != MS::kSuccess) {
		MGlobal::displayError("Failed to get translate plugs");
		return status;
	}

	MFnAnimCurve animCurveX, animCurveY, animCurveZ;
	MObject animCurveObjX, animCurveObjY, animCurveObjZ;

	if (translateXPlug.isConnected()) {
		MPlugArray connections;
		translateXPlug.connectedTo(connections, true, false);
		for (unsigned int i = 0; i < connections.length(); ++i) {
			MObject connectedNode = connections[i].node();
			if (connectedNode.hasFn(MFn::kAnimCurve)) {
				MGlobal::deleteNode(connectedNode);
			}
		}
	}

	if (translateYPlug.isConnected()) {
		MPlugArray connections;
		translateYPlug.connectedTo(connections, true, false);
		for (unsigned int i = 0; i < connections.length(); ++i) {
			MObject connectedNode = connections[i].node();
			if (connectedNode.hasFn(MFn::kAnimCurve)) {
				MGlobal::deleteNode(connectedNode);
			}
		}
	}

	if (translateZPlug.isConnected()) {
		MPlugArray connections;
		translateZPlug.connectedTo(connections, true, false);
		for (unsigned int i = 0; i < connections.length(); ++i) {
			MObject connectedNode = connections[i].node();
			if (connectedNode.hasFn(MFn::kAnimCurve)) {
				MGlobal::deleteNode(connectedNode);
			}
		}
	}

	animCurveObjX = animCurveX.create(translateXPlug, NULL, &status);
	animCurveObjY = animCurveY.create(translateYPlug, NULL, &status);
	animCurveObjZ = animCurveZ.create(translateZPlug, NULL, &status);

	std::vector<int> frameNumbers = getKeyFrameNumbers();

	MTime::Unit timeUnit = MTime::uiUnit();

	for (size_t i = 0; i < points.size() && i < frameNumbers.size(); ++i) {
		MTime currentTime(frameNumbers[i], timeUnit);

		MStatus keyStatus;
		unsigned int keyIndexX = animCurveX.addKey(currentTime, points[i].x,
			MFnAnimCurve::kTangentLinear,
			MFnAnimCurve::kTangentLinear,
			NULL, &keyStatus);
		if (keyStatus != MS::kSuccess) {
			MGlobal::displayError(MString("Failed to set X keyframe at frame ") + frameNumbers[i]);
			return keyStatus;
		}

		unsigned int keyIndexY = animCurveY.addKey(currentTime, points[i].y,
			MFnAnimCurve::kTangentLinear,
			MFnAnimCurve::kTangentLinear,
			NULL, &keyStatus);
		if (keyStatus != MS::kSuccess) {
			MGlobal::displayError(MString("Failed to set Y keyframe at frame ") + frameNumbers[i]);
			return keyStatus;
		}

		unsigned int keyIndexZ = animCurveZ.addKey(currentTime, points[i].z,
			MFnAnimCurve::kTangentLinear,
			MFnAnimCurve::kTangentLinear,
			NULL, &keyStatus);
		if (keyStatus != MS::kSuccess) {
			MGlobal::displayError(MString("Failed to set Z keyframe at frame ") + frameNumbers[i]);
			return keyStatus;
		}
	}

	M3dView::active3dView().refresh();

	MGlobal::displayInfo(MString("Successfully set ") + (int)points.size() +
		" keyframes on camera trajectory starting from frame " + m_startFrame);

	return MS::kSuccess;
}

int CameraLaunchCmd::calculateFlightFrames()
{
	// Handle case where there's no vertical velocity or gravity is positive
	if (m_velocity.y <= 0.0 || m_gravity >= 0.0) {
		// Default to 120 frames (about 5 seconds at 24fps)
		return 120;
	}

	// Calculate flight time using projectile motion: t = 2 * v0y / |g|
	double flightTime = (2.0 * m_velocity.y) / fabs(m_gravity);

	// Get the current frame rate
	MTime::Unit timeUnit = MTime::uiUnit();
	MTime oneFrame(1.0, timeUnit);
	double secondsPerFrame = oneFrame.asUnits(MTime::kSeconds);
	double fps = 1.0 / secondsPerFrame;

	int flightFrames = (int)(flightTime * fps);

	// Ensure we have at least a few frames
	return (flightFrames > 0) ? flightFrames : 60;
}

std::vector<int> CameraLaunchCmd::getKeyFrameNumbers()
{
	std::vector<int> frameNumbers;

	frameNumbers.push_back(m_startFrame);

	// Apex frame
	double timeToApex = -m_velocity.y / m_gravity;
	MTime::Unit timeUnit = MTime::uiUnit();
	MTime oneFrame(1.0, timeUnit);
	double secondsPerFrame = oneFrame.asUnits(MTime::kSeconds);
	int apexFrame = 1 + (int)(timeToApex / secondsPerFrame);
	frameNumbers.push_back(apexFrame);

	int totalFrames = calculateFlightFrames();
	frameNumbers.push_back(1 + totalFrames);

	return frameNumbers;
}