# HUD original de In Extremis

Actualización: [disparo del arma equipada — fase 1](equipped-weapon-fire.md).
Las menciones anteriores a ausencia de disparo quedan sustituidas por ese informe.

Actualización vigente: [Paso 201 — controles e interfaces](step-201-interfaces-and-controls.md).
Los controles y el alcance descritos abajo corresponden a revisiones anteriores.

## Resultado de esta fase

Reconstrucción visual integrada en la vista 3D: panel inferior, esquinas del
casco, barras de oxígeno y energía/salud, silueta corporal, radar (activo con batería desde Paso204),
ventana de equipo vacía, iconos de las seis armas y cifras de munición. Se ha
recuperado también el dibujado de los caracteres de mensajes, sin inventar su
temporización ni generar mensajes automáticos.

Las barras reciben ahora el estado real de `player::Vitals` mediante
`SurvivalSession`: salud inicial 1000 y oxígeno inicial 288000, con consumo
temporal y causas de muerte documentados en [player-vitals.md](player-vitals.md).
Se retira el título HUD DEMO. La selección visual de arma 0 y munición 500 son
datos estáticos de la plantilla original de nueva partida recuperada; todavía
no hay arma funcional. El mapa 2D conserva su representación y controles, sin
este HUD, y suspende la simulación vital del port.

No se han modificado ni copiado al repositorio recursos originales. Las imágenes
de diagnóstico permanecen en `build/hud-debug/`, ignorado por Git. No hay
dependencias gráficas nuevas. El módulo HUD no contiene reglas de salud,
oxígeno, combate o inventario: recibe únicamente sus entradas visuales.

## Fuentes y convención de evidencia

La investigación usa los archivos locales de `~/Desktop/In_Extremis/` y las
rutinas x86 de 32 bits de `EXTREMIS.EXE`, SHA-256:

```text
7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397
```

Las direcciones de código de este documento son **offsets absolutos de archivo**.
Los punteros a datos que aparecen en instrucciones no son esos offsets: en esta
imagen, el puntero de datos se traduce sumando `0x200`. Los IDs de sprites del
documento y del port empiezan en **0**; la API original de dibujo empieza en **1**.
No se han usado capturas externas como prueba de coordenadas. La captura local
generada es del port y sirve para revisión, no para probar equivalencia con DOS.

**Confirmado** significa respaldado por bytes del recurso o instrucciones
desensambladas. Las decisiones de presentación del port y los comportamientos
pendientes se indican expresamente; no se deduce una mecánica de un nombre de
archivo o del aspecto de un icono.

## Resolución, pantalla y viewport

La pantalla lógica es **320×200**. La inicialización dibuja el panel en `(0,150)`
y copia al destino el rectángulo inclusivo `(0,150)..(319,199)`
(`0x139d8..0x13a9b`). La escena utiliza **320×150**, filas 0..149, y las rutinas
de arma de primera persona limitan el recorte inferior a 149
(`0x13ac8..0x13ae3`). Las esquinas del casco se superponen a la escena, arriba
en filas 0..9 y abajo en 140..149. El panel inferior ocupa una zona reservada;
no se dibuja encima de las últimas 50 filas de una escena de 200 filas.

Con HUD, el port renderiza la escena a 320×150 con el framebuffer software
existente y compone después una imagen de 320×200. Una sola textura SDL con
`SDL_SCALEMODE_NEAREST` presenta el conjunto. No se filtran sprites ni se
posicionan los indicadores en coordenadas independientes de ventana.

La política de presentación del port corrige la pantalla VGA a **4:3**, centrada,
con bandas negras cuando hace falta: 800×600 dentro de una ventana 960×600,
o 1440×1080 dentro de 1920×1080. Es la interpretación de presentación del port
para los píxeles VGA no cuadrados, no una instrucción de relación de aspecto
recuperada del EXE. En tamaños no enteros hay columnas o filas de distinto
número de píxeles físicos por el muestreo nearest; no hay suavizado. Para
inspeccionar cada texel de forma uniforme se puede revisar el buffer lógico
antes de la corrección de aspecto.

**Diferencia conservada del renderizador 3D:** se recupera el tamaño del viewport,
pero no se reescribe la proyección. El port conserva su FOV de 70 grados,
horizonte centrado (75 en este viewport), alturas y sombreado actuales. La
investigación previa de planos identifica focal original 200 y horizonte 50;
véase `floor-ceiling-rendering.md`. La escena no se presenta como una emulación
píxel a píxel del rasterizador DOS. Esta diferencia no altera las coordenadas
del HUD. Sin recursos válidos del HUD se conserva la vista anterior a ventana
completa y se informa del error.

## Recursos y formato

Se reutilizan `spr_format::load` y `wall_textures::loadPalette`. No ha sido
necesario introducir otro decodificador ni generar texturas sustitutas.

| Recurso | Hallazgo |
| --- | --- |
| `CASQUE.SPR` | 39430 bytes, 86 imágenes; panel, esquinas, radar, barras, cuerpo, iconos y tipografía. |
| `ARME0.SPR`..`ARME5.SPR` | Primera imagen de cada banco: icono de 57×30 para el panel. Las demás imágenes no se integran en esta tarea. |
| `PAL1.PAL` | Paleta del nivel 1, compartida por escena e interfaz. No se sustituye por PAL_DI. |
| `INVENT_A.SPR`, `INVENT_F.SPR` | 27 imágenes por archivo; primera imagen 184×180. Recursos identificados de una interfaz adicional; no se reconstruye su pantalla modal. |
| `INVENT_T.SPR` | 76 imágenes, tipografía de 9 píxeles de alto; no es la fuente de mensajes normales recuperada. |
| `BOUSSOLE.SPR` | Diez imágenes, primera de 32×32. No se añade una brújula al HUD sin recuperar su ruta de activación. |
| `B_CODE.SPR`, `B_PORTE.SPR`, `CH_LEVEL.SPR` | Bancos con paneles de 196×100 e iconos. Su existencia no demuestra indicadores de tarjetas permanentemente visibles. |
| `PAUSE.SPR` | Una imagen de 105×28; no se inventa una nueva mecánica de pausa. |
| `TYPO_3X5.BIN`, `TYPO_3X8.BIN` | Archivos presentes; 278 y 634 bytes. No hacen falta para los mensajes del HUD, que usan CASQUE. Su decodificación queda pendiente. |

El contenedor SPR conserva el formato ya usado por OBJET/ADALLE: word inicial,
contador uint16 en +2, tabla de offsets uint32 relativos a archivo +4, cabecera
de imagen de ocho bytes (ancho, alto, dos anclajes) seguida de `ancho*alto`
índices, por filas. CASQUE empieza su primer registro en `0x15c`; el panel es
el segundo registro, en `0x1fa`, con píxeles desde `0x202`. No hay compresión ni
paleta incrustada en estos SPR. Los anclajes no sustituyen las posiciones fijas
recuperadas del código de interfaz.

PAL contiene 256 tríos RGB VGA de seis bits. El port expande cada componente con
`(v*255+31)/63`, igual que el resto del motor. `0x7ad4..0x7ae3` carga 768 bytes
del nombre de paleta construido para el nivel en el buffer `[0x4af4]`; la
aplicación selecciona `level.palBank`, que vale 1 en el nivel 1. Una variante de
paleta verde aparece en `0x7ae8..0x7b12`; sus condiciones de uso y los efectos
globales de alerta no se incorporan aquí.

La transparencia depende de la operación, no del RGB negro: panel, barras,
radar, cuerpo y números se copian opacos en las rutas recuperadas; los mensajes
omiten **índice 0** (`0x140d2..0x140d7`). Otros índices negros conservan su
opacidad. Las esquinas se copian mediante prefijos de fila, no una eliminación
de colores ni un estiramiento rectangular del sprite entero.

## Composición y coordenadas confirmadas

| Elemento | Imagen desde 0 | Posición / tamaño lógico | Operación |
| --- | --- | --- | --- |
| Panel inferior | CASQUE[1] | 0,150 / 320×50 | Copia del recurso original. |
| Oxígeno completo | CASQUE[17] | 68,162 / 43×10 | Verde con PAL1. |
| Energía/salud completa | CASQUE[16] | 68,178 / 43×10 | Naranja/rojo con PAL1. |
| Segmento vacío de ambas barras | CASQUE[18] | Mismo origen / hasta 43×10 | Sobrescribe desde la izquierda. |
| Silueta corporal | CASQUE[19] | 125,155 / 31×41 | Copia opaca; no representa daño calculado. |
| Radar inactivo | CASQUE[4] | 0,171 / 46×29 | Imagen estática de rejilla. |
| Radar activo | CASQUE[5..12] | 0,171 / 46×29 | Selección visual de ocho imágenes. |
| Ventana de equipo vacía | Primitiva | 183,155..241,196 inclusive | Relleno de índice 0. |
| Icono de arma | ARME[n][0] | 263,170 / 57×30 | Solo selección visual, sin arma funcional. |
| Dígitos de munición | CASQUE[26..35] | Tabla inferior / 3×6 cada dígito | Tres cifras, avance X=4. |
| Mensajes | CASQUE[42..85] | Inicio 8,16 | Tipografía variable de 10 píxeles de alto. |
| Esquinas superiores | CASQUE[2] | Dos extremos, filas 0..9 | Prefijos de fila, lado derecho reflejado. |
| Esquinas inferiores | CASQUE[3] | Dos extremos, filas 140..149 | Prefijos en orden inverso, lado derecho reflejado. |

El panel crudo tiene grandes zonas verdes que **no son un fondo definitivo**:
las operaciones de radar, cuerpo, equipo e icono de arma las reemplazan. No se
han retocado esos colores en los archivos ni se ha usado una máscara inventada.

El oxígeno usa `[0x548d]`, máximo 288000; energía/salud usa `[0x5491]`, máximo
1000. La relación con el agotamiento está corroborada por las ramas de final de
partida en `0x76a9..0x76d9`, además del contador y aviso de oxígeno de la
investigación de audio. Esto identifica los indicadores; no introduce sus reglas.
La longitud vacía es `floor((máximo-valor)*43/máximo)`. El recurso gris ocupa las
columnas izquierdas y el color restante queda a la derecha; no se dibuja una
barra plana de color ni se vacía desde la derecha. El port limita los valores
al rango para evitar divisiones inválidas o lecturas fuera del recurso.

Las coordenadas de munición se leen de la tabla de registros de 112 bytes en
archivo `0x56d7`, cuyo puntero se establece en `0x7f28..0x7f3d`:

| Arma visual | Origen de las tres cifras |
| --- | --- |
| 0 | 265,188 |
| 1 | 265,191 |
| 2 | 271,191 |
| 3 | 287,193 |
| 4 | 268,191 |
| 5 | 269,192 |

La tabla de esquinas en archivo `0x13d89` contiene pares de cantidad de dwords
y salto de fuente. Las longitudes efectivas son `60,48,40,32,24,20,16,12,8,4`;
la fuente avanza siempre una fila de 60 bytes. Abajo se usa la tabla al revés,
manteniendo el orden de filas del sprite. Se reproduce la inversión horizontal
de cada píxel, no solo de grupos de cuatro bytes.

## Tipografía y elementos dinámicos

`0x14499..0x144d9` asigna letras A..Z a IDs originales 43..68, números 0..9
a 69..78 y signos `? ! ' . , : -` a IDs específicos; el byte 255 selecciona el
cursor, ID 86. El port convierte a índices desde cero. El avance es ancho+1;
espacio avanza 9; CR reinicia X=8 y suma 11 a Y; LF se ignora. Después de
superar X=312 se vuelve a X=8 y se avanza una fila de texto. Los glifos
conservan sus propios colores y transparencia, sin una fuente del sistema.

La rutina original administra hasta cinco mensajes (`0x14399..0x14414`), revela
caracteres por llamada a `0x14415`, presenta un cursor y acumula `[0x1428]`;
retira el mensaje más antiguo al llegar a 800 unidades (`0x14574..0x14596`).
El renderer dibuja el texto que recibe como ya visible. `messages::Queue`
administra ahora cinco entradas, deduplicación y expiración a 800 ticks
(5 segundos nominales activos), y separa mensajes por 14 filas. Se usa para
avisos originales de seguridad y panel vacío. **No implementa todavía
cursor automático ni escritura progresiva:** el texto aparece completo.
Recorta antes de invadir el panel y omite caracteres no soportados,
como protección; no reproduce accesos inválidos de la rutina DOS.

El radar usa CASQUE[4] apagado y CASQUE[5..12] activo. Desde el Paso204 se
conecta a los registros reales y batería; véase la sección del Paso204 al final.
El juego nuevo conserva la batería ausente y el detector apagado del EXE.

La silueta tiene seis posibles superposiciones de heridas, CASQUE[20..25].
`0x13f4a..0x13faa` recorre seis registros de diez bytes desde `[0x5493]`,
decrementa su estado y usa `([0x5423]>>2)&1` para intermitencia. No se simulan
heridas, parpadeo ni avisos de salud sin esas reglas y su reloj verificados.

La ventana de equipo sí tiene dibujo condicionado por `[0x544f]`,
`[0x5453..0x545d]` y `[0x544b]`, usando CASQUE[36..41]
(`0x14205..0x14372`). Las formas y coordenadas están identificadas, pero no se
asignan nombres definitivos a todos esos estados ni se inventan objetos poseídos.
La ventana queda vacía. No se ha confirmado un conjunto de iconos permanentes
de tarjetas/códigos en el HUD normal; los bancos de interfaces adicionales se
mantienen pendientes de rastrear.

## Rutinas reproducibles del ejecutable

| Offset de archivo | Evidencia |
| --- | --- |
| `0x68b9..0x68fd` | Carga `data\\Casque.spr` mediante el nombre en puntero 0x4a08 y prepara su primera tira. |
| `0x7ad4..0x7ae3` | Carga la paleta del nivel, 768 bytes. |
| `0x7efe..0x7f3d` | Carga los bancos BALLE/ARME seleccionados y selecciona el descriptor de arma. |
| `0x13984..0x13aa1` | Inicializa panel y barras, icono de arma, estado visual y copia las últimas 50 filas. |
| `0x13cb8..0x13d88` | Copia las esquinas superiores e inferiores y refleja el lado derecho. |
| `0x13db1..0x13f48` | Radar y copia de la silueta corporal en los offsets 54720 y 49725 del framebuffer. |
| `0x13fac..0x14014` | Calcula el segmento vacío de oxígeno y salud. |
| `0x14015..0x14091` | Divide el valor de munición en centenas, decenas y unidades y dibuja tres cifras. |
| `0x14092..0x140e7` | Copia con índice 0 transparente, usada por los mensajes. |
| `0x140e8..0x1413e` | Copia un prefijo de N columnas del gráfico vacío, diez filas. |
| `0x1413f..0x14183` | Copia de un dígito de 3×6 sin filtro. |
| `0x14184..0x14378` | Limpia y dibuja los iconos condicionales de equipo. |
| `0x14399..0x14599` | Cola, caracteres visibles, cursor y expiración de mensajes. |

No se ha demostrado que el casco pueda conmutarse con una tecla durante el
juego normal. El port lo muestra al entrar en 3D cuando sus recursos están
disponibles; no añade una tecla de ocultación. Las pantallas modales y sus
transiciones no están reconstruidas. No se afirma que el HUD permanezca visible
durante menús, cinemáticas, inventario o muerte.

## Arquitectura y conexión posterior

- `original_hud.h/.cpp`: carga y valida las dimensiones del banco, conserva los
  índices y prepara una tabla RGBA. Dibuja con copias y primitivas recuperadas.
  No depende de SDL, audio, puertas, movimiento o reglas del juego.
- `hud::State`: salud, oxígeno, selección opcional de icono de arma, munición,
  ocupación de botella/tarjetas/inyectores, fase opcional de radar y texto ya visible. Los campos son entradas visuales;
  `message` es una vista prestada válida durante `draw`. No existe inventario
  general ni reloj oculto dentro del renderizador.
- `FirstPersonRenderer`: renderiza la escena con los módulos existentes,
  copia su buffer a la zona superior del buffer compuesto y llama al HUD.
  Conserva buffers y textura entre fotogramas; redimensiona solo al cambiar
  el modo/dimensiones internas. La presentación usa el tamaño actual de salida.
- `main.cpp`: carga opcionalmente los recursos y pasa `survival.hudState()`.
  El dibujo no se acumula en main. El texto SDL de coordenadas se mantiene en
  mapa 2D y en la ruta sin HUD; no tapa la imagen original en la vista con casco.

Salud y oxígeno están conectados al estado real sin modificar el dibujo de
las barras. Arma/munición y equipo proceden ahora de `player::Inventory`.
Botella CASQUE[36] en (184,157), tarjetas [37] con x205 e inyectores [38]
con x215 usan las tablas Y originales de `0x14379`/`0x14391`; se cuentan
también envases vacíos. Véase `items-and-inventory.md` para transferencias y límites.
El controlador de mensajes ya suministra los avisos de seguridad/panel vacío;
el cursor y revelado progresivo siguen pendientes. Radar y heridas necesitan condiciones
y reloj originales. Menús de inventario, selección de sustitución y permisos
siguen pendientes, sin trasladar lógica al renderer. El slot de arma ausente se limpia en negro
como política de preview segura, no como un estado de desarme confirmado en DOS.

## Verificación y validación visual

```sh
cmake -S . -B build
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
./build/hud_tests ~/Desktop/In_Extremis/DATA
python3 tools/inspect_hud.py --data-dir ~/Desktop/In_Extremis/DATA \
  --exe ~/Desktop/In_Extremis/EXTREMIS.EXE
./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB
```

`inspect_hud.py` exige el SHA-256 verificado antes de usar offsets. Genera
`catalog.json` (dimensiones, anclajes, hash de píxeles y tablas de coordenadas),
`casque-catalog.png` y `executable-evidence.txt`. No escribe fuera de
`build/hud-debug/`. El runtime no usa Python, Pillow ni esas imágenes generadas.

Resultado de la fase visual inicial: compilación correcta; **14/14 suites CTest y 12/12 pruebas Python**
superadas. La prueba opcional compara los 86 payloads CASQUE con los bytes
originales y los 256 colores con PAL1; compone el nivel real con paredes,
planos, puertas, sprites y HUD. Produce `build/hud-debug/level1-hud.bmp`.
También se ha ejecutado `hud_tests` con recursos reales bajo AddressSanitizer
y UndefinedBehaviorSanitizer, sin errores. Las suites existentes cubren las
regresiones de audio, puertas, movimiento, colisiones y renderizado.

Los tests del HUD comprueban filas reservadas, espejo y longitudes de esquinas,
barra llena/media/vacía, dirección y redondeo, valores extremos, fuente,
transparencia, cifras, recursos inválidos/ausentes y framebuffer incorrecto sin
escrituras. La integración usa superficies y renderer software SDL, sin ventana
ni dispositivo físico de audio; verifica formato de presentación y alternancia
entre framebuffer con HUD y la ruta anterior. No se ha validado interactivamente
una partida DOS ni se ha implementado ninguna animación temporizada nueva.

En el port, pulsar **TAB** para entrar en 3D. Revisar barra verde superior,
naranja inferior, cuerpo, radar a la izquierda, slot vacío y arma a la derecha;
no deben quedar los grandes huecos verdes del panel crudo. Las esquinas del
casco deben cerrar los extremos de la escena. Redimensionar a 4:3, panorámico y
vertical: el conjunto se centra con bandas negras y conserva su aspecto.
WASD, ratón, E, O y F mantienen sus funciones. Al volver con TAB, el mapa no
debe incluir el casco. Comparar también puertas y pisadas; sus módulos no se
han modificado. Tras la integración vital el oxígeno disminuye en 3D activa;
las pruebas rápidas de daño/recuperación/muerte se describen en `player-vitals.md`.

## Diferencias y trabajo pendiente

No hay gráficos sustitutos del panel o las barras. Las diferencias explícitas
son las pantallas modales de equipo aún pendientes, la política para arma ausente,
la presentación VGA a 4:3 elegida por el port y la proyección 3D preexistente.
El recorte defensivo de mensajes y el clamp de valores evitan reproducir
accesos inseguros con datos ajenos al rango original. No son mecánicas DOS.

Quedan por recuperar las reglas y temporización de heridas, cursor,
cursor/revelado de mensajes, alertas y cambios de paleta; las pantallas de inventario,
tarjetas y códigos; y las condiciones exactas de ocultación/transición del HUD.
El estado vital, consumo, recuperación y muerte lógica se han incorporado en
un módulo separado; véase `player-vitals.md`. Los cuatro paneles del nivel 1
ya actualizan inventario/equipo; siguen pendientes combate, presentaciones de
muerte, disparos, menús y enemigos. No se han incorporado reglas de gameplay al renderer del HUD.

## Pantallas modales de interacción

`original_interactions` compone ahora las interfaces originales sobre el mismo
framebuffer, después del HUD. Conserva 320×200, viewport 320×150 y 4:3.
Teclado/armario/lector/plano usan ventana (62,25), 196×100; INVENT_A ocupa
(68,10), 184×180 y puede cubrir parte del HUD como en su disposición original.
PAL_DI se aplica a sus gráficos y fuentes sin recolorear ni editar los bancos.
No se ha reconstruido aún el fundido/cambio global de paleta de la escena DOS.

G abre y Espacio confirma; el HUD recibe el inventario resultante al cerrar,
sin cambios antes de la transferencia. Mientras el modal posee teclado/ratón,
no se ejecutan controles del mundo ni consumo vital. Véase
[original-interactions.md](original-interactions.md) para gráficos, coordenadas,
controles, fuentes, evidencia y diferencias de presentación.

## Paso 202: mira continua

La capa ARME[3] ahora se dibuja centrada en(160,75) también en reposo. Se conserva
el recurso y la composición cuerpo/fogonazo original. Es una diferencia deliberada
frente a DOS, donde solo aparece en disparo y su centro es(160,58). La trayectoria
se ha alineado con el eje óptico del renderer; véase [enemy-system.md](enemy-system.md).
Los controles vigentes son WASD/E/I/ESC del Paso201; las referencias históricas a
G/Espacio en este documento no describen los controles actuales.

## Paso 203: combate integrado

El origen visual de BALLE se deriva del centro de la capa de fogonazo ARME[2],
con sus offsets y recoil. El HUD sigue dibujando las mismas capas y mira estable;
no se han cambiado recursos. Véase [enemy-system.md](enemy-system.md) para separar
vuelo visual de colisión lógica. La salud refleja el ataque recuperado (2 unidades
durante8 pasos de reacción); los indicadores específicos de miembro y flash de
paleta DOS siguen pendientes. Las capturas step203 de hud_tests utilizan el
compositor real e incluyen estados MONS1 y salida del proyectil.

## Paso 204 — detector original funcional

La pantalla inactiva usa CASQUE[4] en(0,171),46×29. Cuando[0x5461]≠0,
0x13e00..0x13e28 usa `([0x5423]&7)+5` en índices desde0: CASQUE[5..12].
No se inventa una animación de barrido nueva. El port comparte el contador
normalizado18Hz de enemigos (adaptación temporal del Paso203, no un reloj
fijo18Hz demostrado para DOS). El contador avanza también con detector apagado.

### Cálculo de contactos, separado del dibujo

0x100f6..0x101e4 recorre registros de28bytes desde[0x519e], en orden original,
sin seleccionar por velocidad o IA. Incluye inmóviles y moribundos; solo omite
salud0xffff. Añade el origen del sector a XY y resta posición del jugador.
No consulta visibilidad, paredes, puertas, decoraciones ni proyectiles.
Hasta diez registros pasan a la lista[0x5463], antes del filtro de barrido.

0x10181..0x1019d y0x1117a..0x111ee giran con `φ=180−ánguloJugador` (720/vuelta).
Se usan literalmente las tablas Q14 del EXE, coseno en archivo0x3164 y seno
como su desplazamiento540 (archivo0x3704). Con deltas enteros globales:

```
x = floor((dy*sinQ14(φ) - dx*cosQ14(φ)) / 2^20)
y = floor((dy*cosQ14(φ) + dx*sinQ14(φ)) / 2^20)
```

Esto incluye el desplazamiento aritmético6: **64 unidades de mundo por unidad
de detector**. Se conserva el signo X de la rutina, sin imponer otro por analogía
con Alien. El ángulo del port se redondea a unidades720 y la posición continua
a coordenadas enteras; esa conversión es adaptación al movimiento del port.
Los deltas fuera del dominio signed16 se omiten en lugar de emular overflow.

La ventana original es `−27≤x<27`, `−3≤y<32`, no un radio circular. Equivale
aproximadamente a lateral[−1728,1728) y profundidad[−192,2048), antes de recorte.
La rutina de dibujo puede recortar por completo un contacto ya admitido.

0x13e8f..0x13ee7 calcula rumbo de `(-x,y)` mediante tabla atan original257bytes
(archivo0x3ca4), lo divide entre60, y obtiene
`d=abs(fase-rumbo/60-1)`. Solo dibuja si d≤2, con CASQUE[13+d]:5×5,3×3,1×1.
No se añade un wrap angular o una interpolación no presentes en esta rutina.
El origen del marcador es **(24+x,198−y)**. Los píxeles de índice0 son transparentes;
se recorta exactamente al rectángulo x0..45,y171..199. La base se repinta cada
frame, sin acumular estelas o puntos duplicados del frame anterior.

`detector::scan` produce una lectura pequeña e independiente del HUD: fase y
hasta10 coordenadas/intensidades. El HUD solo dibuja esa lectura. No mantiene
copias de enemigos, no calcula IA ni lanza rayos; observa los registros actuales.
Al retirarse un enemigo deja de aparecer; los efectos visuales independientes
no vuelven a entrar como detecciones.

### Batería, inventario y estados de uso

0x1d8be contieneDWORD−1 de batería del juego nuevo;0x6abd apaga el detector.
0x18fba..0x1900d admite batería de clase0 y convierte segundos a ticks×160.
0x16b5a..0x16bb8 consume un tick por interrupción nominal si radar u óptica está
activo, y pone0 a batería y dispositivos al agotarse. El port usa el mismo
reloj160Hz, con fracción y pausa/reanudación como Vitals. No cobra por punto,
barrido ni tecla. La óptica no está implementada y no consume carga adicional.

Se extiende la sección0 del inventario ya existente: si tiene carga, E abre sus
acciones; E confirma encendido/apagado y la otra opción sale. Conserva I/ESC para
cerrar y W/S/flechas para navegar.0x1bc72 habilita el radar y0x1bcb7 lo apaga.
Se mantienen textos recuperados como «Radar is running», «Eteindre le radar»
y «Battery utilized for the radar»; «Batterie vide» procede de archivo0x23152.
La opción de óptica DOS queda fuera del menú funcional del port; no se simula.
La sustitución de una batería todavía cargada continúa protegida por Capacity.

INVENT_A[11] representa ausencia y[16] presencia, en(199,66).0x14336..0x14372
muestra CASQUE[40] con batería presente y dispositivos apagados, o[41] activo,
en(222,156). El indicador y la animación se apagan con carga0. Los mensajes
específicos de agotamiento y el porcentaje de carga de la interfaz DOS aún
necesitan completar su integración; no se sustituyen por sonidos inventados.

El nivel1 real no contiene baterías en sus paneles y no se modifica su dotación.
Para comprobar el detector se usa la batería explícita **de diagnóstico**, no
una batería añadida al nivel: `--vitals-debug`, F11. Esta tecla está deshabilitada
en una partida normal. Después se puede apagar/encender desde I con los controles
existentes. La prueba comprueba que F11 no altere inventario en modo normal.

### Salpicadura y audio

GOUTTE.SPR añade16 imágenes originales para43 gotas escalonadas.0x13aa2..0x13b93
las compone sobre arma/retícula y debajo del casco, mensajes y modales, recortadas
a320×150; no toca el panel inferior ni el detector. El contador independiente
0..29 y los límites/recursos se detallan en [enemy-system.md](enemy-system.md).
Las rutinas de cálculo y dibujo del radar no solicitan audio; tampoco la de
GOUTTE. BANK17 «explo monstre» pertenece a la muerte, no al barrido o a cada gota.
El sonido genérico de menú ya existente se conserva, sin nuevos pitidos.

Capturas de verificación con recursos reales: `build/hud-debug/step204-death-*.bmp`,
`step204-radar.bmp` y `step204-review.png`. Muestran la composición de explosión,
gotas, arma, mira y HUD inferior, más la lectura del nivel original. Las tablas,
hashes y asociaciones se exportan con tools/inspect_enemies.py; motion_detector_tests con
EXTREMIS.EXE compara las720 entradas Q14 y257 de atan contra el ejecutable.
