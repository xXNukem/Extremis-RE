# Auditoría y correcciones de interfaces — 2026-11-02

Actualización vigente: [Paso 201 — controles e interfaces](step-201-interfaces-and-controls.md).
Los controles y el alcance descritos abajo corresponden a revisiones anteriores.

Este documento describe la revisión actual. Sustituye los diagnósticos anteriores
sobre resaltado, inventario, paneles 0x3002 y posición inicial. Los cambios locales
anteriores se han conservado; no se han realizado commits, pushes ni cambios a
recursos originales. C++17, SDL3, framebuffer 320×200 y presentación 4:3 continúan.

## Estado encontrado

El árbol contenía una implementación extensa sin commit: HUD, puertas, colisión,
objetos, salud/O2, audio, sprites, renderer y modales. La interfaz y la transferencia
ya estaban separadas: abrir no recogía, selección 0 aceptaba, 1 rechazaba; izquierda
restaba y derecha sumaba con límites. C001 desbloqueaba exclusivamente puerta 7.
Se han conservado esas operaciones y sus pruebas. No se ha reimplementado inventario.

Había errores en la representación y carencias concretas:

- CH_OBJ4A/CHARME4A[13] ya contienen las dos palabras brillantes. Los sprites
  [14]/[15] son máscaras oscuras (índice 220), no indicadores brillantes. Se
  oscurecía precisamente la opción que se iba a ejecutar: producía la aparente
  inversión horizontal y la discrepancia entre iluminación y confirmación.
- El blitter ignoraba anchorX/anchorY. El dedo de CHARME[7] tiene pivote (20,19);
  se dibujaba 20 píxeles a la derecha y 19 abajo respecto a DOS, sobre la palma.
- Los avisos de tarjetas se dibujaban en y146 sobre la primera entrada, sin
  retirar las filas anteriores.
- No había entrada al modo general del inventario.
- initialCamera buscaba el interior más despejado y una visual larga: (3500,900).
  No era una posición de inicio recuperada del DOS.
- Los dos paneles 0x3002 estaban excluidos. La documentación anterior los llamó
  narrativos incorrectamente: su ruta carga el selector de ascensor CH_LEVEL.

## Evidencia reproducible

EXE SHA256: `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Las direcciones siguientes son offsets absolutos de archivo; puntero de datos +0x200
se convierte en offset de archivo. `tools/inspect_interactions.py` verifica el hash
y exporta manifiesto, pivotes, plantilla, registros de ascensor y desensamblado a
`build/panel-debug`. No modifica EXE, GAME.LAB, SPR, fuentes ni paletas.

| Tema | Evidencia | Resultado actual |
| --- | --- | --- |
| YES/NO | 0x18f71..0x18fb4, 0x199dc..0x19a26; equivalente armas 0x8b04..0x8c18 | 0=Yes a (90,96), 1=No a (117,96); izquierda→0, derecha→1, sin wrap; confirmar 0 transfiere una vez |
| Máscaras | CH_OBJ4A/CHARME4A[13..15], PAL_DI | Fondo contiene brillo; [14]/[15] solo usan 0 y 220. Se oscurece la opción no seleccionada |
| Pivotes | 0x2314..0x2346, CHARME[7..9] | Se resta pivote antes de recortar; frames (20,19), (20,16), (18,10) |
| Mano | 0x196b4..0x19707, 0x19e6d..0x19f0f | Base CHARME[6], dedo [7..9], desplazamiento de anclaje +42,+9; mano sube 4 y 8 píxeles durante pulsación |
| Tarjetas | 0x1ce21..0x1ce70, 0x1d05a..0x1d0c5 | Región x72..247, y146..186; filas cada 11; aviso posee región exclusiva |
| Tecla I | [0x541f] contiene 0x17; 0x16df7..0x16e16, 0x6eda..0x6f29 | I abre inventario en modo 0; distinta de controles CFG serializados en 0x6826..0x6895 |
| Inventario | 0x1b9d0..0x1c038, 0x1c166..0x1c8b7 | INVENT_A[0], (68,10), seis categorías; marcas y134/110/86/60/41/14, recursos originales |
| Inicio | plantilla 0x1d8a0, carga 0x1de72..0x1de9c; sector0 GAME.LAB | local (300,320), ángulo180/720, sector0 origen(3200,0): global(3500,320), +Y |
| Panel inicial | GAME.LAB 0x402, sector1/pared6; 0xa60e→0x19a4f | 0x3606/0, código IJEKAH, transferencia de plano. No contiene inyector ni usa rama de suministro vacío |
| Ascensores | 0xa515→0xce1a, 0xcf54 modo2, 0xd0db..0xd1c4, 0xd63b..0xd941 | Paneles sector0/pared6 y sector9/pared6; selector familia E, cuatro filas, arriba/abajo/Espacio |

### YES/NO y diferencia explícita de presentación

La rama lógica previa ya era correcta. No se han intercambiado flechas ni acciones.
El EXE aplica la máscara oscura a la opción seleccionada. Esta revisión conserva
recursos, paleta, disposición, estado inicial y transferencia, pero invierte qué
opción se oscurece para cumplir el requisito explícito de **opción iluminada =
acción seleccionada**. No se afirma que esa convención luminosa sea idéntica al
DOS. Al mantener ambas palabras y oscurecer la opuesta, derecha desplaza el brillo
a No y confirmar no recoge. Izquierda desplaza el brillo a Yes y confirma recogida.
Se aplica también a armas. Escape cancela sin modificación; es extensión del port,
no se atribuye a DOS. Reabrir reinicia Yes. Repeticiones SDL se ignoran.

### Inventario y estados

I abre `Screen::Inventory` sin panel objetivo. Arriba/abajo eligen batería,
oxígeno, inyectores, tarjetas, mina y Exit. Espacio en botella/inyectores llenos
abre confirmación; Do not use rechaza. Con dos inyectores llenos se elige 1 o 2,
o Exit. Se usan las operaciones de Inventory/Vitals existentes; el envase queda
vacío. Las tarjetas se consultan: no desbloquean una puerta desde el inventario
general. Los lectores conservan su selección y validación contextual.

No hay objetos iniciales fabricados. Batería y mina se presentan ausentes porque
sus mecánicas no están implementadas en el modelo actual. No se incorporan radar,
óptica o colocación de minas. Faltan sus acciones, ANM de uso, valores numéricos de
carga del inventario y variantes completas de confirmación DOS. Las frases francesas
se traducen al inglés usando INVENT_T; los píxeles de los recursos se conservan.

`ItemSession` posee el modal y consume exclusivamente su teclado. Apertura no
confirma. La actualización intercambia/limpia solicitudes, ignora repeat y ventana
ajena, descarta acciones al perder foco y cierra tras muerte. Se mantienen pausa
de O2, puertas, animación y mensajes, y descarte del intervalo de reanudación.
Los sonidos ya iniciados siguen reproduciéndose. Escape sale del modal completo,
incluidos submenús (adaptación del port). No se cambia WASD/E/G/Tab.

Tarjetas incompatibles: el aviso sustituye las filas del menú; arriba/abajo restaura
el menú y cambia selección, Escape cierra. Sin tarjetas se muestra aviso y Exit.
No se oculta ni elimina el texto original de rechazo. Confirmar de nuevo una tarjeta
incompatible mantiene su aviso, sin cambiar inventario ni puertas.

### Ascensor recuperado y transición pendiente

Recursos añadidos: CH_LEVEL.SPR (10 imágenes) y TYPO_3X5.BIN (278 bytes), PAL_DI.
Vista estable (62,25), mano [6], dedo [7] con pivote; botones x142, y31/47/63/79;
mano x136, y87−16×selección, dedo anclado +42,+9 y offset vertical +4.
Etiquetas de 3×5 en x200, pares de líneas y82/88,66/72,50/56,34/40; color24.

Tabla EXE en 0xdacf, registros de 54 bytes. Cuatro rutas de cuatro bytes,
posición de llegada de tres words y 32 caracteres. Selección visual se transforma
con `(~selección)&3`; no se interpreta el índice como número de nivel.

- Registro0: una ruta activa a nivel0/sector0, etiqueta C200; posición(300,320,180).
- Registro1: rutas a nivel0/sector9 (C200) y nivel1/sector0 (B101);
  posición(300,280,540).
- 0xd63b..0xd692 valida pares nivel/sector en catálogo de tarjeta E; E009 evita
  ese filtro. E001 apunta desde 0xda45 a la lista en archivo0xda65 e incluye
  (0,0),(0,9),(1,0),(1,14),(2,0),(2,22),(3,9).

Se integran los dos paneles reales y E001/E009, sin inventar ubicaciones ni
movimientos. Otros IDs E no tienen todavía sus catálogos integrados. Los botones
inactivos no hacen nada; un destino habilitado y autorizado muestra
`DESTINATION UNAVAILABLE`, aviso del port, sin consumir tarjeta ni mover jugador.
**No hay viaje implementado, ni siquiera entre las dos cabinas del mismo nivel.**
Faltan la secuencia A_LEVEL, animaciones, carga/retorno de nivel y su persistencia.
La interfaz está separada de esa mecánica pendiente. Escape cierra.

### Audio

La cola de mensajes 0x14399..0x14414 no llama al reproductor. La rama de rechazo
0x961d..0x9634 solo prepara texto; la rama aceptada 0x95ab..0x95b3 solicita BANK[19].
No se ha demostrado un beep independiente para bloqueo, compartimento vacío o
rechazo: no se añade. Se conservan BANK[12] para confirmar Yes y No y BANK[28]
para teclas, más puertas[27], pisadas[7] y muerte[25/26].

BANK[11] está demostrado al desplegar/cerrar inventario (0x1baa7,0x1c038), selector
(0x1ac21) y panel de ascensor (0xcfcb,0xd539). Se solicita una vez en los bordes de
las nuevas interfaces, nunca desde render ni por mensaje persistente. Al omitir
zoom, la sincronización es con apertura/cierre del modal, no con frames DOS.
BANK[18] aparece en inserción de tarjeta (0xd095), pendiente con esa animación.
El nombre “moteur ascenseur” no justifica un bucle durante viaje ni otro sonido:
la condición y coordinación completas de la secuencia de viaje siguen pendientes.

## Validación y limitaciones

Las pruebas sintéticas ejercitan los dos sentidos desde ambos estados, límites,
selección inicial y reapertura, brillo lógico, Yes/No reales, rechazo sin cambio,
aceptación única, repeat, cancelación, pivote en 14 teclas, recorte, aviso exclusivo,
inventario, uso de inyector elegido, restricciones E, cierre de ascensor y transición
pendiente. Las pruebas con originales verifican máscaras de índice220, recursos,
puerta7/C001, nueve paneles, inicio sin colisión y ausencia de repetición BANK[11]
durante 100 actualizaciones. Se generan composiciones BMP en build/panel-debug.

La comprobación visual es de composiciones del renderer con recursos auténticos;
no se ha ejecutado una comparación interactiva DOS fotograma a fotograma. Animación
del dedo usa secuencia recuperada reposo/intermedio/pulsado/intermedio y offsets,
pero un paso por actualización del port: falta recuperar su ritmo DOS. No se
afirma reconstrucción cinematográfica completa. Se conservan limitaciones anteriores
de CFG, ratón modal, paleta global, cierre temporizado de lectores, sustitución
completa de recipientes y cadencia del plano. La persistencia es durante la sesión;
no se ha añadido guardar/cargar partidas.

Resultado final: compilación normal y con ASan/UBSan correctas, **18/18 CTest**
en ambas configuraciones y **16/16 unittest Python**. También pasan interaction,
inventory y door_control con recursos originales bajo sanitizadores, sin errores
reportados. `git diff --check` no encuentra problemas. La hoja de revisión visual
está en `build/panel-debug/review-contact-sheet.png`.

Comandos de comprobación:

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
./build/interaction_tests ~/Desktop/In_Extremis/DATA
./build/inventory_tests ~/Desktop/In_Extremis/GAME.LAB
./build/door_control_tests ~/Desktop/In_Extremis/GAME.LAB
python3 tools/inspect_interactions.py ~/Desktop/In_Extremis
ctest --test-dir build/interface-sanitized --output-on-failure
```

## Comprobación manual

Ejecutar `./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB --vitals-debug`.
Tab entra en 3D; el marcador inicial debe estar en(3500,320), mirando+Y, dentro de
la cabina. Las coordenadas siguientes sirven para localizar objetivos caminando;
no se ha añadido teletransporte de depuración.

1. G ante suministro, código **IJEKAH** si se solicita. Yes izquierda/No derecha.
   Derecha ilumina No, otra derecha permanece en No; Espacio cierra sin recoger.
   Reabrir: Yes iluminado; izquierda permanece, derecha→No, izquierda→Yes.
   Espacio acepta una vez. Revisar I y volver al armario: vacío o arma intercambiada.
   Escape desde cualquier opción no recoge. Mantener Espacio no duplica.
2. Teclado: mover mano por ambas filas y extremos; pulsar Espacio, observar índice
   adelantarse sobre tecla y volver. No debe quedar encima de la palma ni escapar
   del panel. Borrar arriba derecha; validar abajo derecha.
3. I: recorrer categorías, consultar E/C, botella e inyectores. Con inyector lleno,
   confirmar Do not use conserva salud/carga; Use injector aplica efecto y deja
   envase vacío. Dos llenos permiten elegir slot. Escape o Exit devuelve captura.
   Salud/O2 deben permanecer constantes mientras no se confirme uso.
4. E001: sector3/pared6, (4350,2300) mirando+X. C001: sector7/pared10,
   (1500,2950) mirando+Y. En lector7/17, (2500,2550) mirando+Y, G.
   Elegir E001: rechazo legible sin texto de tarjeta superpuesto. Arriba/abajo
   restaura filas; escoger C001 desbloquea. Cerrar y **E** abre puerta7.
   C001 debe permanecer. Repetir lector no re-bloquea.
5. Panel inicial1/6, (3250,1100) mirando−X: código→TRANSFER→mundo, sin inyector.
   No confundirlo con suministro ni con ascensor. Su extra0 no significa vacío.
6. Ascensor sector0/pared6: (3500,50) mirando−Y, o sector9/pared6:
   (2900,3350) mirando+Y. G. Sin E debe indicar falta de tarjeta y Exit.
   Con E001: seleccionar tarjeta→cuatro destinos originales, arriba/abajo mueve
   mano. Confirmar fila inactiva no mueve. Fila activa autorizada indica destino
   no disponible; no hay cambio de nivel. Escape cierra y conserva E001.
7. Escuchar confirmación tanto Yes como No, teclas, C001 aceptada y motor de las
   nuevas interfaces al abrir/cerrar. Un mensaje persistente no repite sonido.
   Comprobar también puertas, pasos y muerte con sus pruebas anteriores.

## Archivos de esta revisión

- include/src: original_interactions, item_session, level_items, player_inventory,
  game_audio (pares .h/.cpp).
- include/game_lab.h, src/game_lab.cpp, src/first_person_view.cpp.
- tests/interaction_tests.cpp, tests/inventory_tests.cpp, tests/first_person_tests.cpp.
- tools/inspect_interactions.py.
- docs/interface-audit-2026-11-02.md y referencias actualizadas en
  original-interactions, items-and-inventory, player-control y audio-system.

Los restantes archivos locales modificados o sin seguimiento ya existían al
comenzar esta revisión y se han conservado.
