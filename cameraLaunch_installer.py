"""
CameraLaunch Installer Script

Instructions:
1. Download the CameraLaunch folder with all files
2. Open Maya
3. Run this script in Maya's Script Editor
4. A custom shelf will be created
"""

import maya.cmds as cmds
import maya.mel as mel
import sys
import os

class CameraLaunchInstaller:
	def __init__(self):
		self.shelf_name = "CameraLaunch"
		self.version = "1.0"

	def install(self):
		print("Installing CameraLaunch")

		self._add_to_path()

		self._create_shelf()

		self._save_shelf()

	def _add_to_path(self):
		script_path = r"C:/Users/brend/Desktop/Art/TDSummer/CameraLaunch"
		if script_path not in sys.path:
			sys.path.append(script_path)
		print(f"Added to Python path: {script_path}")

	def _create_shelf(self):
		try:
			import cameraLaunch_shelf
			cameraLaunch_shelf.create_shelf()
		except ImportError as e:
			print(f"Error: Could not import cameraLaunch_shelf: {e}")
			print(f"Current Python path: {sys.path}")

	def _save_shelf(self):
		try:
			cmds.shelfLayout(self.shelf_name, edit=True, save=True)
		except:
			try:
				mel.eval('saveAllShelves $gShelfTopLevel')
			except:
				print(f"Warning: Could not auto-save {self.shelf_name} shelf. Please save manually.")

	def uninstall(self):
		if cmds.shelfLayout(self.shelf_name, exists=True):
			cmds.deleteUI(self.shelf_name, layout=True)
			print(f"{self.shelf_name} shelf has been removed.")
			try:
				mel.eval('saveAllShelves $gShelfTopLevel')
			except:
				pass
		else:
			print(f"{self.shelf_name} shelf not found.")

def install_cameralaunch():
	installer = CameraLaunchInstaller()
	installer.install()

def uninstall_cameralaunch():
	installer = CameraLaunchInstaller()
	installer.uninstall()

def reload_script():
    module_name = __name__
    if module_name in sys.modules:
        del sys.modules[module_name]
    cmds.warning("Script reloaded - reimport to get changes")

# To uninstall, please complete the Herculean task of adding the letters "un" to the beginning of the following function:
if __name__ == "__main__":
	reload_script()
	install_cameralaunch()