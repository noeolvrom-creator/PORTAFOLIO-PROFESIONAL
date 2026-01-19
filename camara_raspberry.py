import cv2
import numpy as np
import time
from tensorflow.lite.python.interpreter import Interpreter

# Configuración de parámetros
img_height = 180
img_width = 180
class_names = ['CINTURON', 'CRUCE', 'FONDO_BLANCO', 'PASO_PEATONAL', 'P_REUNION', 'STOP']
# ['cint', 'est', 'fondo', 'pase', 'peso', 'ret']

# Cargar el modelo TFLiteqº
interpreter = Interpreter(model_path="modelo_entrenado.tflite")
interpreter.allocate_tensors()

# Obtener detalles de entrada y salida
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Verificar dimensiones de entrada y salida del modelo
print("Forma esperada de entrada:", input_details[0]['shape'])
print("Forma de salida del modelo:", output_details[0]['shape'])

# Verificar si el modelo requiere escala de entrada específica (por ejemplo, [0, 1] o [-1, 1])
input_scale, input_zero_point = input_details[0]['quantization']
print("Escalado de entrada:", input_scale, input_zero_point)

# Inicializar la cámara
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("No se pudo acceder a la cámara.")
    exit()

print("Presiona 'q' para salir.")

# Función para convertir la salida en probabilidades
def softmax(x):
    exp_x = np.exp(x - np.max(x))
    return exp_x / exp_x.sum(axis=-1, keepdims=True)

# Función para obtener la clase más confiable
def get_predicted_class(output_data, threshold=0.5):
    max_confidence = np.max(output_data)
    if max_confidence > threshold:
        predicted_class = class_names[np.argmax(output_data)]
        return predicted_class, max_confidence
    else:
        return "Nada Detectado", 0

try:
    while True:
        start_time = time.time()

        # Capturar un frame de la cámara
        ret, frame = cap.read()
        if not ret:
            print("Error al capturar el frame.")
            break

        # Preprocesar la imagen
        img = cv2.resize(frame, (img_width, img_height))
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)  # Convertir de BGR a RGB
        img_array = np.array(img, dtype=np.float32)

        # Normalizar la imagen
        if input_scale != 0:
            img_array = (img_array / 255.0 - input_zero_point) / input_scale

        # Expansión de dimensiones para que coincida con la entrada del modelo
        img_array = np.expand_dims(img_array, axis=0)

        # Asignar al tensor de entrada
        interpreter.set_tensor(input_details[0]['index'], img_array)

        # Realizar predicción
        interpreter.invoke()

        # Obtener resultados
        output_data = interpreter.get_tensor(output_details[0]['index'])[0]

        # Si no tiene softmax, aplicarlo manualmente
        if output_details[0]['quantization'] == (0.0, 0):
            output_data = softmax(output_data)

        # Obtener la clase predicha y su confianza
        predicted_class, confidence = get_predicted_class(output_data)

        # Mostrar resultados de todas las clases
        print(f"Probabilidades: {dict(zip(class_names, output_data))}")
        if predicted_class == "Nada Detectado":
            confidence_message = "Confianza baja"
        else:
            confidence_message = f"{confidence * 100:.2f}% de confianza"

        print(f"La imagen probablemente pertenece a '{predicted_class}' con {confidence_message}.")

        # Mostrar el resultado en la ventana de vista en vivo
        cv2.putText(
            frame,
            f"{predicted_class}: {confidence_message}",
            (10, 30),
            cv2.FONT_HERSHEY_SIMPLEX,
            1,
            (0, 255, 0),
            2,
            cv2.LINE_AA,
        )
        cv2.imshow("Vista en vivo", frame)

        # Control de tiempo para mantener un frame constante
        elapsed_time = time.time() - start_time
        wait_time = max(1, int(33 - elapsed_time * 1000))  # Aprox 30 FPS
        key = cv2.waitKey(wait_time) & 0xFF

        if key == ord('q'):
            print("Saliendo del programa.")
            break

finally:
    cap.release()
    cv2.destroyAllWindows()