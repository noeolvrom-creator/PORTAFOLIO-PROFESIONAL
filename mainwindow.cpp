#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , referencia(0)
    , velocidadActual(0)
    , serial(new QSerialPort(this))
    , dataTimer(new QTimer(this))
    , sindataTimer(new QTimer(this))

{
    ui->setupUi(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(ui->tabWidget);
    this->centralWidget()->setLayout(layout);


    ui->tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    populateSerialPorts();

    // Configuración del QTimer para actualizar la gráfica
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::updateGraph);
    connect(sindataTimer, &QTimer::timeout, this, &MainWindow::sineupdateGraph);


    // Conexión para leer datos del serial cuando estén disponibles
    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readSerialData);

    // Conexión para manejar errores de desconexión
    connect(serial, &QSerialPort::errorOccurred, this, [](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::ResourceError) {
            QMessageBox::critical(nullptr, "Error", "Se perdió la conexión con el dispositivo");
        }
    });
    // Agregar opciones de baudios al comboBox
    ui->baud->addItem("9600", QSerialPort::Baud9600);
    ui->baud->addItem("115200", QSerialPort::Baud115200);


    // Configuración de la gráfica
    ui->real->addGraph(); // Gráfica para la referencia (línea azul)
    ui->real->graph(0)->setPen(QPen(Qt::blue));
    ui->real->addGraph(); // Gráfica para la velocidad medida (línea roja)
    ui->real->graph(1)->setPen(QPen(Qt::red));

    ui->real->xAxis->setLabel("Tiempo (s)");
    ui->real->yAxis->setLabel("Referencia");
    ui->real->xAxis->setRange(0, 10); // 10 segundos en el eje X
    ui->real->yAxis->setRange(0, 300); // Rango de velocidad en Y


    ui->tester->addGraph(); // Gráfica para la referencia (línea azul)
    ui->tester->graph(0)->setPen(QPen(Qt::blue));
    ui->tester->addGraph(); // Gráfica para la velocidad medida (línea roja)
    ui->tester->graph(1)->setPen(QPen(Qt::red));

    ui->tester->xAxis->setLabel("Tiempo (s)");
    ui->tester->yAxis->setLabel("Referencia");
    ui->tester->xAxis->setRange(0, 10); // 10 segundos en el eje X
    ui->tester->yAxis->setRange(0, 300); // Rango de velocidad en Y



   /* QPixmap pixmap1("C:/Users/noeol/Downloads/descarga (1).png");

    // Verificar si la imagen se cargó correctamente
    if (pixmap1.isNull()) {
        qDebug() << "La imagen no se pudo cargar.";
    } else {
        // Mostrar la imagen en un QLabel
        ui->arduino->setPixmap(pixmap1);

        // Aarduinola imagen para que se ajuste al tamaño del QLabel (opcional)
        ui->arduino->setScaledContents(true);
    }*/
}

MainWindow::~MainWindow()
{
    if (serial->isOpen()) {
        serial->close();
    }
    delete ui;
}

QVector<double> MainWindow::logspace(double start, double end, int numPoints) {
    QVector<double> freqs(numPoints);
    double factor = pow(10.0, (end - start) / (numPoints - 1));
    freqs[0] = pow(10.0, start);
    for (int i = 1; i < numPoints; ++i) {
        freqs[i] = freqs[i - 1] * factor;
    }
    return freqs;
}

QVector<double> linspace(double min, double max, int points) {
    QVector<double> freqs(points);
    double step = (max - min) / (points - 1); // Paso entre puntos

    for (int i = 0; i < points; ++i) {
        freqs[i] = min + i * step; // Calcula cada punto
    }

    return freqs;
}


void MainWindow::on_cargar_clicked()
{
    fcn_flag = true;
    ui->magnitud->clearGraphs(); // Elimina todas las gráficas
    ui->magnitud->replot();      // Redibuja el gráfico para reflejar los cambios
    ui->fase->clearGraphs(); // Elimina todas las gráficas
    ui->fase->replot();      // Redibuja el gráfico para reflejar los cambios
    QString entrada = ui->numerador->text();
    QStringList numCoeficientes = entrada.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);

    numerador.clear();

    for (const QString& coefStr : numCoeficientes) {
        bool ok;
        double coef = coefStr.toDouble(&ok);
        if (ok) {
            numerador.append(coef);
        } else {
            // Manejo de error si la conversión falla (opcional)
            QMessageBox::information(this, "Error", "La conversión no se pudo realizar");
        }
    }

    // Verificar el tamaño del vector numerador y mostrar el último elemento disponible


    entrada = ui->denominador->text();
    QStringList denCoeficientes = entrada.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    magData.clear();
    phaseData.clear();
    freqData.clear();
    phaseAjustada.clear();
    denominador.clear();

    for (const QString& coefStr : denCoeficientes) {
        bool ok;
        double coef = coefStr.toDouble(&ok);
        if (ok) {
            denominador.append(coef);
        } else {
            // Manejo de error si la conversión falla (opcional)
            QMessageBox::information(this, "Error", "La conversión no se pudo realizar");
        }
    }

    if (numerador.size() < 1 || denominador.size() < 1) {
        QMessageBox::information(this, "Error", "Ingresa al menos un coeficiente válido");
    }

}

QVector<double> MainWindow::unwrapPhase(QVector<double> phase) {
    QVector<double> unwrapped(phase);

    // Ajusta la fase inicial si es necesario para que comience en -360 grados
    if (unwrapped[0] > 0) {
        for (int i = 0; i < unwrapped.size(); ++i) {
            unwrapped[i] -= 360.0;
        }
    }

    // Aplica el "unwrap" para evitar saltos mayores a 180 grados
    for (int i = 1; i < unwrapped.size(); ++i) {
        double diff = unwrapped[i] - unwrapped[i - 1];

        if (diff > 180.0) {
            unwrapped[i] -= 360.0;
        } else if (diff < -180.0) {
            unwrapped[i] += 360.0;
        }
    }

    return unwrapped;
}





void MainWindow::on_graficar_bode_clicked()
{
    rng_flag = 1;
    tpComplejo s;
    tpComplejo res;
    QString min_input = ui->f_start->text();
    QString max_input = ui->f_end->text();

    if(min_input.toDouble()<=0 || min_input=="" || max_input=="" || min_input.toDouble()>=max_input.toDouble()){
        QMessageBox::information(this, "Error", "Ingresa un rango de frecuencia válido");
    }
    if(ui->numerador->text() == "" || ui->denominador->text() == "" || fcn_flag == false){
        QMessageBox::information(this, "Error", "Ingresa una función de transferencia");
    }

    double min = log10(min_input.toDouble());
    double max = log10(max_input.toDouble());
    auto frequencies = logspace(min, max, 50000);


    // Calcular la magnitud y fase
    for (const auto& freq : frequencies) {
        s.pImaginaria = freq;
        s.pReal =  0;
        res  = transferFunction(numerador,denominador,s);
        sistema =  complex2Pol(res);


        // Almacena los resultados en vectores de QCustomPlot
        magData.append(20 * log10(sistema.magnitud)); // Magnitud en dB
        phaseData.append(sistema.angulo); // Fase en grados
        freqData.append(freq);   // Frecuencia en rad/s
    }
    phaseAjustada = unwrapPhase(phaseData);
    // Obtener el valor mínimo y máximo
    double valorMinimo = *std::min_element(magData.begin(), magData.end());
    double valorMaximo = *std::max_element(magData.begin(), magData.end());
    // Crear la gráfica para la magnitud
    ui->magnitud->addGraph(); // Gráfica de magnitud
    ui->magnitud->graph(0)->setData(freqData, magData);
    ui->magnitud->graph(0)->setPen(QPen(Qt::blue));
    ui->magnitud->yAxis->setLabel("Magnitud (dB)");
    ui->magnitud->xAxis->setLabel("Frecuencia (rad/s)");

    // Configura el eje X para que sea logarítmico
    ui->magnitud->xAxis->setScaleType(QCPAxis::stLogarithmic);
    ui->magnitud->xAxis->setRange(min_input.toDouble(), max_input.toDouble()); // Rango en frecuencia

    // Formato logarítmico en el eje x
    ui->magnitud->xAxis->setNumberFormat("eb");
    ui->magnitud->xAxis->setNumberPrecision(0);

    // Ajuste automático del eje Y en función de los datos de magnitud
    ui->magnitud->yAxis->setRange(valorMinimo-10, valorMaximo+10); // Ajusta según los datos
    //ui->magnitud->graph(0)->rescaleValueAxis(true, true);

    // Redibuja la gráfica de magnitud
    ui->magnitud->replot();


    // Obtener el valor mínimo y máximo
    valorMinimo = *std::min_element(phaseAjustada.begin(), phaseAjustada.end());
    valorMaximo = *std::max_element(phaseAjustada.begin(), phaseAjustada.end());
    // Crear la gráfica para la fase
    ui->fase->addGraph(); // Gráfica de fase
    ui->fase->graph(0)->setData(freqData, phaseAjustada);
    ui->fase->graph(0)->setPen(QPen(Qt::red));
    ui->fase->yAxis->setLabel("Fase (grados)");
    ui->fase->xAxis->setLabel("Frecuencia (rad/s)");

    // Configura el eje X para que sea logarítmico
    ui->fase->xAxis->setScaleType(QCPAxis::stLogarithmic);
    ui->fase->xAxis->setRange(min_input.toDouble(), max_input.toDouble()); // Rango en frecuencia

    // Formato logarítmico en el eje x
    ui->fase->xAxis->setNumberFormat("eb");
    ui->fase->xAxis->setNumberPrecision(0);

    // Ajuste automático del eje Y en función de los datos de fase
    ui->fase->yAxis->setRange(valorMinimo-10, valorMaximo+10); // Ajusta según los datos
    //ui->fase->graph(0)->rescaleValueAxis(true, true);

    // Redibuja la gráfica de fase
    ui->fase->replot();


}


void MainWindow::on_Evaluar_clicked()
{
    tpComplejo s;
    tpComplejo res;
    fasor eqvP;
    QString freq_input = ui->freq_val->text();
    float freq = freq_input.toFloat();
    s.pImaginaria = freq;
    s.pReal =  0;
    res  = transferFunction(numerador,denominador,s);
    eqvP = complex2Pol(res);
    ui->val_mag->setText(QString::number(eqvP.magnitud));
    ui->val_ang->setText(QString::number(eqvP.angulo)+"");
}


void MainWindow::on_graficar_nyquist_clicked()
{

    QString min_input = ui->f_start->text();
    QString max_input = ui->f_end->text();

    if(min_input.toDouble()<=0 || min_input=="" || max_input=="" || min_input.toDouble()>=max_input.toDouble()){
        QMessageBox::information(this, "Error", "Ingresa un rango de frecuencia válido");
    }else{
        if(ui->numerador->text() == "" || ui->denominador->text() == "" || fcn_flag == false || rng_flag == false){
            QMessageBox::information(this, "Error", "Ingresa una función de transferencia");
        }else{
            // Limpia cualquier gráfica anterior
            ui->nyquist->clearGraphs();
            ui->nyquist->replot();

            // Crea los vectores de datos
            QVector<double> x(50000), y(50000), yn(50000), magEscalar(50000);
            for (int i = 0; i < x.size(); ++i) {
                magEscalar[i] = pow(10, (magData[i] / 20));  // Magnitud en escala lineal
                x[i] = magEscalar[i] * cos(phaseAjustada[i] * M_PI / 180);  // Parte real
                y[i] = magEscalar[i] * sin(phaseAjustada[i] * M_PI / 180);  // Parte imaginaria positiva
                yn[i] = -y[i];  // Parte imaginaria negativa
            }

            // Gráfica positiva
            ui->nyquist->addGraph();
            ui->nyquist->graph(0)->setData(x, y);
            ui->nyquist->graph(0)->setLineStyle(QCPGraph::lsNone);  // Sin línea, solo puntos
            ui->nyquist->graph(0)->setScatterStyle(QCPScatterStyle::ssDot);  // Puntos finos
            ui->nyquist->graph(0)->setPen(QPen(Qt::darkBlue, 2));  // Color de los puntos
            ui->nyquist->graph(0)->setBrush(QBrush(Qt::NoBrush));  // Sin relleno en los puntos

            // Gráfica negativa
            ui->nyquist->addGraph();
            ui->nyquist->graph(1)->setData(x, yn);
            ui->nyquist->graph(1)->setLineStyle(QCPGraph::lsNone);  // Sin línea, solo puntos
            ui->nyquist->graph(1)->setScatterStyle(QCPScatterStyle::ssDot);  // Puntos finos
            ui->nyquist->graph(1)->setPen(QPen(Qt::darkRed, 2));  // Color de los puntos
            ui->nyquist->graph(1)->setBrush(QBrush(Qt::NoBrush));  // Sin relleno en los puntos

            // Encuentra los valores mínimos y máximos de los datos
            double valorMinY = std::min(*std::min_element(y.begin(), y.end()), *std::min_element(yn.begin(), yn.end()));
            double valorMaxY = std::max(*std::max_element(y.begin(), y.end()), *std::max_element(yn.begin(), yn.end()));
            double valorMinX = *std::min_element(x.begin(), x.end());
            double valorMaxX = *std::max_element(x.begin(), x.end());

            // Calcula el rango máximo (el mayor entre ambos ejes)
            double rangoX = valorMaxX - valorMinX;
            double rangoY = valorMaxY - valorMinY;
            double rangoMaximo = std::max(rangoX, rangoY) / 2.0; // La mitad para centrar la gráfica
            // Calcula el centro de los ejes
            double centroX = (valorMaxX + valorMinX) / 2.0;
            double centroY = (valorMaxY + valorMinY) / 2.0;
            // Ajusta los rangos de los ejes para que sean iguales
            ui->nyquist->xAxis->setRange(centroX - rangoMaximo, centroX + rangoMaximo);
            ui->nyquist->yAxis->setRange(centroY - rangoMaximo, centroY + rangoMaximo);
            // Habilitar interacciones de zoom y desplazamiento (pan)
            ui->nyquist->setInteraction(QCP::iRangeZoom, true); // Habilita zoom con la rueda del ratón
            ui->nyquist->setInteraction(QCP::iRangeDrag, true); // Habilita arrastrar para mover la vista
            // Configurar el zoom para que funcione en ambos ejes simultáneamente
            ui->nyquist->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical); // Zoom en ambos ejes
            // Configurar el arrastre para que afecte ambos ejes
            ui->nyquist->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical); // Arrastre en ambos ejes
            // Ajustar la velocidad de zoom (opcional, por defecto es sensible)
            ui->nyquist->axisRect()->setRangeZoomFactor(0.85); // Incrementa o disminuye la velocidad del zoom4
            ui->nyquist->replot();


        }
    }




}



void MainWindow::on_pushButton_clicked()
{
    if(ui->numerador->text() == "" || ui->denominador->text() == "" || fcn_flag == false){
        QMessageBox::information(this, "Error", "Ingresa una función de transferencia");
    }else{
        double margen, respuesta, wn,muestreo;
        QString e_margen = ui->margen_l->text();
        QString e_respuesta = ui->respuesta->text();
        QString e_muestreo = ui->muestreo->text();
        margen = e_margen.toDouble();
        respuesta = e_respuesta.toDouble();
        wn = 1.0/respuesta;

        tpComplejo s;
        tpComplejo res;
        double anguloControlador , magnitudControlador;
        s.pImaginaria = wn;
        res  = transferFunction(numerador,denominador,s);

        eqv = complex2Pol(res);
        s.pReal =  0;
        double ki,kp,kd;
        //if(flag != 0){
            muestreo = e_muestreo.toDouble();
            anguloControlador = tan((margen-175-eqv.angulo+((180.0*wn*muestreo)/(2*M_PI)))*(M_PI/180.0));
            magnitudControlador = 1.0/eqv.magnitud;
            double c = ((wn*wn)/(anguloControlador*anguloControlador)) + (wn*wn);

            kd = sqrt((magnitudControlador*magnitudControlador)/c);
            if(kd>0.0){
                ui->kd->setText(QString::number(kd));
            }else{
                ui->kd->setText(QString::number(0));
            }

            kp = (wn*kd)/anguloControlador;
            if(kp>0.0){
                ui->kp->setText(QString::number(kp));
            }else{
                ui->kp->setText(QString::number(0));
            }

            ki = wn*kp*tan((5.0*M_PI)/180.0);

            if(ki>0.0){
                ui->ki->setText(QString::number(ki));
            }else{
                ui->ki->setText(QString::number(0));
            }
            if(ki<0.0 || kp<0.0 || kd<0.0){
                QMessageBox::information(this, "Error", "No se pudo sintonizar el sistema, prueba cambiando el margen de fase");
            }
    }


}



/*void MainWindow::on_com_activated(int index)
{
    if (serial->isOpen()) {
        serial->close();
    }

    QString portName = ui->com->itemText(index);
    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "Error", "No se pudo abrir el puerto seleccionado.");
    }
}*/

void MainWindow::on_baud_activated(int index)
{
    com = index;
    serial->setBaudRate(ui->baud->itemData(index).toInt());
    //qDebug() << "Baudrate cambiado a: " << ui->baud->itemData(index).toInt();
}

void MainWindow::readSerialData()
{
    static QString buffer;
    buffer += serial->readAll();

    int index = buffer.indexOf('\n');
    while (index != -1) {
        QString message = buffer.left(index).trimmed();
        buffer.remove(0, index + 1);
        index = buffer.indexOf('\n');

        if (message.startsWith("REF:") && message.contains(",VEL:")) {
            QStringList parts = message.split(",");
            if (parts.size() == 2) {
                referencia = parts[0].mid(4).toDouble();
                velocidadActual = parts[1].mid(4).toDouble();
            }
        }
    }
}

void MainWindow::updateGraph()
{
    double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

    // Agrega los puntos a las dos gráficas (referencia y velocidad actual)
    ui->real->graph(0)->addData(currentTime, referencia);
    ui->real->graph(1)->addData(currentTime, velocidadActual);

    // Ajusta el rango en el eje X para mostrar los últimos 10 segundos
    ui->real->xAxis->setRange(currentTime - 10, currentTime);


    bool foundRange0, foundRange1;
    QCPRange range0 = ui->real->graph(0)->data()->valueRange(foundRange0);
    QCPRange range1 = ui->real->graph(1)->data()->valueRange(foundRange1);

    if (foundRange0 && foundRange1) {
        double minY = std::min(range0.lower, range1.lower); // Mínimo entre las dos gráficas
        double maxY = std::max(range0.upper, range1.upper); // Máximo entre las dos gráficas

        // Agregar un pequeño margen al rango para que no se recorten los extremos
        double margin = 0.1 * (maxY - minY);
        ui->real->yAxis->setRange(minY - margin, maxY + margin);
    }
    // Configuración inicial de la gráfica (se puede ejecutar una vez en el constructor también)
    ui->real->xAxis->setLabel("Tiempo (s)");
    ui->real->yAxis->setLabel("Valor");

    // Reconfigura las leyendas (si no se hace antes en el constructor)
    ui->real->legend->setVisible(true);
    ui->real->graph(0)->setName("Referencia");
    ui->real->graph(1)->setName("Velocidad Actual");

    ui->real->replot();
}

void MainWindow::on_conectar_clicked()
{

    if (serial->isOpen()) {
        serial->close();
    }

    QString portName = ui->com->currentText();
    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "Error", "No se pudo abrir el puerto seleccionado.");
    }

}

void MainWindow::populateSerialPorts()
{
    ui->com->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->com->addItem(info.portName());
    }
}

// Comando para iniciar el motor
void MainWindow::on_start_clicked() {
    flag =1;

    if (serial->isOpen()) {
        QByteArray data = "START:\n";
        serial->write(data);
        dataTimer->start(1); // Actualiza cada 100 ms
        bool ok;
        int value = ui->ref->text().toInt(&ok);
        if (ok) {
            data = "REF:" + QByteArray::number(value) + "\n";
            serial->write(data);
        }
    } else {
        QMessageBox::warning(this, "Advertencia", "Seleccione un puerto COM válido.");
    }
}

// Comando para detener el motor
void MainWindow::on_stop_clicked() {
    dataTimer->stop();
    if (serial->isOpen()) {
        QByteArray data = "STOP:1\n";
        serial->write(data);
    } else {
        QMessageBox::warning(this, "Advertencia", "Seleccione un puerto COM válido.");
    }
}

// Cerrar conexión
void MainWindow::on_conectar_2_clicked() {
    if (serial->isOpen()) {
        serial->close();
    }
}

// Enviar parámetros PID
void MainWindow::on_load_clicked() {
    if (serial->isOpen()) {
        bool ok;
        QByteArray data;

        // Enviar KP
        double kp = ui->kp->text().toDouble(&ok);
        if (ok) {
            data = "KP:" + QByteArray::number(kp, 'f', 6) + "\n";
            serial->write(data);
        }

        // Enviar KI
        double ki = ui->ki->text().toDouble(&ok);
        if (ok) {
            data = "KI:" + QByteArray::number(ki, 'f', 6) + "\n";
            serial->write(data);
        }

        // Enviar KD
        double kd = ui->kd->text().toDouble(&ok);
        if (ok) {
            data = "KD:" + QByteArray::number(kd, 'f', 6) + "\n";
            serial->write(data);
        }
    } else {
        QMessageBox::warning(this, "Advertencia", "Seleccione un puerto COM válido.");
    }
}



void MainWindow::on_prueba_clicked()
{
    if(ui->numerador->text() == "" || ui->denominador->text() == "" || fcn_flag == false){
        QMessageBox::information(this, "Error", "Ingresa una función de transferencia");
    }else{
        // Limpiar gráficos previos
        ui->frecuencia->clearGraphs();
        ui->frecuencia->replot();

        ui->tester->clearGraphs();
        ui->tester->replot();


        tpComplejo s;
        tpComplejo res;
        fasor eqvP;
        QString freq_input = ui->hz->text();
        QString amp_input = ui->amp->text();
        float amp = amp_input.toFloat();
        float freq = freq_input.toFloat() * 2.0 * M_PI;
        s.pImaginaria = freq;
        s.pReal = 0;
        res = transferFunction(numerador, denominador, s);
        eqvP = complex2Pol(res);

        QVector<double> x(50000), y(50000), yo(50000);

        // Llenado de datos para las señales
        for (int i = 0; i < x.size(); ++i) {
            x[i] = i * 0.0001;  // Ajusta el incremento según la frecuencia deseada
            y[i] = amp*eqvP.magnitud * sin((freq * x[i]) + eqvP.angulo);
            yo[i] = amp*sin(freq * x[i]);
        }

        ui->m_prueba->setText(QString::number(eqvP.magnitud));
        ui->a_prueba->setText(QString::number(eqvP.angulo));

        // Añadir el primer gráfico para la señal `yo` (Señal excitadora)
        ui->frecuencia->addGraph();
        ui->frecuencia->graph(0)->setData(x, yo);
        ui->frecuencia->graph(0)->setPen(QPen(Qt::blue));
        ui->frecuencia->graph(0)->setName("Señal excitadora");  // Nombre para la leyenda

        // Añadir el segundo gráfico para la señal `y` (Señal de salida)
        ui->frecuencia->addGraph();
        ui->frecuencia->graph(1)->setData(x, y);
        ui->frecuencia->graph(1)->setPen(QPen(Qt::red));
        ui->frecuencia->graph(1)->setName("Señal de salida");  // Nombre para la leyenda

        // Configurar leyenda
        ui->frecuencia->legend->setVisible(true);  // Hacer visible la leyenda
        ui->frecuencia->legend->setFont(QFont("Helvetica", 9));  // Ajusta la fuente de la leyenda
        ui->frecuencia->legend->setBrush(QBrush(Qt::white));  // Fondo blanco para la leyenda

        // Etiquetas de los ejes
        ui->frecuencia->yAxis->setLabel("Magnitud");
        ui->frecuencia->xAxis->setLabel("Tiempo (s)");


        // Agregar gráficas
        ui->tester->addGraph(); // Gráfica de referencia
        ui->tester->addGraph(); // Gráfica de posición

        // Configurar estilos de las gráficas
        ui->tester->graph(0)->setPen(QPen(Qt::blue)); // Referencia en azul
        ui->tester->graph(1)->setPen(QPen(Qt::red));  // Posición en rojo

        // Configurar ejes
        ui->tester->xAxis->setLabel("Tiempo (s)");
        ui->tester->yAxis->setLabel("Posición (grados)");

        // Mostrar leyenda
        ui->tester->legend->setVisible(true);
        ui->tester->graph(0)->setName("Referencia");
        ui->tester->graph(1)->setName("Posición");

        // Configurar rangos iniciales
        ui->tester->xAxis->setRange(0, 10);
        ui->tester->yAxis->setRange(0, 360);


        // Ajuste automático del rango del eje x
        double period = (freq != 0) ? (2.0 * M_PI / freq) : 1;  // Período de la señal en segundos
        double cycles = 5.0;  // Número de ciclos que deseas mostrar en la gráfica
        ui->frecuencia->xAxis->setRange(0, cycles * period);  // Rango de tiempo basado en la frecuencia

        // Ajuste automático del rango del eje y
        double maxMagnitude = std::max(*std::max_element(y.begin(), y.end()), *std::max_element(yo.begin(), yo.end()));
        double minMagnitude = std::min(*std::min_element(y.begin(), y.end()), *std::min_element(yo.begin(), yo.end()));
        ui->frecuencia->yAxis->setRange(minMagnitude - 0.1 * fabs(minMagnitude), maxMagnitude + 0.1 * fabs(maxMagnitude));  // Agrega un margen del 10%

        // Actualizar la gráfica
        ui->frecuencia->replot();

    }




}


void MainWindow::sineupdateGraph()
{
    // Obtener el tiempo actual en segundos
    double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

    // Agregar puntos a las gráficas
    ui->tester->graph(0)->addData(currentTime, referencia);      // Gráfica de referencia
    ui->tester->graph(1)->addData(currentTime, velocidadActual); // Gráfica de posición

    // Ajustar el rango del eje x para mostrar los últimos 10 segundos
    ui->tester->xAxis->setRange(currentTime - 10, currentTime);

    // Ajustar automáticamente el rango del eje y
    bool foundRange0, foundRange1;
    QCPRange range0 = ui->tester->graph(0)->data()->valueRange(foundRange0);
    QCPRange range1 = ui->tester->graph(1)->data()->valueRange(foundRange1);

    if (foundRange0 && foundRange1) {
        double minY = std::min(range0.lower, range1.lower); // Mínimo entre las dos gráficas
        double maxY = std::max(range0.upper, range1.upper); // Máximo entre las dos gráficas

        // Agregar un pequeño margen al rango para que no se recorten los extremos
        double margin = 0.1 * (maxY - minY);
        ui->tester->yAxis->setRange(minY - margin, maxY + margin);
    }

    // Etiquetas y leyenda
    ui->tester->xAxis->setLabel("Tiempo (s)");
    ui->tester->yAxis->setLabel("Salida");

    ui->tester->legend->setVisible(true);
    ui->tester->graph(0)->setName("Señal de excitación");
    ui->tester->graph(1)->setName("Señal de salida");

    // Actualizar la gráfica
    ui->tester->replot();
}



void MainWindow::on_test_clicked()
{
    bool ok;

    QString muestreo_input = ui->muestreo->text();
    QString hz_input =  ui->hz->text();
    QString amp_input = ui->amp->text();
    float amp = amp_input.toFloat(&ok);
    float hz = hz_input.toFloat(&ok);
    float muestreo = muestreo_input.toFloat();
    float freq_lim = 1 / muestreo;
    QByteArray data;
    serial->write(data);
    if (serial->isOpen()) {
        if (hz <= freq_lim) {
            sindataTimer->start(1); // Actualiza cada 100 ms
            //if (ok) {
                data = "AMP:" + QByteArray::number(amp, 'f', 6) + "\n";
                serial->write(data);
                data = "FREQ:" + QByteArray::number(hz, 'f', 6) + "\n";
                serial->write(data);

                // Enviar comando para iniciar el motor
                serial->write("STARTS:\n");
            //}

        } else {
            QMessageBox::warning(this, "Advertencia", "Ingresa una frecuencia adecuada que no rebase la frecuencia de muestreo");
        }
    } else {
        QMessageBox::warning(this, "Advertencia", "Seleccione un puerto COM válido.");
    }
}



void MainWindow::on_pushButton_2_clicked()
{
    sindataTimer->stop();
    if (serial->isOpen()) {
        QByteArray data = "STOP:1\n";
        serial->write(data);
    } else {
        QMessageBox::warning(this, "Advertencia", "Seleccione un puerto COM válido.");
    }
}

