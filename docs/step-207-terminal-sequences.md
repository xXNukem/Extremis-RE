# Paso 207 — secuencias de taquillas y terminales

Se reconstruyen las fases gráficas con SPR/ANM originales; no se cambian el
inventario, las condiciones del mapa, los destinos de ascensor ni otras
mecánicas. Se conservan los cambios locales anteriores. Sin commits ni pushes.

## Evidencia y resultado

Referencia: EXTREMIS.EXE SHA256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Offsets absolutos de archivo; los punteros de datos corresponden a archivo +0x200.
Se han leído los bloques de interacción, sin repetir la auditoría general.

| Secuencia | Evidencia DOS | Implementación |
|---|---|---|
| Aparición/cierre | 0x19bf7..0x19c6c, 0x191dd..0x192c3 | Ventana que crece/se recoge alrededor de (160,75), 4..100 en pasos de 4; entrada y retirada de la mano |
| Código | 0x19ca8..0x19d32, 0x19f5e..0x1a01c | Prompt original, `OK...`, `WRONG CODE!!...` y `CODE ZONE C200 ALREADY SEIZED`; espera de 70 retrazos antes de continuar |
| Puerta del panel | 0x189c1..0x18b51; 0x1a027..0x1a1bd | CH_OBJ2/CHARME2/B_PLAN2: desplazamiento y reducción de imagen ampliada; CH_OBJ3/CHARME3/B_PLAN3: dos hojas, máscaras laterales y recorte x100..218 |
| Presentación del objeto | 0x18cb6..0x18f2b; tablas 0x1835e y 0x7fe2 | Expansión de fondo, desplazamiento del objeto, aparición del display Yes/No con posiciones propias de suministro/arma |
| Recogida confirmada | 0x19429..0x194c2 y 0x8d2a..0x8dbe | Cierre seguido de A_ARME.ANM, tanto suministros como armas; No no reproduce la película |
| Lectores C | 0x952b..0x9676, 0x9883..0x99a1, 0x99a2..0x9aae | Inserción, LEDs del lector de puerta, `CONTROLLER CARD..`, texto progresivo (`CONTROL PANEL....` / `CARD ACCEPTED....` en consulta de códigos), resultado temporizado, retirada y cierre; decisión de acceso después de insertar/comprobar |
| Ascensor E | 0xd03f..0xd0d9, 0xd7d9..0xd813 | Tarjeta CH_LEVEL[5], trayectoria x62→78 / y125→41, recorte desde y54; entrada de mano en 19 pasos. Conserva selector/destinos existentes |
| Plano inicial | 0x1a348..0x1a56b | TRANSFER, contador, zona, hojas de apertura y permiso únicamente al completar |

La cinemática A_ARME.ANM es la confirmación del personaje, no un vídeo de un
arma concreta. Ambos llamadores apuntan a ese mismo archivo. ANM1 contiene
paleta VGA de 768 bytes y cinco fotogramas 320×200: primero completo, después
parches con 200 pares skip/count. `0x14b5a..0x14d18` confirma LZSS: ventana de
4096 bytes inicializada a espacios, cursor 4078, flags MSB primero, referencias
12 bits y longitudes 3..18. El lector valida tamaños, spans, firma, paleta y
truncamientos. La película usa su paleta propia y devuelve el renderer normal
al terminar; no modifica la paleta del mundo. `0x1470f..0x14959` y los argumentos
de ambos llamadores confirman espera estricta >32 ticks a 160 Hz por frame,
entrada por rampa de 4 y salida por rampa de 2. La primera espera queda absorbida
por la entrada de paleta nominal, como en esa ruta DOS.

Audio nuevo vinculado a fases, nunca a repintados: BANK[11] ventana/puerta del
panel, [27] hojas, [18] tarjeta y [26] confirmación cinematográfica. Se mantiene
[19] aceptación de tarjeta y el audio previo de teclas/Yes-No. Son peticiones
únicas; no se introduce un bucle global que afecte a sonidos del mundo.

## Terminal inicial: por qué parecía no informar

Sector 1 / pared 6, `0x3606`, continúa siendo el terminal del plano C200 y no
entrega objetos. Había texto TRANSFER en el renderer, pero faltaban `OK...`,
las etapas de apertura y, sobre todo, el tiempo correcto: el port terminaba en
26 actualizaciones. La revisión del callee **0x1a54b..0x1a564** demuestra que
el contador de transferencia baja **una unidad cuando [0x1428]>8**, no cuatro
por iteración. Los pasos de cuatro de 0x1a1d1..0x1a1e5 mueven las hojas.

Esta evidencia corrige expresamente la interpretación temporal de los informes
anteriores, incluidos 206 y 206B. Se mantienen la clasificación del terminal,
la validación del código y el evento que concede el mapa. El contador nominal
100→0 requiere **900 ticks / 160 Hz = 5,625 s**, empieza con las hojas y muestra
cero durante el cierre. `plans_[zone]` solo se activa al completar; cancelar
antes no lo activa. Reabrir una zona autorizada muestra el aviso original antes
de repetir la transferencia. No se inventa un mensaje HUD de «objeto recogido»
ni de éxito del mapa: el feedback original está en el display.

Antes de disponer del plano, M conserva literalmente
`ATTENTION:\r\n  ACCESS NOT POSSIBLE WITH CARD!!`. También se conserva el bloqueo
por enemigos y su limitación ya documentada; no se modifica su cálculo.

## Arquitectura y controles

`ItemSession` conserva operaciones de inventario/puerta y permisos. Las fases
están en `item_animation.cpp`; el renderer de secuencias en
`interaction_animation.cpp`; el códec ANM no depende de SDL ni de la sesión.
`main` entrega tiempo transcurrido a la sesión. Se pausa con pérdida de foco,
se descarta el primer intervalo al reanudar y se consumen acciones durante las
transiciones sin ejecutarlas después. Los recursos faltantes/incorrectos siguen
impidiendo abrir interfaces invisibles; el audio no es requisito para avanzar.

Se conservan E/Espacio, flechas/WASD, I y Escape. Escape sigue siendo cancelación
inmediata del port, también durante animación; no deshace una recogida ya
confirmada. La convención validada «opción iluminada = acción seleccionada»
permanece. Un código erróneo permite reintentar/Escape tras su aviso, conservando
la adaptación previa, en vez del cierre del modal que hace esta ruta DOS.

## Límites explícitos

Confirmados: recursos, orden principal de fases, trayectorias/tablas indicadas,
textos, contador del plano, códec, condiciones de recogida/permiso y película.
No hay una nueva comparación interactiva DOS fotograma a fotograma.

Las iteraciones geométricas sin reloj demostrado se presentan a **70 pasos/s
nominales**, con cuantización al frame del port: es adaptación, no una medición
de su duración DOS. Las esperas de retrazo se convierten usando 70 Hz nominales;
las esperas del PIT usan 160 Hz. El plano conserva fracciones; no se reproducen
retrasos de polling DOS. El cambio global de paleta del fondo modal, bordes
intermedios, movimientos completos entre teclas, pulsaciones de botones del
ascensor y todos los detalles del dedo siguen pendientes. La rampa ANM se aplica
a RGB expandido y no pretende una comparación exacta de cada paso VGA.

El selector de tarjetas ya existente se conserva con apertura por ventana;
su cierre intermedio antes del lector y las animaciones de sustitución de
recipientes llenos no están reconstruidos completos. No se reconstruye viaje
de ascensor ni A_LEVEL; tampoco se añaden películas de uso INJECT/OXYGEN.
No se reproduce la repetición de BANK[12] condicionada por bit16 del contador
del plano ni todos los sonidos/transiciones de voz del DOS.

## Archivos de este paso

- Modificados: `CMakeLists.txt`, `include/original_interactions.h`,
  `include/item_session.h`, `include/game_audio.h`, `src/original_interactions.cpp`,
  `src/item_session.cpp`, `src/game_audio.cpp`, `src/main.cpp`,
  `tests/interaction_tests.cpp`, `tests/original_map_tests.cpp`.
- Nuevos: `include/anm_format.h`, `src/anm_format.cpp`,
  `src/interaction_animation.cpp`, `src/item_animation.cpp`, `tests/anm_tests.cpp`
  y este documento.

Los demás archivos que aparecen en git status pertenecían al trabajo anterior.

## Validación

Compilados InExtremisPort y las cuatro suites pertinentes: original_interactions,
original_map, player_input_events y original_anm. **4/4 correctas**, también
con ASan/UBSan. Pruebas adicionales con DATA/GAME.LAB originales: carga y
composición de secuencias, cinco frames ANM, transferencia real con IJEKAH y
apertura/cierre del mapa con restauración de escena/HUD.

Se comprueban 900 ticks a 30/60/120/160 FPS, ausencia de permiso anticipado,
cancelación, foco, confirmación consumida durante apertura, desbloqueo de puerta
tras insertar tarjeta y cierre automático del lector; también LZSS con referencias
solapadas, deltas vacíos, conservación de píxeles y datos truncados/incorrectos. Se corrigió además la inicialización del origen de
un sector sintético del test de mapa, que era indeterminada bajo sanitizadores.
Las pruebas de lógica existentes esperan las fases; no existe bypass de animación
ni concesión directa de permisos para tests. Composiciones revisadas en
`build/panel-debug/step207-sequences.png` y BMP `sequence-*` / `weapon-movie-*`.
Logs locales `/tmp/step207-*.log`. Sin recursos originales añadidos al repositorio.
