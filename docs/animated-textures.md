# Texturas animadas de paredes

## Resultado y alcance

Se han identificado e implementado **3 materiales de DALLE1: 11, 12 y 13**,
con **4 secuencias** que utilizan las **20 imágenes de ADALLE.SPR**. La animación
consiste en copiar rectángulos opacos sobre DALLE en memoria. No son grupos de
texturas completas consecutivas de DALLE, ni un efecto de paleta en estas rutinas.
Los identificadores de GAME.LAB permanecen intactos.

Las secuencias, los destinos y su orden están demostrados por el ejecutable.
**La velocidad en segundos sigue siendo provisional: 100 ms por paso global.**
Esto no equivale a haber recuperado la velocidad visual original. Falta medirla
en el juego original. No se han añadido efectos a materiales candidatos sin pruebas.

## Evidencia reproducible

Las direcciones siguientes son **offsets absolutos del archivo EXTREMIS.EXE**,
desensamblado en modo x86 de 32 bits, no direcciones virtuales. Referencia SHA-256:
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Los hashes de DALLE1, PAL1 y ADALLE están en `dalle-format.md`.

| Código | Evidencia |
| --- | --- |
| `0x6986..0x69a7` | Guarda el bloque cargado en `[0x4aec]`, usado como origen ADALLE. La cadena de nombre en el ejecutable, dirección de datos `0x49f8` (archivo `0x4bf8`), es ADALLE.SPR. |
| `0x6aab` | Inicializa el contador global word `[0x5423]` a cero. |
| `0x71e8` | Llama a la rutina `0x14d1f` antes de dibujar el mundo. |
| `0x7387` | Incrementa `[0x5423]` una vez en el bucle principal. No hay un temporizador individual por pared para estas secuencias. |
| `0x15180..0x151cb` | Dos composiciones independientes sobre el material 12: `tick & 7` y `tick % 6`. |
| `0x151d0..0x151ed` | Material 13: `(tick & 0x60) >> 5`, cuatro imágenes que duran 32 pasos cada una. |
| `0x151f2..0x1524b` | Material 11: tabla de máscaras de 32 bytes, siete indicadores, inversión mediante el bit 5 del contador. |
| `0x1524f..0x1526e` | Tabla de máscaras de los indicadores, reproducida abajo. |
| `0x152c0..0x1530c` | Compositor ADALLE → DALLE: bloque destino `[0x4ac4]`, registro `AX * 4440`, desplazamiento `74 * CX + DX`; copia todos los índices mediante MOVSB, sin prueba de transparencia. |

El compositor consulta la tabla SPR con `BX * 4` y suma 4 al resultado. Esa API
usa índices **desde 1**; nuestros índices de imagen y los catálogos empiezan en
**0**. AX, por el contrario, ya es un índice DALLE desde 0. Esta diferencia evita
un desplazamiento de un fotograma al reproducir la secuencia.

Los bloques anteriores a `0x15180` también usan el contador, pero sus llamadas
a `0x1526f` y `0x1530d` escriben en el banco OBJET (`[0x4acc]`), no en DALLE.
Se han excluido por ser animaciones de objetos, fuera de esta tarea. Las cuatro
llamadas directas localizadas a `0x152c0` son las tres composiciones y el bucle
de indicadores descritos aquí; el bloque común no está condicionado al nivel.
Esto no demuestra que no existan otros mecanismos en todo el juego.

La rutina de paleta `0x1468e..0x146bb` escribe los 256 colores mediante los
puertos VGA 0x3c8/0x3c9 y aplica parámetros de transformación. Su existencia no
prueba un ciclo de paleta para estos paneles. No se sustituye ningún ciclo de
paleta identificado por fotogramas inventados: el mecanismo demostrado aquí
es la copia de índices, usando la paleta que ya emplea el material.

## Secuencias y asignación

Coordenadas de destino `(x,y)` desde la esquina superior izquierda de DALLE,
60 columnas × 74 filas. ADALLE contiene imágenes indexadas por filas; DALLE
original se almacena por columnas. La conversión al almacenamiento RGB por
filas del port conserva ambos ejes, sin rotar ni invertir las imágenes.

| Material | Aspecto | ADALLE desde 0 | Destino | Secuencia |
| --- | --- | --- | --- | --- |
| 11 | Siete barras verdes | 18 y 19, de 37×3 | `(12,14 + 7*i)`, i=0..6 | Máscara global, período 64 pasos |
| 12 | Monitor izquierdo | 4..11, de 24×17 | `(10,25)` | `4 + (tick & 7)`, período 8 |
| 12 | Monitor derecho | 12..17, de 15×17 | `(36,25)` | `12 + tick % 6`, período 6 |
| 13 | Pantalla naranja/roja | 0..3, de 38×31 | `(11,17)` | `(tick >> 5) & 3`, período 128 |

Tabla de máscaras, hexadecimal:

```text
00 01 02 04 08 10 20 40 20 10 08 04 02 01 00 01
03 07 0f 1f 3f 7f 3f 1f 0f 07 03 01 00 7f 00 7f
```

Con el bit 5 del contador a cero se leen los bits 0..6 en orden; con él a uno,
6..0. Un bit a uno selecciona ADALLE 19, a cero ADALLE 18. Incluye barrido,
acumulación y parpadeos concretos, sin aleatoriedad. Los cuatro canales repiten
conjuntamente a los 384 pasos, salvo la discontinuidad del módulo 6 al desbordar
el contador original de 16 bits. El port conserva también ese desbordamiento.

La correspondencia GAME.LAB sigue siendo `wall.type & 0xff`; no existe un nuevo
flag de animación añadido al mapa. En el nivel 1 se encuentran **20 segmentos**
(índices desde cero):

| Material | Sector: paredes |
| --- | --- |
| 11, 16 segmentos | 1: 9,12; 7: 5,13; 12: 5,6,7,8,25,26,27,28; 13: 6,9,16,19 |
| 12, 2 segmentos | 5: 28; 12: 3 |
| 13, 2 segmentos | 4: 14; 10: 11 |

No se han supuesto animaciones para las variantes parecidas o sombreadas de
otros índices DALLE. Tampoco se añaden secuencias condicionadas a eventos ni
animación de planos horizontales: no han quedado demostradas en este análisis.

## Implementación

- `texture_bank` carga y valida ADALLE una sola vez: 9972 bytes, 20 imágenes,
  tabla de offsets relativa a archivo+4, tamaños y límites de cada registro.
  Reutiliza exactamente la expansión de PAL existente. El negro sigue opaco.
- `wall_animation.h/.cpp` separa la selección de imágenes, la composición y el
  reloj global. No depende de SDL, geometría, puertas o renderizador.
- `TexturePreview` conserva la propiedad del banco de materiales y actualiza
  sus regiones animadas cuando cambia el paso global. Si falta ADALLE o su
  estructura no está validada, avisa y conserva las paredes estáticas.
- `main.cpp` llama a `update(elapsedSeconds)` una vez por fotograma, también en
  vista 2D. Los cambios de vista no reinician ni detienen el reloj. Tras un delta
  largo se compone directamente el estado actual, sin acumular retraso.
- El renderizador sigue leyendo los mismos índices y píxeles de los mismos
  materiales. No añade cálculos temporales ni búsquedas de animación por píxel.
  No hay recargas ni reservas de memoria durante la composición habitual.

Solo se reemplazan **2618 píxeles** entre los tres materiales por paso
(7×37×3 + 24×17 + 15×17 + 38×31).
No se ha realizado trabajo de optimización ni migración a GPU. Puertas,
colisiones, entrada, planos y visibilidad conservan sus implementaciones.

## Diagnóstico y validación

```sh
python3 tools/inspect_original_code.py ~/Desktop/In_Extremis/EXTREMIS.EXE 0x15180 0x1524f
python3 tools/inspect_original_code.py ~/Desktop/In_Extremis/EXTREMIS.EXE 0x152c0 0x1530d
python3 tools/inspect_animated_textures.py --data-dir ~/Desktop/In_Extremis/DATA --exe ~/Desktop/In_Extremis/EXTREMIS.EXE
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/release -j 4
ctest --test-dir build/release --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
./build/release/wall_animation_tests --original-data ~/Desktop/In_Extremis/DATA
```

El diagnóstico valida el hash del ejecutable, lee la tabla directamente del
EXE y genera `build/texture-debug/animated/composed-panels.png` y `audit.json`.
El catálogo permite contrastar las regiones con ADALLE y comprobar el barrido,
los dos monitores y la pantalla roja. No se incluyen imágenes ni recursos
originales en archivos versionados.

Pruebas C++: selección y duración en pasos para los 65536 valores del contador,
casos del barrido y parpadeo, ciclo conjunto, actualización a distinto FPS,
límites temporales y desbordamiento, composición por filas sin reflejos,
conservación de todos los píxeles estáticos y fallback sin ADALLE. La integración
se prueba con actualizaciones equivalentes sin dependencia del modo de vista;
la alternancia TAB real sigue requiriendo comprobación interactiva.

Resultado de esta tarea: compilaciones habitual y Release correctas; **10/10
suites CTest y 7/7 pruebas Python superadas**. La comprobación adicional de solo
lectura con los recursos originales valida que el material cambia al avanzar
un paso y vuelve al estado inicial a los 384 pasos. El programa se ha iniciado
con GAME.LAB original y registra la carga de 54 materiales, 20 imágenes ADALLE
y las 15 puertas. Esto verifica carga/arranque, no toda la interacción visual.

Archivos creados: `include/wall_animation.h`, `src/wall_animation.cpp`,
`tests/wall_animation_tests.cpp`, `tools/inspect_animated_textures.py` y este
documento. Modificados: `CMakeLists.txt`, `include/texture_bank.h`,
`src/texture_bank.cpp`, `include/texture_preview.h`, `src/texture_preview.cpp`,
`src/main.cpp`, `tools/extract_dalle.py` y `docs/dalle-format.md`.

Pendiente: comparar la velocidad con capturas temporizadas del original;
verificar in situ los 20 segmentos, la orientación sobre cada pared y TAB;
comprobar manualmente F, E, O y el recorrido habitual. La revisión del catálogo
confirma coherencia de la composición, no una comparación del juego original
ejecutándose. No se afirma una reproducción temporal exacta.
