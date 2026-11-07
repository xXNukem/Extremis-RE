# Objetos e inventario originales: nivel 1

Mapa jugable implementado: [Paso 206 — mapa original](step-206-original-map.md).

Auditoría vigente: [Paso 205 — funcionalidades DOS pendientes](step-205-original-functionality-audit.md).
Incluye estado real, nuevas evidencias y límites de validación; no implementa funciones.

Actualización: [disparo del arma equipada — fase 1](equipped-weapon-fire.md).
Las menciones anteriores a ausencia de disparo quedan sustituidas por ese informe.

Actualización vigente: [Paso 201 — controles e interfaces](step-201-interfaces-and-controls.md).
Los controles y el alcance descritos abajo corresponden a revisiones anteriores.

Estado actualizado: [auditoría de interfaces de 2026-11-02](interface-audit-2026-11-02.md).
Ese informe corrige los diagnósticos históricos que siguen en este documento.

**Actualización de interfaces:** la recogida directa y la primera C automática
descritas como adaptaciones en las investigaciones anteriores se han sustituido
por los flujos de [original-interactions.md](original-interactions.md). Ahora G
abre pantalla, el código C200 se comprueba cuando corresponde y Espacio confirma
la elección. Se integran también consulta de códigos (8/7) y transferencia del
plano inicial (1/6), sin convertirlos en suministros. Los apartados anteriores
sobre presentación pendiente conservan el historial de evidencia; el estado
actual, diferencias y controles están en el documento nuevo.

## Alcance y certeza

Esta fase recupera cuatro paneles de pared, sus transferencias de contenido y
los slots originales del jugador. No convierte los 27 registros Table A en
recogibles. Los tipos 0x4000/0x5000 de Table A son otra clasificación y no se
interpretan como clases de inventario. Se conservan sprites, animaciones y
colisiones preexistentes.

Las reglas de transferencia que siguen están respaldadas por instrucciones
EXE y parámetros de GAME.LAB. La interacción inmediata mediante G/H/J es una
**adaptación explícita del port**: las pantallas modales, confirmación visual,
selección de slot y animaciones originales de los armarios siguen pendientes.
No se presenta esta adaptación como reproducción completa de la interfaz DOS.

## Catálogo del primer nivel

Sectores y paredes se numeran desde cero. Offsets absolutos de GAME.LAB:

| Sector/pared | Offset | Tipo | Extra | Contenido | Frente A → B |
|---|---|---|---|---|---|
| 3/6 | 0x68e | 0x3205 | 0x3001 | Tarjeta familia E, identificador interno 1 | (4400,2400) → (4400,2200) |
| 7/10 | 0xcf6 | 0x3205 | 0x4001 | Tarjeta familia C, identificador interno 1 | (1400,3000) → (1600,3000) |
| 8/6 | 0xe28 | 0x3104 | 0x11f4 | Arma 1, munición 500 | (3000,2800) → (3000,2600) |
| 8/8 | 0xe38 | 0x3205 | 0x2064 | Inyector, carga interna 100 | (3000,2400) → (2800,2400) |

Los recursos murales ya decodificados representan los paneles (familias de
material DALLE 4/5); no hay que añadir un OBJET.SPR para estos cuatro objetos.
El gráfico del panel permanece al retirar el contenido: la rutina muta el
campo extra, no elimina una entidad Table A ni la pared. El port conserva ese
comportamiento. El armario de armas retiene el arma depositada a cambio.

No se identifican botellas de oxígeno, baterías, bombas o munición suelta entre
estos suministros del nivel 1. Su existencia en otras rutas o bancos gráficos
no autoriza colocarlas en el mapa. Los mensajes de menú «Carte E987»,
«Carte C987», «Carte N987» confirman las familias, pero el código mostrado no
se deduce directamente del identificador 1: no se inventan E001/C001.

## Formato y activación

La pared ocupa cuatro uint16: vértice A, vértice B, tipo, extra. La ruta original
`0x10b9c..0x10c18` reconoce la familia 0x3000 y una acción explícita. Extra es
copiado a [0x5204], subtipo a [0x5202], puntero de pared a [0x5206]. El subtipo
1 abre el armario de armas (`0x8042`), el 2 abre suministros (`0x183ce`), según
el dispatcher `0xa508`. El lector 0x3308 se integra en la revisión de seguridad
al final de este documento. Pantallas 0x3002, códigos 0x3508 y mecanismos
0x3606 quedan fuera de esta implementación.

`0x10bc1..0x10bda`: distancia normal en [0,90), proyección longitudinal en
[0,longitud). `0x10bdc..0x10bfc`: diferencia angular [120,240) respecto a la
tangente. El original usa **720 unidades por vuelta** (`0x3ee8..0x3f6c`):
esto corresponde a mirar hacia el panel dentro de aproximadamente ±30°.
`0x1117a..0x111ee` contiene la transformación de coordenadas.

El port reproduce esas magnitudes con geometría flotante y añade comprobación
conservadora de línea libre hasta el panel con el mundo de colisiones actual.
No permite recoger desde detrás ni a través de un obstáculo intermedio. No
reproduce la cuantización entera DOS ni su límite angular semiabierto exacto.
Elige el panel accesible más cercano; DOS lo encuentra durante su recorrido
de colisiones del sector. Estos detalles pueden diferir en contactos límite.
No existe recogida automática por pasar junto al panel.

## Almacenamiento y efectos

`0x183ce..0x183fb`: nibble superior de extra = clase; 12 bits inferiores =
parámetro. Clase 1 botella, 2 inyector, 3/4/5 tarjetas E/C/N. No se implementan
clase 0 batería ni 6 bomba: no son necesarias para los cuatro paneles presentes.

| Estado original | Slots | Regla integrada |
|---|---|---|
| Reserva O2 [0x544f] | 1, -1 ausente, 0 vacía | Parámetro ×160 ticks; intercambio con O2 del traje al usar |
| Tarjetas [0x5453/55/57] | 3, -1 ausente | Guarda ((clase−3)<<12)\|identificador |
| Inyectores [0x5459/5b] | 2, -1 ausente, 0 vacío | Guarda carga; usar uno lleno restaura salud a 1000 y lo deja vacío |
| Arma/munición | 1 equipada | Intercambia arma y munición con el contenido del armario |

- Botella: `0x19039..0x190b3`. Si no existía, panel pasa a 0xffff; si se
  sustituye una vacía, deja 0x1000 en la pared. Una botella ya llena requiere
  elección modal original: el port devuelve capacidad y conserva ambos estados.
  Se admite hasta 1800 segundos para mantener el límite vital reconstruido;
  no se extrapola esta restricción defensiva como formato universal de niveles.
- Inyectores: `0x190b4..0x1912e`. Se recorre en orden el primer slot ausente o
  vacío; vacío del slot 0 tiene prioridad sobre ausencia del slot 1. Deja
  0xffff si estaba ausente, 0x2000 si intercambia envase vacío. Con dos llenos,
  no se inventa selección de sustitución.
- Tarjetas: `0x19133..0x191a6`. Primer slot ausente y extra=0xffff. Con tres
  tarjetas DOS permite elegir una sustitución (`0x193ca..0x19419`); pendiente
  en el port, que conserva el panel y devuelve capacidad.
- Arma: `0x8de6..0x8e1f`. Nuevo estado = nibble/12 bits del panel; el panel
  recibe arma/munición anterior. Pulsar otra vez intercambia de nuevo, no duplica
  ni consume un arma. No hay disparos ni consumo de munición implementados.
- Uso de botella: `0x1bd21..0x1bd36` excluye ausente/vacía;
  `0x1bd59..0x1bd6e` intercambia reservas, no suma ni rellena gratuitamente.
- Uso de inyector: `0x1be19..0x1bf37` restaura 1000 y vacía el slot. Su carga
  100 no es una curación de 100 puntos. El port usa el primer inyector lleno;
  la selección original mediante menú queda pendiente. No se revive a muertos.

La nueva partida conserva arma 0 y munición 500 del template original ya
investigado. Inventory y World nuevos restablecen slots/paneles; no hay guardado.
F10 sigue siendo exclusivamente un reinicio diagnóstico de Vitals, no de nivel.

## HUD, mensajes y audio

`0x14184..0x14378` limpia (183,155)..(241,196) y dibuja equipo CASQUE.SPR:

| Imagen | Tamaño | Posición |
|---|---|---|
| 36 botella | 18×38 | (184,157), si el slot está ocupado incluso vacío |
| 37 tarjeta | 7×10 | x205, Y=171; 161/181; 157/171/185 según cantidad |
| 38 inyector | 5×17 | x215, Y=168; 157/178 según cantidad |

Tablas Y en archivo EXE `0x14379` y `0x14391`. Se cuentan slots ocupados, no
solo envases llenos. Se conservan PAL1, píxeles y gráficos originales, viewport
320×150, imagen 320×200 y presentación 4:3. Icono del arma y munición ahora
proceden del inventario real. La lógica de curación/intercambio sigue en Vitals.

La confirmación de suministro `0x18f97..0x18f9f` y de arma `0x8c0b..0x8c13`
solicita BANK[12], voz 1, vía `0x13123`. Se integra una petición por transferencia
aceptada, no por fotograma ni al intentar recoger un panel vacío. Su posición
respecto a la animación modal no se reproduce todavía. BANK[11] aparece en la
animación de armario `0x1858b..0x18593`, además de su nombre «moteur ascenseur»;
BANK[18] y otras peticiones pertenecen a esa presentación. No se añaden bucles
de motor ni sonidos de H/J sin reconstruir su secuencia exacta.

El mensaje original de panel vacío es «CE BOITIER EST VIDE!» (puntero 0x2173f,
archivo 0x2193f). Ahora se muestra mediante la cola descrita en la revisión
de seguridad. Los logs de terminal describen resultados del port, no
simulan mensajes DOS. No se inventan avisos, códigos o mensajes originales.

## Arquitectura

- `player_inventory`: slots y transferencias, sin renderer/audio/SDL; reutiliza
  Vitals para toda recuperación, sin duplicar salud u oxígeno.
- `level_items`: catálogo y contenido mutable separado de GAME.LAB inmutable;
  acceso geométrico y persistencia, sin alterar decoraciones.
- `item_session`: carga y solicitudes de entrada, coordina World, Inventory,
  Vitals y confirmación GameAudio. Sin framework ECS ni nuevas reglas de puertas.
- `SurvivalSession::hudState`: adaptación visual de valores y ocupación;
  `original_hud`: solo dibuja datos recibidos.

## Pendientes y diferencias explícitas

Faltan interfaces modales de armario/inventario, elección y animación de
sustitución, presentación progresiva de mensajes, códigos y otros usos de tarjetas,
baterías/bombas, efectos de munición y armas funcionales. Las tarjetas se
almacenan y dibujan; C001 ya desbloquea la puerta 7 mediante su lector asociado.

La clasificación física de Table A merece una investigación aparte: la ruta
`0x10db7` y siguientes consulta extents por imagen mediante [0x5b1f], omite
banderas 0x2000 y usa 0x1000 en ramas de bloqueo. No se ha reconstruido todavía
toda la política de contacto/forma; no se asignan obstáculos arbitrarios a los
27 sprites. Se conserva la colisión actual de paredes/puertas.

No hay nueva respiración, sonidos de daño genéricos, enemigos, disparos, códigos
funcionales, carrera o pantallas ficticias. No se modifican recursos originales.

## Evidencia reproducible y pruebas

Referencia EXTREMIS.EXE SHA256:
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Todas las direcciones citadas son offsets absolutos del archivo; punteros de
datos se convierten sumando 0x200.

```sh
python3 tools/inspect_items.py ~/Desktop/In_Extremis
./build/inventory_tests ~/Desktop/In_Extremis/GAME.LAB
ctest --test-dir build --output-on-failure
```

La herramienta escribe catálogo, offsets, tablas HUD y desensamblado en
`build/items-debug`; requiere libLTO de las Command Line Tools como los otros
inspectores. Los tests cubren los cuatro registros reales, capacidades, envases,
no duplicación, intercambio, curación, O2 con una botella sintética de la rama
confirmada, muerte, distancias/orientación/oclusiones, conservación de recursos,
reinicio y HUD. Las pruebas SDL usan drivers dummy, sin dispositivo físico.

## Comprobación manual

```sh
./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB --vitals-debug
```

Entrar en vista 3D con el control existente Tab. Moverse con los controles
habituales, mirar al panel desde su cara frontal, a menos de 90 unidades normales,
y pulsar **G**. En Mac son letras normales G/H/J, sin Fn. No sustituyen E de
puertas ni O de diagnóstico de apertura. Referencias geométricas para orientarse
con las coordenadas del mapa (no son comandos de teletransporte):

| Panel | Punto de observación | Mirar |
|---|---|---|
| Tarjeta E | (4350,2300) | +X |
| Tarjeta C | (1500,2950) | +Y |
| Arma | (2950,2700) | +X |
| Inyector | (2900,2450) | −Y |

1. Recoger ambas tarjetas: observar uno y después dos iconos en sus Y originales;
   segundo G sobre panel vacío no añade nada ni repite confirmación.
2. Recoger inyector: aparece un icono, sin curación automática. Usar F5 (en un
   teclado Mac puede requerir **Fn/🌐 + F5**) para bajar salud; **H** restaura
   salud completa. El icono permanece porque queda un inyector vacío; otro H
   no cura. La consola permite comprobar el resultado sin mensajes inventados.
3. G en el armario de armas cambia icono a arma 1 y mantiene 500. Otro G devuelve
   arma 0; el armario no desaparece. No debe disparar.
4. J no hace nada en este nivel al no haber botella identificada. La rama de
   intercambio O2 se valida automáticamente con fixture, sin colocar botellas
   ficticias en el mapa. El consumo real continúa como antes.
5. De espaldas, demasiado lejos o detrás de un obstáculo no hay transferencia.
   Tras F8 (o Fn/🌐+F8) muerte por O2, G/H/J no modifican estado. F10 reinicia
   solo Vitals; para comprobar reinicio completo de objetos, cerrar y relanzar.
6. Comprobar puertas, pisadas, sprites, mapa, escalado y bordes del HUD. No se
   afirma validación auditiva/manual nueva: esta lista queda para el usuario.

## Resultado de verificación de esta fase

- Compilación CMake completa: correcta.
- CTest: 17/17 suites correctas (incluidas audio, movimiento/colisiones,
  puertas, sprites, planos, renderizador, HUD y supervivencia).
- Herramientas Python: 16/16 tests correctos.
- AddressSanitizer + UndefinedBehaviorSanitizer: 4/4 suites de inventario,
  Vitals, sesión y HUD correctas; catálogo real de GAME.LAB y composición HUD
  con recursos originales correctos, sin diagnósticos de sanitizadores.
- Validación visual/auditiva interactiva nueva: pendiente del usuario.

Archivos de esta tarea: `include/` y `src/` para player_inventory, level_items,
item_session (nuevos), survival_session, original_hud y game_audio (integrados);
`src/main.cpp`, `CMakeLists.txt`; tests inventory, survival_session, hud y
`test_inspect_items.py`; herramientas `inspect_items.py` e
`inspect_audio_code.py`; documentos items-and-inventory, audio-system,
player-vitals, original-hud y sprite-rendering. Los cambios locales anteriores
se han conservado. No se han realizado commits, pushes ni ediciones de datos DOS.

## Revisión tras comprobación manual: lector de seguridad y mensajes

El panel inicial junto a la salida del jugador es sector 1/pared 6,
`type=0x3606`, `extra=0`, A=(3200,1000), B=(3200,1200). **No es un inyector**.
El dispatcher `0xa608..0xa613` entra en la interfaz `0x19a4f`, con selección y
entrada de código (`0x19d3d` y siguientes); esa interfaz continúa pendiente. No
se le asigna una recogida ni se simula su funcionamiento. El inyector verdadero
está en 8/8 (2900,2450 mirando −Y como punto de observación).

Hay un lector de seguridad en **sector 7/pared 17**, tipo `0x3308`, extra
`0x0107`, A=(2400,2600), B=(2600,2600). Su byte inferior referencia la puerta
**7** y el superior requiere identificador **1**. Es la única puerta con bit
0x0400 inicial del nivel. El otro terminal, 8/7 `0x3508 extra=1`, pertenece a
la interfaz de código, no se trata como un lector C ni se desbloquea con G.

Evidencia adicional, offsets absolutos EXE:

| Rango | Operación confirmada |
|---|---|
| `0xa567..0xa5aa` | Subtipo 3: obtiene puerta del byte bajo; si 0x0400 ya está limpio, anuncia seguridad ya desactivada; si no, abre lector 0x92f0 |
| `0x937c..0x9395` | Separa byte bajo = puerta, byte alto = identificador requerido |
| `0x9434..0x9455` | Selección modo 3, comprueba cancelación del selector |
| `0x1ac94..0x1acbc`, `0x1afbc..0x1aff1` | Filtra tarjetas por ((modo−2)<<12): modo 3 acepta nibble 1 = familia C; devuelve el valor packed de la tarjeta elegida |
| `0x9482..0x948d`, `0x95a1..0x95a9` | Enmascara AH y compara byte bajo de tarjeta con identificador requerido |
| `0x95c2..0x95df` | XOR 0x0400 sobre estado de la puerta y selecciona texto de desbloqueo |
| `0x961d..0x9634` | Selecciona texto de tarjeta rechazada; no toca inventario |
| `0x1c93c..0x1c975` | Convierte 12 bits de identificador a tres dígitos: las tarjetas identificadas pueden denominarse E001/C001; la incertidumbre anterior queda resuelta |

Se integra G en el lector con la misma geometría de acceso de los armarios.
**Adaptación del port:** selecciona la primera tarjeta C almacenada; el selector
modal DOS continúa pendiente. No busca automáticamente una tarjeta que coincida
si la primera C es incorrecta. No consume tarjetas, no abre directamente, no
re-bloquea al repetir G, ni altera otras puertas. Después del desbloqueo se usa
E en la puerta. Fuera de alcance, tras pared, de espaldas, sin foco/3D o muerto,
no se permite usar el lector. Se conservan las aproximaciones de colisión/ángulo
ya documentadas.

Los mensajes pasan por `messages::Queue`, se dibujan en CASQUE con el mismo
renderer del HUD, y aparecen al:

- Usar E sobre una puerta bloqueada: texto original desde archivo `0x21aef`.
- Aceptar tarjeta: `VALID CARD..... DOOR UNLOCKING!` (`0x21d37`).
- Rechazar su identificador: `IMPOSSIBLE TO UNLOCK THIS DOOR...` (`0x21d57`).
- Repetir lector desbloqueado: `THE SECURITY IS ALREADY DISABLE` (`0x21b32`).
- Intentar retirar de nuevo un suministro vacío: `THIS COMPARTMENT IS EMPTY!`,
  traducción inglesa autorizada de `CE BOITIER EST VIDE!` (`0x2193f`).

Sin tarjeta C se mantiene bloqueada y se anuncia el aviso original de puerta
bloqueada. Esto es una adaptación explícita: DOS muestra el modal francés
`Pas de carte de sécurité` (archivo `0x23554`) y cancela el selector. No se
traduce ni se atribuye al original el nuevo enrutamiento de ese aviso.

`0x14399..0x14414` confirma cola de cinco y deduplicación sin renovar edad;
`0x1454d..0x14596` acumula elapsed y expira a 800 ticks (5 segundos nominales).
Se integra esa duración independiente de FPS, congelada con la simulación.
Se conservan origen (8,16), CR=11 filas, separación de mensajes=14 y glifos
originales; se recorta sin invadir el panel inferior. **Pendiente:** revelado
por llamada y cursor DOS; se muestra el texto completo inmediatamente, sin
inventar una cadencia de teletipo.

No se añade audio de éxito ni animaciones modales en esta revisión; su secuencia
sigue pendiente. Puertas y pisadas mantienen sus efectos.

Comprobación: recoger C001 en 7/10, llegar a (2500,2550) mirando +Y y pulsar G
frente al lector. Debe aparecer desbloqueo; G de nuevo muestra ya desactivada.
Acercarse a la puerta 7 y pulsar E para abrir. Antes de la tarjeta, E anuncia
seguridad bloqueada; O continúa siendo bypass diagnóstico, no validación de la
mecánica. El inyector se encuentra en otro sector, no en el panel inicial.

Verificación de esta revisión: compilación completa sin errores, 17/17 suites
CTest, 16/16 pruebas Python y 5/5 suites ASan/UBSan (incluida seguridad de puertas).
La prueba con GAME.LAB real confirma que existe exactamente una puerta bloqueada,
que el lector referencia ID 7 y que el desbloqueo conserva todos los demás bits.
La integración prueba falta de C, orientación, oclusión, muerte, conservación de
tarjeta, aviso en hudState y repetición sin re-bloqueo. La cola se comprueba a
30/60/144 FPS y en pausa; el dibujo valida posiciones y separación de mensajes.

Archivos de esta revisión: `game_messages.h/.cpp` nuevos; integración en
`door_control`, `level_items`, `item_session`, `survival_session`, `player_input`,
`original_hud`, `main.cpp`, `CMakeLists.txt`; pruebas door_control, inventory,
survival_session y hud; `tools/inspect_items.py`; documentación de objetos,
puertas y HUD. Sin cambios de recursos originales, commits ni pushes.

## Idioma e interfaces omitidas: aclaración tras prueba manual

El idioma visible del port será inglés. Se mantienen los textos ingleses
recuperados; las cadenas francesas necesarias se traducen en presentación y se
documenta la cadena fuente. No se parchean EXE, SPR ni otros recursos originales.
El aviso de panel vacío se muestra ahora como `THIS COMPARTMENT IS EMPTY!`,
traducción de `CE BOITIER EST VIDE!` (EXE 0x2193f), no como una cadena inglesa
hallada en el ejecutable. La revisión de src/include no encuentra otros textos
franceses visibles actualmente integrados. Esto no supone traducir todavía
los menús que aún no están implementados ni modificar las voces originales.

La ausencia de interfaz no es exclusiva del panel inicial. La recogida directa
es una adaptación incompleta, no una excepción demostrada del juego original:

| Interacción | Ruta original | Estado actual del port |
|---|---|---|
| Suministros y tarjetas en armario | 0xa55d → 0x183ce, confirmación 0x18f97 y transferencia 0x19039..0x191a6 | G transfiere directamente; faltan pantalla y selección/confirmación |
| Armario de armas | 0xa525 → 0x8042, confirmación 0x8c0b e intercambio 0x8de6 | G intercambia directamente; falta interfaz |
| Lector C de puerta 7 | 0xa5a5 → 0x92f0 y selector 0x1ab2e | G aplica primera C; faltan pantalla del lector y selector original |
| Terminal 8/7, tipo 0x3508 | 0xa601 → 0x9ace | Interfaz/código pendientes; no se trata como armario |
| Panel inicial 1/6, tipo 0x3606 | 0xa60e → 0x19a4f | Interfaz y operación pendientes; no contiene un inyector |

No son todas la misma pantalla: cada ruta carga/compone su interfaz y añade
animaciones, selección y sonidos. No basta con colocar una confirmación genérica
sobre el mundo para reproducirlas. La reconstrucción de esos flujos debe sustituir
la recogida/intercambio directo y conservar las transferencias ya verificadas.
La falta de esta presentación no significa que el panel inicial deba permanecer
inerte en la versión final: su funcionamiento completo aún no está integrado.

Recursos inspeccionados reutilizando el decodificador SPR existente, sin cambios:

- CH_OBJ2.SPR: 1 imagen 196×128; CH_OBJ3.SPR: 12 imágenes, base 196×100.
- CH_OBJ4A.SPR y CH_OBJ4F.SPR: 31 imágenes cada uno. Se ha contrastado
  visualmente el índice 14: **Yes** en A, **Oui** en F. El índice 12 inglés
  contiene **Working order**. Debe elegirse A al integrar la interfaz inglesa.
- B_PORTE.SPR: 5 imágenes; B_CODE.SPR: 10 imágenes; sus bases son 196×100.
- En suministros, 0x18b6b..0x18b86 dibuja la base de interfaz en (62,25).
  La paleta se carga desde el nombre ubicado en archivo 0x18326, antes de la
  composición (0x1844b..0x1845b). La secuencia completa requiere recuperar sus
  estados; estas medidas no autorizan reutilizar una sola pantalla para todos.

Las inspecciones visuales quedan en build/panel-debug (diagnósticos locales,
no recursos nuevos del juego). Esta revisión corrige idioma y documenta el
alcance pendiente; **no afirma haber implementado las interfaces modales**.
