#!/usr/bin/env python3
"""
Shadow Bias Controller - Real-time GUI for adjusting Vibe3D shadow bias parameters
"""

import sys
import json
from pathlib import Path
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QSlider, QDoubleSpinBox, QPushButton, QGroupBox
)
from PyQt6.QtCore import Qt, QTimer


class ShadowBiasController(QMainWindow):
    def __init__(self):
        super().__init__()
        self.config_file = Path("shadow_config.json")
        
        # Default values
        self.bias_values = {
            "depthBiasConstant": 1.345,
            "depthBiasSlope": 1.627,
            "receiverBiasMultiplier": 0.159,
            "cascade0": 2.277,
            "cascade1": 1.500,
            "cascade2": 3.000,
            "cascade3": 6.000
        }
        
        self.init_ui()
        self.load_config()
        self.update_all_displays()
        
    def init_ui(self):
        self.setWindowTitle("Vibe3D Shadow Bias Controller")
        self.setGeometry(100, 100, 500, 600)
        
        # Main widget and layout
        main_widget = QWidget()
        self.setCentralWidget(main_widget)
        layout = QVBoxLayout(main_widget)
        
        # Title
        title = QLabel("Shadow Bias Real-Time Controller")
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px;")
        title.setAlignment(Qt.AlignmentFlag.AlignCenter)
        layout.addWidget(title)
        
        # Sender-side bias group (C++ pipeline)
        sender_group = QGroupBox("Sender-Side Bias (Pipeline)")
        sender_layout = QVBoxLayout()
        
        self.constant_bias_slider, self.constant_bias_spin = self.create_slider_row(
            "Depth Bias Constant:", 0.0, 10.0, 1.345, sender_layout
        )
        self.slope_bias_slider, self.slope_bias_spin = self.create_slider_row(
            "Depth Bias Slope:", 0.0, 10.0, 1.627, sender_layout
        )
        
        sender_group.setLayout(sender_layout)
        layout.addWidget(sender_group)
        
        # Receiver-side bias group (Shader)
        receiver_group = QGroupBox("Receiver-Side Bias (Shader)")
        receiver_layout = QVBoxLayout()
        
        self.multiplier_slider, self.multiplier_spin = self.create_slider_row(
            "Bias Multiplier:", 0.0, 5.0, 0.159, receiver_layout
        )
        
        receiver_group.setLayout(receiver_layout)
        layout.addWidget(receiver_group)
        
        # Cascade-specific bias group
        cascade_group = QGroupBox("Cascade-Specific Receiver Bias")
        cascade_layout = QVBoxLayout()
        
        self.cascade0_slider, self.cascade0_spin = self.create_slider_row(
            "Cascade 0 (Near):", 0.0, 20.0, 2.277, cascade_layout
        )
        self.cascade1_slider, self.cascade1_spin = self.create_slider_row(
            "Cascade 1:", 0.0, 20.0, 1.5, cascade_layout
        )
        self.cascade2_slider, self.cascade2_spin = self.create_slider_row(
            "Cascade 2:", 0.0, 20.0, 3.0, cascade_layout
        )
        self.cascade3_slider, self.cascade3_spin = self.create_slider_row(
            "Cascade 3 (Far):", 0.0, 20.0, 6.0, cascade_layout
        )
        
        cascade_group.setLayout(cascade_layout)
        layout.addWidget(cascade_group)
        
        # Control buttons
        button_layout = QHBoxLayout()
        
        reset_btn = QPushButton("Reset to Defaults")
        reset_btn.clicked.connect(self.reset_to_defaults)
        button_layout.addWidget(reset_btn)
        
        save_btn = QPushButton("Save Config")
        save_btn.clicked.connect(self.save_config)
        button_layout.addWidget(save_btn)
        
        layout.addLayout(button_layout)
        
        # Status label
        self.status_label = QLabel("Ready - Values will be saved automatically")
        self.status_label.setStyleSheet("padding: 10px; background-color: #e0e0e0;")
        layout.addWidget(self.status_label)
        
        # Auto-save timer (saves every 100ms when values change)
        self.auto_save_timer = QTimer()
        self.auto_save_timer.timeout.connect(self.auto_save)
        self.auto_save_timer.start(100)
        
    def create_slider_row(self, label_text, min_val, max_val, default_val, layout):
        """Create a row with label, slider, and spinbox"""
        row_layout = QHBoxLayout()
        
        label = QLabel(label_text)
        label.setMinimumWidth(180)
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
        self.bias_values["depthBiasConstant"] = self.constant_bias_spin.value()
        self.bias_values["depthBiasSlope"] = self.slope_bias_spin.value()
        self.bias_values["receiverBiasMultiplier"] = self.multiplier_spin.value()
        self.bias_values["cascade0"] = self.cascade0_spin.value()
        self.bias_values["cascade1"] = self.cascade1_spin.value()
        self.bias_values["cascade2"] = self.cascade2_spin.value()
        self.bias_values["cascade3"] = self.cascade3_spin.value()
        self.status_label.setText("Values changed - will auto-save...")
    
    def auto_save(self):
        """Automatically save config file"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.bias_values, f, indent=2)
        except Exception as e:
            print(f"Error auto-saving: {e}")
    
    def save_config(self):
        """Manually save configuration"""
        try:
            with open(self.config_file, 'w') as f:
                json.dump(self.bias_values, f, indent=2)
            self.status_label.setText("Configuration saved successfully!")
            self.status_label.setStyleSheet("padding: 10px; background-color: #90EE90;")
            QTimer.singleShot(2000, lambda: self.status_label.setStyleSheet("padding: 10px; background-color: #e0e0e0;"))
        except Exception as e:
            self.status_label.setText(f"Error saving: {e}")
            self.status_label.setStyleSheet("padding: 10px; background-color: #FFB6C1;")
    
    def load_config(self):
        """Load configuration from file"""
        try:
            if self.config_file.exists():
                with open(self.config_file, 'r') as f:
                    loaded = json.load(f)
                    self.bias_values.update(loaded)
        except Exception as e:
            print(f"Error loading config: {e}")
    
    def update_all_displays(self):
        """Update all UI elements with current values"""
        self.constant_bias_spin.setValue(self.bias_values["depthBiasConstant"])
        self.slope_bias_spin.setValue(self.bias_values["depthBiasSlope"])
        self.multiplier_spin.setValue(self.bias_values["receiverBiasMultiplier"])
        self.cascade0_spin.setValue(self.bias_values["cascade0"])
        self.cascade1_spin.setValue(self.bias_values["cascade1"])
        self.cascade2_spin.setValue(self.bias_values["cascade2"])
        self.cascade3_spin.setValue(self.bias_values["cascade3"])
    
    def reset_to_defaults(self):
        """Reset all values to defaults"""
        self.bias_values = {
            "depthBiasConstant": 1.345,
            "depthBiasSlope": 1.627,
            "receiverBiasMultiplier": 0.159,
            "cascade0": 2.277,
            "cascade1": 1.5,
            "cascade2": 3.0,
            "cascade3": 6.0
        }
        self.update_all_displays()
        self.save_config()


def main():
    app = QApplication(sys.argv)
    controller = ShadowBiasController()
    controller.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
