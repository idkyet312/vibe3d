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
            "emissiveStrength": 0.0
        }
        
        self.init_ui()
        self.load_config()
        self.update_all_displays()
        
    def init_ui(self):
        self.setWindowTitle("Vibe3D Material Controller")
        self.setGeometry(100, 100, 400, 450)
        
        # Main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Title
        title = QLabel("Cube Material Controller")
        title.setStyleSheet("font-size: 16px; font-weight: bold; padding: 10px;")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title)
        
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
    
    def on_value_changed(self):
        """Called when any value changes"""
        self.material_values["albedoR"] = self.albedo_r_spin.value()
        self.material_values["albedoG"] = self.albedo_g_spin.value()
        self.material_values["albedoB"] = self.albedo_b_spin.value()
        self.material_values["roughness"] = self.roughness_spin.value()
        self.material_values["metallic"] = self.metallic_spin.value()
        
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
            "emissiveStrength": 0.0
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
