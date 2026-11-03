# Colisiones estáticas del jugador

Estado actualizado: las puertas móviles ya se incorporan mediante `DoorControl`;
la investigación y el algoritmo estáticos descritos aquí permanecen intactos.
Véase también [puertas e interacción](door-rendering.md#interacción-individual-y-colisiones-dinámicas).

## Alcance y fuentes

Auditoría estática del EXTREMIS.EXE original cuyo SHA-256 figura en
[wall-texture-mapping.md](wall-texture-mapping.md). Los offsets siguientes son
**offsets absolutos del archivo**, no direcciones virtuales. Se leyó el ejecutable
sin ejecutarlo ni modificarlo. No se afirma equivalencia visual/física completa.

## Confirmado en el ejecutable

### Posición y ruta del jugador

- `0x168a4..0x168e8` pasa la posición anterior (`[0x51da]`, `[0x51dc]`), la propuesta
  (`[0x51e6]`, `[0x51e8]`), orientación (`[0x51de]`) y sector (`[0x51ee]`) a
  `0x1074b`. Son palabras de 16 bits; representan coordenadas locales al sector,
  no una estructura circular declarada en el ejecutable.
- `0x168f0..0x1691e` guarda el resultado corregido, orientación y sector, y lo
  convierte en la posición actual. Esto conecta la rutina analizada con el
  jugador, además de otros llamadores que mueven entidades.
- `0x10750..0x10754` prepara el sector con `0x11335` (el mismo parser de geometría
  ya auditado). `0x10793` recorre `[0x51bc]`, la tabla de paredes; `0x107b4..0x107d7`
  obtiene ambos extremos mediante `[0x51b6]`, la tabla de vértices.

### Detección y respuesta frente a paredes

- `0x10759..0x1078c`: prepara límites de búsqueda de la posición propuesta con
  márgenes de 300/301 unidades. Son límites de selección de candidatos, **no**
  un radio de jugador demostrado. `0x107db..0x10823` descarta segmentos cuyos
  extremos quedan fuera por el mismo lado de esos límites.
- `0x10843..0x108aa` obtiene la orientación y extensión del segmento;
  `0x108ba..0x1090f` transforma posición anterior/propuesta al sistema del segmento
  mediante `0x1117a`. Esa rutina rota con las tablas trigonométricas existentes.
- `0x10849..0x10877` establece márgenes longitudinales de +80/-80 para segmentos
  alineados con los ejes y +30/-30 cuando ambas componentes son distintas de cero.
  `0x108b1` fija otro umbral en 80. No demuestra un disco uniforme de radio 30
  ni de radio 80; hay tolerancias distintas según orientación y operación.
- `0x10c1e..0x10c85` comprueba el cruce del umbral perpendicular e interpola la
  componente longitudinal. Si hay contacto dentro del tramo extendido,
  `0x10c8b..0x10cc2` fija la componente perpendicular en umbral+2, conserva
  la componente longitudinal propuesta, aplica la rotación inversa y escribe
  la nueva posición. Esto respalda **deslizamiento tangencial**, no solo cancelar
  todo el movimiento. `0x10d29..0x10d42` limita las correcciones repetidas a tres
  en esa ruta. No se copia literalmente este algoritmo entero al port.

### Sectores y conexiones

- `0x10916..0x10931` distingue las primeras paredes usando el número de vecinos
  y un argumento de modo. En el nivel 1 las conexiones están agrupadas al comienzo
  de la tabla de paredes. No se presupone que `extra` sea el vecino.
- `0x10988..0x10a64` consulta la tabla de estados por `wall.extra`, prueba banderas
  y progreso de puerta; el paso depende de estado y modo. Las colisiones dinámicas
  se excluyen expresamente de esta entrega.
- `0x10a74..0x10a88` selecciona el vecino mediante la tabla `[0x51b0]` y el índice
  de pared. `0x10d66..0x10db5` suma el origen del sector anterior a ambas posiciones,
  carga el nuevo sector, lo devuelve en el argumento correspondiente y resta su
  origen: hay transición explícita de sector con conservación de posición global.

### Polígonos internos

El parser en `0x11434..0x11442` conserva el bloque de polígonos en `[0x51c8]`.
La ruta `0x0f62e..0x0f64f` consulta sus offsets, salta los índices de vértices y
accede al campo adicional y las conexiones. `0x0f661..0x0f6ce` recorre esas conexiones,
consulta una tabla de visitados y utiliza sus coordenadas para un coste hacia otro
polígono. Es evidencia de una red consultada por rutinas de búsqueda; interpretar
esa ruta como navegación es una **hipótesis fundada**, no una semántica completa
confirmada de todos los campos.

No se ha encontrado en la ruta física del jugador auditada evidencia de que
cada arista de estos polígonos sea un muro. El bucle de contacto usa paredes.
Convertir todos los contornos internos en obstáculos introduciría barreras sin
respaldo. El port conserva sus datos y su visualización, sin añadir colisión a
esas aristas.

## Aproximación provisional del port

No está demostrado el volumen completo original, especialmente extremos y
esquinas. Se emplea un **círculo de radio configurable de 30 unidades**; es una
decisión provisional para esta fase y no se presenta como radio original.
Los valores observados de 80/30 no se interpretan como una única forma circular.
Habrá que contrastar el margen de contacto y la comodidad de paso con el original.

Configuración central en `collision::Settings` (`include/player_collision.h`),
accesible desde `player::Settings::collision`:

| Parámetro | Valor provisional | Propósito |
| --- | ---: | --- |
| `radius` | 30 unidades | Volumen circular del jugador |
| `skin` | 0.0001 unidades | Separación numérica al detenerse |
| `maxContacts` | 8 | Límite conservador de contactos por desplazamiento |

### Geometría y algoritmo

`collision::staticWorld` copia una vez los extremos globales de todas las paredes
cuyo nibble alto no es `0x1000`: 250 registros en este nivel, incluidos marcos y
superficies especiales. No cambia GAME.LAB, no usa texturas y no recibe puertas
animadas. Las 30 conexiones se excluyen siempre, incluso con puertas cerradas.
No inventa límites adicionales en sectores o polígonos: en un mapa abierto por
sus datos, no habrá una frontera invisible para impedir salir.

`collision::move` barre el círculo por **todo el vector solicitado**, sin depender
de pasos de un tamaño máximo por FPS. Equivale a mover su centro contra cada
segmento expandido por el radio: dos planos laterales limitados a la longitud del
segmento y dos círculos en los extremos. La entrada en cada extremo se obtiene
resolviendo la cuadrática de intersección, con una forma estable de la raíz.
Esto cubre diagonales, orientaciones arbitrarias, extremos y movimientos rápidos.
Los segmentos de longitud cero se tratan como un extremo circular.

Se elige el contacto de menor tiempo entre todos los segmentos, se avanza hasta
antes del impacto, y se elimina solo la componente entrante de la parte restante.
Ese vector tangencial **se barre otra vez** contra toda la geometría: no se aplica
un deslizamiento sin comprobar esquinas o paredes adicionales. Si se agota el
límite de contactos, se descarta el residuo sin examinar y se conserva la última
posición segura. El resultado es conservador y puede perder parte del movimiento
en encuentros muy complejos; no permite atravesar una pared por agotar iteraciones.

No se implementa empuje para recolocar automáticamente posiciones inicialmente
inválidas: si el cuerpo ya se solapa, se impide profundizar y se permite salir.
En el caso degenerado de centro exactamente sobre un segmento, se detiene el
movimiento. La cámara inicial original es válida y se mantiene sin cambios.
El algoritmo es bidireccional: ambas caras de una pared estática son sólidas.

### Integración

- `player_collision.h/.cpp`: geometría estática, barrido y deslizamiento, sin SDL.
- `player_control.h/.cpp`: calcula el movimiento normalizado existente y entrega
  el vector al resolvedor opcional; no contiene detección geométrica. Conserva
  velocidades, ratón, altura y orientación, incluso si una pared detiene la posición.
- `player_input.h/.cpp`: toma prestada la geometría estática y la entrega al control.
  No incorpora matemáticas de colisión ni cambia captura/foco/teclas.
- `main.cpp`: construye el mundo estático una vez y lo pasa al módulo de entrada.
  El lector, renderizadores, materiales, cámara inicial y lógica de puertas no cambian.

## Pruebas y comprobaciones pendientes

Compilación C++17 correcta. Pasan las **seis suites CTest** (incluidas todas las
anteriores) y las **siete pruebas Python**. La nueva suite no depende de SDL:
pared por ambas caras, contacto y salida, deslizamiento, esquina y orden de
segmentos, abertura demasiado estrecha, pared oblicua, extremos y tangencias,
segmentos degenerados, portal, polígonos internos, coordenadas globales, integración
WASD diagonal, giro junto a muro y movimientos hasta 10.000.000 unidades en un paso.
También compara un movimiento deslizante completo con 60 pasos temporales.

Auditoría adicional con GAME.LAB original: 250 segmentos estáticos, cruce central
verificado en las 30 conexiones dirigidas y 10.000 desplazamientos aleatorios
reproducibles de hasta ±1500 unidades por eje desde la cámara inicial. Tras cada
uno se comprobó la distancia a todos los muros: sin solapamientos del radio.
La comprobación queda en el directorio ignorado `build/check_original_collision.cpp`.

Pendiente: validación manual de la sensación del radio, esquinas y pasos del
nivel, contraste con una ejecución original, reconstrucción exacta de su volumen
y tolerancias, y colisiones de puertas móviles (actualmente atravesables a propósito).
La prueba de estrés verifica separación final; las pruebas sintéticas de barrido
son las que comprueban que no se salta al otro lado por desplazamientos rápidos.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p test_extract_dalle.py
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x168a4 0x16925
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x1074b 0x10db7
./build/InExtremisPort "$HOME/Desktop/In_Extremis/GAME.LAB"
```

## Integración dinámica de puertas

`staticWorld` sigue excluyendo conexiones `0x1000` y no se altera el mundo estático.
`DoorControl::prepareFrame` forma otra lista de movimiento con esos mismos segmentos
y las caras móviles actuales de `doors::visibleFace`. El barrido de círculo existente
resuelve ambos tipos juntos; no cambian radio, tolerancias ni deslizamiento estático.
Se elige la cara de aproximación y se retiene hasta que todo el cuerpo supera el
plano para evitar solapamientos al intercambiar las dos caras de retracción opuesta.
La retención es una aproximación de paso, no geometría nueva en GAME.LAB.

Se añadieron consultas de distancia al segmento y de rayo al mismo módulo, sin SDL.
La consulta de solapamiento reutiliza la proyección al punto más cercano; la del
rayo incluye casos colineales y permite impedir interacciones a través de muros.
Estas consultas no sustituyen ni modifican el algoritmo de movimiento estático.

El avance de cierre se rechaza e invierte antes de alcanzar el cuerpo. La lista
se actualiza antes del movimiento del jugador: tanto un cierre rápido como un
desplazamiento rápido se comprueban, conservando la última posición segura.
La activación individual, políticas de acceso, limitación de una puerta por pulsación
y condiciones originales están documentadas en `door-rendering.md`.

La limitación anterior de puertas atravesables queda resuelta. Permanecen pendientes
el volumen original exacto, los ajustes estáticos fuera de esta tarea, la comparación
visual de la retención de cara y las reglas originales completas de cierre y acceso.

## Revisión del mecanismo compartido

Se confirma que paredes y puertas ya pasan por el mismo `collision::move`, sobre
una lista de segmentos combinada. No se ha creado ni eliminado un segundo
resolvedor, porque no existía. Las distancias públicas `distanceToSegment` y
`signedDistanceToLine` centralizan las consultas; la segunda sustituye la fórmula
local de aproximación a puerta. La primera comparte la proyección al punto más
cercano utilizada por la detección de penetración. Rayo/oclusión, barrido, extremos
y deslizamiento permanecen en este módulo. No cambian parámetros ni respuesta física.

Las regresiones comparan los mismos movimientos, incluidos desplazamiento rápido
y deslizamiento, contra una superficie estática y contra la misma geometría
proporcionada por una puerta a 0/25/50 % de apertura. Los resultados coinciden.
Se conservan las pruebas de esquinas, extremos, cierre seguro y portal.
