# Suelo y techo texturizados

Auditoría vigente: [Paso 205 — funcionalidades DOS pendientes](step-205-original-functionality-audit.md).
Incluye estado real, nuevas evidencias y límites de validación; no implementa funciones.

## Recursos y correspondencia confirmada

Investigación estática del mismo EXTREMIS.EXE documentado en
[wall-texture-mapping.md](wall-texture-mapping.md), GAME.LAB y DATA originales.
Los offsets indicados son absolutos en el archivo EXE. No se ejecutó ni modificó
el juego original durante esta auditoría.

Los planos **no usan registros DALLE**. Se cargan desde `DATA/SOL<n>.DAS`, con la
paleta externa `PAL<n>.PAL` seleccionada por el nivel. El número SOL es el mismo
campo que el banco DALLE del encabezado de GAME.LAB; no se deduce de un tipo de
pared, un sector o `Polygon.extra`.

Evidencias:

- `0x11346..0x1134a` conserva el banco DALLE del encabezado en `[0x518e]`.
- `0x7a08..0x7a37` utiliza ese mismo campo para completar el nombre `Sol<n>.das`.
  La plantilla está en el offset de archivo `0x4be9`.
- `0x7c72..0x7c9d` carga el archivo en `[0x4ae4]`.
- `0x71ed` llama a `0x12ac4` antes del dibujo de paredes (`0x7272 -> 0x12658`).
- `0x12c1f..0x12c24` pasa el inicio SOL al muestreador para el plano inferior.
  `0x12cc4..0x12cce` pasa **SOL + 128** para el superior.
- `0x12c95..0x12cb5` dibuja el inferior usando la altura de ojo y filas 65–149.
  `0x12d3f..0x12d63` dibuja el superior usando altura de ojo menos 250 y filas 0–35.
  Las filas 36–65 se rellenan con bandas de color desde otro recurso.

La ruta utiliza un par **global por banco/nivel**, repetido en el mundo. No hay
selección de otra textura por sector o polígono en esa rutina: la consulta de
sector al inicio se usa para sumar su origen a la posición del jugador. Por eso
usar el mismo material de suelo en todo el nivel 1 es la correspondencia demostrada,
no una asignación provisional elegida por apariencia.

## Estructura SOL1

`SOL1.DAS`: 32768 bytes, SHA-256
`e562c1a3df03809b001971addf55afb7e6ee0a3a57cf526e352de03ead231e9e`.
Sin encabezado ni compresión en la ruta observada. Dos imágenes indexadas de
128×128 intercaladas por filas dentro de un bloque de 256×128:

```text
suelo(x,y) = bytes[y * 256 + x]
techo(x,y) = bytes[y * 256 + 128 + x]
0 <= x,y < 128
```

El muestreador usa BL para la coordenada horizontal, BH para la vertical y
`0x7f7f` como máscara (`0x15fca..0x16026`). Su dirección efectiva es
`base + (v & 127) * 256 + (u & 127)`. Sumado al desplazamiento de 128 del techo,
confirma ambas dimensiones y la disposición intercalada. Interpretar el archivo
como dos bloques consecutivos de 16384 bytes sería incorrecto.

Con PAL1, el suelo es un panel metálico con rejillas; el techo contiene luminarias
verdes. Se reutiliza exactamente la conversión de paleta que ya usa DALLE.
El índice 0 sigue siendo un color opaco; no se inventa transparencia.
Los diez archivos SOL tienen 32768 bytes, pero esta implementación acepta solo
el banco SOL1 investigado. Otros bancos producen un aviso y fondo plano, sin
asignarles materiales no validados.

## Escala, orientación y proyección

`0x12adf..0x12b50` suma posición local y origen del sector, reduce X e Y módulo
400 y construye un cuadrado de 400 unidades alineado con los ejes del mundo.
`0x12b57..0x12c0e` rota sus vértices respecto a la cámara. El sampler de líneas
`0x15bfb` proyecta ese cuadrado horizontal:

- `0x15c82..0x15cbe` obtiene profundidad mediante altura × 200 / (fila − 50).
- `0x15e1e..0x15f77` calcula inicio e incrementos de coordenadas por fila.
- `0x15ff7..0x16038` avanza a lo largo de la fila mediante punto fijo y máscaras,
  copiando índices de paleta sin prueba de transparencia ni iluminación por distancia.

Es una proyección de plano con interpolación horizontal en espacio de textura.
No es un estiramiento de una imagen sobre la mitad de la pantalla ni una textura
fijada al movimiento de la cámara.

El port conserva FOV=70°, ojo=128 y techo=256. Calcula la profundidad desde los
centros de píxel, con el mismo focal y el mismo vector derecha `(sin(a), -cos(a))`
que usa el renderizador existente:

```text
offset = y + 0.5 - alto / 2
profundidad = altura_hasta_plano * focal / abs(offset)
lateral = (x + 0.5 - ancho / 2) / focal
mundo = cámara + profundidad * (frente + derecha * lateral)
```

Cada fila necesita una división y una base/incremento constantes; la textura se
muestrea por vecino más cercano. U aumenta con X y V con Y. Se normaliza el módulo
400 también para posiciones negativas y se redondea únicamente el error numérico
junto a la unión de repetición para evitar costuras de un texel por cancelación.
Una fila exactamente en el horizonte conserva el fondo plano; no se divide por cero.
Las filas contiguas y las ventanas de altura par/impar se prueban por separado.

Los planos se dibujan en el framebuffer antes de las columnas de pared/puerta:
estas conservan sus impactos y sobrescriben los planos en su región proyectada.
No hay nuevo cambio de geometría, materiales verticales, portales o cámara.
No se sombrea adicionalmente el material SOL: la ruta original copia sus índices.

**Adaptaciones y límites de fidelidad:** el original usa un viewport 320×150,
horizonte en 50, alturas de 250, un desplazamiento de profundidad de 50 y aritmética
entera/punto fijo. El port mantiene su proyección ya validada y un horizonte centrado.
La escala del port normaliza los 128 texeles a 400 unidades; el original utiliza
coeficientes de borde `127 << 16` en varias ramas del sampler. La fase y redondeo
exactos por texel, así como las bandas centrales originales, requieren comparación
con capturas del juego. No se afirma reproducción píxel a píxel del rasterizador.

## F y estado inicial

`0x6e90..0x6eac` consulta el scancode `0x21` (F), consume su pulsación y conmuta
`[0x542d]`. `0x12ac5..0x12acd` comprueba ese mismo valor para omitir o ejecutar
ambos planos conjuntamente. La rama de fondo de paredes en `0x12769..0x127be`
también depende del indicador y usa colores procedentes de BANK.DSS.

El estado **se guarda en la configuración**: `0x6895..0x689a` carga el byte 12 de
EXTREMIS.CFG y `0x1dfb4..0x1dfba` lo escribe. El CFG original suministrado tiene
13 bytes y termina en cero: inicia sin texturas de planos. `PlaneAppearance` lee
ese byte si el archivo tiene el tamaño demostrado. Si falta o no es válido, usa
texturas activadas por defecto como decisión provisional. No escribe el CFG.

En el port, F física (`SDL_SCANCODE_F`) alterna una sola vez por KEY_DOWN sin
repetición y solo para la ventana correspondiente. Actúa también en 2D, conservando
la elección para cuando se vuelve al 3D. E y los demás controles siguen separados.

Al desactivar F se recuperan los colores actuales: techo `(20,23,32)` y suelo
`(45,39,34)`. **Estos colores planos son el fallback del port**, no una reconstrucción
de los gradientes/bandas de BANK.DSS del original, cuyo encaje con la proyección
actual no se ha reproducido. Al activar vuelve el mismo par de texturas sin recarga.
Si SOL/PAL falta o es inválido, se informa y se mantiene ese fallback; las paredes
pueden seguir texturizadas. También funciona SOL cuando DALLE no está disponible.

Se corrigió una fila que podía conservar píxeles antiguos al alternar F en la ruta
plana SDL con alturas impares: sus rectángulos se separan ahora en filas enteras,
igual que el framebuffer CPU. No cambia ninguna coordenada de pared proyectada.

## Organización y particularidades futuras

- `texture_bank.h/.cpp`: decodifica SOL1 reutilizando lectura exacta y paleta de DALLE.
- `plane_material.h/.cpp`: selección del par y coordenadas periódicas del material.
- `plane_renderer.h/.cpp`: intersección perspectiva y rasterizado de filas, sin SDL.
- `plane_appearance.h/.cpp`: recursos disponibles, estado inicial y tecla F; sin proyección.
- `first_person_view`: llama al rasterizado antes de sus columnas ya existentes.
  `main.cpp` solo carga el estado, entrega eventos y pasa el material activo.

No se encontró un contador de animación, máscara de transparencia o propiedad por
polígono en SOL1 ni en esta ruta de selección. No se implementan animaciones de
planos. No se descartan futuras modificaciones de paleta o recursos desde otras
rutinas; deben trazarse antes de asociarles ANM, SPR, propiedades o materiales nuevos.

## Verificación

Pasan **nueve suites CTest** y las **siete pruebas Python** anteriores. La suite nueva
cubre todos los texeles de un SOL sintético intercalado, selección suelo/techo,
paleta, ejes y coordenadas conocidas, periodo y negativos, rotación de cámara,
horizonte exacto/cercano, F (repetición, liberación y ventana), CFG apagado y fallback
activado, restauración de texturas, dimensiones inválidas y recursos truncados.
El raster comprueba colores de píxeles concretos para detectar rotaciones/inversiones,
tres tamaños/aspectos de salida y que los planos no sobrescriben una pared delante.
Se mantienen las suites de E, movimiento, colisiones, puertas y texturas de pared.

Prueba adicional con GAME.LAB, SOL1 y PAL1 originales: 144 fotogramas con puertas
animadas, ocho ángulos y tres tamaños; las vistas restauradas tras abrir/cerrar
coinciden píxel a píxel con su estado inicial. La ejecución local sin optimización
procesó el lote en aproximadamente 3,1 s (mezcla de resoluciones, sin garantía de
FPS interactivos). Captura ignorada: `build/texture-debug/planes-level1.png`.
No se copia ningún recurso binario original al repositorio.

Comprobación manual pendiente: pulsar TAB y F (el CFG suministrado inicia apagado),
recorrer el nivel, mirar rejillas/luminarias al girar, cruzar puertas con E,
comparar plano/texturado, redimensionar la ventana y evaluar el rendimiento.
La correspondencia SOL1 y los desplazamientos de cada plano están confirmados por
el ejecutable; los redondeos y ajustes de proyección indicados arriba son adaptados.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p test_extract_dalle.py
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x12ac4 0x12d9c
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x15bfb 0x1607e
./build/InExtremisPort "$HOME/Desktop/In_Extremis/GAME.LAB"
```
