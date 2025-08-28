import maya.cmds as cmds
import math

def launch_selected_camera(velocity=20.0, angle=45.0, duration=3.0, gravity=-9.81, frame_rate=24.0):
	
	selected = cmds.ls(selection=True, type='transform')
	if not selected:
		cmds.warning("Please select a camera.")
		return

	camera = selected[0]
	camera_shapes = cmds.listRelatives(camera, shapes=True, type='camera')
	if not camera_shapes:
			cmds.warning(f"'{camera}' is not a camera")
			return

	frame_rate = 24.0
	start_pos = cmds.xform(camera, query=True, worldSpace = True, translation=True)
	start_x, start_y, start_z = start_pos

	angle_rad = math.radians(angle)
	velocity_x = velocity * math.cos(angle_rad)
	velocity_y = velocity * math.sin(angle_rad)
	velocity_z = 0

	current_frame = cmds.currentTime(query=True)
	total_frames = int(duration * frame_rate)

	cmds.cutKey(camera, attribute=['translateX', 'translateY', 'translateZ'])

	for frame in range(total_frames + 1):
		time_sec = frame / frame_rate
		pos_x = start_x + velocity_x * time_sec
		pos_y = start_y + velocity_y * time_sec + 0.5 * gravity * time_sec * time_sec
		pos_z = start_z + velocity_z * time_sec

		target_frame = current_frame + frame
		cmds.setKeyframe(camera, attribute='translateX', value=pos_x, time=target_frame)
		cmds.setKeyframe(camera, attribute='translateY', value=pos_y, time=target_frame)
		cmds.setKeyframe(camera, attribute='translateZ', value=pos_z, time=target_frame)

	cmds.keyTangent(camera, attribute=['translateX', 'translateY', 'translateZ'], 
			   time=(current_frame, current_frame + total_frames),
			   inTangentType='linear', outTangentType='linear')

	cmds.playbackOptions(minTime=current_frame, maxTime=current_frame + total_frames)

	print(f"Camera '{camera}' launched with velocity={velocity}, angle={angle}°, duration={duration}s")
	pass