# Mini File Sync

Mini File Sync es un programa desarrollado en C para Linux que detecta archivos nuevos dentro de un directorio de origen y los copia automáticamente hacia un directorio de respaldo.

El sistema revisa el directorio cada cinco segundos y utiliza hasta cuatro workers para distribuir las tareas de copia. También registra estadísticas como archivos copiados, bytes transferidos, errores, rendimiento en KB/s y archivos procesados por segundo.

## Requisitos

El proyecto necesita las herramientas de compilación de C:

```bash
sudo apt update
sudo apt install build-essential -y
```

## Compilación

Abrir una terminal en la carpeta donde se encuentra el `Makefile` y ejecutar:

```bash
make clean
make
```

Esto genera el ejecutable:

```text
minisync
```

## Preparar los directorios

Crear las carpetas de origen y respaldo:

```bash
mkdir -p origen backup
```

- `origen`: contiene los archivos que serán vigilados.
- `backup`: almacena las copias realizadas por MiniSync.

## Ejecución

Ejecutar MiniSync indicando el directorio de origen y el directorio de respaldo:

```bash
./minisync "$PWD/origen" "$PWD/backup"
```

El programa continuará trabajando en segundo plano y revisará el directorio de origen cada cinco segundos.

## Prueba rápida

Crear varios archivos dentro de `origen`:

```bash
for i in {1..5}; do
    echo "Contenido del archivo $i" > "origen/archivo$i.txt"
done
```

Después de aproximadamente cinco segundos, los archivos aparecerán dentro de `backup`.

Para comprobarlos:

```bash
find backup -type f
```

También se puede verificar su contenido:

```bash
cat backup/archivo1.txt
```

## Salida esperada

Cuando se detectan cambios, el monitor muestra la cantidad de archivos y los workers utilizados:

```text
[MONITOR] Detectados 5 archivos nuevos/modificados. Lanzando 4 workers.
[MONITOR] Asignando archivo1.txt al Worker 1
[MONITOR] Asignando archivo2.txt al Worker 2
[MONITOR] Asignando archivo3.txt al Worker 3
[MONITOR] Asignando archivo4.txt al Worker 4
[MONITOR] Asignando archivo5.txt al Worker 1
```

Cada worker informa las órdenes recibidas y los archivos procesados:

```text
[WORKER 1] Recibió orden de copiar: archivo1.txt
[2026-07-16 20:53:02] worker1: copiado /ruta/origen/archivo1.txt
```

También se muestran las estadísticas acumuladas:

```text
[2026-07-16 20:53:02] stats -> copiados: 200 | bytes: 5092 | errores: 0 | Rendimiento: 0.53 KB/s | 21.36 arch/s
```

El rendimiento expresado en KB/s puede ser bajo cuando los archivos contienen pocos bytes. En estos casos, el valor de archivos procesados por segundo representa mejor el funcionamiento del programa.

## Funcionamiento de los workers

MiniSync crea una cantidad de workers de acuerdo con el número de archivos detectados, utilizando como máximo cuatro.

- Un archivo detectado utiliza un worker.
- Dos archivos detectados utilizan dos workers.
- Tres archivos detectados utilizan tres workers.
- Cuatro o más archivos utilizan un máximo de cuatro workers.

Las tareas se distribuyen entre los workers. Cuando existen más archivos que workers, estos reciben nuevas órdenes después de terminar las anteriores.

Por ejemplo, con cinco archivos:

```text
archivo1.txt -> Worker 1
archivo2.txt -> Worker 2
archivo3.txt -> Worker 3
archivo4.txt -> Worker 4
archivo5.txt -> Worker 1
```

El uso de varios workers permite procesar distintos archivos de manera concurrente y reducir el tiempo total de sincronización cuando existe una gran cantidad de cambios.

## Detener el programa

Para detener todas las instancias de MiniSync:

```bash
pkill -x minisync
```

Para comprobar que terminó:

```bash
pgrep -af minisync
```

Si no aparece ningún proceso, MiniSync se detuvo correctamente.

## Limpiar la compilación

Para eliminar el ejecutable y los archivos objeto:

```bash
make clean
```

Para volver a compilar:

```bash
make
```

## Tecnologías utilizadas

El proyecto aplica conceptos de Sistemas Operativos como:

- Procesos creados mediante `fork()`.
- Workers concurrentes.
- Pipes para la comunicación entre procesos.
- Memoria compartida POSIX.
- Semáforos POSIX.
- Colas de mensajes POSIX.
- Recorrido recursivo de directorios.
- Estadísticas compartidas.
- Llamadas al sistema `open()`, `read()`, `write()` y `fdatasync()`.
- Medición de tiempo mediante `gettimeofday()`.


## Conclusión

Mini File Sync permite aplicar de forma práctica conceptos de concurrencia y comunicación entre procesos en Linux. El sistema distribuye las tareas entre un máximo de cuatro workers, mantiene estadísticas de funcionamiento y sincroniza automáticamente los archivos nuevos o modificados del directorio de origen hacia el directorio de respaldo.
