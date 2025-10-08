import maya.cmds as cmds
from PySide6 import QtWidgets, QtCore


class SimpleButtonUI(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(SimpleButtonUI, self).__init__(parent)

        self.setWindowTitle("Launch Camera")
        self.setMinimumSize(350, 200)

        self.create_ui()

    def create_ui(self):
        main_layout = QtWidgets.QVBoxLayout(self)

        # Camera selection
        camera_layout = QtWidgets.QHBoxLayout()
        camera_label = QtWidgets.QLabel("Camera:")
        self.camera_field = QtWidgets.QLineEdit()
        self.camera_field.setPlaceholderText("Select a camera or leave empty")
        camera_layout.addWidget(camera_label)
        camera_layout.addWidget(self.camera_field)
        main_layout.addLayout(camera_layout)

        # Velocity X
        vel_x_layout = QtWidgets.QHBoxLayout()
        vel_x_label = QtWidgets.QLabel("Velocity X:")
        self.vel_x_spin = QtWidgets.QDoubleSpinBox()
        self.vel_x_spin.setRange(-1000, 1000)
        self.vel_x_spin.setValue(10.0)
        vel_x_layout.addWidget(vel_x_label)
        vel_x_layout.addWidget(self.vel_x_spin)
        main_layout.addLayout(vel_x_layout)

        # Velocity Y
        vel_y_layout = QtWidgets.QHBoxLayout()
        vel_y_label = QtWidgets.QLabel("Velocity Y:")
        self.vel_y_spin = QtWidgets.QDoubleSpinBox()
        self.vel_y_spin.setRange(-1000, 1000)
        self.vel_y_spin.setValue(10.0)
        vel_y_layout.addWidget(vel_y_label)
        vel_y_layout.addWidget(self.vel_y_spin)
        main_layout.addLayout(vel_y_layout)

        # Velocity Z
        vel_z_layout = QtWidgets.QHBoxLayout()
        vel_z_label = QtWidgets.QLabel("Velocity Z:")
        self.vel_z_spin = QtWidgets.QDoubleSpinBox()
        self.vel_z_spin.setRange(-1000, 1000)
        self.vel_z_spin.setValue(3.0)
        vel_z_layout.addWidget(vel_z_label)
        vel_z_layout.addWidget(self.vel_z_spin)
        main_layout.addLayout(vel_z_layout)

        # Gravity
        gravity_layout = QtWidgets.QHBoxLayout()
        gravity_label = QtWidgets.QLabel("Gravity:")
        self.gravity_spin = QtWidgets.QDoubleSpinBox()
        self.gravity_spin.setRange(-100, 100)
        self.gravity_spin.setValue(-9.81)
        gravity_layout.addWidget(gravity_label)
        gravity_layout.addWidget(self.gravity_spin)
        main_layout.addLayout(gravity_layout)

        # Start Frame
        frame_layout = QtWidgets.QHBoxLayout()
        frame_label = QtWidgets.QLabel("Start Frame:")
        self.frame_spin = QtWidgets.QSpinBox()
        self.frame_spin.setRange(0, 10000)
        self.frame_spin.setValue(1)
        frame_layout.addWidget(frame_label)
        frame_layout.addWidget(self.frame_spin)
        main_layout.addLayout(frame_layout)

        # Launch button
        self.run_button = QtWidgets.QPushButton("Launch")
        self.run_button.clicked.connect(self.on_button_clicked)
        main_layout.addWidget(self.run_button)

    def get_selected_camera(self):
        selection = cmds.ls(selection=True, type='camera')
        if selection:
            return selection[0]

        # Check if selected transform has a camera shape
        selection = cmds.ls(selection=True)
        for obj in selection:
            shapes = cmds.listRelatives(obj, shapes=True, type='camera')
            if shapes:
                return obj

        return None

    def on_button_clicked(self):
        # Get camera
        selected_camera = self.get_selected_camera()
        camera = selected_camera if selected_camera else self.camera_field.text()

        if not camera:
            QtWidgets.QMessageBox.warning(self, "No Camera", "Please select a camera or enter a camera name.")
            return

        velocity = (
            self.vel_x_spin.value(),
            self.vel_y_spin.value(),
            self.vel_z_spin.value()
        )
        gravity = self.gravity_spin.value()
        start_frame = self.frame_spin.value()

        try:
            cmds.cameraLaunch(
                camera=camera,
                velocity=velocity,
                gravity=gravity,
                startFrame=start_frame
            )
            print(f"Camera launched: {camera} with velocity {velocity}")
        except Exception as e:
            QtWidgets.QMessageBox.critical(self, "Error", f"Failed to launch camera:\n{str(e)}")


def show_ui():
    global simple_ui

    try:
        simple_ui.close()
        simple_ui.deleteLater()
    except:
        pass

    simple_ui = SimpleButtonUI()
    simple_ui.show()


show_ui()
