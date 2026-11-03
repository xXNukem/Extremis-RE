# Marcos y puertas del nivel 1

## Geometría comprobada

Auditoría de GAME.LAB y DALLE1/PAL1 originales, sin modificar los recursos.
Las coordenadas son globales (`sector.gx/gy + vertex.x/y`); índices desde cero.

- Hay 30 segmentos `type & 0xf000 == 0x1000`: todos miden 200 unidades,
  son horizontales o verticales y forman 15 pares coincidentes con sentidos
  opuestos entre sectores. Cada par comparte `extra`; aparecen los valores
  0–14 una vez por abertura (dos registros por valor). No son índices del
  sector vecino: por ejemplo, sectores 1/2 comparten `extra=1`.
- Hay 60 segmentos `0x2000`: todos miden 100 unidades y tienen `extra=0`.
  Cada conexión tiene exactamente dos laterales en su sector, perpendiculares
  a ella y unidos a sus extremos. Los 60 marcos cumplen esta relación.
  La separación entre laterales es 200; su profundidad es 100. Es una
  identificación consistente en este nivel, no una regla inferida para otros.
- Ejemplo: sector 0, conexión `(3400,500) -> (3600,500)`, tipo `0x1000`.
  Laterales: `(3600,500) -> (3600,400)` y `(3400,400) -> (3400,500)`,
  ambos `0x2003`. El sector 1 tiene la conexión inversa y laterales hacia Y=600.

Esto confirma dos superficies laterales y una abertura central en planta.
No demuestra una malla de marco frontal completo ni un dintel adicional.

## Materiales y negro original

Se conserva la selección comprobada `type & 0xff` y la orientación 60×74.

| Tipo de lateral | Registros | Índice DALLE | Columnas 0–29 | Columnas 30–59 |
| --- | ---: | ---: | --- | --- |
| `0x2003` | 4 | 3 | Gráfico, ningún índice 0 | 2220 índices 0 |
| `0x2016` | 14 | 22 | Gráfico, ningún índice 0 | 2220 índices 0 |
| `0x2020` | 14 | 32 | Gráfico, ningún índice 0 | 2220 índices 0 |
| `0x202a` | 14 | 42 | Gráfico, ningún índice 0 | 2220 índices 0 |
| `0x2034` | 14 | 52 | Gráfico, ningún índice 0 | 2220 índices 0 |

PAL1[0] es RGB `(0,0,0)`. Las regiones negras existen en los archivos originales:
son la mitad derecha de cada imagen, no un fallo del búfer o de orientación.
Las imágenes son superficies laterales estrechas dentro de un registro del
mismo tamaño que una pared normal. Su apariencia por sí sola no prueba su uso;
el código original sí distingue su intervalo horizontal.

## Evidencia del ejecutable y causa

Offsets absolutos del mismo EXTREMIS.EXE documentado (incluido su hash) en
[la auditoría de materiales](wall-texture-mapping.md):

- `0x11927..0x11950`: inicializa el inicio horizontal a 0 y el recorrido a 60;
  si `(wall.type & 0x3000) == 0x2000`, lo sustituye por 30.
- `0x11c09..0x11c19`: transmite inicio/recorrido al registro proyectado (+12/+14).
- `0x12879..0x12895`: los pasa a las variables del rasterizador.
- `0x15476..0x15486`, `0x154f8..0x154ff`: construye las columnas desde ese
  inicio, con el contador limitado al recorrido menos uno: 0–29 para marcos.
- `0x15795..0x157a8`: multiplica la columna por 74 para direccionar DALLE.
  `0x1591e..0x15925` copia los píxeles sin comparar su índice con cero.

El port dibujaba siempre U sobre las 60 columnas. Por eso extendía sobre la
segunda mitad del lateral una región que el original no usa para `0x2000`.
La causa demostrada es el intervalo U, no la selección de material, la altura,
la orientación, una transparencia negra ni una puerta móvil ausente.
Afectaba a los 60 laterales de las cinco variantes, cuando eran visibles.
No se ha demostrado un significado de transparencia o espacio reservado para
puerta en esos bytes negros; su propósito exacto como relleno queda abierto.

## Corrección y límites

`wallTextureColumnCount` en `wall_material.h/.cpp` define el intervalo activo:
30 de las 60 columnas para la familia `0x2000`; ancho completo en las demás.
El muestreo usa `min(columnas - 1, int(U * columnas))`: incluso U=1 queda
acotado a 29. V conserva las 74 filas y el recorte de perspectiva existente.
La regla se escala proporcionalmente para texturas sintéticas/de previsualización,
con al menos una columna. No cambia ningún archivo DALLE ni elimina negro.
La correspondencia A→B existente no se invierte.

Materiales y renderizador mantienen responsabilidades separadas; no se introduce
una entidad de puerta para corregir un intervalo de textura. No cambian geometría,
cámara, FOV, alturas, portales, visor 2D, polígonos, controles ni carga de GAME.LAB.

## Puertas móviles: investigación y representación cerrada

### Estado e identificación comprobados

La tabla no es un arreglo independiente creado por el rasterizador: está dentro
del nivel cargado desde GAME.LAB. `0x1136f..0x11378` lee el desplazamiento relativo
de 16 bits en el encabezado del nivel `+0x0a` y guarda `base + offset + 2` en
`[0x5196]`. Las dos primeras posiciones son el contador de entradas. El llamador
`0x11609..0x11666` pasa ese puntero a `0x117fe`; allí `[ebp+30]` se indexa mediante
`wall.extra * 2` (`0x11ad0..0x11ae0`). No es un índice del sector vecino.

En el archivo original: nivel en offset absoluto 126; desplazamiento de tabla
302 (`0x012e`), contador en offset absoluto 428, 15 palabras desde 430 hasta 459.
La siguiente sección empieza en `base + 334`, exactamente después de la tabla.
Los estados 0–14 son cero excepto el 7, que vale `0x0400`. Todos tienen
el byte bajo a cero: el original dibuja la superficie completa en esa posición.
El bit `0x0400` se comprueba en la ruta de interacción `0x109c5..0x109cc`;
su significado completo queda pendiente y se conserva sin reinterpretarlo.

Las 30 conexiones son dos caras inversas de 15 superficies con estado compartido.
En este nivel **todas tienen evidencia de puerta inicialmente cerrada**; no se ha
identificado un paso permanentemente libre entre esas 15 parejas. Ni el índice
DALLE cero ni la ausencia de un dibujo convencional de puerta prueban un paso
libre: `0x1000` usa el material 0 y atraviesa la misma ruta con estado inicial cero.
En otros datos, un estado abierto permite un paso en ese instante; no demuestra
por sí solo que la conexión carezca de puerta. No se generaliza a otros niveles.

### Material, ubicación y espesor

El índice sigue siendo el byte bajo del tipo y apunta a DALLE, con PAL del nivel.
Las caras usan 0, 9, 23, 33, 43 o 53, sin una tabla nueva de materiales ni SPR:
`0x128da..0x128f1` pasa el material al mismo rasterizador que las paredes,
y `0x15501..0x15518` calcula el registro de 4440 bytes.
Los pares de materiales, por `extra`, son:

| Estado/ID | Materiales de las dos caras |
| --- | --- |
| 0, 8 | 0 / 0 |
| 1, 4, 5, 6, 12, 13 | 53 / 23 (el orden depende del sector) |
| 2, 3, 7, 10, 11 | 33 / 43 (el orden depende del sector) |
| 9, 14 | 9 / 43 o 33 / 9, respectivamente |

El código `0x11863..0x118f2` parte de los extremos del propio segmento. En estado
cero `0x11aed..0x11b42` conserva ambos extremos; no genera un desplazamiento
normal, espesor ni cantos adicionales. La representación gráfica demostrada
es una superficie vertical sobre la conexión, con dos caras coincidentes, no
una pared de 100 unidades de grosor. Los 100 corresponden a los laterales.
Esto no demuestra cómo el original calcula colisiones o cualquier volumen físico.

### Movimiento y actualización (solo investigados)

`0x1146d..0x114b9` actualiza la tabla en memoria. Si AH bit 0 está activo,
incrementa AL con un paso derivado del tiempo; al llegar a 90 invierte ese bit.
En la otra dirección reduce AL hasta cero. La ruta `0x10a2f..0x10a43` activa
`0x0100` sobre la palabra almacenada. No hay una enumeración explícita de estados:
el byte bajo es progreso y `0x0100` interviene en su dirección. El tramo 60–90
ya no dibuja superficie y parece proporcionar permanencia abierta; no se ha
reproducido su temporización ni todas sus condiciones de activación.

`0x11ae0..0x11b89` muestra cómo se representa ese progreso `s`:

- `s >= 60`: omite la superficie central.
- `s < 60`: el extremo A pasa a `B + (A - B) * (60 - s) / 60`.
  El extremo B permanece fijo. Las alturas se recalculan para esa nueva posición,
  sin desplazamiento vertical de la puerta.
- El intervalo de columnas también se reduce de 60 a `60 - s` (con ajustes por
  clipping cuando un extremo está detrás del plano cercano).

Esto confirma una retracción **lateral a lo largo del segmento**, no una subida
vertical. Como las dos caras tienen sentidos opuestos, el desplazamiento de A
se expresa en sentidos globales opuestos según la cara visible. Por eso todavía
no se afirma que sea un único sólido rígido deslizante ni una puerta de dos hojas:
hace falta contrastar visualmente una animación original desde ambos lados.
La pose cerrada no depende de resolver esa discrepancia.

### Implementación y alcance

- `game_lab.h/.cpp` conserva las palabras originales en `connectionStates`, con
  validación de offset, contador y límite de la siguiente sección. No aplica
  lógica de puertas. Un offset cero deja la tabla ausente (compatibilidad con
  fixtures sintéticos anteriores) y no permite inferir puertas.
- `door.h/.cpp` contiene los datos de puerta, sus caras y su estado original.
  Inicializa una puerta cerrada únicamente para dos caras inversas de
  distintos sectores, no degeneradas, con el mismo ID válido y una de las
  palabras cerradas demostradas (`0` o `0x0400`). Parejas ambiguas/incompletas,
  estados desconocidos, parciales o abiertos no generan superficies inventadas.
- `main.cpp` construye esa lista una vez, sin modificar las paredes.
- `castSceneRay` combina impactos de pared con las caras de puerta orientadas
  hacia la cámara, usando la misma intersección y escogiendo el impacto más
  cercano. La cara se selecciona siguiendo la condición original A-izquierda /
  B-derecha (`0x11b8f..0x11b9c`). Material y U conservan el sentido de esa cara.
- La proyección, la altura global ya existente del prototipo (256), V, iluminación,
  marcos de 30 columnas y conversión DALLE no cambian. La puerta usa el mismo
  rasterizador, incluyendo fallback plano y previsualización manual N/M.
  `wallTextureIndex` sigue excluyendo portales: las caras independientes pasan
  su material original de byte bajo, no convierten una pared del mapa en opaca.
- `castWallRay` mantiene la omisión de conexiones y la selección inicial de cámara
  existente. Solo la vista 3D recibe las puertas. No cambian mapa 2D, polígonos,
  controles previos, recursos, movimiento del jugador o interacciones.

### Verificación e incertidumbres

**Verificado estáticamente contra el ejecutable y los recursos originales:**
ubicación y contenido inicial de la tabla, relación `extra`, paridad de caras,
material DALLE, posición cerrada, intervalo horizontal y fórmula lateral.
No se ha ejecutado el juego original ni se afirma una comparación visual exacta.

Se compilaron las tres suites existentes y una nueva suite de puertas: las cuatro
pasan, al igual que las siete pruebas Python. Las pruebas nuevas cubren material
cero, banderas, estados ausentes/abiertos/desconocidos, parejas incompletas o
ambiguas, selección de cara, extremos, U sin inversión, oclusión y dibujo SDL
texturado/plano. El lector prueba la tabla y rechaza tamaños/offsets/solapamientos
inválidos. Con los recursos originales se reconocen 15 puertas y se renderizaron
24 vistas (ocho orientaciones en tres tamaños). Se abrió la aplicación con nivel 1.

**Pendiente de comprobación visual:** animación original por ambas caras,
composición y altura exactas del juego original (el port conserva su escala
actual), reglas completas del bit `0x0400`, activación, velocidad y permanencia
abierta. La representación cerrada ya cuenta con validación visual del usuario.
La animación implementada a continuación sigue pendiente de esa validación.

## Animación de retracción y control temporal O

Se amplía el mismo `Door` con `State { Closed, Opening, Open, Closing }` y un
progreso normalizado independiente `p` entre 0 y 1. Los extremos y materiales
cerrados de `faces`, `originalState` y `connectionStates` nunca se modifican.
`toggle` invierte el destino conservando `p`; `update` avanza o retrocede según
los segundos transcurridos, limita los extremos y cambia al estado terminal.
Elimina únicamente el error de redondeo numérico en los extremos. No existe
estado global de progreso compartido ni lógica nueva en el lector de GAME.LAB.

Se vuelve a contrastar `0x11ad0..0x11b42` con el ejecutable: la fórmula confirmada
es una **reducción del segmento visible con recorte/desplazamiento de textura**,
no una hoja completa trasladada que se dibuja detrás del marco:

```text
p = s / 60
A_visible = B + (A_original - B) * (1 - p)
B_visible = B_original
U_visible = [0, 1 - p)
```

`visibleFace` obtiene esa superficie sin alterar el segmento original. La misma
fracción reduce longitud e intervalo U; la densidad de textura por unidad de
pared permanece constante. En un rayo fijo la textura se desliza a medida que
cambia A. `WallHit.textureSpan` transmite el intervalo al muestreador existente;
este limita el píxel del extremo al último texel del intervalo activo. Las
paredes estáticas mantienen su fracción 1 y los marcos sus primeras 30 columnas.
No cambian V, altura, proyección ni sombreado. Con `p=1` no hay caras visibles;
el rayo puede alcanzar la superficie de detrás. El cierre restaura exactamente
geometría, sentido U, material y muestreo originales.

Se conserva la fórmula **por cara** del original: como las dos caras usan
extremos opuestos, la retracción tiene sentido global opuesto según la cara
visible. No se añade grosor, bisagra, segunda hoja ni un sólido rígido inventado.
La comparación visual de la animación original desde ambos lados sigue pendiente.
El port interpola el progreso de forma continua; no reproduce los pasos enteros
de la rutina original ni su aritmética entera de proyección.

**Parámetro provisional:** recorrido completo en **1 segundo**, tanto al abrir
como al cerrar. No es una velocidad recuperada del juego. Se usan los segundos
reales entre fotogramas, incluso si cambia el modo 2D/3D o un fotograma tarda más;
la limitación temporal existente de la cámara permanece separada e intacta.
Los estados finales se mantienen hasta otra orden: no se reproduce la posible
espera 60–90 ni el cierre automático original, cuya temporización sigue pendiente.

**O (depuración, sin repetición automática):** primera pulsación abre las 15
puertas; segunda las cierra; durante una transición invierte la dirección sin
reiniciar el progreso. La función colectiva llama al control individual de cada
puerta. Se permite también en 2D; el mapa permanece igual y al volver al 3D se
ve el progreso actual. No añade interacciones de uso, colisiones, sonido ni
movimiento del jugador. El indicador original `0x0400` se conserva como dato;
este control de depuración actúa sobre las 15 puertas sin interpretar bloqueos.
TAB, P, flechas, Escape y N/M conservan su comportamiento.

Archivos de esta ampliación: `include/door.h`, `src/door.cpp`,
`include/first_person_view.h`, `src/first_person_view.cpp`, `src/main.cpp`,
`tests/door_tests.cpp` y este documento. No cambia CMake ni los recursos.

Verificación de la ampliación:

- Compilación C++17/SDL3 correcta; pasan las cuatro suites CTest existentes y las
  siete pruebas Python.
- Pruebas de estado inicial, apertura, cierre, inversión en ambas direcciones,
  independencia y equivalencia temporal entre un paso de 1 segundo y 60 pasos
  de 1/60, al abrir y cerrar.
- Pruebas de caras en 25/50/75/100 %, extremos fijos, materiales, abertura libre
  y restauración exacta. Prueba SDL con textura asimétrica: verifica columnas
  concretas a lo largo del movimiento, ausencia de estiramiento, altura constante,
  pared revelada al abrir y píxel inicial recuperado al cerrar.
- Con GAME.LAB y DALLE1/PAL1 originales: 144 fotogramas, ocho ángulos por tres
  tamaños, con estados cerrado, 25/50/75/100 % y cerrado otra vez. En las 24 vistas
  la imagen final coincide píxel a píxel con la inicial. Capturas de depuración
  en `build/texture-debug/door-animation-1.png` a `door-animation-4.png`.
- La fórmula está verificada estáticamente contra el ejecutable; velocidad,
  temporización y comparación animada con el juego original siguen pendientes.

## Interacción individual y colisiones dinámicas

Esta sección sustituye el alcance anterior de puertas atravesables y añade
colisión e interacción al mismo sistema de puertas. No cambia su representación
ni las reglas de colisiones estáticas.

### Nueva evidencia del original

- **Acción y tecla:** el texto `SHOOT or OPEN` está en offset de archivo `0x23860`.
  El menú selecciona esa cadena para acción 4 en `0x1e783..0x1e790` y almacena su
  tecla en `[0x5419]` en `0x1e52a..0x1e556`. El cargador `0x67fe..0x6886` lee
  EXTREMIS.CFG (13 bytes); `0x687a..0x687d` coloca su byte 9 en `[0x5419]`.
  En la configuración suministrada ese byte es `0x39`, scancode de **Espacio**.
  No se afirma una tecla universal para todas las instalaciones: es configurable.
  El port asigna Espacio a puertas; no implementa todavía la parte de disparo.
- **Selección original:** `0x10916..0x10931` identifica una conexión del sector;
  `0x10937..0x10982` comprueba coordenadas locales al segmento, con límites
  perpendiculares ±180 y longitudinales 0–200. No es un rayo global de 200 unidades.
  `0x10988..0x109a0` obtiene el estado mediante `wall.extra`.
- **Orientación/activación:** `0x10a03..0x10a43` compara la orientación con la
  dirección de pared y un argumento de modo; una rama exige el intervalo angular
  120–240. Si procede, activa `0x0100` sobre la palabra de estado. Esa acción se
  encuentra en la ruta de movimiento, no en una función independiente de selección
  por pantalla. No se atribuye al original el selector por rayo del port.
- **Acceso:** `0x109a8..0x109fc` comprueba el modificador de cara `0x0800` y el
  indicador de estado `0x0400`; bajo determinadas condiciones muestra el mensaje
  `IMPOSSIBLE TO OPEN PORTAL!` (texto en `0x21afb`). `0x95c2..0x95df` conmuta
  `0x0400` en la tabla y selecciona `VALID CARD..... DOOR UNLOCKING!` (texto en
  `0x21d37`). La rama de fallo usa el texto en `0x21d57`. Esto confirma una ruta
  de tarjeta/desbloqueo, pero no reconstruye inventario, tipos de tarjeta ni todos
  los modificadores. El nivel 1 conserva `0x0400` solo en la puerta con ID 7.
- **Colisión original y paso:** `0x10a53..0x10a64` prueba `(estado & 0x01ff)`
  frente a `0x013c` antes de la transición; `0x10adc..0x10b1f` corrige la posición
  local usando límites 80–120. No es evidencia de un disco barrido contra la hoja
  parcial. La colisión geométrica del port es una aproximación pedida para esta fase.
- **Jugador cruzando y cierre:** `0x10a9a..0x10adb` escribe `0x0140`, conservando
  `0x0400`, durante el paso. Esto vuelve a situar el estado en la fase abierta
  (progreso 64 con bit de dirección activo). La actualización `0x11498..0x114b6`
  luego avanza hasta 90 o reduce a cero. No se ha demostrado un barrido contra
  el cuerpo al cerrar ni un mecanismo general de protección antiaplastamiento.

### Modelo del port y seguridad provisional

`DoorControl` (`door_control.h/.cpp`) coordina las puertas existentes con un solo
jugador, sin SDL ni renderizado. Cada fotograma actualiza sus animaciones mediante
`doors::update`, consulta `doors::visibleFace` y compone una lista temporal con los
muros estáticos y la cara de aproximación de cada puerta todavía visible.
El controlador de movimiento entrega esa lista al mismo `collision::move`:
no hay un segundo algoritmo de contacto ni cambios del radio (30 por defecto),
el deslizamiento o el barrido continuo contra extremos.

Cerrada ocupa su segmento completo; parcialmente abierta usa su longitud visible
actual y permite pasar solo donde cabe el volumen circular entre la hoja y los
laterales; abierta al 100 % no añade ningún segmento. La colisión funciona desde
ambos lados y usa siempre la geometría del fotograma que se va a mostrar.

**Particularidad de las dos caras:** su retracción original es opuesta. Unir ambas
caras como dos obstáculos sólidos taparía huecos que el jugador ve abiertos.
Se elige la cara orientada hacia su lado de aproximación; se mantiene mientras
el centro esté a menos de `radio + skin` del plano. Cuando todo el cuerpo lo
supera se permite elegir la otra cara. Esta retención es provisional: evita crear
un obstáculo dentro del cuerpo por cambiar de cara a mitad de cruce. La otra cara
renderizada sigue su geometría original; durante el cruce puede diferir de la
cara retenida para contacto. No representa un sólido rígido de dos caras, y esta
transición concreta necesita validación visual desde ambos lados.

Antes de aceptar un avance de cierre se evalúa la geometría propuesta frente al
círculo del jugador, con `radio + skin`. Al crecer desde B, el segmento propuesto
contiene toda el área barrida durante ese paso, incluso con un delta time grande.
Si alcanzaría al jugador, se conserva el progreso anterior y se invierte a Opening.
No lo empuja, teletransporta ni aplica daño. Esta política también protege O y
funciona estando en el mapa 2D. La apertura solo reduce el segmento retenido.
Es una regla de seguridad provisional, no una reproducción demostrada del original.

### Selección y controles

**Espacio**, sin repetición de tecla, actúa solo en 3D con foco. El selector lanza
un rayo horizontal desde la posición y orientación actuales, con alcance provisional
configurable de **200 unidades**. Selecciona como máximo la abertura original más
cercana, incluso con la hoja retraída, por lo que se puede cerrar una puerta abierta.
Rechaza la acción si un muro o una hoja actual aparece antes en el rayo. No selecciona
puertas detrás del jugador ni salta una puerta cercana para activar otra al fondo.
Una segunda pulsación invierte la dirección manteniendo el progreso existente.

La interacción normal rechaza `originalState & 0x0400` e informa en el log que
faltan las condiciones de acceso/inventario. Es una política conservadora: no
implementa aún las excepciones del modificador `0x0800`, tarjetas ni desbloqueo.
**O** sigue siendo una prueba global que ignora esa restricción de acceso, pero
respeta la protección del cuerpo. **F queda sin asignar**, reservada para texturas
de suelo/techo. WASD, ratón, flechas, TAB, P, N/M y Escape no cambian.

No se añade cierre automático ni se modifica la velocidad provisional de un segundo.

### Verificación de esta ampliación

Pasan siete suites CTest y siete pruebas Python. La suite nueva sin SDL comprueba:
puerta cerrada y abierta, huecos parciales suficientes/insuficientes con laterales,
ambos lados, barrido rápido, retención de cara al cruzar, uso de una sola puerta,
orientación/alcance, oclusión, indicador de acceso y bypass O, cierre completo en
un fotograma largo sobre el cuerpo, e inversión de un cierre parcial sin avanzar
sobre el jugador. Se conserva la cobertura anterior de geometría y píxeles SDL.

Con GAME.LAB original se probaron las dos aproximaciones de las 15 puertas:
30 bloqueos cerrados y 30 cruces abiertos correctos. La puerta ID 7 devuelve acceso
restringido por ambos lados; solo para comprobar su apertura física se usó la
anulación explícita de depuración. Prueba local en `build/check_dynamic_doors.cpp`.

Pendiente de comprobación manual: interacción con Espacio y O mientras se camina,
sensación de alcance, apertura parcial y transición de cara al cruzar desde ambos
lados. Pendiente de ingeniería inversa: flujo completo de tarjetas, indicadores
especiales y protección original durante el cierre. No hay comparación ejecutada
con la animación/colisión del juego original en tiempo real.

## Corrección de E y revisión del resolvedor común

**Diagnóstico comprobado:** la versión anterior solo encolaba interacción para
`event.key.key == SDLK_SPACE`. E nunca llegaba a `DoorControl::interact`; no era
un fallo de animación ni una apertura individual bloqueada para todas las puertas.
La asignación venía de la acción original configurable, documentada arriba, pero
el control solicitado para el port es ahora **E física (`SDL_SCANCODE_E`)**.
No se añaden acciones a F ni se usa E como apertura global. Espacio deja de ser
la tecla de interacción del port; esto no cambia los hechos de ingeniería inversa.

`isDoorInteractionPress` acepta únicamente KEY_DOWN de E, en la ventana correcta,
sin repetición. `PlayerInput` encola una petición y la consume una vez en 3D con
foco. Se encontró además que antes se perdía silenciosamente una petición en el
fotograma de cambio de captura: ahora se conserva para el siguiente fotograma
estable si el destino es 3D con foco; en 2D/sin foco se rechaza con un mensaje.
El movimiento acumulado del ratón continúa descartándose en esas transiciones.

Trazas temporales de SDL, solo al pulsar E:

- Detección y encolado de E; aplazamiento o rechazo por modo/foco si procede.
- Índice y `stateIndex` original de la puerta candidata, distancia del rayo y alcance.
- Resultado: aceptada, fuera de alcance, sin abertura en el rayo frontal, ocluida,
  o condición de acceso `0x0400`. En oclusión se muestra la distancia del obstáculo.
- Estado anterior y posterior con nombres (`Closed`, `Opening`, `Open`, `Closing`),
  más progreso conservado. Una aceptación cambia el estado y la actualización
  existente mueve la hoja en los siguientes fotogramas.

El resultado estructurado `DoorControl::UseTrace` no depende de SDL: el módulo de
entrada lo convierte en mensajes. La búsqueda informa también de una candidata
frontal lejana para distinguir alcance de ausencia de puerta apuntada. No permite
activarla ni cambia la selección previa. El alcance permanece en 200 unidades.
No se modifican las restricciones: la puerta ID 7 del nivel original conserva
`0x0400`; se informa de que falta inventario/acceso. O sigue siendo el bypass global
explícito de depuración, con protección al cuerpo. No se concede una tarjeta ficticia.

**Revisión de colisiones:** no había dos resolvedores de paredes y puertas:
`DoorControl::rebuildWorld` ya componía una única lista de `collision::Segment`, y
`player::update` la entregaba a `collision::move`. Se conserva esa arquitectura.
`distanceToSegment` y `signedDistanceToLine` quedan centralizadas en
`player_collision.h/.cpp`; se elimina el cálculo de distancia firmada duplicado
que tenía `door_control.cpp`. Solapamiento usa la distancia al mismo punto más
cercano que la detección de penetración. Intersecciones para interacción siguen
en `rayDistance`. Barrido continuo, extremos, respuesta al solapamiento y eliminación
de la componente entrante para deslizar siguen exclusivamente en el resolvedor
existente, sin cambios de radio, tolerancias, retención de cara ni protección de cierre.
Las paredes proporcionan segmentos estáticos; las puertas añaden segmentos derivados
de `visibleFace` cada fotograma. No se unifican sus clases ni se toca el renderizador.

Pruebas nuevas/extendidas: eventos reales de la cola SDL (sin ventana gráfica),
E física aunque el símbolo lógico sea distinto, otra ventana, repetición, liberación
y otras teclas; paso de pulsación a puerta única, Opening y progreso animado;
diagnósticos de puerta lejana, detrás, muro intermedio y acceso; equivalencia de
contactos y deslizamiento entre segmentos estáticos y dinámicos cerrados/parciales.
Pasan ocho suites CTest y siete pruebas Python. Queda pendiente la validación
manual de foco/captura y E en la ventana; las pruebas de eventos no simulan foco
del sistema operativo. Se mantienen las limitaciones físicas provisionales ya descritas.

## Verificación reproducible

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p test_extract_dalle.py
cmake --build build --target inspect_wall_materials
./build/inspect_wall_materials "$HOME/Desktop/In_Extremis/GAME.LAB"
python3 tools/inspect_original_code.py "$HOME/Desktop/In_Extremis/EXTREMIS.EXE" 0x11927 0x11950
./build/InExtremisPort "$HOME/Desktop/In_Extremis/GAME.LAB"
```

El inspector ahora incluye extremos globales, longitud, tipo y extra para
contrastar las relaciones. Las pruebas SDL usan una textura asimétrica 60×74
con media imagen negra y negro también dentro de la región activa: comprueban
las cinco variantes, paredes normales/especiales, ejes sin inversión, negro
opaco, perspectiva y recorte en tres tamaños. También se ejecutan las pruebas
existentes de selección, portales, carga y decodificación.
Durante la corrección previa de marcos se renderizaron 24 vistas del nivel original (ocho ángulos en tres
tamaños), verificando los 250 segmentos opacos y conservando las 30 conexiones
omitidas. La aplicación se abrió con GAME.LAB y sus recursos originales; TAB
permite revisar el resultado en 3D.

### Lector C del nivel 1 integrado

La limitación anterior sobre tarjetas se ha resuelto para el lector 7/17,
0x3308/0x0107. Su tarjeta C001 limpia exclusivamente 0x0400 de la puerta 7;
no abre por sí sola ni consume tarjeta. Se usa G en lector y después E en puerta.
Las puertas bloqueadas anuncian el texto original en el HUD, en vez de dejar
la explicación solo en consola. Selección modal, otros mecanismos y la excepción
de cara 0x0800 permanecen pendientes. Evidencias, pruebas y adaptaciones en
[items-and-inventory.md](items-and-inventory.md#revisión-tras-comprobación-manual-lector-de-seguridad-y-mensajes).

### Selección modal C001

El uso automático de la primera C se ha sustituido por el selector original
INVENT_A/INVENT_T. G abre; arriba/abajo eligen tarjeta y Espacio la presenta.
La E se rechaza por familia; C con ID incorrecto no desbloquea. C001 conserva el
resultado previo sobre puerta 7 y no se consume. El aviso del lector se muestra
sobre B_PORTE; después de cerrarlo se usa E para abrir. Repetir G tras desbloquear
no vuelve a activar 0x0400. El terminal 8/7 también usa C001, pero consulta códigos
sobre B_CODE sin alterar puertas. Evidencia y validación en
[original-interactions.md](original-interactions.md).
