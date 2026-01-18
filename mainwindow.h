#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <math.h>
#include <cmath>
#include <algorithm> // Para std::min_element y std::max_element
#include <iostream>
#include <vector>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_cargar_clicked();
    void on_graficar_bode_clicked();

    void on_Evaluar_clicked();

    void on_graficar_nyquist_clicked();

    void on_pushButton_clicked();

    void on_baud_activated(int index);

    void on_conectar_clicked();

    void on_start_clicked();

    void on_stop_clicked();

    void on_conectar_2_clicked();

    void on_load_clicked();

    void on_prueba_clicked();

    void on_test_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
    bool fcn_flag = false;
    QVector<double> unwrapPhase(QVector<double> phase);

    double referencia;       // Almacena el valor de referencia de velocidad
    double velocidadActual;  // Almacena el valor actual de la velocidad medida
    struct tpComplejo {
        float pReal = 0.0;
        float pImaginaria = 0.0;
    };

    struct fasor{
        float magnitud ;
        float angulo ;
    };
    fasor eqv;
    QVector<double> adjustPhase(QVector<double> phase);
    QVector<double> magData, phaseData, freqData, phaseAjustada, escData, freqEsc;
    QVector<double> numerador;
    QVector<double> denominador;
    QVector<double> logspace(double start, double end, int numPoints);
    fasor sistema;
    fasor complex2Pol(tpComplejo &complex_num){
        fasor numPol;
        float a,b,c;
        a = pow(complex_num.pImaginaria,2);
        b = pow(complex_num.pReal,2);
        c = a+b;
        numPol.angulo = atan2(complex_num.pImaginaria, complex_num.pReal) * 180.0 / M_PI;
        numPol.magnitud =  sqrt(c);
        return numPol;
    }



    tpComplejo sumar ( tpComplejo &num1 , tpComplejo &num2 ){
        tpComplejo res ;
        res . pReal = num1 . pReal + num2 . pReal ;
        res . pImaginaria = num1 . pImaginaria + num2 . pImaginaria ;
        return res ;
    }


    tpComplejo restar ( tpComplejo &num1 , tpComplejo &num2 ){
        tpComplejo res ;
        res . pReal = num1 . pReal - num2 . pReal ;
        res . pImaginaria = num1 . pImaginaria - num2 . pImaginaria ;

        return res ;
    }


    tpComplejo multiplicar ( tpComplejo &num1 , tpComplejo &num2 ){
        tpComplejo res ;
        res . pReal = num1 . pReal * num2 . pReal
                    - num1 . pImaginaria * num2 . pImaginaria ;
        res . pImaginaria = num1 . pReal * num2 . pImaginaria
                          + num1 . pImaginaria * num2 . pReal ;

        return res ;
    }


    tpComplejo dividir ( tpComplejo &num1 , tpComplejo &num2 ){
        tpComplejo res ;

        float denom = num2 . pReal * num2 . pReal
                      + num2 . pImaginaria * num2 . pImaginaria ;

        res . pReal = num1 . pReal * num2 . pReal
                    + num1 . pImaginaria * num2 . pImaginaria ;
        res . pReal = res . pReal / denom ;

        res . pImaginaria = num1 . pImaginaria * num2 . pReal
                          - num1 . pReal * num2 . pImaginaria ;
        res . pImaginaria = res . pImaginaria / denom ;

        return res ;
    }

    // Función para calcular la potencia de un número complejo usando el Teorema de Moivre
    tpComplejo potenciaCompleja(tpComplejo &z, int n) {
        tpComplejo resultado;

        // 1. Calcula el módulo r y el ángulo θ del número complejo z
        double r = std::sqrt(z.pReal * z.pReal + z.pImaginaria * z.pImaginaria);
        double theta = std::atan2(z.pImaginaria, z.pReal);

        // 2. Aplica el Teorema de Moivre: r^n * (cos(nθ) + i * sin(nθ))
        double rPotencia = std::pow(r, n);
        resultado.pReal = rPotencia * std::cos(n * theta);
        resultado.pImaginaria = rPotencia * std::sin(n * theta);

        return resultado;
    }
    tpComplejo transferFunction(QVector<double> num,QVector<double> den,tpComplejo &s){
        tpComplejo numt;
        tpComplejo dent;
        tpComplejo aux ;
        tpComplejo aux1 ;
        tpComplejo aux2 ;
        tpComplejo tFnc;
        numt.pImaginaria = 0.0;
        numt.pReal = 0.0;
        aux.pImaginaria = 0.0;
        aux.pReal = 0.0;
        // Evalúa el polinomio del numerador
        for (int i = 0; i < num.size(); ++i) {
            aux.pReal = num[i];
            aux1 = potenciaCompleja(s, (num.size() - i - 1));
            aux2 =  multiplicar ( aux ,aux1);
            numt = sumar(numt,aux2);
        }

        // Evalúa el polinomio del denominador
        for (int i = 0; i < den.size(); ++i) {
            aux.pReal = den[i];
            aux1 = potenciaCompleja(s, (den.size() - i - 1));
            aux2 =  multiplicar ( aux ,aux1);
            dent = sumar(dent,aux2);
        }
        tFnc = dividir(numt,dent);
        return tFnc;
    }

    QVector<double> linspace(double start, double end, int num) {
        QVector<double> result;

        if (num <= 0) return result;  // Si el número de puntos es 0 o negativo, devuelve un vector vacío
        if (num == 1) {
            result.append(start);  // Si solo se requiere un punto, devuelve el valor inicial
            return result;
        }

        double step = (end - start) / (num - 1);  // Tamaño del paso entre puntos

        for (int i = 0; i < num; ++i) {
            result.append(start + i * step);  // Genera y añade cada punto
        }

        return result;
    }

    fasor bode(QVector<double> numerador,QVector<double> denominador,float value){
        tpComplejo s;
        tpComplejo res;
        fasor eqv;
        auto frequencies = linspace(0, value, 5000);

        QVector<double> magData1, phaseData1, freqData1, phaseAjustada1;

        // Calcular la magnitud y fase
        for (const auto& freq : frequencies) {
            s.pImaginaria = freq;
            s.pReal =  0;
            res  = transferFunction(numerador,denominador,s);
            sistema =  complex2Pol(res);


            // Almacena los resultados en vectores de QCustomPlot
            magData1.append(20 * log10(sistema.magnitud)); // Magnitud en dB
            phaseData1.append(sistema.angulo); // Fase en grados
            freqData1.append(freq);   // Frecuencia en rad/s
        }
        phaseAjustada1 = unwrapPhase(phaseData1);
        eqv.angulo = phaseAjustada1[4999];
        eqv.magnitud = magData1[4999];
        return eqv;
    }
    QSerialPort *serial;
    void readSerialData();
    void updateGraph();
    QTimer *dataTimer;
    void populateSerialPorts();
    QVector<double> linspace(double min,double step, double max, int points);
    int flag = 0;
    void sineupdateGraph();
    QTimer *sindataTimer;
    int com;
    bool rng_flag = 0;
};
#endif // MAINWINDOW_H
