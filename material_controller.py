#!/usr/bin/env python3
"""
Material Controller - Real-time GUI for adjusting Vibe3D PBR material properties
"""

import sys
import json
from pathlib import Path
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QSlider, QDoubleSpinBox, QPushButton, QGroupBox, QComboBox
)
from PyQt6.QtCore import Qt, QTimer


class MaterialController(QMainWindow):
    def __init__(self):
        super().__init__()
        self.config_file = Path("build/material_config.json")
        
        # Material presets
        self.presets = {
            "Rough Plastic": {
                "roughness": 0.8, "metallic": 0.0,
                "albedoR": 0.8, "albedoG": 0.3, "albedoB": 0.2
            },
            "Smooth Plastic": {
                "roughness": 0.2, "metallic": 0.0,
                "albedoR": 0.2, "albedoG": 0.6, "albedoB": 0.9
            },
            "Rough Metal": {
                "roughness": 0.7, "metallic": 1.0,
                "albedoR": 0.7, "albedoG": 0.7, "albedoB": 0.7
            },
            "Polished Metal": {
                "roughness": 0.1, "metallic": 1.0,
                "albedoR": 0.8, "albedoG": 0.8, "albedoB": 0.8
            },
            "Gold": {
                "roughness": 0.2, "metallic": 1.0,
                "albedoR": 1.0, "albedoG": 0.78, "albedoB": 0.34
            },
            "Copper": {
                "roughness": 0.3, "metallic": 1.0,
                "albedoR": 0.95, "albedoG": 0.64, "albedoB": 0.54
            },
            "Chrome": {
                "roughness": 0.05, "metallic": 1.0,
                "albedoR": 0.9, "albedoG": 0.9, "albedoB": 0.9
            },
            "Aluminum": {
                "roughness": 0.4, "metallic": 1.0,
                "albedoR": 0.91, "albedoG": 0.92, "albedoB": 0.92
            },
            "Rubber": {
                "roughness": 0.9, "metallic": 0.0,
                "albedoR": 0.1, "albedoG": 0.1, "albedoB": 0.1
            },
            "Wood": {
                "roughness": 0.7, "metallic": 0.0,
                "albedoR": 0.55, "albedoG": 0.35, "albedoB": 0.2
            },
            "Glass": {
                "roughness": 0.05, "metallic": 0.0,
                "albedoR": 0.95, "albedoG": 0.95, "albedoB": 0.95
            },
            "Ceramic": {
                "roughness": 0.3, "metallic": 0.0,
                "albedoR": 0.9, "albedoG": 0.9, "albedoB": 0.85
            }
        }
        
        # Default values
        self.material_values = {
            "roughness": 0.5,
            "metallic": 0.0,
            "albedoR": 0.8,
            "albedoG": 0.3,
            "albedoB": 0.2,
            "ambientStrength": 0.001,
            "lightIntensity": 8.0,
            "emissiveR": 0.0,
            "emissiveG": 0.0,
            "emissiveB": 0.0,
            "emissiveStrength": 0.0,
            "lightYaw": 225.0,      # Horizontal angle (degrees)
            "lightPitch": 45.0      # Vertical angle (degrees)
        }
        
        self.init_ui()
        self.load_config()
        self.update_all_displays()
        
    def init_ui(self):
        self.setWindowTitle("Vibe3D Material Controller")
        self.setGeometry(100, 100, 400, 650)
        
        # Main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Title
        title = QLabel("Cube Material Controller")
        title.setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title)
        
        # Material Presets dropdown
        preset_layout = QHBoxLayout()
        preset_label = QLabel("Material Preset:")
        preset_label.setMinimumWidth(140)
        preset_layout.addWidget(preset_label)
        
        self.preset_combo = QComboBox()
        self.preset_combo.addItems([
            "Custom",
            "Rough Plastic",
            "Smooth Plastic",
            "Rough Metal",
            "Polished Metal",
            "Gold",
            "Copper",
            "Chrome",
            "Aluminum",
            "Rubber",
            "Wood",
            "Glass",
            "Ceramic"
        ])
        self.preset_combo.currentTextChanged.connect(self.apply_preset)
        preset_layout.addWidget(self.preset_combo)
        layout.addLayout(preset_layout)
        
        # Albedo Color group
        albedo_group = QGroupBox("Cube Color (RGB)")
        albedo_layout = QVBoxLayout()
        
        self.albedo_r_slider, self.albedo_r_spin = self.create_slider_row(
            "Red Channel:", 0.0, 1.0, 0.8, albedo_layout
        )
        self.albedo_g_slider, self.albedo_g_spin = self.create_slider_row(
            "Green Channel:", 0.0, 1.0, 0.3, albedo_layout
        )
        self.albedo_b_slider, self.albedo_b_spin = self.create_slider_row(
            "Blue Channel:", 0.0, 1.0, 0.2, albedo_layout
        )
        
        # Color preview box
        self.color_preview = QLabel("Color Preview")
        self.color_preview.setMinimumHeight(50)
        self.color_preview.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.update_color_preview()
        albedo_layout.addWidget(self.color_preview)
        
        albedo_group.setLayout(albedo_layout)
        layout.addWidget(albedo_group)
        
        # Material Properties group
        material_group = QGroupBox("Material Properties")
        material_layout = QVBoxLayout()
        
        self.roughness_slider, self.roughness_spin = self.create_slider_row(
            "Roughness:", 0.0, 1.0, 0.5, material_layout
        )
        self.metallic_slider, self.metallic_spin = self.create_slider_row(
            "Metallic:", 0.0, 1.0, 0.0, material_layout
        )
        
        material_group.setLayout(material_layout)
        layout.addWidget(material_group)
        
        # Lighting group
        lighting_group = QGroupBox("Lighting")
        lighting_layout = QVBoxLayout()
        
        self.ambient_slider, self.ambient_spin = self.create_slider_row(
            "Skylight (Ambient):", 0.0, 0.1, 0.001, lighting_layout
        )
        self.light_intensity_slider, self.light_intensity_spin = self.create_slider_row(
            "Sun Intensity:", 0.0, 20.0, 8.0, lighting_layout
        )
        
        self.light_yaw_slider, self.light_yaw_spin = self.create_slider_row(
            "Sun Direction (Yaw):", 0.0, 360.0, 225.0, lighting_layout
        )
        self.light_pitch_slider, self.light_pitch_spin = self.create_slider_row(
            "Sun Angle (Pitch):", 0.0, 90.0, 45.0, lighting_layout
        )
        
        lighting_group.setLayout(lighting_layout)
        layout.addWidget(lighting_group)
        
        # Control buttons
        button_layout = QHBoxLayout()
        
        reset_btn = QPushButton("Reset to Default")
        reset_btn.clicked.connect(self.reset_to_defaults)
        button_layout.addWidget(reset_btn)
        
        layout.addLayout(button_layout)
        
        # Status label
        self.status_label = QLabel("Ready - Material will be saved automatically")
        self.status_label.setStyleSheet("padding: 5px; background-color: #e0e0e0;")
        layout.addWidget(self.status_label)
        
        # Auto-save timer (saves every 100ms when values change)
        self.auto_save_timer = QTimer()
        self.auto_save_timer.timeout.connect(self.auto_save)
        self.auto_save_timer.start(100)
        
    def create_slider_row(self, label_text, min_val, max_val, default_val, layout):
        """Create a row with label, slider, and spinbox"""
        row_layout = QHBoxLayout()
        
        label = QLabel(label_text)
        label.setMinimumWidth(140)
        row_layout.addWidget(label)
        
        slider = QSlider(Qt.Orientation.Horizontal)
        slider.setMinimum(int(min_val * 1000))
        slider.setMaximum(int(max_val * 1000))
        slider.setValue(int(default_val * 1000))
        slider.valueChanged.connect(lambda: self.on_value_changed())
        row_layout.addWidget(slider)
        
        spinbox = QDoubleSpinBox()
        spinbox.setMinimum(min_val)
        spinbox.setMaximum(max_val)
        spinbox.setValue(default_val)
        spinbox.setSingleStep(0.01)
        spinbox.setDecimals(3)
        spinbox.valueChanged.connect(lambda: self.on_value_changed())
        row_layout.addWidget(spinbox)
        
        # Connect slider and spinbox
        slider.valueChanged.connect(lambda v: spinbox.setValue(v / 1000.0))
        spinbox.valueChanged.connect(lambda v: slider.setValue(int(v * 1000)))
        
        layout.addLayout(row_layout)
        return slider, spinbox
    
    def apply_preset(self, preset_name):
        """Apply a material preset"""
        if preset_name == "Custom":
            return
        
        if preset_name in self.presets:
            preset = self.presets[preset_name]
            
            # Update material values
            self.material_values["roughness"] = preset["roughness"]
            self.material_values["metallic"] = preset["metallic"]
            self.material_values["albedoR"] = preset["albedoR"]
            self.material_values["albedoG"] = preset["albedoG"]
            self.material_values["albedoB"] = preset["albedoB"]
            
            # Update UI displays
            self.roughness_spin.setValue(preset["roughness"])
            self.metallic_spin.setValue(preset["metallic"])
            self.albedo_r_spin.setValue(preset["albedoR"])
            self.albedo_g_spin.setValue(preset["albedoG"])
            self.albedo_b_spin.setValue(preset["albedoB"])
            
            self.update_color_preview()
            self.save_config()
            self.status_label.setText(f"Applied preset: {preset_name}")
            self.status_label.setStyleSheet("padding: 5px; background-color: #90EE90;")
            QTimer.singleShot(2000, lambda: self.status_label.setStyleSheet("padding: 5px; background-color: #e0e0e0;"))
    
    def on_value_changed(self):
        """Called when any value changes"""
        self.material_values["albedoR"] = self.albedo_r_spin.value()
        self.material_values["albedoG"] = self.albedo_g_spin.value()
        self.material_values["albedoB"] = self.albedo_b_spin.value()
        self.material_values["roughness"] = self.roughness_spin.value()
        self.material_values["metallic"] = self.metallic_spin.value()
        self.material_values["ambientStrength"] = self.ambient_spin.value()
        self.material_values["lightIntensity"] = self.light_intensity_spin.value()
        self.material_values["lightYaw"] = self.light_yaw_spin.value()
        self.material_values["lightPitch"] = self.light_pitch_spin.value()
        
        # Set preset to "Custom" when user manually changes values
        if self.preset_combo.currentText() != "Custom":
            self.preset_combo.blockSignals(True)
            self.preset_combo.setCurrentText("Custom")
            self.preset_combo.blockSignals(False)
        
        self.update_color_preview()
        self.status_label.setText("Material changed - will auto-save...")
    
    def update_color_preview(self):
        """Update the albedo color preview box"""
        r = int(self.material_values["albedoR"] * 255)
        g = int(self.material_values["albedoG"] * 255)
        b = int(self.material_values["albedoB"] * 255)
        color = f"rgb({r}, {g}, {b})"
        self.color_preview.setStyleSheet(f"background-color: {color}; border: 2px solid #333;")
        self.color_preview.setText(f"RGB({r}, {g}, {b})")
    
    def auto_save(self):
        """Automatically save config file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.material_values, f, indent=2)
        except Exception as e:
            print(f"Error auto-saving: {e}")
    
    def save_config(self):
        """Manually save configuration"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.material_values, f, indent=2)
            self.status_label.setText("Material saved successfully!")
            self.status_label.setStyleSheet("padding: 5px; background-color: #90EE90;")
            QTimer.singleShot(2000, lambda: self.status_label.setStyleSheet("padding: 5px; background-color: #e0e0e0;"))
        except Exception as e:
            self.status_label.setText(f"Error saving: {e}")
            self.status_label.setStyleSheet("padding: 10px; background-color: #FFB6C1;")
    
    def load_config(self):
        """Load configuration from file"""
        try:
            if self.config_file.exists():
                with open(self.config_file, 'r') as f:
                    loaded = json.load(f)
                    self.material_values.update(loaded)
        except Exception as e:
            print(f"Error loading config: {e}")
    
    def update_all_displays(self):
        """Update all UI elements with current values"""
        self.albedo_r_spin.setValue(self.material_values["albedoR"])
        self.albedo_g_spin.setValue(self.material_values["albedoG"])
        self.albedo_b_spin.setValue(self.material_values["albedoB"])
        self.roughness_spin.setValue(self.material_values["roughness"])
        self.metallic_spin.setValue(self.material_values["metallic"])
        self.ambient_spin.setValue(self.material_values["ambientStrength"])
        self.light_intensity_spin.setValue(self.material_values["lightIntensity"])
        self.light_yaw_spin.setValue(self.material_values["lightYaw"])
        self.light_pitch_spin.setValue(self.material_values["lightPitch"])
        self.update_color_preview()
    
    def reset_to_defaults(self):
        """Reset all values to defaults"""
        self.material_values = {
            "roughness": 0.5,
            "metallic": 0.0,
            "albedoR": 0.8,
            "albedoG": 0.3,
            "albedoB": 0.2,
            "ambientStrength": 0.001,
            "lightIntensity": 8.0,
            "emissiveR": 0.0,
            "emissiveG": 0.0,
            "emissiveB": 0.0,
            "emissiveStrength": 0.0,
            "lightYaw": 225.0,
            "lightPitch": 45.0
        }
        self.update_all_displays()
        self.save_config()
        self.status_label.setText("Reset to default material!")
        self.status_label.setStyleSheet("padding: 5px; background-color: #90EE90;")
        QTimer.singleShot(2000, lambda: self.status_label.setStyleSheet("padding: 5px; background-color: #e0e0e0;"))


def main():
    app = QApplication(sys.argv)
    controller = MaterialController()
    controller.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
