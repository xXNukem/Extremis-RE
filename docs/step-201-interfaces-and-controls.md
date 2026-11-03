# Paso 201: controles e interfaces

Actualización: [disparo del arma equipada — fase 1](equipped-weapon-fire.md).
Las menciones anteriores a ausencia de disparo quedan sustituidas por ese informe.

Esta revisión conserva el árbol local anterior y los recursos originales. No hay
commits ni pushes. El mensaje de requisitos recibido termina en «Verifica qué»
dentro de 8.3; se han completado los requisitos disponibles. Para carrera y armas
se ha aplicado el alcance mínimo descrito aquí; no se presume una continuación
que no se ha recibido.

## Auditoría inicial y causas

- E pertenecía a PlayerInput (puertas), G/H/J a ItemSession; los modales solo
  aceptaban flechas/Espacio. Eso impedía el esquema contextual solicitado.
- I solo abría; no existía su rama de cierre. ESC ya se reconocía, pero update
  descartaba cancelación antes de procesarla si la cámara/simulación no estaba
  activa. Se reproduce ese caso en prueba, sin atribuirlo a un evento de foco
  concreto no observado de la sesión manual.
- Los lectores calculaban «hay tarjeta de la familia requerida» y, si era falso,
  borraban la lista visible. Una E001 sola ante lector C aparecía como ausencia;
  al añadir C001 se mostraban ambas. No era un contador >1 ni un off-by-one.
- El ascensor navega sobre índice0 abajo..3 arriba: y=87−16×índice para la mano.
  ABAJO sumaba, provocando ascenso visual. La confirmación ya usaba ese índice;
  se conserva su transformación de rutas y se corrige la navegación.
- BANK[11] tiene un bucle nativo DSS. El port lo iniciaba una sola vez por apertura,
  pero no paraba la voz después del zoom omitido. Repetir aperturas podía acumular
  instancias. La prueba anterior solo contaba peticiones: no comprobaba finalización.
- YES/NO, pivotes de dedo, avisos de tarjetas y transferencia de plano inicial ya
  estaban corregidos según las pruebas disponibles. Se conservan sus algoritmos,
  recursos y coordenadas y se añaden regresiones con el nuevo control principal.
- ARME0..5 solo aportaban su imagen0 al icono del HUD. Faltaba su imagen1 de reposo
  en primera persona. El propietario de arma/munición seguía siendo Inventory.

## Controles actuales y propiedad de la entrada

| Control | Mundo 3D | Modal |
| --- | --- | --- |
| WASD | Desplazamiento | Navegación visual correspondiente |
| E | Panel válido cercano; si no lo hay, puerta | Confirmar/pulsar |
| Z | Usar inyector existente | Sin consumo |
| X | Usar reserva O2 existente | Sin consumo |
| Shift izquierdo/derecho | Correr mientras se mantiene y se intenta desplazar | Sin movimiento/consumo de carrera |
| I | Abrir inventario | Cerrar inventario, incluidos submenús |
| ESC | Conserva salida del juego | Cerrar/cancelar; tiene prioridad |
| Tab | Mapa2D/vista3D | Capturado por interfaz |
| Ratón | Mirar | No modifica cámara |
| Flechas/Espacio | Flechas conservan giro existente | Alternativas de navegación/confirmación |

G/H/J dejan de tener acciones de interacción o consumibles. No se añaden otras
teclas obligatorias.

ItemSession concentra las acciones discretas y el arbitraje panel/puerta usando
World::target y DoorControl::interact existentes. PlayerInput conserva captura,
movimiento continuo, ratón y actualización de colisiones/puertas. Se ha retirado
su segundo procesador de E. Un panel impide también activar una puerta detrás.
No se sustituye el sistema de seguridad ni se hace búsqueda automática de tarjeta.

Cada tecla tiene seguimiento KeyDown/KeyUp: repeat y otro KeyDown sin liberación
no crean una segunda acción. Perder foco descarta pendientes y libera ese estado.
I/ESC de apertura pendiente se capturan incluso antes de actualizar el modal;
no llegan a la salida de la aplicación. ESC se procesa antes de la condición de
simulación activa y no lo sobrescribe otra tecla del mismo lote. E de apertura
no entra a confirmar; confirmar/cerrar tampoco reenvía E al mundo.

Main resuelve el cambio de contexto antes de consultar movimiento continuo.
Al recuperar captura se exige liberar WASD/flechas/Shift que quedaron mantenidos
desde el modal o la pérdida de foco. Esto evita que la misma pulsación de navegación
mueva al jugador al cerrar. No se ha creado un framework de UI ni un segundo
inventario.

## Comportamientos corregidos

Inventario: I y ESC cierran vacío, con objetos y desde confirmación de uso. Exit
sigue funcionando mediante E. Cerrar no cambia objetos, salud ni O2; la sesión
conserva tarjetas y envases. Se han probado aperturas/cierres consecutivos y
cancelación cuando update recibe active=false.

Tarjetas: se muestra cualquier tarjeta poseída, aun incompatible, con 0/1/2/3
entradas y Exit. La compatibilidad se decide al confirmar y muestra el rechazo
existente. C001 sola desbloquea únicamente puerta7 y permanece en inventario;
la puerta se abre con una nueva E tras cerrar el resultado. Lector desactivado
no vuelve a bloquear. Se mantiene la sustitución de tarjetas ya implementada.

Ascensor: W/flechaarriba aumentan índice (sube mano), S/flechaabajo disminuyen
(baja mano). Límites0..3. E confirma la ruta correspondiente a la mano; no se
invierten índices de destino ni etiquetas para esconder el fallo. Se conservan
E001/E009 y los dos paneles existentes. Sigue pendiente el viaje, incluyendo
cambio entre cabinas: no se añaden desplazamientos arbitrarios ni nuevos niveles.

YES/NO: A=izquierda/Yes, D=derecha/No, E confirma. La máscara oscura se aplica a
la opción contraria, como se decidió en la revisión anterior: brillo=acción.
No/Escape conserva objeto; Yes transfiere una vez. Estado inicial y reapertura
reinician Yes. Flechas/Espacio continúan funcionando. Es la convención luminosa
solicitada para el port; no se afirma que la máscara seleccionada DOS sea brillante.

Dedo y textos: se conservan los pivotes de CHARME[7..9], composición independiente,
transparencia y secuencia de pulsación. Continúa pendiente su ritmo histórico.
Los avisos siguen teniendo su propia región inferior de inventario, sin mezclar
filas del menú; navegación restaura la lista. No se eliminan mensajes.

Panel inicial: sector1/pared6, 0x3606/extra0, GAME.LAB0x402. Continúa siendo código
más transferencia de plano, no un armario de suministros ni un inyector. Las ramas
de vacío no se aplican a su extra0. No se altera su contenido ni se añade objeto.

## Audio y evidencia

Referencia EXE SHA256:
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Offsets absolutos; punteros de datos se convierten a archivo sumando0x200.

BANK[11], «moteur ascenseur»: 3853 muestras a5000Hz, loop_flag=65535,
loop_begin=0, loop_end=3852, metadato continuous=255. No se ha confundido ese
metadato con una orden de repetición; el loop del propio DSS basta para reproducir
el fallo. La reproducción nativa del mezclador era correcta: faltaba su ciclo de vida.

- 0xcfcb / 0x1baa7: solicitar motor para despliegue.
- 0xd03a / 0x1bb0f: llamar a0x13090 después del zoom.
- 0x13090..0x13102: limpiar solicitudes y parar canales mediante0x464a.
- 0x13123..0x13159 y0x45ab..0x4649: petición directa, configuración del loop
  DSS y sustitución de voz DOS.
- 0xd539 /0x1c038: petición de cierre; también hay parada al acabar esa secuencia.

Como las vistas actuales omiten el zoom, GameAudio::interfaceMotor solicita
Playback::Once: reproduce los3853 frames completos una vez, sin loop DSS ni
repeat forzado. La duración sale del payload (0,7706s), no de un temporizador
inventado. Repetir la petición sustituye su voz previa; no apila motores.
Puede oírse una cola corta tras cerrar, pero siempre termina. No se hace stop-all
ni se cortan puertas, pisadas, mensajes o muerte. El modo Native sigue siendo
el predeterminado: BANK[26] conserva su loop y se prueba que la terminación del
motor no lo detiene. La sincronización cinematográfica exacta queda pendiente.

## Carrera

Input añade run; Settings conserva moveSpeed300 y añade runMultiplier2. Shift
modifica únicamente el vector de desplazamiento normalizado: no multiplica giro
ni sensibilidad, y diagonales siguen teniendo la misma velocidad total. Se
reutiliza el barrido de colisión, incluida la protección de puertas. Soltar Shift
vuelve a marcha sin persistir un toggle. Pasos siguen la distancia efectiva.

Evidencia:0x1658d..0x165b5 fija paso base y límite doble;0x16726..0x16778 usa
el modificador[0x5409], acelera/decelera y limita velocidad.0x16ad8..0x16b00
duplica consumoO2 con ese modificador. Se conecta el parámetro ya existente de
Vitals: caminar160ticks/s; correr320ticks/s. Salud no se modifica por correr.

Adaptaciones explícitas: velocidad base300 ya era provisional; el port aplica
instantáneamente factor2 al desplazamiento, también lateral y hacia atrás, sin
reconstruir la aceleración/condiciones de marcha atrás DOS. El O2 doble se aplica
cuando Shift y desplazamiento se intentan, no estando quieto. No hay stamina,
fatiga, balanceo de cámara o daño de carrera inventados. Los requisitos detallados
de la continuación del prompt no están disponibles para contrastar otras preferencias.

## Arma original en primera persona

Se reutiliza hud::Resources/Renderer y PAL del nivel. ARME[n][0] continúa como
icono57×30; ARME[n][1] se compone transparentemente en viewport, antes de casco,
mensajes y modal. La selección proviene de Inventory, incluido intercambio de
armario; no hay una selección visual independiente. Morir retira el arma del viewport.
No se añaden disparos, combate, fogonazos ni animaciones especulativas.

0x7f28..0x7f3d selecciona descriptor de112bytes desde datos0x54d7
(archivo0x56d7).0x1810f..0x18153 selecciona spriteID2 (índice1) en reposo y
lee coordenadas+8,+10.0x18267 confirma blit de píxeles del recurso.

| ARME | Tamaño | Esquina nativa x,y |
| --- | --- | --- |
| 0 | 25×28 | 148,122 |
| 1 | 33×37 | 144,113 |
| 2 | 26×34 | 147,116 |
| 3 | 27×50 | 147,100 |
| 4 | 46×33 | 137,117 |
| 5 | 25×47 | 148,103 |

Todas terminan en y150. Se conserva transparencia0, colores originales, 320×200,
viewport320×150, escala nearest y aspect ratio. Hoja visual con escena original:
`build/hud-debug/step201-weapons-sheet.png`. No se modifica ningún SPR.

## Pruebas y comprobación manual

Se amplían suites existentes para:

- WASD/E y flechas/Espacio; KeyUp, repeat, otra ventana, foco y apertura sin confirmar.
- I/ESC repetidos vacío/con objetos; I+ESC antes de update; cierre en estado inactivo.
- 0/1/2/3 tarjetas, E001 incompatible visible, C001 sola, puerta7 y conservación.
- Ascensor cuatro posiciones, límites, dirección de mano y confirmación de ruta.
- A/D/E de recogida, máscaras y no duplicación; panel no activa puerta detrás.
- BANK[11] real: Native sigue en loop, Once consume3853frames y termina,
  veinte reinicios mantienen una voz; muerte26 continúa independiente.
- Carrera y diagonales, 60FPS/frente a paso largo, colisiones y giro sin cambio;
  O2 doble y pausa modal.
- Seis armas: posición, imagen correcta, transparencia, ausencia tras muerte,
  integración escena/HUD/modal y framebuffer inválido sin escrituras.

Resultado final: compilación normal y ASan/UBSan correctas; **18/18 CTest** en
ambas, **16/16 unittest Python**. Pasan además cinco suites con recursos originales
(interaction, inventory, door_control, audio y HUD) bajo sanitizadores, sin errores
reportados de memoria o framebuffer. `git diff --check` correcto. Se revisó
visualmente la hoja de seis armas sobre geometría, texturas y casco originales.

Compilar con `cmake --build build -j4`; pruebas con
`ctest --test-dir build --output-on-failure` y unittest Python existentes.
Con recursos: interaction_tests DATA, inventory_tests GAME.LAB,
door_control_tests GAME.LAB, hud_tests DATA y audio_tests build/audio-test-data DATA.
Sanitizadores: build/interface-sanitized (ASan+UBSan).
`tools/inspect_interactions.py ~/Desktop/In_Extremis` exporta la evidencia ampliada.

Para revisión manual, ejecutar el juego y Tab para entrar en3D:

1. I→W/S→I; repetir I→ESC, vacío y tras recoger objetos. No debe quedar atrapado.
2. E ante panel; soltar E antes de confirmar. WASD mueve mano; E pulsa, borrar arriba
   derecha y validar abajo derecha. Código del nivel **IJEKAH**.
3. A/D en recogida: brillo a izquierda/derecha, E sobre No deja objeto; reabrir y E
   sobre Yes lo guarda una vez. Mantener E no vuelve a pulsar. ESC no recoge.
4. E001 en(4350,2300), mirada+X; con ella sola, lector de puerta en(2500,2550),
   mirada+Y: debe verse y rechazarse por tipo. C001 en(1500,2950), mirada+Y:
   con ella sola E en lector desbloquea; cerrar resultado y nueva E en puerta7 abre.
5. E en ascensor inicial(3500,50), mirada−Y, con E001: W sube mano, S baja;
   E confirma fila; destinos aún no disponibles. ESC sale y el motor termina.
   Repetir aperturas/cierres: no debe quedar sonido continuo.
6. W con Shift izquierdo/derecho: mayor velocidad; soltar vuelve a marcha.
   En pared no atraviesa; diagonal no acelera más. I pausa movimiento/O2.
   Soltar teclas usadas en modal antes de reanudar movimiento.
7. Z usa inyector y X botella solo en mundo, si existen. En cualquier modal no
   consumen. Confirmar uso en inventario mediante E sigue funcionando.
8. Arma0 visible al inicio. Intercambiar arma en armario y comprobar que vista e
   icono cambian juntos. HUD y ratio permanecen iguales. No hay acción de disparo.
9. Panel inicial(3250,1100), mirada−X: E/código/TRANSFER; ningún inyector añadido.

Las imágenes revisadas son composiciones del port con recursos auténticos. Sigue
pendiente una comparación DOS interactiva completa y la validación manual de
sensación de carrera/captura de ratón en la ventana.

## Archivos

Control: src/main.cpp, include/src item_session, player_input, player_control,
survival_session. Audio: include/src audio_mixer, audio_player y src/game_audio.cpp.
Presentación: include/src original_hud. No se cambian los recursos originales ni
original_interactions.cpp (se conservan sus correcciones gráficas).
Pruebas: interaction_tests, survival_session_tests, player_input_tests,
player_control_tests, audio_tests, hud_tests. CMakeLists solo ajusta el enlace del
test de entrada al coordinador real. tools/inspect_interactions.py añade evidencia.
Documentación: este informe y referencias actuales de controles/HUD/audio/interfaces.

Pendientes: viajes y catálogosE restantes, zoom/ANM y cadencias DOS, batería/minas,
confirmaciones históricas completas y detalles del prompt cortado. No se añaden
niveles, renderizadores nuevos ni un sistema de combate.
