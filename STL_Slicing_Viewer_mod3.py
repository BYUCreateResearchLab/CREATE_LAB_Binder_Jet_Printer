import sys
import os
import vtk
import trimesh
import numpy as np
from datetime import datetime
from PIL import Image, ImageDraw, ImageChops
import random
import trimesh.path.polygons as path_polygons

from PyQt5.QtWidgets import (QMainWindow, QApplication, QWidget, QVBoxLayout, QHBoxLayout,
                             QSlider, QLabel, QLineEdit, QGridLayout, QFrame, QPushButton,
                             QFileDialog, QGroupBox, QRadioButton, QMessageBox, QComboBox,
                             QProgressDialog)
from PyQt5.QtCore import Qt
from vtk.qt.QVTKRenderWindowInteractor import QVTKRenderWindowInteractor

# --- Configuration ---
BASE_OUTPUT_DIR = "C:\\Users\\CB140LAB\\Desktop\\Noah\\ComplexMultiNozzle\\Slicing"
BED_SIZE = (100.0, 100.0, 35.0)
NOZZLE_COUNT = 128
DITHER_PASSES = True
ACTIVE_COLOR = (0.0, 1.0, 1.0)  # Cyan
INACTIVE_COLOR = (1.0, 0.5, 0.0) # Orange


class SlicerMainWindow(QMainWindow):
    def __init__(self, parent=None):
        super(SlicerMainWindow, self).__init__(parent)
        self.setWindowTitle("Multi-STL Interactive Slicer")
        self.setGeometry(100, 100, 1200, 900)

        # --- Central Widget and Layouts ---
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.main_layout = QHBoxLayout(self.central_widget)

        # --- VTK and PyQt Integration ---
        self.vtkWidget = QVTKRenderWindowInteractor(self.central_widget)
        self.main_layout.addWidget(self.vtkWidget, 5)

        # --- Control Panel ---
        self.control_panel = QFrame()
        self.control_panel.setFrameShape(QFrame.StyledPanel)
        self.main_layout.addWidget(self.control_panel, 2)

        self.controls_layout = QVBoxLayout(self.control_panel)
        self.controls_layout.setAlignment(Qt.AlignTop)

        # --- State Management for Multiple Models ---
        self.models = {}
        self.active_model_key = None
        self.slicing_params = {
            "print_freq": 1000.0, "droplet_spacing": 0.05,
            "line_spacing": 0.13716, "layer_height": 0.05
        }

        # --- Initialize VTK Scene ---
        self.renderer = vtk.vtkRenderer()
        self.renderer.SetBackground(0.2, 0.3, 0.4)
        self.vtkWidget.GetRenderWindow().AddRenderer(self.renderer)
        self.interactor = self.vtkWidget.GetRenderWindow().GetInteractor()

        # --- Create and Add Visual Aids ---
        self.box_dims = list(BED_SIZE)
        self.create_visual_aids()

        # --- Create UI Controls ---
        self.create_viewer_controls()
        self.create_slicer_controls()

        # --- Final Setup ---
        self.set_isometric_view()
        self.interactor.Initialize()

    def show_message(self, title, message, icon=QMessageBox.Information):
        msg_box = QMessageBox(self); msg_box.setIcon(icon)
        msg_box.setText(message); msg_box.setWindowTitle(title)
        msg_box.setStandardButtons(QMessageBox.Ok); msg_box.exec_()

    #==========================================================================
    # Model Management and Loading
    #==========================================================================

    def add_stl(self, file_path):
        """Adds a new STL file to the scene, allowing for duplicates."""
        # --- Generate a unique key for the model ---
        short_name = os.path.basename(file_path)
        unique_key = short_name
        counter = 1
        while unique_key in self.models:
            unique_key = f"{short_name} ({counter})"
            counter += 1
            
        # --- VTK Pipeline ---
        reader = vtk.vtkSTLReader()
        reader.SetFileName(file_path)
        reader.Update()

        if reader.GetOutput().GetNumberOfPoints() == 0:
            self.show_message("Error", f"Could not read STL file or file is empty: {file_path}", QMessageBox.Critical)
            return

        try:
            trimesh_mesh = trimesh.load(file_path)
        except Exception as e:
            self.show_message("Trimesh Error", f"Failed to load mesh with trimesh: {e}", QMessageBox.Critical)
            return
        
        base_transform = vtk.vtkTransform()
        user_transform = vtk.vtkTransform()
        transform_filter = vtk.vtkTransformPolyDataFilter()
        transform_filter.SetInputConnection(reader.GetOutputPort())
        transform_filter.SetTransform(base_transform)
        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputConnection(transform_filter.GetOutputPort())
        actor = vtk.vtkActor()
        actor.SetMapper(mapper)
        actor.SetUserTransform(user_transform)
        self.renderer.AddActor(actor)

        self.models[unique_key] = {
            "reader": reader, "trimesh_mesh": trimesh_mesh, "actor": actor,
            "base_transform": base_transform, "user_transform": user_transform,
            "origin_type": "centroid",
            "transform_state": {
                "translate_x": 50.0, "translate_y": 50.0, "translate_z": 0.0,
                "rotate_x": 0.0, "rotate_y": 0.0, "rotate_z": 0.0
            }
        }

        self.model_selector.addItem(unique_key, unique_key)
        self.model_selector.setCurrentIndex(self.model_selector.count() - 1)
        self.on_model_selection_changed(self.model_selector.currentIndex())

    def remove_active_model(self):
        """Removes the currently selected model from the scene."""
        if not self.active_model_key:
            return

        key_to_remove = self.active_model_key
        model_data = self.models[key_to_remove]

        # Remove actor from renderer
        self.renderer.RemoveActor(model_data["actor"])
        
        # Remove from data dictionary
        del self.models[key_to_remove]
        
        # Remove from UI dropdown
        # This will automatically trigger on_model_selection_changed
        current_index = self.model_selector.findData(key_to_remove)
        self.model_selector.removeItem(current_index)
        
        self.vtkWidget.GetRenderWindow().Render()


    def on_model_selection_changed(self, index):
        """Handles changing the active model from the dropdown."""
        if index < 0:
            self.active_model_key = None
            self.transform_group.setEnabled(False) # Disable controls if no model is selected
            self.origin_group.setEnabled(False)
            return
            
        self.active_model_key = self.model_selector.itemData(index)
        if not self.active_model_key: return

        self.transform_group.setEnabled(True)
        self.origin_group.setEnabled(True)
        self.update_controls_from_state()
        self.update_actor_colors()
        self.vtkWidget.GetRenderWindow().Render()

    def update_actor_colors(self):
        """Sets the color of actors based on whether they are active."""
        for key, model_data in self.models.items():
            prop = model_data["actor"].GetProperty()
            if key == self.active_model_key:
                prop.SetColor(ACTIVE_COLOR)
                prop.SetEdgeVisibility(True)
                prop.SetEdgeColor(0,0,0)
            else:
                prop.SetColor(INACTIVE_COLOR)
                prop.SetEdgeVisibility(False)
    
    def open_file_dialog(self):
        """Opens a dialog to select one or more STL files."""
        default_stl_folder = "C:\\Users\\CB140LAB\\Desktop\\Noah\\ComplexMultiNozzle\\STL_Files"
        options = QFileDialog.Options()
        options |= QFileDialog.DontUseNativeDialog
        
        file_paths, _ = QFileDialog.getOpenFileNames(self, 
                                                     "Open STL Files", 
                                                     default_stl_folder, 
                                                     "STL Files (*.stl *.STL);;All Files (*)",
                                                     options=options)
        if file_paths:
            for path in file_paths:
                self.add_stl(path)

    #==========================================================================
    # Visuals and Transformations
    #==========================================================================

    def create_visual_aids(self):
        axes = vtk.vtkAxesActor(); axes.SetTotalLength(20, 20, 20)
        self.renderer.AddActor(axes)
        w, d, h = self.box_dims
        plane_definitions = [
            ((0, 0, 0), (w, 0, 0), (0, d, 0), (0.3, 0.3, 0.3), 0.5),
            ((0, d, 0), (w, d, 0), (0, d, h), (0.7, 0.7, 0.7), 0.5),
            ((w, 0, 0), (w, d, 0), (w, 0, h), (0.7, 0.7, 0.7), 0.5),
        ]
        for origin, p1, p2, color, opacity in plane_definitions:
            plane = vtk.vtkPlaneSource(); plane.SetOrigin(origin); plane.SetPoint1(p1); plane.SetPoint2(p2)
            mapper = vtk.vtkPolyDataMapper(); mapper.SetInputConnection(plane.GetOutputPort())
            actor = vtk.vtkActor(); actor.SetMapper(mapper)
            actor.GetProperty().SetColor(color); actor.GetProperty().SetOpacity(opacity)
            self.renderer.AddActor(actor)

    def set_isometric_view(self):
        self.renderer.ResetCamera()
        camera = self.renderer.GetActiveCamera()
        camera.SetPosition(150, 150, 150)
        camera.SetFocalPoint(self.box_dims[0]/2, self.box_dims[1]/2, self.box_dims[2]/2)
        camera.SetViewUp(0, 0, 1)
        self.renderer.ResetCameraClippingRange()
        self.vtkWidget.GetRenderWindow().Render()
        
    def set_origin(self, origin_type):
        if not self.active_model_key: return
        model = self.models[self.active_model_key]
        model["origin_type"] = origin_type
        
        polydata = model["reader"].GetOutput()
        tx, ty, tz = 0, 0, 0
        if origin_type == "centroid":
            center_filter = vtk.vtkCenterOfMass(); center_filter.SetInputData(polydata)
            center_filter.SetUseScalarsAsWeights(False); center_filter.Update()
            center = center_filter.GetCenter()
            tx, ty, tz = -center[0], -center[1], -center[2]
        else:
            bounds = polydata.GetBounds()
            x_min, x_max, y_min, y_max, z_min, _ = bounds
            if origin_type == "bottom_left":
                tx, ty, tz = -x_min, -y_min, -z_min
            elif origin_type == "bottom_center":
                tx = -(x_min + x_max) / 2.0; ty = -(y_min + y_max) / 2.0; tz = -z_min
        
        model["base_transform"].Identity()
        model["base_transform"].Translate(tx, ty, tz)
        model["actor"].GetMapper().GetInputConnection(0, 0).GetProducer().Update()
        self.vtkWidget.GetRenderWindow().Render()

    def update_transform(self):
        if not self.active_model_key: return
        model = self.models[self.active_model_key]
        state = model["transform_state"]
        
        model["user_transform"].Identity()
        model["user_transform"].Translate(state["translate_x"], state["translate_y"], state["translate_z"])
        model["user_transform"].RotateZ(state["rotate_z"])
        model["user_transform"].RotateY(state["rotate_y"])
        model["user_transform"].RotateX(state["rotate_x"])
        self.vtkWidget.GetRenderWindow().Render()
        
    def place_on_bottom(self):
        if not self.active_model_key: return
        model = self.models[self.active_model_key]
        bounds = model["actor"].GetBounds()
        z_min = bounds[4]
        model["transform_state"]["translate_z"] -= z_min
        self.update_controls_from_state()
        self.update_transform()

    #==========================================================================
    # UI Controls and Handlers
    #==========================================================================

    def create_viewer_controls(self):
        model_group = QGroupBox("Scene Controls")
        model_layout = QGridLayout()
        self.model_selector = QComboBox()
        self.model_selector.currentIndexChanged.connect(self.on_model_selection_changed)
        open_button = QPushButton("Open STL Files"); open_button.clicked.connect(self.open_file_dialog)
        iso_button = QPushButton("Isometric View"); iso_button.clicked.connect(self.set_isometric_view)
        place_button = QPushButton("Place Active on Bottom"); place_button.clicked.connect(self.place_on_bottom)
        
        remove_button = QPushButton("Remove Active Model")
        remove_button.setStyleSheet("background-color: #D32F2F; color: white;")
        remove_button.clicked.connect(self.remove_active_model)
        
        model_layout.addWidget(QLabel("Active Model:"), 0, 0, 1, 2)
        model_layout.addWidget(self.model_selector, 1, 0, 1, 2)
        model_layout.addWidget(open_button, 2, 0)
        model_layout.addWidget(iso_button, 2, 1)
        model_layout.addWidget(place_button, 3, 0, 1, 2)
        model_layout.addWidget(remove_button, 4, 0, 1, 2)
        model_group.setLayout(model_layout)
        self.controls_layout.addWidget(model_group)
        
        self.origin_group = QGroupBox("Rotation Origin")
        origin_layout = QVBoxLayout()
        self.rb_centroid = QRadioButton("Centroid"); self.rb_centroid.toggled.connect(lambda: self.set_origin("centroid"))
        self.rb_bottom_left = QRadioButton("Bottom Left Corner"); self.rb_bottom_left.toggled.connect(lambda: self.set_origin("bottom_left"))
        self.rb_bottom_center = QRadioButton("Bottom Face Center"); self.rb_bottom_center.toggled.connect(lambda: self.set_origin("bottom_center"))
        origin_layout.addWidget(self.rb_centroid); origin_layout.addWidget(self.rb_bottom_left); origin_layout.addWidget(self.rb_bottom_center)
        self.origin_group.setLayout(origin_layout)
        self.controls_layout.addWidget(self.origin_group)

        self.transform_group = QGroupBox("Placement and Orientation")
        grid = QGridLayout()
        self.sliders = {}
        self.line_edits = {}
        self.add_slider_input(grid, 0, "Translate X", "translate_x", 0, self.box_dims[0])
        self.add_slider_input(grid, 1, "Translate Y", "translate_y", 0, self.box_dims[1])
        self.add_slider_input(grid, 2, "Translate Z", "translate_z", -50, 50)
        self.add_slider_input(grid, 3, "Rotate X", "rotate_x", 0, 360)
        self.add_slider_input(grid, 4, "Rotate Y", "rotate_y", 0, 360)
        self.add_slider_input(grid, 5, "Rotate Z", "rotate_z", 0, 360)
        self.transform_group.setLayout(grid)
        self.controls_layout.addWidget(self.transform_group)

        # Disable controls initially until a model is loaded
        self.transform_group.setEnabled(False)
        self.origin_group.setEnabled(False)

    def add_slider_input(self, layout, row, label_text, state_key, min_val, max_val):
        label = QLabel(label_text)
        slider = QSlider(Qt.Horizontal); slider.setRange(int(min_val * 10), int(max_val * 10))
        line_edit = QLineEdit("0.0"); line_edit.setFixedWidth(60)
        slider.valueChanged.connect(lambda v, le=line_edit, sk=state_key: self.update_from_slider(v, le, sk))
        line_edit.returnPressed.connect(lambda le=line_edit, s=slider, sk=state_key: self.update_from_line_edit(le, s, sk))
        self.sliders[state_key] = slider
        self.line_edits[state_key] = line_edit
        layout.addWidget(label, row, 0); layout.addWidget(slider, row, 1); layout.addWidget(line_edit, row, 2)

    def create_slicer_controls(self):
        slicer_group = QGroupBox("Slicing Controls")
        grid = QGridLayout()
        self.param_edits = {}
        params = {"Layer Height (mm)": "layer_height", "Droplet Spacing (mm)": "droplet_spacing",
                  "Line Spacing (mm)": "line_spacing", "Print Frequency (Hz)": "print_freq"}
        row = 0
        for label_text, key in params.items():
            label = QLabel(label_text); edit = QLineEdit(str(self.slicing_params[key])); edit.setFixedWidth(80)
            self.param_edits[key] = edit
            grid.addWidget(label, row, 0); grid.addWidget(edit, row, 1)
            row += 1
        slice_button = QPushButton("Slice Scene"); slice_button.setStyleSheet("font-weight: bold; background-color: #4CAF50; color: white;")
        slice_button.clicked.connect(self.start_slicing_process)
        grid.addWidget(slice_button, row, 0, 1, 2)
        slicer_group.setLayout(grid)
        self.controls_layout.addWidget(slicer_group)
        self.controls_layout.addStretch()

    def update_from_slider(self, value, line_edit, state_key):
        if not self.active_model_key: return
        float_val = value / 10.0
        line_edit.setText(f"{float_val:.1f}")
        self.models[self.active_model_key]["transform_state"][state_key] = float_val
        self.update_transform()

    def update_from_line_edit(self, line_edit, slider, state_key):
        if not self.active_model_key: return
        try:
            float_val = float(line_edit.text())
            self.models[self.active_model_key]["transform_state"][state_key] = float_val
            slider.blockSignals(True); slider.setValue(int(float_val * 10)); slider.blockSignals(False)
            self.update_transform()
        except ValueError:
            current_val = self.models[self.active_model_key]["transform_state"][state_key]
            line_edit.setText(f"{current_val:.1f}")
            
    def update_controls_from_state(self):
        if not self.active_model_key: return
        model = self.models[self.active_model_key]
        state = model["transform_state"]
        
        for widget in self.sliders.values(): widget.blockSignals(True)
        for widget in self.line_edits.values(): widget.blockSignals(True)
        self.rb_centroid.blockSignals(True)
        self.rb_bottom_left.blockSignals(True)
        self.rb_bottom_center.blockSignals(True)

        for key, value in state.items():
            self.sliders[key].setValue(int(value * 10))
            self.line_edits[key].setText(f"{value:.1f}")
            
        origin = model["origin_type"]
        if origin == "centroid": self.rb_centroid.setChecked(True)
        elif origin == "bottom_left": self.rb_bottom_left.setChecked(True)
        elif origin == "bottom_center": self.rb_bottom_center.setChecked(True)

        for widget in self.sliders.values(): widget.blockSignals(False)
        for widget in self.line_edits.values(): widget.blockSignals(False)
        self.rb_centroid.blockSignals(False)
        self.rb_bottom_left.blockSignals(False)
        self.rb_bottom_center.blockSignals(False)

    #==========================================================================
    # Slicing Logic
    #==========================================================================

    def start_slicing_process(self):
        if not self.models:
            self.show_message("Error", "No STL files loaded to slice.", QMessageBox.Warning)
            return

        try:
            for key, edit in self.param_edits.items():
                self.slicing_params[key] = float(edit.text())
        except ValueError:
            self.show_message("Error", "Invalid slicing parameter. Please enter numbers only.", QMessageBox.Critical)
            return
            
        print("\n--- Starting Scene Slicing ---")
        
        scene_meshes = []
        for key, model_data in self.models.items():
            mesh = model_data["trimesh_mesh"].copy()
            base_matrix = self.vtk_matrix_to_numpy(model_data["base_transform"].GetMatrix())
            user_matrix = self.vtk_matrix_to_numpy(model_data["user_transform"].GetMatrix())
            mesh.apply_transform(base_matrix)
            mesh.apply_transform(user_matrix)
            scene_meshes.append(mesh)

        combined_mesh = trimesh.util.concatenate(scene_meshes)

        # --- Progress Dialog Setup ---
        progress_dialog = QProgressDialog("Preparing to slice...", "Cancel", 0, 100, self)
        progress_dialog.setWindowModality(Qt.WindowModal)
        progress_dialog.setWindowTitle("Slicing Progress")
        progress_dialog.setMinimumDuration(0) 
        progress_dialog.setValue(0)
        progress_dialog.show()
        QApplication.processEvents() 

        try:
            self.execute_slicing(combined_mesh, progress_dialog)
        finally:
            progress_dialog.close()


    def vtk_matrix_to_numpy(self, vtk_matrix):
        matrix = np.zeros((4, 4))
        for i in range(4):
            for j in range(4):
                matrix[i, j] = vtk_matrix.GetElement(i, j)
        return matrix

    def execute_slicing(self, slicing_mesh, progress_dialog):
        timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        output_dir_slicer = os.path.join(BASE_OUTPUT_DIR, f"scene_{timestamp}")
        non_divided_non_moved_dir = os.path.join(output_dir_slicer, "non-divided_non-moved")
        non_divided_dir = os.path.join(output_dir_slicer, "non-divided")
        divided_dir = os.path.join(output_dir_slicer, "divided")
        os.makedirs(non_divided_non_moved_dir, exist_ok=True)
        os.makedirs(non_divided_dir, exist_ok=True)
        os.makedirs(divided_dir, exist_ok=True)
        
        bounds = slicing_mesh.bounds
        if (slicing_mesh.is_empty or not np.all(np.isfinite(bounds)) or
            bounds[0][0] < 0 or bounds[1][0] > BED_SIZE[0] or
            bounds[0][1] < 0 or bounds[1][1] > BED_SIZE[1] or
            bounds[1][2] > BED_SIZE[2] or bounds[0][2] < 0):
            error_message = (f"One or more parts are out of bed boundaries or scene is empty.\n\n"
                             f"X: {bounds[0][0]:.2f} to {bounds[1][0]:.2f}\n"
                             f"Y: {bounds[0][1]:.2f} to {bounds[1][1]:.2f}\n"
                             f"Z: {bounds[0][2]:.2f} to {bounds[1][2]:.2f}\n\n"
                             "Slicing aborted.")
            self.show_message("Slicing Error", error_message, QMessageBox.Critical)
            return
            
        self.create_header_file(output_dir_slicer, bounds)
        
        layer_height = self.slicing_params['layer_height']
        droplet_spacing_x = self.slicing_params['droplet_spacing']
        line_spacing_y = self.slicing_params['line_spacing']
        img_width_pixels = int(np.ceil(BED_SIZE[0] / droplet_spacing_x))
        img_height_pixels = int(np.ceil(BED_SIZE[1] / line_spacing_y))
        min_z_mesh, max_z_mesh = bounds[0][2], bounds[1][2]

        if layer_height > 0.0001:
            total_layers = int(np.ceil((max_z_mesh - min_z_mesh) / layer_height))
        else:
            total_layers = 1
        progress_dialog.setMaximum(total_layers)
        progress_dialog.setLabelText("Slicing layers...")
        QApplication.processEvents()

        current_z = max_z_mesh
        slice_count = 0
        CLOSE_CUT_PLANE = True
        slicing_cancelled = False
        
        shift_log_path = os.path.join(output_dir_slicer, "layer_y_shifts.txt")
        with open(shift_log_path, 'w') as shift_log:
            shift_log.write("Layer,Y_Shift_Pixels\n")
            while current_z > min_z_mesh - layer_height:
                if progress_dialog.wasCanceled():
                    slicing_cancelled = True
                    print("Slicing cancelled by user.")
                    break

                slice_count += 1
                progress_dialog.setValue(slice_count)
                QApplication.processEvents()

                plane_top_z = current_z
                plane_bottom_z = max(current_z - layer_height, min_z_mesh - layer_height)
                print(f"Processing Slice {slice_count}/{total_layers}: Z={plane_bottom_z:.3f} to {plane_top_z:.3f}mm")

                top_slice = slicing_mesh.slice_plane(plane_origin=[0, 0, plane_top_z], plane_normal=[0, 0, -1], cap=CLOSE_CUT_PLANE)
                if top_slice.is_empty:
                    current_z = plane_bottom_z
                    continue
                final_slice = top_slice.slice_plane(plane_origin=[0, 0, plane_bottom_z], plane_normal=[0, 0, 1], cap=CLOSE_CUT_PLANE)
                if final_slice.is_empty:
                    current_z = plane_bottom_z
                    continue

                img = Image.new('1', (img_width_pixels, img_height_pixels), 1)
                draw = ImageDraw.Draw(img)
                for face, normal in zip(final_slice.faces, final_slice.face_normals):
                    if normal[2] > 0.5:
                        face_vertices_3d = final_slice.vertices[face]
                        coords_mm = face_vertices_3d[:, :2]
                        coords_mm[:, 1] = BED_SIZE[1] - coords_mm[:, 1]
                        pixels = (coords_mm / [droplet_spacing_x, line_spacing_y]).astype(int)
                        draw.polygon([tuple(p) for p in pixels], fill=0)

                original_layer_path = os.path.join(non_divided_non_moved_dir, f"layer_{slice_count:04d}.bmp")
                img.save(original_layer_path)
                y_pixel_shift = random.randint(0, 90) if DITHER_PASSES else 0
                shifted_img = ImageChops.offset(img, 0, y_pixel_shift)
                if y_pixel_shift > 0:
                    draw_shifted = ImageDraw.Draw(shifted_img)
                    draw_shifted.rectangle([0, 0, img_width_pixels, y_pixel_shift], fill=1)
                shifted_layer_path = os.path.join(non_divided_dir, f"layer_{slice_count:04d}.bmp")
                shifted_img.save(shifted_layer_path)
                self.split_image(shifted_img, NOZZLE_COUNT, divided_dir, slice_count)
                shift_log.write(f"{slice_count},{y_pixel_shift}\n")
                current_z = plane_bottom_z

        if slicing_cancelled:
            self.show_message("Slicing Cancelled", "The slicing process was cancelled by the user.", QMessageBox.Warning)
        else:
            progress_dialog.setValue(total_layers)
            completion_message = (f"Slicing completed successfully!\n\n"
                                  f"Total layers: {slice_count}\n"
                                  f"Output saved in: {output_dir_slicer}")
            print("\n" + completion_message)
            self.show_message("Slicing Complete", completion_message)
        
    def create_header_file(self, output_dir, bounds):
        """
        Creates the print_parameters.txt file with keys that match the C++ parser.
        """
        model_names = list(self.models.keys())
        print_speed_x = self.slicing_params['print_freq'] * self.slicing_params['droplet_spacing']
        
        start_x = bounds[0][0]
        start_y = bounds[0][1]

        header_content = (
            f"--- Print Parameters ---\n"
            f"Source STL File: {', '.join(model_names)}\n"
            f"Slicing Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n"
            
            f"--- Slicing & Resolution ---\n"
            f"Layer Height (Z): {self.slicing_params['layer_height']:.4f} mm\n"
            f"Droplet Spacing (X-axis resolution): {self.slicing_params['droplet_spacing']:.4f} mm\n"
            f"Line Spacing (Y-axis resolution): {self.slicing_params['line_spacing']:.4f} mm\n\n"
            
            f"--- Printer & Motion ---\n"
            f"Print Frequency: {self.slicing_params['print_freq']} Hz\n"
            f"Calculated Print Speed (X-axis): {print_speed_x:.2f} mm/s\n"
            f"Nozzle Count: {NOZZLE_COUNT}\n"
            f"Y-Shift Per Layer: {DITHER_PASSES}\n\n"

            f"--- Positioning ---\n"
            f"Part Position (Start X, Y): {start_x:.3f}mm, {start_y:.3f}mm\n"
        )
        with open(os.path.join(output_dir, "print_parameters.txt"), 'w') as f:
            f.write(header_content)
        print("Successfully created header file with corrected parameters.")

    def split_image(self, image, strip_height, output_dir, layer_num):
        """
        Splits the layer image into smaller passes (strips) and saves only the non-blank ones.
        On pass 6, it forces rows 90-127 to be white to correct for artifacts.
        """
        width, height = image.size
        for i in range((height + strip_height - 1) // strip_height):
            pass_num = i + 1
            strip = image.crop((0, i * strip_height, width, (i + 1) * strip_height))

            # Per user request: on pass 6, force rows 90-127 to white.
            # This is to correct an artifact where areas outside the bed were becoming black.
            if pass_num == 6:
                draw = ImageDraw.Draw(strip)
                # The rectangle coordinates are (x0, y0, x1, y1). This covers up to y1-1.
                # To cover rows 90 through 127, y1 needs to be 128.
                # The fill color for white in a '1' mode image is 1.
                draw.rectangle([0, 90, strip.width, 128], fill=1)
            
            # For a '1' mode image, white is 1 and black is 0.
            # getextrema() returns the min and max pixel values.
            # If the strip is all white, min and max will both be 1.
            if strip.getextrema() == (1, 1):
                # This strip is blank (all white), so we skip saving it.
                continue
                
            strip.save(os.path.join(output_dir, f"layer_{layer_num:04d}_pass_{pass_num:02d}.bmp"))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SlicerMainWindow()
    window.show()
    window.interactor.Start() 
    sys.exit(app.exec_())
