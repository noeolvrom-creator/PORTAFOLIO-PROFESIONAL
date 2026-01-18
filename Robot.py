from PyQt5.QtWidgets import ( 
    QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QLabel, QLineEdit,
    QPushButton, QComboBox, QWidget, QFileDialog
)
from PyQt5.QtSerialPort import QSerialPort, QSerialPortInfo
from PyQt5.QtCore import QTimer
import csv

class MotorControlInterface(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Control de Motor - Interfaz")
        self.setGeometry(100, 100, 800, 600)

        # Serial Port Setup
        self.serial = QSerialPort()
        self.serial.readyRead.connect(self.receive_data)

        # Timer to constantly check data
        self.timer = QTimer()
        self.timer.timeout.connect(self.receive_data)
        self.timer.start(100)  # Verificar datos cada 100 ms

        # File for real-time data saving
        self.data_file = None
        self.csv_writer = None

        # Main Layout
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # Controls for COM Port and Baud Rate
        self.port_label = QLabel("Puerto:")
        self.port_combo = QComboBox()
        self.port_combo.addItems([port.portName() for port in QSerialPortInfo.availablePorts()])

        self.baud_label = QLabel("Baudios:")
        self.baud_combo = QComboBox()
        self.baud_combo.addItems(["9600", "115200", "250000", "2000000"])

        self.connect_button = QPushButton("Conectar")
        self.connect_button.clicked.connect(self.connect_serial)

        self.disconnect_button = QPushButton("Desconectar")
        self.disconnect_button.clicked.connect(self.disconnect_serial)

        com_layout = QHBoxLayout()
        com_layout.addWidget(self.port_label)
        com_layout.addWidget(self.port_combo)
        com_layout.addWidget(self.baud_label)
        com_layout.addWidget(self.baud_combo)
        com_layout.addWidget(self.connect_button)
        com_layout.addWidget(self.disconnect_button)
        layout.addLayout(com_layout)

        # Cartesian Position Inputs
        self.position_cartesian_label = QLabel("Posición cartesiana:")
        layout.addWidget(self.position_cartesian_label)

        position_layout = QHBoxLayout()
        position_layout.addWidget(QLabel("X:"))
        self.position_cartesian_x_input = QLineEdit()
        position_layout.addWidget(self.position_cartesian_x_input)

        position_layout.addWidget(QLabel("Y:"))
        self.position_cartesian_y_input = QLineEdit()
        position_layout.addWidget(self.position_cartesian_y_input)

        position_layout.addWidget(QLabel("Z:"))
        self.position_cartesian_z_input = QLineEdit()
        position_layout.addWidget(self.position_cartesian_z_input)

        layout.addLayout(position_layout)

        # Separate row for Time of Execution
        time_layout = QHBoxLayout()
        time_layout.addWidget(QLabel("Tiempo de Ejecución (TE):"))
        self.execution_time_input = QLineEdit()
        time_layout.addWidget(self.execution_time_input)
        layout.addLayout(time_layout)

        # Send Position Button
        self.send_button = QPushButton("Enviar posición")
        self.send_button.clicked.connect(self.send_position)
        layout.addWidget(self.send_button)

        # Data Display Sections
        self.sections = {
            "Posición Articular Calculada": ["articulacion_0", "articulacion_1", "articulacion_2"],
            "Posición Articular Real": ["pos_0", "pos_1", "pos_2"],
            "Error Articular": ["error_0", "error_1", "error_2"],
            "Posición Cartesiana Real": ["c_real_0", "c_real_1", "c_real_2"],
            "Señal de Control U": ["u_0", "u_1", "u_2"],
            "Porcentaje PWM": ["pwm_0", "pwm_1", "pwm_2"],
            "Corriente": ["corriente_0", "corriente_1", "corriente_2"]
        }

        self.data_inputs = {}

        for section, keys in self.sections.items():
            layout.addWidget(QLabel(section))
            section_layout = QHBoxLayout()
            for key in keys:
                label = QLabel(key.replace("_", " ").capitalize() + ":")
                line_edit = QLineEdit("0")
                line_edit.setReadOnly(True)
                section_layout.addWidget(label)
                section_layout.addWidget(line_edit)
                self.data_inputs[key] = line_edit
            layout.addLayout(section_layout)

        # Save Data Button
        self.save_button = QPushButton("Guardar Datos")
        self.save_button.clicked.connect(self.save_data)
        layout.addWidget(self.save_button)

    def connect_serial(self):
        if self.serial.isOpen():
            self.serial.close()

        port_name = self.port_combo.currentText()
        baud_rate = int(self.baud_combo.currentText())
        self.serial.setPortName(port_name)
        self.serial.setBaudRate(baud_rate)

        if self.serial.open(QSerialPort.ReadWrite):
            print("Conexión exitosa")
            # Open a CSV file for real-time data saving
            self.data_file = open("realtime_data.csv", "w", newline="")
            self.csv_writer = csv.writer(self.data_file)
            # Write header row
            self.csv_writer.writerow([
                "articulacion_0", "articulacion_1", "articulacion_2",
                "pos_0", "pos_1", "pos_2",
                "error_0", "error_1", "error_2",
                "c_real_0", "c_real_1", "c_real_2",
                "u_0", "u_1", "u_2",
                "pwm_0", "pwm_1", "pwm_2",
                "corriente_0", "corriente_1", "corriente_2"
            ])
        else:
            print("Error al conectar al puerto")

    def disconnect_serial(self):
        if self.serial.isOpen():
            self.serial.close()
            print("Conexión cerrada")
            # Close the CSV file for real-time data saving
            if self.data_file:
                self.data_file.close()
                self.data_file = None
                self.csv_writer = None
        else:
            print("El puerto ya está desconectado")

    def send_position(self):
        if not self.serial.isOpen():
            print("El puerto serial no está abierto.")
            return

        x = self.position_cartesian_x_input.text()
        y = self.position_cartesian_y_input.text()
        z = self.position_cartesian_z_input.text()
        te = self.execution_time_input.text()

        self.serial.write(f"X:{x}\n".encode())
        self.serial.write(f"Y:{y}\n".encode())
        self.serial.write(f"Z:{z}\n".encode())
        self.serial.write(f"T:{te}\n".encode())

    def receive_data(self):
        while self.serial.canReadLine():
            try:
                data = self.serial.readLine().data().decode('utf-8', errors='ignore').strip()
                values = data.split(",")

                if len(values) == 21:
                    keys = [
                        "articulacion_0", "articulacion_1", "articulacion_2",
                        "pos_0", "pos_1", "pos_2",
                        "error_0", "error_1", "error_2",
                        "c_real_0", "c_real_1", "c_real_2",
                        "u_0", "u_1", "u_2",
                        "pwm_0", "pwm_1", "pwm_2",
                        "corriente_0", "corriente_1", "corriente_2"
                    ]

                    for i, key in enumerate(keys):
                        self.data_inputs[key].setText(values[i])

                    # Write data to CSV file in real-time
                    if self.csv_writer:
                        self.csv_writer.writerow(values)
                else:
                    print(f"Error: se esperaban 21 valores, pero se recibieron {len(values)}.")
            except UnicodeDecodeError as e:
                print(f"Error de decodificación: {e}")

    def save_data(self):
        file_path, _ = QFileDialog.getSaveFileName(self, "Guardar Datos", "", "Archivos de Texto (*.txt)")

        if file_path:
            try:
                with open(file_path, "w") as file:
                    for key, input_field in self.data_inputs.items():
                        file.write(f"{key}: {input_field.text()}\n")
                print(f"Datos guardados en {file_path}")
            except Exception as e:
                print(f"Error al guardar los datos: {e}")

if __name__ == "__main__":
    app = QApplication([])
    window = MotorControlInterface()
    window.show()
    app.exec_()
