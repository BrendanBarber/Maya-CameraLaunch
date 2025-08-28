import maya.cmds as cmds
import maya.mel as mel

def create_shelf():
	shelf_name = "CameraLaunch"

	if cmds.shelfLayout(shelf_name, exists=True):
		cmds.deleteUI(shelf_name, layout=True)

	shelf = cmds.shelfLayout(shelf_name, parent="ShelfLayout")

	tools = [
		{
			'label': 'Launch',
			'annotation': 'CameraLaunch: Launch selected camera',
			'image': 'Camera.png',
			'command': 'import cameraLaunch_core; cameraLaunch_core.launch_selected_camera()'
		}
	]

	# Create buttons
	for tool in tools:
		cmds.shelfButton(
			parent=shelf,
			label=tool['label'],
			annotation=tool['annotation'],
			image=tool['image'],
			command=tool['command'],
			sourceType="python"
		)

	# Save
	try:
		cmds.shelfLayout(shelf_name, edit=True, save=True)
		mel.eval('saveAllShelves $gShelfTopLevel')
	except:
		print("Warning: Could not save CameraLaunch shelf.")

	print("CameraLaunch shelf created.")