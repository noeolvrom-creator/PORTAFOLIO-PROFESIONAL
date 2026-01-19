import matplotlib.pyplot as plt
import numpy as np
import tensorflow as tf
from tensorflow.keras import layers, Sequential
import pathlib

# Ruta del dataset descargado (ajusta la ruta según dónde esté tu archivo zip)
dataset_zip_path = r"C:\Users\noeol\Documents\IMT\7º SEMESTRE\CONTROL DIGITAL\U3\SENALES.zip"

# Descomprimir el dataset
import zipfile
import os

extract_path = "dataset_señalamientos"
with zipfile.ZipFile(dataset_zip_path, 'r') as zip_ref:
    zip_ref.extractall(extract_path)

data_dir = pathlib.Path(extract_path)
print(f"Dataset directory: {data_dir}")

# Contar imágenes
image_count = len(list(data_dir.glob('*/*.jpg')))
print(f"Total imágenes: {image_count}")

# Configuración para el entrenamiento
batch_size = 32
img_height = 180
img_width = 180

train_ds = tf.keras.utils.image_dataset_from_directory(
    data_dir,
    validation_split=0.2,
    subset="training",
    seed=123,
    image_size=(img_height, img_width),
    batch_size=batch_size
)

val_ds = tf.keras.utils.image_dataset_from_directory(
    data_dir,
    validation_split=0.2,
    subset="validation",
    seed=123,
    image_size=(img_height, img_width),
    batch_size=batch_size
)

class_names = train_ds.class_names
print(f"Clases encontradas: {class_names}")

# Mostrar algunas imágenes del dataset
plt.figure(figsize=(10, 10))
for images, labels in train_ds.take(1):
    for i in range(9):
        ax = plt.subplot(3, 3, i + 1)
        plt.imshow(images[i].numpy().astype("uint8"))
        plt.title(class_names[labels[i]])
        plt.axis("off")

# Optimización del dataset
AUTOTUNE = tf.data.AUTOTUNE
train_ds = train_ds.cache().shuffle(1000).prefetch(buffer_size=AUTOTUNE)
val_ds = val_ds.cache().prefetch(buffer_size=AUTOTUNE)

# Normalización
normalization_layer = layers.Rescaling(1. / 255)

# Construcción del modelo
num_classes = len(class_names)

model = Sequential([
    layers.Rescaling(1. / 255, input_shape=(img_height, img_width, 3)),
    layers.Conv2D(16, 3, padding='same', activation='relu'),
    layers.MaxPooling2D(),
    layers.Conv2D(32, 3, padding='same', activation='relu'),
    layers.MaxPooling2D(),
    layers.Conv2D(64, 3, padding='same', activation='relu'),
    layers.MaxPooling2D(),
    layers.Flatten(),
    layers.Dense(128, activation='relu'),
    layers.Dense(num_classes)
])

model.compile(
    optimizer='adam',
    loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=True),
    metrics=['accuracy']
)

model.summary()

# Entrenamiento
epochs = 10
history = model.fit(
    train_ds,
    validation_data=val_ds,
    epochs=epochs
)

# Visualización de resultados
acc = history.history['accuracy']
val_acc = history.history['val_accuracy']
loss = history.history['loss']
val_loss = history.history['val_loss']

epochs_range = range(epochs)

plt.figure(figsize=(8, 8))
plt.subplot(1, 2, 1)
plt.plot(epochs_range, acc, label='Training Accuracy')
plt.plot(epochs_range, val_acc, label='Validation Accuracy')
plt.legend(loc='lower right')
plt.title('Training and Validation Accuracy')

plt.subplot(1, 2, 2)
plt.plot(epochs_range, loss, label='Training Loss')
plt.plot(epochs_range, val_loss, label='Validation Loss')
plt.legend(loc='upper right')
plt.title('Training and Validation Loss')
plt.show()

# Guardar el modelo
model.save("sign_classifier_model")

# Prueba con una imagen nueva
test_image_path = r"C:\Users\noeol\Documents\IMT\7º SEMESTRE\CONTROL DIGITAL\U3\SENALES\NO_PASAR\IMG_20241127_132318_1.jpg"  # Cambia a la ruta de tu imagen
img = tf.keras.utils.load_img(
    test_image_path, target_size=(img_height, img_width)
)
img_array = tf.keras.utils.img_to_array(img)
img_array = tf.expand_dims(img_array, 0)  # Crear un batch

predictions = model.predict(img_array)
score = tf.nn.softmax(predictions[0])

print(
    "La imagen probablemente pertenece a '{}' con una confianza de {:.2f}%."
    .format(class_names[np.argmax(score)], 100 * np.max(score))
)
