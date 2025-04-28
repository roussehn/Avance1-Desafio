/*
 * Programa demostrativo de manipulaciónprocesamiento de imágenes BMP en C++ usando Qt.
 *
 * Descripción:
 * Este programa realiza las siguientes tareas:
 * 1. Carga una imagen BMP desde un archivo (formato RGB sin usar estructuras ni STL).
 * 2. Modifica los valores RGB de los píxeles asignando un degradado artificial basado en su posición.
 * 3. Exporta la imagen modificada a un nuevo archivo BMP.
 * 4. Carga un archivo de texto que contiene una semilla (offset) y los resultados del enmascaramiento
 *    aplicados a una versión transformada de la imagen, en forma de tripletas RGB.
 * 5. Muestra en consola los valores cargados desde el archivo de enmascaramiento.
 * 6. Gestiona la memoria dinámicamente, liberando los recursos utilizados.
 *
 * Entradas:
 * - Archivo de imagen BMP de entrada ("I_O.bmp").
 * - Archivo de salida para guardar la imagen modificada ("I_D.bmp").
 * - Archivo de texto ("M1.txt") que contiene:
 *     • Una línea con la semilla inicial (offset).
 *     • Varias líneas con tripletas RGB resultantes del enmascaramiento.
 *
 * Salidas:
 * - Imagen BMP modificada ("I_D.bmp").
 * - Datos RGB leídos desde el archivo de enmascaramiento impresos por consola.
 *
 * Requiere:
 * - Librerías Qt para manejo de imágenes (QImage, QString).
 * - No utiliza estructuras ni STL. Solo arreglos dinámicos y memoria básica de C++.
 *
 * Autores: Augusto Salazar Y Aníbal Guerra
 * Fecha: 06/04/2025
 * Asistencia de ChatGPT para mejorar la forma y presentación del código fuente
 */

#include <fstream>
#include <iostream>
#include <QCoreApplication>
#include <QImage>

using namespace std;
unsigned char* loadPixels(QString input, int &width, int &height);
bool exportImage(unsigned char* pixelData, int width,int height, QString archivoSalida);
unsigned int* loadSeedMasking(const char* nombreArchivo, int &seed, int &n_pixels);
void realizarXOR(unsigned char* img1, unsigned char* img2, unsigned char* resultado, int n_pixeles) {
    for (int i = 0; i < n_pixeles * 3; i++) {
        resultado[i] = img1[i] ^ img2[i];
    }
}
unsigned char rotarDerecha(unsigned char valor, int bits) {
    return (valor >> bits) | (valor << (8 - bits));
}
unsigned char rotarIzquierda(unsigned char valor, int bits) {
    return (valor << bits) | (valor >> (8 - bits));
}
unsigned char desplazarDerecha(unsigned char valor, int bits) {
    return valor >> bits;
}
unsigned char desplazarIzquierda(unsigned char valor, int bits) {
    return valor << bits;
}
bool comprobarLaMascara(unsigned char* imagen, unsigned char* mascara, unsigned int* valoresTxt, int width, int height, int maskWidth, int maskHeight, int seed, int n_pixeles) {
    int totalPixels = width * height;
    int startIndex = seed * 3;
    for (int k = 0; k < n_pixeles; ++k) {
        if (startIndex + k * 3 + 2 >= totalPixels * 3) {
            cout << "rrror: esta fuera del rango" << endl;
            return false;
        }

        for (int c = 0; c < 3; ++c) {
            int suma = imagen[startIndex + k * 3 + c] + mascara[k * 3 + c];
            if (suma != valoresTxt[k * 3 + c]) {
                return false;
            }
        }
    }
    return true;
}
void aplicarXOR(unsigned char* img, const unsigned char* mask, int totalBytes) {
    for (int i = 0; i < totalBytes; ++i) {
        img[i] ^= mask[i];
    }
}
void rotarBufferIzquierda(unsigned char* buffer, int totalBytes, int bits) {
    for (int i = 0; i < totalBytes; ++i) {
        buffer[i] = rotarIzquierda(buffer[i], bits);
    }
}
bool comprobarLaMascara(unsigned char* imagen, unsigned char* mascara,
                        unsigned int* valoresTxt, int width, int height,
                        int seed, int n_pixeles) {
    int totalPixels = width * height;
    int startIndex = seed * 3;

    for (int k = 0; k < n_pixeles; ++k) {
        unsigned int idx = startIndex + k * 3;
        if (idx + 2 >= static_cast<unsigned int>(totalPixels * 3)) {
            cout << "Error: fuera de rango en comprobarLaMascara" << endl;
            return false;
        }

        for (int c = 0; c < 3; ++c) {
            int suma = imagen[idx + c] + mascara[k * 3 + c];
            if (suma != static_cast<int>(valoresTxt[k * 3 + c])) {
                return false;
            }
        }
    }
    return true;
}
void desenmascarar(unsigned char* img, const unsigned char* mask,
                   const unsigned int* S, int seed, int totalBytes) {
    if (!img || !mask || !S || totalBytes <= 0)
        return;
    for (int k = 0; k < totalBytes; ++k) {
        img[seed + k] = static_cast<unsigned char>((S[k] - mask[k]) & 0xFF);
    }
}
void reconstruirImagen() {
    int width = 0, height = 0;

    // Paso 1: Cargar imagen transformada final (P3.bmp)
    unsigned char* img = loadPixels("P3.bmp", width, height);
    if (!img) {
        cout << " No se pudo cargar P3.bmp" << endl;
        return;
    }

    // Paso 2: Cargar imagen aleatoria usada para XOR (I_M.bmp)
    int w2 = 0, h2 = 0;
    unsigned char* imRand = loadPixels("I_M.bmp", w2, h2);
    if (!imRand || w2 != width || h2 != height) {
        cout << " No se pudo cargar I_M.bmp o tamaño incorrecto" << endl;
        delete[] img;
        return;
    }

    // Paso 3: Cargar máscara (M.bmp)
    int mw = 0, mh = 0;
    unsigned char* mask = loadPixels("M.bmp", mw, mh);
    if (!mask) {
        delete[] img;
        delete[] imRand;
        return;
    }

    int totalBytes = width * height * 3;
    int totalMaskBytes = mw * mh * 3;

    // Paso 4: Cargar datos M1.txt y M2.txt
    int seed1 = 0, n1 = 0;
    unsigned int* S1 = loadSeedMasking("M1.txt", seed1, n1);
    int seed2 = 0, n2 = 0;
    unsigned int* S2 = loadSeedMasking("M2.txt", seed2, n2);

    if (!S1 || !S2) {
        cout << " Error al cargar archivos de enmascaramiento" << endl;
        delete[] img;
        delete[] imRand;
        delete[] mask;
        delete[] S1;
        delete[] S2;
        return;
    }

    // Puedes cambiar entre seed * 3 o solo seed según resultados
    int offset1 = seed1;
    int offset2 = seed2;

    // Verifica si las regiones están dentro del rango
    if (offset1 + totalMaskBytes > totalBytes || offset2 + totalMaskBytes > totalBytes) {
        cout << " Regiones de enmascaramiento invalidas" << endl;
        delete[] img;
        delete[] imRand;
        delete[] mask;
        delete[] S1;
        delete[] S2;
        return;
    }

    // === PASO 3 INVERSO: XOR final ===
    aplicarXOR(img, imRand, totalBytes);
    exportImage(img, width, height, "debug_paso3_XOR.bmp");

    // === PASO 2 INVERSO: desenmascarar con S2 y rotar izquierda ===
    desenmascarar(img, mask, S2, offset2, totalMaskBytes);
    bool valido2 = comprobarLaMascara(img, mask, S2, width, height, seed2, n2);
    cout << "🔍 Validacion M2.txt: " << (valido2 ? " Correcto" : " Incorrecto") << endl;
    exportImage(img, width, height, "debug_paso2_desenmascara.bmp");

    rotarBufferIzquierda(img, totalBytes, 3);
    exportImage(img, width, height, "debug_paso2_rotado.bmp");

    // === PASO 1 INVERSO: desenmascarar con S1 y XOR ===
    desenmascarar(img, mask, S1, offset1, totalMaskBytes);
    bool valido1 = comprobarLaMascara(img, mask, S1, width, height, seed1, n1);
    cout << "🔍 Validacion M1.txt: " << (valido1 ? " Correcto" : " Incorrecto") << endl;
    exportImage(img, width, height, "debug_paso1_desenmascara.bmp");

    aplicarXOR(img, imRand, totalBytes);
    exportImage(img, width, height, "debug_final.bmp");

    // Exportar imagen final reconstruida
    exportImage(img, width, height, "I_D.bmp");
    cout << " Imagen reconstruida y guardada como I_D.bmp" << endl;

    // Liberar memoria
    delete[] img;
    delete[] imRand;
    delete[] mask;
    delete[] S1;
    delete[] S2;
}
int main()
{
    // Definición de rutas de archivo de entrada (imagen original) y salida (imagen modificada)
    QString archivoEntrada = "I_O.bmp";
    QString archivoSalida = "I_D.bmp";

    // Variables para almacenar las dimensiones de la imagen
    int height = 0;
    int width = 0;

    // Carga la imagen BMP en memoria dinámica y obtiene ancho y alto
    unsigned char *pixelData = loadPixels(archivoEntrada, width, height);

    // Simula una modificación de la imagen asignando valores RGB incrementales
    // (Esto es solo un ejemplo de manipulación artificial)
    for (int i = 0; i < width * height * 3; i += 3) {
        pixelData[i] = i;     // Canal rojo
        pixelData[i + 1] = i; // Canal verde
        pixelData[i + 2] = i; // Canal azul
    }

    // Exporta la imagen modificada a un nuevo archivo BMP
    bool exportI = exportImage(pixelData, width, height, archivoSalida);

    // Muestra si la exportación fue exitosa (true o false)
    cout << exportI << endl;

    // Libera la memoria usada para los píxeles
    delete[] pixelData;
    pixelData = nullptr;

    // Variables para almacenar la semilla y el número de píxeles leídos del archivo de enmascaramiento
    int seed = 0;
    int n_pixels = 0;

    // Carga los datos de enmascaramiento desde un archivo .txt (semilla + valores RGB)
    unsigned int *maskingData = loadSeedMasking("M1.txt", seed, n_pixels);

    // Muestra en consola los primeros valores RGB leídos desde el archivo de enmascaramiento
    for (int i = 0; i < n_pixels * 3; i += 3) {
        cout << "Pixel " << i / 3 << ": ("
             << maskingData[i] << ", "
             << maskingData[i + 1] << ", "
             << maskingData[i + 2] << ")" << endl;
    }

    // Libera la memoria usada para los datos de enmascaramiento
    if (maskingData != nullptr){
        delete[] maskingData;
        maskingData = nullptr;
    }
    reconstruirImagen();
    return 0; // Fin del programa
}


unsigned char* loadPixels(QString input, int &width, int &height){
/*
 * @brief Carga una imagen BMP desde un archivo y extrae los datos de píxeles en formato RGB.
 *
 * Esta función utiliza la clase QImage de Qt para abrir una imagen en formato BMP, convertirla al
 * formato RGB888 (24 bits: 8 bits por canal), y copiar sus datos de píxeles a un arreglo dinámico
 * de tipo unsigned char. El arreglo contendrá los valores de los canales Rojo, Verde y Azul (R, G, B)
 * de cada píxel de la imagen, sin rellenos (padding).
 *
 * @param input Ruta del archivo de imagen BMP a cargar (tipo QString).
 * @param width Parámetro de salida que contendrá el ancho de la imagen cargada (en píxeles).
 * @param height Parámetro de salida que contendrá la altura de la imagen cargada (en píxeles).
 * @return Puntero a un arreglo dinámico que contiene los datos de los píxeles en formato RGB.
 *         Devuelve nullptr si la imagen no pudo cargarse.
 *
 * @note Es responsabilidad del usuario liberar la memoria asignada al arreglo devuelto usando `delete[]`.
 */

    // Cargar la imagen BMP desde el archivo especificado (usando Qt)
    QImage imagen(input);

    // Verifica si la imagen fue cargada correctamente
    if (imagen.isNull()) {
        cout << "Error: No se pudo cargar la imagen BMP." << std::endl;
        return nullptr; // Retorna un puntero nulo si la carga falló
    }

    // Convierte la imagen al formato RGB888 (3 canales de 8 bits sin transparencia)
    imagen = imagen.convertToFormat(QImage::Format_RGB888);

    // Obtiene el ancho y el alto de la imagen cargada
    width = imagen.width();
    height = imagen.height();

    // Calcula el tamaño total de datos (3 bytes por píxel: R, G, B)
    int dataSize = width * height * 3;

    // Reserva memoria dinámica para almacenar los valores RGB de cada píxel
    unsigned char* pixelData = new unsigned char[dataSize];

    // Copia cada línea de píxeles de la imagen Qt a nuestro arreglo lineal
    for (int y = 0; y < height; ++y) {
        const uchar* srcLine = imagen.scanLine(y);              // Línea original de la imagen con posible padding
        unsigned char* dstLine = pixelData + y * width * 3;     // Línea destino en el arreglo lineal sin padding
        memcpy(dstLine, srcLine, width * 3);                    // Copia los píxeles RGB de esa línea (sin padding)
    }

    // Retorna el puntero al arreglo de datos de píxeles cargado en memoria
    return pixelData;
}

bool exportImage(unsigned char* pixelData, int width,int height, QString archivoSalida){
/*
 * @brief Exporta una imagen en formato BMP a partir de un arreglo de píxeles en formato RGB.
 *
 * Esta función crea una imagen de tipo QImage utilizando los datos contenidos en el arreglo dinámico
 * `pixelData`, que debe representar una imagen en formato RGB888 (3 bytes por píxel, sin padding).
 * A continuación, copia los datos línea por línea a la imagen de salida y guarda el archivo resultante
 * en formato BMP en la ruta especificada.
 *
 * @param pixelData Puntero a un arreglo de bytes que contiene los datos RGB de la imagen a exportar.
 *                  El tamaño debe ser igual a width * height * 3 bytes.
 * @param width Ancho de la imagen en píxeles.
 * @param height Alto de la imagen en píxeles.
 * @param archivoSalida Ruta y nombre del archivo de salida en el que se guardará la imagen BMP (QString).
 *
 * @return true si la imagen se guardó exitosamente; false si ocurrió un error durante el proceso.
 *
 * @note La función no libera la memoria del arreglo pixelData; esta responsabilidad recae en el usuario.
 */

    // Crear una nueva imagen de salida con el mismo tamaño que la original
    // usando el formato RGB888 (3 bytes por píxel, sin canal alfa)
    QImage outputImage(width, height, QImage::Format_RGB888);

    // Copiar los datos de píxeles desde el buffer al objeto QImage
    for (int y = 0; y < height; ++y) {
        // outputImage.scanLine(y) devuelve un puntero a la línea y-ésima de píxeles en la imagen
        // pixelData + y * width * 3 apunta al inicio de la línea y-ésima en el buffer (sin padding)
        // width * 3 son los bytes a copiar (3 por píxel)
        memcpy(outputImage.scanLine(y), pixelData + y * width * 3, width * 3);
    }

    // Guardar la imagen en disco como archivo BMP
    if (!outputImage.save(archivoSalida, "BMP")) {
        // Si hubo un error al guardar, mostrar mensaje de error
        cout << "Error: No se pudo guardar la imagen BMP modificada.";
        return false; // Indica que la operación falló
    } else {
        // Si la imagen fue guardada correctamente, mostrar mensaje de éxito
        cout << "Imagen BMP modificada guardada como " << archivoSalida.toStdString() << endl;
        return true; // Indica éxito
    }

}

unsigned int* loadSeedMasking(const char* nombreArchivo, int &seed, int &n_pixels){
/*
 * @brief Carga la semilla y los resultados del enmascaramiento desde un archivo de texto.
 *
 * Esta función abre un archivo de texto que contiene una semilla en la primera línea y,
 * a continuación, una lista de valores RGB resultantes del proceso de enmascaramiento.
 * Primero cuenta cuántos tripletes de píxeles hay, luego reserva memoria dinámica
 * y finalmente carga los valores en un arreglo de enteros.
 *
 * @param nombreArchivo Ruta del archivo de texto que contiene la semilla y los valores RGB.
 * @param seed Variable de referencia donde se almacenará el valor entero de la semilla.
 * @param n_pixels Variable de referencia donde se almacenará la cantidad de píxeles leídos
 *                 (equivalente al número de líneas después de la semilla).
 *
 * @return Puntero a un arreglo dinámico de enteros que contiene los valores RGB
 *         en orden secuencial (R, G, B, R, G, B, ...). Devuelve nullptr si ocurre un error al abrir el archivo.
 *
 * @note Es responsabilidad del usuario liberar la memoria reservada con delete[].
 */

    // Abrir el archivo que contiene la semilla y los valores RGB
    ifstream archivo(nombreArchivo);
    if (!archivo.is_open()) {
        // Verificar si el archivo pudo abrirse correctamente
        cout << "No se pudo abrir el archivo." << endl;
        return nullptr;
    }

    // Leer la semilla desde la primera línea del archivo
    archivo >> seed;

    int r, g, b;

    // Contar cuántos grupos de valores RGB hay en el archivo
    // Se asume que cada línea después de la semilla tiene tres valores (r, g, b)
    while (archivo >> r >> g >> b) {
        n_pixels++;  // Contamos la cantidad de píxeles
    }

    // Cerrar el archivo para volver a abrirlo desde el inicio
    archivo.close();
    archivo.open(nombreArchivo);

    // Verificar que se pudo reabrir el archivo correctamente
    if (!archivo.is_open()) {
        cout << "Error al reabrir el archivo." << endl;
        return nullptr;
    }

    // Reservar memoria dinámica para guardar todos los valores RGB
    // Cada píxel tiene 3 componentes: R, G y B
    unsigned int* RGB = new unsigned int[n_pixels * 3];

    // Leer nuevamente la semilla desde el archivo (se descarta su valor porque ya se cargó antes)
    archivo >> seed;

    // Leer y almacenar los valores RGB uno por uno en el arreglo dinámico
    for (int i = 0; i < n_pixels * 3; i += 3) {
        archivo >> r >> g >> b;
        RGB[i] = r;
        RGB[i + 1] = g;
        RGB[i + 2] = b;
    }

    // Cerrar el archivo después de terminar la lectura
    archivo.close();

    // Mostrar información de control en consola
    cout << "Semilla: " << seed << endl;
    cout << "Cantidad de píxeles leidos: " << n_pixels << endl;

    // Retornar el puntero al arreglo con los datos RGB
    return RGB;
}



