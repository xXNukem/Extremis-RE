# Comparación de enemigos por registro original

**2026-11-24.** El informe de Windows describe ausencias parciales. En macOS se han seguido las 30 tablas desde el archivo hasta la actualización y la capa de sprites. La desaparición del **nivel 27, registro 8**, se reproduce en macOS y tiene una explicación en la colocación original. Falta una captura de Windows con nivel, registro, posición de cámara y momento para atribuirle ese caso o identificar otra divergencia.

## Diagnóstico de la carga en Windows

Desde la instalación afectada, `check_level_banks.exe --enemy-load "C:\Extremis\GAME.LAB"` llama a `game_lab::loadLevel` y a `enemies::System::load` para cada nivel. La fila `level` indica los registros leídos, los conservados y los objetos instanciados. Cada fila `record` incluye nivel, ordinal original desde cero, las 14 palabras leídas, si se conservaron en `System::records()` y el número de instancias con ese ordinal al terminar la carga. Un enemigo en espera con salud distinta de `0xffff` debe tener una instancia. Los errores identifican el nivel y la etapa.

El ejecutable DOS escribe el bloque del nivel en el archivo de trabajo al salir. Por ello importan tanto el hash del `GAME.LAB` que abrió el port como el de `DATA/DATA.LAB` de esa instalación. Si para el mismo ordinal la plantilla conserva salud positiva y el archivo de trabajo contiene `0xffff`, la retirada de ese registro ya consta en disco. Falta el resultado del PC afectado para atribuir la incidencia a esta u otra causa. [Evidencia del EXE en `0x78b9–0x7919` y `0x7b83–0x7c40`](password-continuation.md#gamelab-during-and-between-sessions)

## Desde el archivo hasta la pantalla

**DATA-confirmed / Port-tested.** `GAME.LAB` SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`. El ID es la pareja **(nivel desde 1, índice desde 0)**, también después de retirar un sprite. Si B es el offset de nivel en el directorio y T es `B + u16le(B+0x0c)`, el registro i empieza en `T+2+28*i`. En las 30 zonas `T+2+28*count` coincide con el comienzo de la tabla de códigos. Una lectura independiente con `struct.unpack_from('<14H', ...)` coincidió con los 14 campos de cada uno de los 672 registros emitidos por el port en el tick 0.

| Niveles, en orden | Definidos en el LAB, interpretados e instanciados al cargar |
|---|---|
| 1–10 | 9, 12, 18, 9, 12, 24, 21, 13, 18, 34 |
| 11–20 | 17, 35, 34, 23, 24, 33, 19, 26, 17, 13 |
| 21–30 | 23, 45, 68, 12, 15, 20, 35, 35, 7, 1 |

Los 672 empiezan con salud positiva y estado 6 (espera). Hay 14 de modo 0, 320 de modo 1, 271 de modo 2, 17 de modo 3 y 50 de modo 4. Un registro de modo 2 puede seguir esperando porque su sector no está conectado. Aun así entra en la escena. `connected` registra la conexión actual con la cámara y `instantiated` la presencia en la lista de sprites. El valor de `state` identifica la acción del actor.

- **Lectura y carga.** `game_lab.cpp` lee enteros de anchura fija, en binario, byte a byte y en orden little-endian. El inicializador de 14 campos usa llaves con evaluación de izquierda a derecha en C++17. El lector comprueba sector y límites antes de añadir el registro. `System::load` copia la lista, valida el banco y los índices de imagen y calcula el origen del sector. Los errores de sector, banco o imagen abortan la carga. No se usan estructuras empaquetadas, alineación del anfitrión ni el signo de `char` para extraer esos campos.
- **Actualización.** `System::rebuildScene` omite la salud `0xffff` y conserva el mapa de índice de sprite a registro. `step` activa el modo 2 al conectar los sectores. Muerte, bomba y el tiempo de espera por polígono inválido de MONS5 pueden retirar un actor. Un generador puede reutilizar un registro retirado de modo 4. Las visitas posteriores a un nivel recuperan la salud y la posición guardadas en `session_travel.cpp`. Comparar una partida nueva con una revisita puede dar poblaciones diferentes.
- **Presentación.** `session_frame.cpp` pasa la escena a `FirstPersonRenderer::draw`. La capa MONS se dibuja después de OBJET y antes de efectos y proyectiles. El rectángulo proyectado puede quedar detrás de la cámara, fuera del encuadre o sin muestras opacas visibles a la profundidad del mundo. La opción de enemigos del menú Debug puede desactivar el sistema completo. En los dos builds probados, el mismo registro y estado inicial producen los mismos resultados individuales.
- **Portabilidad.** La proyección y el movimiento usan coma flotante. Las rutinas de movimiento convierten de nuevo a coordenadas locales de 16 bits. El signo de la fase de MONS5 se recupera explícitamente. Se probaron macOS arm64 AppleClang 21 Debug, Release y RelWithDebInfo con AddressSanitizer y UndefinedBehaviorSanitizer. Las tres configuraciones dieron CSV idénticos en las dos series. Los sanitizadores no emitieron diagnósticos en estos recorridos. Quedan sin medir MSVC, la presentación SDL de Windows y otros recorridos de juego.

## Resultados reproducibles

`enemy_record_probe` es un ejecutable de diagnóstico de CMake. Carga los actores y objetos del nivel original, usa las puertas iniciales y una cámara fija, y registra los ticks de simulación 0, 18 y 120. La serie predeterminada usa la posición inicial o la primera llegada de ascensor de la tabla. `--near-first` desplaza la cámara de prueba hasta 150 unidades en X respecto al registro 0 de cada nivel y la orienta hacia él. Son posiciones de comparación, incluidas algunas fuera del espacio caminable. La forma con cinco números fija nivel, sector, X e Y **locales** y ángulo (0–719). No hay disparos, bombas, viajes ni movimiento del jugador en estas series.

El render de prueba usa la geometría, puertas, OBJET, imágenes MONS, proyección y rasterizador CPU del juego a 320×150, en serie y sin fisheye. Mide la diferencia de color al omitir sólo ese actor de la capa MONS. Si no hay diferencia, se comprueban las escrituras de profundidad con el fondo real de geometría y OBJET y con profundidad infinita. Los motivos distinguen rechazo de proyección, rectángulo fuera de pantalla, ausencia de muestra opaca, oclusión por geometría/OBJET, solapamiento con otro enemigo o igualdad de color, y registro retirado. Es una medición de la capa de mundo previa a HUD, efectos y presentación SDL. Para la partida real se conserva la traza agregada `EXTREMIS_ENEMY_TRACE=1` en `extremis-diagnostics.log`, junto a los ajustes.

| Serie macOS | Instancias en ticks 0 / 18 / 120 | Registros con contribución visible en esos ticks | SHA-256 del CSV con saltos LF |
|---|---|---|---|
| Llegadas | 672 / 672 / 671 | 0 / 0 / 0 | `24b5ecef53d9f3f652cefb144aadc0175b7ad71dccb7d5c2df5f223f30763b7b` |
| Cerca del registro 0 | 672 / 672 / 671 | 26 / 28 / 28 | `d76039931d457069c59b5bc86759e28a05e3d5ab826141068ff60a7045ab4c93` |

El registro 0 del nivel 2, observado desde S4 con cámara local (1550,900) y ángulo 0, produce 10 199 píxeles en el tick 0, 42 837 en el 18 y 42 148 en el 120. Cada CSV completo incluye 2016 filas de datos. Al comparar dos equipos, cotejar primero los 14 campos, la pertenencia a la escena y el motivo por ID y tick. Una pequeña diferencia de redondeo en el número de píxeles se puede investigar por separado de una instancia retirada.

Se repitió el muestreo frente a **cada uno de los 672 registros**. La primera posición en X produjo 517 contribuciones individuales en el tick 0, 514 en el 18 y 505 en el 120. Para los registros ocultos al cargar se ensayaron otros puntos y orientaciones. Se identificaron vistas con contribución individual para **670 registros**. Los otros dos, nivel 21 registros 1 y 2 (LAB `0x27b76` y `0x27b92`), comparten S8, posición (300,100), elevación e imagen inicial. Ambos están instanciados con modos distintos, 4 y 1. Desde S8 (600,100), ángulo 360, retirar cualquiera de los dos cambia cero píxeles porque el otro ocupa su lugar. Retirar ambos cambia 3674 píxeles. Estas mediciones explican por qué contar siluetas puede subestimar la población incluso en un fotograma correcto. [Cámaras y resultados esperados por registro](enemy-visibility.csv)

El comparador `tools/compare_enemy_views.py` verifica el hash de GAME.LAB y repite las 672 vistas iniciales. Informa por separado de diferencias de instanciación, motivo y píxeles. Acepta `--level 2` para reducirlo a un nivel.

En macOS se puede ejecutar desde la raíz del proyecto con el build configurado.

```sh
cmake --build build/debug --target enemy_record_probe
build/debug/enemy_record_probe /ruta/GAME.LAB > arrival.csv
build/debug/enemy_record_probe /ruta/GAME.LAB --near-first > near.csv
build/debug/enemy_record_probe /ruta/GAME.LAB 27 8 150 500 0
python3 tools/compare_enemy_views.py build/debug/enemy_record_probe /ruta/GAME.LAB
```

Equivalente para una compilación Visual Studio ya configurada en `build/package-windows`, desde **cmd.exe**, con SDL3 en el PATH o junto al ejecutable. Repetir con `Debug` en lugar de `Release` para contrastar configuraciones. No se han ejecutado estos comandos en Windows.

```bat
cmake --build build/package-windows --config Release --target enemy_record_probe
build\package-windows\Release\enemy_record_probe.exe C:\Extremis\GAME.LAB > arrival.csv
build\package-windows\Release\enemy_record_probe.exe C:\Extremis\GAME.LAB --near-first > near.csv
build\package-windows\Release\enemy_record_probe.exe C:\Extremis\GAME.LAB 27 8 150 500 0
py tools\compare_enemy_views.py build\package-windows\Release\enemy_record_probe.exe C:\Extremis\GAME.LAB
```

Se puede verificar el hash independiente de CRLF/LF con `python -c "import hashlib,pathlib; print(hashlib.sha256(pathlib.Path('near.csv').read_text(encoding='ascii').encode('ascii')).hexdigest())"`. Para un caso de juego hacen falta el nivel y la cámara del encuentro, la historia de activación y puertas, el hash del LAB y la configuración que presenta el fallo.

## La retirada del registro 8 del nivel 27

**DATA-confirmed.** En el LAB, registro `0x37ae2`, MONS5, salud 20, modo 1, S8, posición y hogar locales (300,500), mundo (1100,900). El polígono inicial es `0xffff`. Los tres polígonos del sector en `0x3880a`, `0x3881e` y `0x38838` cubren los rectángulos locales [100,200]×[0,200], [200,800]×[0,400] y [400,600]×[400,500]. El punto cae fuera de los tres.

**EXE-confirmed.** En el EXE de referencia, `0xe9f5–0xea19` reintenta localizar un MONS5 con polígono `0xffff` usando el sector propio y sus coordenadas locales (`0xf4da–0xf610`). En `0xf1bd–0xf1df`, con polígono inválido, incrementa el byte alto del campo `+0x16` y escribe salud `0xffff` al llegar a 100. Si el polígono es válido, borra ese byte. En el recorrido con el jugador en otro sector este modo 1 espera en su hogar. El port vuelve a localizar en cada paso y comprueba el contador antes del resto de `advance5`. Para este registro, inicialmente a cero, ambas condiciones predicen la misma retirada. Falta observación directa en DOS.

**Port-tested.** Desde la cámara de prueba `27 8 150 500 0`, el registro aporta 5791 píxeles tanto al cargar como en el tick 18. Un muestreo adicional con esa misma cámara da 5791 píxeles, salud 20 y byte alto 99 en el tick 99. En el tick 100 conserva su ID con salud `0xffff` y ya no tiene sprite. Una prueba sintética en `enemy_tests` comprueba que sigue instanciado en el tick 99 y se retira en el 100. No se ha establecido relación entre este registro y las ausencias comunicadas en Windows.

El ensayo opcional `enemy_tests <directorio-fixtures> GAME.LAB` sigue fallando en macOS Debug y Release en **nivel 3, registro 13**, en la aserción conjunta de movimiento, elevación, ataque y daño de MONS5. Es una prueba distinta. Ese registro permanece instanciado en ambas series de comparación.
