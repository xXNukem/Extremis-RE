# Paso 206 — mapa original jugable

Implementación sobre `31effb0` y el trabajo local del
[Paso 205](step-205-original-functionality-audit.md), conservado. Sin commits,
pushes, recursos originales añadidos ni escrituras en la instalación DOS.

## Resultado y controles

El juego comienza ahora en 3D cuando hay cámara válida. **M** abre el mapa
original como adaptación de comodidad; **SDL_SCANCODE_SEMICOLON** conserva la
posición física PC original `0x27` (a la derecha de L, normalmente Ñ en un
teclado español). No se exige escribir el carácter `;` con Shift. Se usa
scancode, no traducción de keycode ni caracteres. No se ha probado físicamente
cada distribución de teclado.

**Espacio** cierra. Mientras el mapa está abierto, Tab, Escape, M, WASD, ratón,
E, I, Z/X, O, F y controles de diagnóstico no actúan sobre el juego. Escape
no cierra el mapa porque su uso no está confirmado en esta ruta DOS; el cierre
de ventana y Quit siguen funcionando. No hay zoom, desplazamiento, navegación
entre pisos, rotación del bitmap ni enemigos/objetos dinámicos superpuestos.

**Tab** conserva la vista técnica de depuración sin requisitos de plano.
La antigua previsualización de texturas N/M pasa a **AvPág/RePág** para no
competir con M. El título y los mensajes de consola indican el cambio.

## Recursos y composición

`original_map::load` usa `spr_format::load`, sin otro decodificador SPR:

| Archivo local | Uso |
|---|---|
| DATA/MAP.SPR | 30 imágenes en orden, índice cero para nivel 1; 89×69 en ese nivel |
| DATA/BOUSSOLE.SPR | Base 32×32, ocho direcciones y marcador 3×3 |
| DATA/PAL_DI.PAL | Colores de toda la interfaz |
| DATA/TYPO_3X8.BIN | Etiqueta de zona, color 102, avance de cuatro píxeles |

Se valida el contador exacto 30/10, tamaños positivos que caben en 320×200,
payloads, tamaños de brújula, fuente y paleta. El lector compartido valida
offsets, orden, truncamientos y dimensiones de cada registro. No se fuerza
que un fixture sintético tenga los 263298 bytes del MAP original, ni que
todos los mapas midan lo mismo. La comprobación opcional con instalación
local contrasta las 30 dimensiones originales y todos sus píxeles.

`original_map::placement` reproduce centro alineado a par, posición y ocho
orientaciones de `0x1a784..0x1a83a`:

```
x = ((320-w)/2) & ~1
y = ((200-h)/2) & ~1
markerX = x + 4*floor(camera.x/200) + 1
markerY = y + h - 4*floor(camera.y/200) - 4
direction = normalize720(round(camera.angle*360/pi)+45)/90
```

La cámara del port ya tiene coordenadas globales y el ángulo cero en +X,
positivo hacia +Y: `initialCamera` convierte el ángulo DOS multiplicando
por `2*pi/720`. No se añade giro, espejo ni compensación de posición. El
redondeo a la unidad angular DOS es una adaptación explícita de cámara
continua a las 720 orientaciones nativas. Se rechazan valores no finitos o
coordenadas fuera del dominio de words positivos; no se reproduce overflow.

Brújula base [0] en (8,16); dirección `direction+1` en índices C++:

| Dirección | Desplazamiento sobre (8,16) |
|---|---|
| 0 | 20,12 |
| 1 | 18,8 |
| 2 | 13,5 |
| 3 | 8,8 |
| 4 | 6,12 |
| 5 | 8,18 |
| 6 | 13,19 |
| 7 | 20,18 |

Son los words del archivo EXE `0x1a58b`. Marcador [9], equivalente a ID DOS 10.
La ruta `0x14092..0x140e7` **ignora anclajes**, a diferencia del dibujador
general de interfaces; tampoco se resta el anclaje al bitmap MAP. Se omite
solo índice cero. El port recorta a pantalla como protección; no limita el
marcador al rectángulo del mapa ni lo desplaza para hacerlo coincidir.

La etiqueta se dibuja en (6,52), `ZONE ` más cuatro caracteres de la tabla
EXE `0x60ef` (C200, B101…G629), copiada como datos técnicos, sin cargar el EXE
en runtime. No se confunde número de zona con banco de paleta/texturas.

### Fondo y paleta: nueva evidencia

Se releyó `0xa64a..0xa758`, llamado por `0x1a61b..0x1a6b1`. El mapa guarda
la escena y remapea cada índice a una rampa gris de PAL_DI mediante:

```
grayIndex = min(254,255-floor((R6+G6+B6)/6))
```

El primer tramo hace una transición de paleta; el segundo genera la tabla
de índices y la aplica a toda la imagen guardada. Por tanto el fondo no es
negro sólido y tampoco debe conservar el color normal del HUD.

La integración pequeña conserva una copia del framebuffer RGBA ya compuesto
en `FirstPersonRenderer`; sobre ella aplica la fórmula y dibuja MAP/BOUSSOLE/
etiqueta con PAL_DI. Mientras permanece abierto reutiliza esa copia: no
acumula conversiones, no vuelve a renderizar el mundo y no altera las paletas
o caches de texturas del juego. Al cerrar, la ruta normal reconstruye escena
y HUD. Conserva la presentación existente 4:3 y nearest.

**Límite de fidelidad del fondo:** el port ya convirtió las texturas a RGB y
aplicó su sombreado; recupera cada componente VGA con `(c8*63+127)/255` antes
del remapeo gris. Es exacto para un color de paleta expandido sin sombreado,
pero no recupera el índice DOS de un color sombreado. Los gráficos del mapa,
brújula y texto sí usan sus índices y PAL_DI directamente. No se intentó una
inversión RGB→índice ni una refactorización del renderer. Se reproduce el
estado final gris; queda pendiente la transición progresiva de apertura del
DOS, que no se sustituye por un fundido inventado.

## Permiso y estado modal

`game_lab::Level::zoneIndex` identifica el registro del nivel, desde cero.
El loader existente sigue cargando únicamente el nivel 1; su índice es cero.
No se implementaron viajes ni un selector de pisos. Quien cargue otro nivel
deberá pasar su índice real; no derivarlo de palBank/dalleBank.

`ItemSession` conserva **30 permisos** propios. `load(level)` cambia la zona
activa sin borrar permisos de otras zonas. `planTransferred()` consulta la
activa; la sobrecarga con índice permite comprobar otra zona. El evento que
concede el permiso sigue siendo la finalización de la transferencia existente
en la pantalla Plan, después del código válido. Aceptar el código, abrir el
terminal o cerrar una transferencia incompleta no concede el permiso.
`resetPlans()` limpia permisos para una partida nueva; construir una sesión
nueva también. No existe soporte de partidas guardadas adecuado al que conectar
esto, ni se añadió persistencia en disco.

El mapa comprueba juego 3D activo, foco, jugador vivo, ausencia de otro modal
o interacción pendiente, bloqueo de enemigos, permiso y recursos disponibles.
No consume ni requiere batería, tarjeta o munición. Cerrar/reabrir conserva
permiso. Cambiar `zoneIndex` cierra el mapa anterior y descarta su reloj/cámara.
Si faltan recursos, se informa en consola y no aparece un modal invisible ni
se muestra automáticamente el mapa técnico como sustituto.

### Denegación y bloqueo EXE

`0x6fc7` usa cadena de archivo `0x21972`:
`ATTENTION: AN ALIEN IS NEAR!!`.
`0x6fe7` usa cadena `0x21990`:
`ATTENTION:\r\n  ACCESS NOT POSSIBLE WITH CARD!!`.

Se conservan los textos literales mediante la cola de mensajes existente
(deduplicación y duración ya implementadas), incluso la referencia confusa a
CARD: **la condición sigue siendo el plano transferido**, no una tarjeta física.
No se añadieron pitidos ni mensajes modernos al HUD.

Nueva comprobación `0xe05e..0xe0c2`: `[0x5200]` se limpia y se activa al
encontrar un enemigo no retirado (`health!=0xffff`) cuyo sector figure en
la lista visible `[0x5128]`. No es un radio de proximidad.

**Límite explícito:** el port no reproduce esa lista DOS completa de portales.
`enemies::System::blocksInterface` es una consulta const que reutiliza el
criterio ya usado por su activación: mismo sector o enemigo dentro del campo
de visión sin pared/puerta interpuesta. No cambia registros, salud, movimiento
o IA. Puede diferir del DOS con enemigos ocultos dentro de un sector visible.
La consulta se usa para acceso al mapa; no se cambiaron los permisos del
inventario existente. Recuperar la lista de sectores es un bloque posterior,
no una distancia inventada para este cambio.

### Tiempo, entrada y reanudación

`original_map::Session` posee cámara/zona congeladas, estado de apertura,
teclas retenidas y reloj de marcador. Se separa del renderer y de los permisos.
Su estado modal participa en la misma condición de actividad de `main` que
las interfaces: pausa enemigos, movimiento, puertas, disparos/proyectiles,
efectos, animación decorativa, salud/O2, batería y mensajes. No altera sus
algoritmos de simulación.

Esto corresponde al bucle `0x1a84d..0x1a875`: dibujo y entrada sin llamar a
actualizaciones vitales (`0x16ad8` O2, `0x16b5a` batería) ni enemigos; el retorno
por `0x7001` reajusta contador temporal. `previousTicks` continúa renovándose
en el port; las transiciones de apertura/cierre descartan su intervalo y los
sistemas existentes descartan el primer frame de reanudación. No se cobran
segundos consultando el mapa al cerrarlo.

Las liberaciones de teclas se siguen observando; ratón, disparo y movimiento
pendientes se invalidan al cambiar de contexto. Abrir y cerrar en una misma
bomba de eventos no permite que una E posterior actúe sobre una puerta.
Una petición de inventario anterior tiene prioridad sobre M. Se ignoran
repeticiones y eventos de otra ventana. Recuperar foco descarta el intervalo
sin foco del reloj del mapa.

Intermitencia: inicialmente oculto, alterna tras **21 ticks nominales a 160 Hz**,
por la condición estricta `>20` de `0x1aa60..0x1aa73`. Semiperiodo 0,13125 s;
periodo 0,2625 s. Se conserva fracción/remainder para independencia de FPS.
**Adaptación temporal:** el DOS reinicia el contador al observar >20, pudiendo
perder ticks por latencia de su bucle; aquí no se reproduce ese retraso variable.
La cadencia nominal tiene evidencia, pero no se afirma medición interactiva DOS.

## Archivos

| Archivos | Cambio |
|---|---|
| include/src original_map.h/.cpp | Carga, validación, fórmulas, paleta y composición |
| include/src map_session.h/.cpp | Entrada física, modalidad, apertura/cierre y reloj |
| include/src interface_font.h/.cpp | Fuente pequeña extraída del renderer existente y reutilizada |
| src/original_interactions.cpp | Delega la misma fuente y validación, sin cambiar las pantallas |
| include/src item_session.h/.cpp | Permisos por zona y limpieza de entrada |
| include/game_lab.h | Identidad de zona explícita, sin modificar el formato del loader |
| include/src game_messages.h/.cpp | Dos cadenas DOS de denegación |
| include/src enemy_system.h/.cpp | Consulta const del bloqueo, sin cambiar simulación |
| include/player_input.h | Invalidación de captura al cambiar de contexto |
| include/src first_person_view.h/.cpp | Composición modal sobre copia del framebuffer y restauración |
| src/main.cpp, src/texture_preview.cpp | Enrutado, pausa, arranque 3D y conflicto de M resuelto |
| tests/original_map_tests.cpp | Fixtures, recursos locales opcionales, acceso, reloj y render |
| tests/interaction_tests.cpp | Permisos persisten al cambiar de zona; reset explícito |
| tests/player_input_tests.cpp | Eventos agrupados y prioridad entre interfaces |
| CMakeLists.txt | Fuentes y suite original_map |

Los archivos/documentos locales del Paso 205 siguen presentes. No atribuir
todos los cambios de `git status` exclusivamente a esta implementación.

## Verificación

```sh
cmake --build build -j4
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
./build/original_map_tests build/map-test-data "$HOME/Desktop/In_Extremis/DATA"
```

Se usan fixtures sintéticos para selección de los 30 registros, tamaños
variables, índice transparente, anclajes ignorados, colores, ocho orientaciones,
límites de coordenadas/framebuffer y recursos erróneos/ausentes. Se comprueban
denegación, ausencia de requisitos de objetos, bloqueo, transferencia real
en fixture, independencia de zonas 0/29, retorno, reinicio, reaperturas,
tecla física/repetición/otra ventana, consumo pausado y reanudación.
El reloj se prueba en límites 20/21/42 ticks y a 30/60/120 FPS.

La prueba opcional carga el GAME.LAB real, interactúa con el terminal en
(3250,1100) mirando −X, introduce **IJEKAH mediante las acciones del teclado**,
espera la transferencia y abre el mapa. No pone directamente el permiso a true
ni añade un bypass al juego. Comprueba que el inicio no tiene bloqueo de aliens,
que el modal no avanza enemigos y que, tras cerrar y reabrir, escena/HUD/paleta
se restauran **byte a byte** en la salida SDL software. Genera únicamente bajo
build dos BMP de revisión; no son capturas del DOS original.

Resultado final: compilación completa correcta, **22/22 suites CTest**,
**16/16 tests Python** y prueba adicional con los recursos originales aprobada.
Logs locales: `/tmp/step206-build.log`, `/tmp/step206-ctest.log`,
`/tmp/step206-python.log`, `/tmp/step206-originals.log`. Las advertencias de
librerías estáticas duplicadas del enlazador no impiden la compilación.
No hay nueva comparación interactiva con DOS ni validación mediante
teclado físico del recorrido completo; las capturas son del port y la
transferencia automatizada utiliza la lógica real del terminal.

## Prueba manual del nivel 1

1. Ejecutar `./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB`.
   Comienza en 3D; no usar F11 ni diagnósticos para conseguir el plano.
2. Pulsar M: debe seguir en juego y mostrar el mensaje DOS de acceso denegado.
3. Recorrer hasta el terminal de plano en (3250,1100), mirando −X. Tab permite
   localizarse en el mapa técnico; volver a 3D para interactuar. E abre teclado.
4. Con flechas/WASD seleccionar I,J,E,K,A,H y confirmar cada una con Espacio/E;
   seleccionar validación abajo a la derecha. Esperar TRANSFER y regreso a 3D.
5. Pulsar M o la posición física de semicolon (Ñ habitual en ES). Debe aparecer
   el bitmap verde con fondo gris, brújula, ZONE C200 y marcador intermitente.
6. Comprobar que WASD, ratón, disparo, E, I, Tab y Escape no actúan. Mantenerlo
   abierto diez segundos; O2/batería/enemigos no deben recuperar ese tiempo.
7. Cerrar con Espacio. Moverse en el mundo y reabrir para comprobar marcador;
   no se mueve al jugador desde el mapa. Repetir con ocho orientaciones.
8. Cerrar/reabrir varias veces: mismo permiso, colores normales al volver,
   sin manos/disparos pendientes. Cerca de enemigos puede denegarse el acceso;
   alejarse o resolver el encuentro, sin deshabilitar la condición.

Si se quiere revisar sin recorrer el nivel, usar el ejecutable **de pruebas**
anterior y `build/map-debug/level1-map.bmp`, `level1-marker.bmp`. Esto está
separado del juego normal y completa la transferencia mediante el terminal;
no hay tecla o parámetro de ejecución para conceder el mapa gratuitamente.
