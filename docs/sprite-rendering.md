# Sprites ambientales del nivel 1

## Resultado y límites

OBJET1 contiene **44 imágenes**. Los **27 registros A del nivel 1** seleccionan
imágenes válidas de ese banco y se representan como **27 instancias**, con
movimiento vertical únicamente para las cuatro gotas de imagen 14.
Esto no implica que las 27 sean objetos interactivos ni que su función esté
clasificada. Las familias 0x4000 y 0x5000 comparten el acceso al banco OBJET:
no se ha convertido ninguna familia en enemigos o decoración por su prefijo.

No se añaden IA, daño, recogida, interacciones, colisiones de objetos, sonidos
ni animaciones adicionales de objetos. Las gotas parten de las alturas del
mapa y repiten su caída. El estado y las animaciones de paredes y puertas no cambian.
No se han abordado rendimiento, SDL_GPU ni grosor de puertas.

## Formato nativo SPR y paleta

Se ha leído el extractor externo `~/Desktop/In_Extremis/extract_spr_inteligente.py`
como referencia, sin ejecutarlo ni escribir en su destino externo. El lector
nativo consume SPR directamente, sin PNGs:

| Posición | Campo |
| --- | --- |
| 0 | Word de cabecera conservado sin interpretación funcional |
| 2 | Número de imágenes, uint16 LE |
| 4 + 4*i | Offset uint32 LE relativo a archivo + 4 |
| Inicio del registro + 0,+2 | Ancho y alto uint16 |
| Inicio + 4,+6 | Anclajes X e Y uint16 |
| Inicio + 8 | `ancho * alto` índices de paleta por filas |

OBJET1.SPR ocupa **58707 bytes**. Se validan la tabla completa, el comienzo del
primer registro, los límites, el orden, las dimensiones y el final exacto de
cada imagen. Se rechazan registros solapados, truncados y fuera del archivo.
La lectura de archivos tiene un límite defensivo de 64 MiB, no un límite del
formato original. Las imágenes y sus anclajes se conservan en memoria.

Se utiliza `level.objetBank` para OBJET y `level.palBank` para PAL. La expansión
de componentes VGA de 6 bits a RGB reutiliza `wall_textures::loadPalette`, con
el mismo redondeo que paredes y planos. `spr_format.h/.cpp` es el único lector
del contenedor compartido por OBJET y ADALLE; sus políticas de transparencia
siguen separadas: ADALLE se copia opaco, OBJET omite únicamente el índice 0.

**No se decide la transparencia por el color RGB.** Un índice distinto de 0
que dé negro o rojo sigue siendo opaco. En PAL1 el índice 0 es negro, pero su
tratamiento transparente procede del código original, no de ese color.

### Los rectángulos rojos

El catálogo confirma rectángulos rojos en las imágenes crudas 15 y 16. El
ejecutable los cubre copiando otras imágenes del mismo OBJET. No son una clave
de transparencia. Se reproduce una vez la composición original en el paso
global 0, manteniéndola fija por ahora:

| Destino OBJET desde 0 | Origen OBJET desde 0 | X,Y |
| --- | --- | --- |
| 15 | 22, 23 | 3,27 y 3,51 |
| 16 | 30 | 2,21 |
| 17 | 33, 38, 40 | 15,65; 15,88; 15,104 |

Las copias incluyen todos los índices, también 0; la transparencia se aplica
después, al dibujar el sprite compuesto. No se ha eliminado rojo ni alterado
el archivo. Las imágenes adicionales 22..43 se cargan como partes gráficas,
sin crear instancias nuevas para ellas. La selección temporal futura de estas
partes queda pendiente. Otros rojos visibles se conservan.

## Evidencia del ejecutable

Direcciones indicadas como **offsets absolutos de archivo**, no direcciones
virtuales, en el EXE SHA-256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.

| Rango | Conclusión respaldada |
| --- | --- |
| `0x7ca4..0x7cff` | Carga OBJET en `[0x4acc]`, lee el número de imágenes y copia los cuatro words de cada cabecera a la tabla de descriptores. |
| `0x7d00..0x7daf` | Para el nivel 1 selecciona la tabla de alturas en dirección de datos 0x5b23, archivo **0x5d23**. |
| `0x11f70..0x11fec` | Selección por byte bajo del tipo; descriptor de 8 bytes; altura proyectada `alturaTabla * 200 / profundidad`; ancho proporcional a ancho/alto del SPR; escala de ambos anclajes. |
| `0x11ff2..0x12007` | Suma a la Y proyectada de referencia el alto de pared multiplicado por `(256 - Param1) / 256`: sitúa el anclaje vertical según Param1. |
| `0x1200e..0x12086` | El bit 0x0200 invierte el anclaje horizontal y 0x0100 el vertical antes de construir el rectángulo. |
| `0x128fb..0x1298b` | La rama con `(type & 0xc000) == 0x4000` incluye las familias 0x4000 y 0x5000; prepara rectángulo, índice bajo + 1 y bits de inversión para el dibujador SPR. |
| `0x2055..0x20fb` | Resolución de imagen desde 1 en la API de dibujo y muestreo escalado; incrementos negativos cuando se invierten los ejes. |
| `0x2273..0x22a9` | EAX a cero; lee un índice en AL; omite la escritura si es cero. Repetido en el bucle desplegado. Prueba directa de transparencia por índice. |
| `0x14d4c..0x14e10` | Composición de imágenes 15,16,17 mediante subimágenes OBJET, dependiendo del contador global. |
| `0x1526f..0x152bf` | Copia rectangular OBJET → OBJET por filas, índices opacos, API de imágenes desde 1. |
| `0x11d0f..0x11d66` | Para el nivel 1, imagen 14: resta 20 a Param1 y suma 256 si el resultado es negativo. Se reproduce el descenso y la repetición de las gotas. |

La proyección genera un rectángulo en pantalla para cada posición transformada,
sin consultar una dirección fija del objeto. Esto respalda el billboard paralelo
al plano de cámara. Param2 se conserva, pero no se usa como ángulo supuesto.
La ordenación/recorte del motor original en `0x12112` y siguientes no se porta
literalmente: se utiliza la profundidad del renderizador actual.

## Coordenadas, alturas y anclajes

Horizontalmente: `x = sector.gx + vertex.x`, `y = sector.gy + vertex.y`, sin
truncar sumas a 16 bits. Verticalmente: `z = Param1`, con suelo 0 y techo 256.
El punto del mapa corresponde al **anclaje**, no al centro ni necesariamente
a la base de la imagen.

Alturas físicas del nivel 1, en orden de imagen 0..21, extraídas de 0x5d23:

```text
100 80 50 50 255 255 150 80 60 165 255 255 50 35 10 255 255 255 150 255 150 150
```

| Referencia visual | Dimensiones | Anclaje X,Y | Altura física |
| --- | --- | --- | --- |
| 02, lámpara | 54×37 | 27,1 | 50 |
| 03, lámpara verde | 54×37 | 27,1 | 50 |
| 07, lámpara de techo | 148×39 | 73,15 | 80 |
| 08, cadáver | 56×31 | 26,21 | 60 |
| 12, cables | 95×29 | 47,23 | 50 |
| 13, mancha | 62×12 | 30,4 | 35 |
| 14, gota | 7×13 | 3,0 | 10 |

En el sector **15 del usuario / índice 14 del código**, todos los elementos
de referencia incluyen cables 0x410c en `(1200,1600)` a z=256 con inversión
vertical y mancha 0x400d en ese mismo punto a z=0. Las gotas 0x400e están en
`(1200,1600)` a z=128,256 y `(1220,1620)` a z=64,192. Aplicar el
anclaje, el bit de inversión y la altura permite reproducir esta disposición.
No se recortan arbitrariamente las filas de un cadáver o mancha que se extienden
por debajo de su anclaje en el suelo, ni las de una lámpara sobre el del techo:
el original proyecta el rectángulo completo, lo cual forma parte de su aspecto.

## Proyección y oclusión del port

`sprite_renderer.h/.cpp` recibe la cámara y la distancia focal ya utilizada por
paredes y planos. No cambia FOV, altura del ojo (128), cámara ni perspectiva.
La profundidad es el producto escalar con la dirección frontal de cámara;
la posición horizontal usa su vector derecho, conservando la convención de Y
actual. La escala por píxel de imagen es:

```text
scale = alturaFisica * focal / (profundidad * altoSPR)
left = centroPantallaX + desplazamientoLateral * focal / profundidad - anchorX * scale
top  = centroPantallaY - (z - alturaOjo) * focal / profundidad - anchorY * scale
```

En un eje invertido, el anclaje pasa a `dimension - anclaje` y se invierte el
muestreo de ese eje. El recorte a ventana conserva las coordenadas del rectángulo
original, sin estirar lo que queda visible. Se usa vecino más próximo. Se omiten
posiciones detrás de cámara, una profundidad menor de 0,01 y datos no válidos.

Cuando hay sprites, el framebuffer incorpora profundidad por píxel. El bucle
de columnas existente aporta la profundidad de paredes y caras móviles de
puertas; no se vuelven a lanzar rayos ni se duplica la geometría. Al ser un
nivel con suelo/techo cerrados y obstáculos de altura completa, el impacto más
cercano bloquea toda la columna para sprites, incluidos sus márgenes respecto
al anclaje en suelo o techo. Así no asoman por encima o debajo de paredes.
Un sprite escribe color y profundidad solo en píxeles
opacos más cercanos. Así los huecos transparentes no ocultan otros objetos y
el resultado no depende del orden de instancias a distinta profundidad.

Los portales siguen omitiéndose como superficies opacas. Puertas parcialmente
abiertas aportan exactamente sus caras actuales; la parte libre permite ver
sprites. La visibilidad conserva las limitaciones del trazado actual: geometría
de altura fija y sin nuevos tipos de portales apilados. Los planos se dibujan
antes, y se conserva la proyección completa de los sprites sobre ese fondo.
No se añade iluminación artificial por distancia a los sprites: el blitter
examinado copia índices originales.

La perspectiva matemática usa la focal actual del port, no la constante 200
de la ventana original de 320 píxeles. Los tamaños relativos y los anclajes sí
proceden del ejecutable; la coincidencia visual exacta requiere comparación
con capturas originales. Para otros bancos, el lector SPR es genérico, pero la
tabla física no se presupone: solo OBJET1 tiene instancias habilitadas.

## Organización, pruebas y diagnóstico

Archivos creados: `include/spr_format.h`, `src/spr_format.cpp`,
`include/sprite_bank.h`, `src/sprite_bank.cpp`, `include/sprite_scene.h`,
`src/sprite_scene.cpp`, `include/sprite_renderer.h`, `src/sprite_renderer.cpp`,
`tests/sprite_tests.cpp`, `tools/inspect_sprites.py` y este documento.
Modificados: `CMakeLists.txt`, `include/texture_bank.h`, `src/texture_bank.cpp`,
`include/first_person_view.h`, `src/first_person_view.cpp` y `src/main.cpp`.

`spr_format` lee el contenedor; `sprite_bank` aplica la paleta y conserva índices;
`sprite_scene` selecciona imágenes, compone su estado inicial y coloca instancias;
`sprite_renderer` proyecta y dibuja sin SDL. La lógica del juego no se mueve al
renderizador. Main solo carga la escena y la entrega a la vista 3D.

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/release -j 4
ctest --test-dir build/release --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
./build/release/sprite_tests ~/Desktop/In_Extremis/GAME.LAB
./build/release/wall_animation_tests --original-data ~/Desktop/In_Extremis/DATA
python3 tools/inspect_sprites.py --data-dir ~/Desktop/In_Extremis/DATA --exe ~/Desktop/In_Extremis/EXTREMIS.EXE
```

La prueba sintética cubre lectura, dimensiones, anclajes, paleta, índice 0 frente
a negro/rojo opacos, datos inválidos, ambas familias, sumas globales mayores de
65535, altura, inversión de ejes, perspectiva, giro, resolución, detrás de cámara,
oclusión entre sprites, paredes y puertas cerradas/abiertas/parciales, y la cara
opuesta de la puerta. La prueba opcional con GAME.LAB verifica 44 imágenes,
27 instancias y las seis copias iniciales contra los píxeles de las fuentes.

Resultados: compilación habitual y Release correctas, **11/11 suites CTest y
7/7 pruebas Python superadas**; comprobaciones adicionales con GAME.LAB, OBJET1,
PAL1 y animaciones ADALLE originales superadas. La comprobación visual se ha
limitado a capturas de diagnóstico del port, pendiente de validación del usuario.

Artefactos locales, nunca recursos versionados:

- `build/texture-debug/sprites/objet1-anchors.png`: catálogo crudo, transparencia
  y cruces de anclaje; `manifest.json` registra dimensiones y hash de OBJET1.
- `initial-patches.png`: antes/después de componer los rectángulos demostrados.
- `scene-0.bmp`, `scene-1.bmp`, `scene-2.bmp`: capturas del renderizador con los
  recursos originales (la prueba opcional las guarda desde la raíz del proyecto).
  La tercera usa cámara `(1500,1900)` orientada a `(1200,1600)` y muestra cables,
  gotas y mancha. Se han inspeccionado capturas de diagnóstico del port;
  esto no equivale a comparar con el juego original ejecutándose.

Pendiente de validación interactiva: recorrer el nivel y comprobar tamaños,
anclajes, cruces de paredes y puertas, TAB, F, E y O, y diferentes tamaños de
ventana. Pendiente de investigación posterior: temporización exacta de gotas y parches
OBJET, funciones de objetos no clasificados, tablas físicas de otros bancos y
cualquier diferencia frente a capturas del original. No hay recursos originales
modificados, commits ni pushes en esta tarea.

## Corrección posterior: gotas inmóviles

La primera entrega dejaba todos los sprites estáticos. `sprite_animation.h/.cpp`
ahora actualiza exclusivamente la altura de la imagen 14: **20 unidades hacia
abajo por paso**, envolviendo con 256 al cruzar el suelo, como el código citado.
No modifica GAME.LAB ni anima el sprite de cables. Conserva las distintas alturas
iniciales y posiciones horizontales de las cuatro gotas.

La cadencia real del original no está calibrada: se utilizan provisionalmente
pasos de **100 ms**, independientes de los FPS, como la cadencia provisional de
las paredes. No es una afirmación de velocidad original. La actualización vive
fuera del renderizador y continúa en vista 2D; la dependencia exacta del original
respecto a la visibilidad de sectores sigue pendiente de investigar.

El ciclo vertical tiene 64 pasos. Se conservan tanto el estado inicial z=256
como el posterior paso por z=0 antes de envolver. Las pruebas cubren descenso,
envoltura, alturas relativas, ciclo, deltas largos/inválidos, independencia de
FPS y conservación de los cables. Main añade una llamada por fotograma; CMake
incluye el módulo y `sprite_tests.cpp` amplía las pruebas.

## Recogibles del nivel 1

La investigación de armarios no reclasifica Table A 0x4000/0x5000 como pickups.
Los cuatro suministros identificados son parámetros de paredes 0x3104/0x3205;
no eliminan sprites decorativos al transferirse. Véase
[items-and-inventory.md](items-and-inventory.md). La colisión de objetos por
extents/flags de las rutas DOS 0x10db7 y siguientes sigue pendiente de completar;
no se añaden obstáculos arbitrarios a las decoraciones existentes.
