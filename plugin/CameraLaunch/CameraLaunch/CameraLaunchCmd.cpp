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
	std::vector<MEulerRotation> trajectoryRots = calculateRotations(trajectoryPoints);
	return setKeyframesOnCamera(trajectoryPoints, trajectoryRots);
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

std::vector<MEulerRotation> CameraLaunchCmd::calculateRotations(const std::vector<MVector> &points)
{
	std::vector<MEulerRotation> rotKeyframes;

	MVector normalizeVelocity = m_velocity.normal();

	// Set start rotation to be facing the direction of the input velocity
	const double PI = atan(1.0) * 4;
	double yaw = std::atan2(normalizeVelocity.x, normalizeVelocity.z) + PI;
	double pitch = std::asin(normalizeVelocity.y);

	rotKeyframes.push_back(MEulerRotation(pitch, yaw, 0));

	// Set the middle rotation to be a pitch of zero, facing along the trajectory
	rotKeyframes.push_back(MEulerRotation(0, yaw, 0));

	// Set the end rotation to be the negation of the input velocity
	MVector flippedNormalizedVelocity = normalizeVelocity * -1;
	pitch = std::asin(flippedNormalizedVelocity.y);
	rotKeyframes.push_back(MEulerRotation(pitch, yaw, 0));

	return rotKeyframes;
}

MStatus CameraLaunchCmd::setKeyframesOnCamera(const std::vector<MVector>& points, const std::vector<MEulerRotation>& rots)
{
	if (points.size() != 3) {
		MGlobal::displayError("Expected exactly 3 points for parabolic trajectory");
		return MS::kFailure;
	}

	std::vector<int> frameNumbers = getKeyFrameNumbers();
	if (frameNumbers.size() != 3) {
		MGlobal::displayError("Expected exactly 3 frame numbers");
		return MS::kFailure;
	}

	MStatus status = clearExistingAnimationCurves();
	if (status != MS::kSuccess) {
		return status;
	}

	status = setKeyframeOnCamera(points[0], rots[0], frameNumbers[0], CameraKeyframeType::START,
		points[0], points[1], points[2],
		frameNumbers[0], frameNumbers[1], frameNumbers[2]);
	if (status != MS::kSuccess) return status;

	status = setKeyframeOnCamera(points[1], rots[1], frameNumbers[1], CameraKeyframeType::MIDDLE,
		points[0], points[1], points[2],
		frameNumbers[0], frameNumbers[1], frameNumbers[2]);
	if (status != MS::kSuccess) return status;

	status = setKeyframeOnCamera(points[2], rots[2], frameNumbers[2], CameraKeyframeType::END,
		points[0], points[1], points[2],
		frameNumbers[0], frameNumbers[1], frameNumbers[2]);
	if (status != MS::kSuccess) return status;

	M3dView::active3dView().refresh();
	MGlobal::displayInfo("Successfully set 3 keyframes for parabolic camera trajectory");

	return MS::kSuccess;
}

MStatus CameraLaunchCmd::setKeyframeOnCamera(const MVector& point, const MEulerRotation& rot, int frameNumber, CameraKeyframeType keyType,
	const MVector& startPoint, const MVector& middlePoint, const MVector& endPoint,
	int startFrame, int middleFrame, int endFrame)
{
	MStatus status = MS::kSuccess;

	// Get camera transform
	MObject cameraTransform = m_cameraPath.transform(&status);
	if (status != MS::kSuccess) {
		MGlobal::displayError("Failed to get camera transform");
		return status;
	}

	MFnTransform transformFn(cameraTransform);
	MPlug translateXPlug = transformFn.findPlug("translateX", false, &status);
	MPlug translateYPlug = transformFn.findPlug("translateY", false, &status);
	MPlug translateZPlug = transformFn.findPlug("translateZ", false, &status);

	MPlug rotationXPlug = transformFn.findPlug("rotateX", false, &status);
	MPlug rotationYPlug = transformFn.findPlug("rotateY", false, &status);

	if (status != MS::kSuccess) {
		MGlobal::displayError("Failed to get translate plugs");
		return status;
	}

	// Get or create anim curves
	MFnAnimCurve animCurveX, animCurveY, animCurveZ;
	MObject animCurveObjX, animCurveObjY, animCurveObjZ;

	MFnAnimCurve animCurveRotX, animCurveRotY;
	MObject animCurveRotObjX, animCurveRotObjY;

	if (!getOrCreateAnimCurve(translateXPlug, animCurveX, animCurveObjX)) {
		return MS::kFailure;
	}
	if (!getOrCreateAnimCurve(translateYPlug, animCurveY, animCurveObjY)) {
		return MS::kFailure;
	}
	if (!getOrCreateAnimCurve(translateZPlug, animCurveZ, animCurveObjZ)) {
		return MS::kFailure;
	}
	if (!getOrCreateAnimCurve(rotationXPlug, animCurveRotX, animCurveRotObjX)) {
		return MS::kFailure;
	}
	if (!getOrCreateAnimCurve(rotationYPlug, animCurveRotY, animCurveRotObjY)) {
		return MS::kFailure;
	}

	MTime currentTime(frameNumber, MTime::uiUnit());
	MStatus keyStatus;

	// Set X keyframe (linear)
	unsigned int keyIndexX = animCurveX.addKey(currentTime, point.x,
		MFnAnimCurve::kTangentLinear,
		MFnAnimCurve::kTangentLinear,
		NULL, &keyStatus);
	if (keyStatus != MS::kSuccess) {
		MGlobal::displayError(MString("Failed to set X keyframe at frame ") + frameNumber);
		return keyStatus;
	}

	// Set Y keyframe with tangents for parabolic curve
	unsigned int keyIndexY = animCurveY.addKey(currentTime, point.y,
		MFnAnimCurve::kTangentFixed,
		MFnAnimCurve::kTangentFixed,
		NULL, &keyStatus);
	if (keyStatus != MS::kSuccess) {
		MGlobal::displayError(MString("Failed to set Y keyframe at frame ") + frameNumber);
		return keyStatus;
	}

	// Set parabolic tangents for Y curve
	setParabolicTangents(animCurveY, keyIndexY, keyType,
		startPoint, middlePoint, endPoint,
		startFrame, middleFrame, endFrame);

	// Set Z keyframe (linear)
	unsigned int keyIndexZ = animCurveZ.addKey(currentTime, point.z,
		MFnAnimCurve::kTangentLinear,
		MFnAnimCurve::kTangentLinear,
		NULL, &keyStatus);
	if (keyStatus != MS::kSuccess) {
		MGlobal::displayError(MString("Failed to set Z keyframe at frame ") + frameNumber);
		return keyStatus;
	}

	unsigned int keyIndexRotX = animCurveRotX.addKey(currentTime, rot.x,
		MFnAnimCurve::kTangentLinear,
		MFnAnimCurve::kTangentLinear,
		NULL, &keyStatus);
	if (keyStatus != MS::kSuccess) {
		MGlobal::displayError(MString("Failed to set X rotation keyframe at frame ") + frameNumber);
		return keyStatus;
	}

	unsigned int keyIndexRotY = animCurveRotY.addKey(currentTime, rot.y,
		MFnAnimCurve::kTangentLinear,
		MFnAnimCurve::kTangentLinear,
		NULL, &keyStatus);
	if (keyStatus != MS::kSuccess) {
		MGlobal::displayError(MString("Failed to set Y rotation keyframe at frame ") + frameNumber);
		return keyStatus;
	}

	return MS::kSuccess;
}

void CameraLaunchCmd::setParabolicTangents(MFnAnimCurve& animCurve, unsigned int keyIndex, CameraKeyframeType keyType,
	const MVector& startPoint, const MVector& middlePoint, const MVector& endPoint,
	int startFrame, int middleFrame, int endFrame)
{
	// Calculate parabolic coefficients
	MTime t0(startFrame, MTime::uiUnit());
	MTime t1(middleFrame, MTime::uiUnit());
	MTime t2(endFrame, MTime::uiUnit());

	double y0 = startPoint.y;
	double y1 = middlePoint.y;
	double y2 = endPoint.y;

	double dt1 = (t1 - t0).asUnits(MTime::uiUnit());
	double dt2 = (t2 - t0).asUnits(MTime::uiUnit());

	double denom = dt1 * dt2 * (dt2 - dt1);

	// Handle potential division by zero
	if (fabs(denom) < 1e-10) {
		MGlobal::displayWarning("Invalid time intervals for parabolic calculation, defaulting to linear tangents");
		return;
	}

	double a = (dt1 * (y2 - y0) - dt2 * (y1 - y0)) / denom;
	double b = (dt2 * dt2 * (y1 - y0) - dt1 * dt1 * (y2 - y0)) / denom;

	// Calculate slopes at start and end points
	double startSlope = b;
	double endSlope = 2.0 * a * dt2 + b;

	MAngle startOutAngle = atan(startSlope);
	MAngle endInAngle = atan(endSlope);
	MAngle flatAngle(0.0);

	double weight = 1.0;

	switch (keyType) {
	case CameraKeyframeType::START:
		// Set Out-Tangent
		animCurve.setTangent(keyIndex, startOutAngle, weight, false, NULL);
		break;

	case CameraKeyframeType::MIDDLE:
		// Set both tangents to flat (0 degrees) for parabolic peak
		animCurve.setTangent(keyIndex, flatAngle, weight, true, NULL);
		animCurve.setTangent(keyIndex, flatAngle, weight, false, NULL);
		break;

	case CameraKeyframeType::END:
		// Set In-tangent
		animCurve.setTangent(keyIndex, endInAngle, weight, true, NULL);
		break;
	}
}

bool CameraLaunchCmd::getOrCreateAnimCurve(MPlug& plug, MFnAnimCurve& animCurve, MObject& animCurveObj)
{
	MStatus status;

	if (plug.isConnected()) {
		MPlugArray connections;
		plug.connectedTo(connections, true, false);
		for (unsigned int i = 0; i < connections.length(); ++i) {
			MObject connectedNode = connections[i].node();
			if (connectedNode.hasFn(MFn::kAnimCurve)) {
				animCurveObj = connectedNode;
				animCurve.setObject(animCurveObj);
				return true;
			}
		}
	}

	// Create new anim curve if none exists
	animCurveObj = animCurve.create(plug, NULL, &status);
	return (status == MS::kSuccess);
}

MStatus CameraLaunchCmd::clearExistingAnimationCurves()
{
	MStatus status;
	MObject cameraTransform = m_cameraPath.transform(&status);
	if (status != MS::kSuccess) {
		MGlobal::displayError("Failed to get camera transform");
		return status;
	}

	MFnTransform transformFn(cameraTransform);
	MPlug translateXPlug = transformFn.findPlug("translateX", false, &status);
	MPlug translateYPlug = transformFn.findPlug("translateY", false, &status);
	MPlug translateZPlug = transformFn.findPlug("translateZ", false, &status);

	if (status != MS::kSuccess) {
		MGlobal::displayError("Failed to get translate plugs");
		return status;
	}

	clearAnimCurveFromPlug(translateXPlug);
	clearAnimCurveFromPlug(translateYPlug);
	clearAnimCurveFromPlug(translateZPlug);

	return MS::kSuccess;
}

void CameraLaunchCmd::clearAnimCurveFromPlug(MPlug& plug)
{
	if (plug.isConnected()) {
		MPlugArray connections;
		plug.connectedTo(connections, true, false);
		for (unsigned int i = 0; i < connections.length(); ++i) {
			MObject connectedNode = connections[i].node();
			if (connectedNode.hasFn(MFn::kAnimCurve)) {
				MGlobal::deleteNode(connectedNode);
			}
		}
	}
}

int CameraLaunchCmd::calculateFlightFrames()
{
	// Handle case where there's no vertical velocity or gravity is positive
	if (m_velocity.y <= 0.0 || m_gravity >= 0.0) {
		// Default to 120 frames
		return 120;
	}

	// Calculate flight time using projectile motion
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