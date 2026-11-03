# Salud, oxígeno y supervivencia originales

La revisión posterior de audio (fase A de objetos/inventario) confirma que las
pérdidas de salud por diagnóstico y el consumo de oxígeno no requieren un sonido
por cada cambio. No se han encontrado disparadores demostrados de respiración
normal o alerta sonora por oxígeno bajo en las rutas vitales examinadas. Las
muertes siguen usando muestras diferentes, 25/26; su presentación visual sigue
pendiente. Véase la sección «Revisión A» de `audio-system.md`: no se ha añadido
audio por analogía con otros juegos ni por el nombre OXYGENE.

## Resultado

El protagonista dispone de estado real independiente del HUD: salud inicial
1000 y oxígeno inicial 288000. El oxígeno consume un tick por tick del reloj
nominal original de 160 Hz mientras la simulación está activa, aunque el jugador
no se desplace. Las barras reciben esos valores internos, sin porcentajes
intermedios ni datos demostrativos. La presentación 4:3, el viewport, las
coordenadas, los sprites y el sentido de vaciado del HUD no cambian.

Se implementan daño en unidades internas suministradas por el futuro llamador,
restauración completa de salud y el intercambio de oxígeno con una reserva.
No existen ataques automáticos, inventario, botellas recogibles ni inyectores
funcionales: esas operaciones están listas para conectarse a sus futuros eventos.
Llegar a cero produce muerte lógica con causa diferenciada y bloquea movimiento,
E y el control global de puertas O. El mapa, la salida y la inspección visual
siguen disponibles. No se inventa una pantalla de muerte: la transición original
queda pendiente y se indica en el título de la ventana y el registro.

## Referencia y nivel de certeza

EXE SHA-256:

```text
7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397
```

Los rangos de código son **offsets absolutos de archivo**, desensamblados como
x86 de 32 bits. Los valores entre corchetes son direcciones de datos vistas en
las instrucciones; en esta imagen se traducen a archivo sumando 0x200.
`tools/inspect_vitals.py` verifica el hash, extrae la plantilla y genera las
instrucciones con sus bytes en `build/vitals-debug/`. No se ejecuta ni modifica
el binario original y no se versionan sus recursos.

**Confirmado:** constantes, cargas, operaciones, ramas y selección de sonidos
indicadas abajo. **Adaptación del port:** reloj nominal independiente de FPS,
pausa por mapa/foco, controles de diagnóstico y espera de presentación de muerte.
**Pendiente:** mecanismos y tiempos que todavía no se han seguido por completo.

## Variables e inicialización

| Dirección de datos | Tipo / uso demostrado | Inicial / escala |
| --- | --- | --- |
| `[0x5491]` | Word: energía/salud, leída por HUD y condición de muerte | 1000 / 1000 |
| `[0x548d]` | Dword: oxígeno del traje | 288000 / 288000 |
| `[0x544f]` | Dword: reserva de botella, intercambiada con el traje | -1: ausente en la plantilla nueva |
| `[0x1428]` | Word: contador de tiempo transcurrido | Incrementado por IRQ del temporizador |
| `[0x142c]` | Word: suspensión del contador | No cero congela sus incrementos |
| `[0x5409]` | Word: modificador de uso de oxígeno y velocidad | No cero duplica el decremento de oxígeno |
| `[0x5429]` | Word: condición de fin/estado del jugador | Se marca -1 en varias rutas fatales |

La plantilla de nueva partida está en archivo **0x1d8a0**, 42 bytes.
`0x1da7b` prepara ESI=0x1d6a0 (su dirección de datos); la selección de partida
nueva llega a `0x1de72`. El cargador lee quince words y tres dwords en orden.
`0x1dee2..0x1dee8` carga salud desde plantilla +28; `0x1def6..0x1defb` carga
oxígeno desde +38. Sus bytes contienen 1000 y 288000; la reserva en +34 es -1.
Las posiciones globales de esas variables están inicialmente a cero en el
archivo EXE: no se han confundido esos ceros con los valores de nueva partida.

La misma plantilla contiene selección de arma 0 y munición 500. El adaptador
del HUD recibe ahora esa selección desde Inventory y puede cambiarla por el
intercambio original de armario, sin disparos ni gasto de munición. Son datos iniciales del descriptor, no un sistema
de combate ni los antiguos valores demostrativos 000.

`Vitals::reset()` repone salud, oxígeno, fracción de reloj y causa de muerte.
La aplicación inicializa ese estado al comenzar. Un reinicio normal completo
requiere cerrar y volver a abrir el nivel; no se ha creado un menú de nueva
partida ni carga/guardado. F10 en diagnóstico solo reinicia los valores vitales,
manteniendo posición, puertas y mundo: se anuncia expresamente así.

## Reloj y consumo de oxígeno

| Rutina | Operaciones verificadas |
| --- | --- |
| `0x183c..0x189c` | Programa el PIT con BX=0x1d21, divisor **7457**, e instala la rutina de reloj. |
| `0x18bd..0x1917` | Si `[0x142c] != 0`, omite los incrementos; en otro caso incrementa `[0x1428]` una vez por interrupción. |
| `0x446a..0x447f` | El driver de audio divide explícitamente su frecuencia entre **160** para encadenar el reloj previo. Confirma la referencia nominal de 160 Hz. |
| `0x4825..0x484d` | Incrementa el divisor de encadenamiento y llama a la interrupción anterior al alcanzar su límite. |
| `0x6c00..0x6c39` | Inicializa el contador transcurrido a 1 y el descendente a 159. |
| `0x6c6c..0x6c8c` | Suspende el contador antes de una ruta modal y lo habilita después. |
| `0x16ad8..0x16b04` | Carga `[0x1428]` en EBX; si `[0x5409] != 0` hace `shl ebx`; resta EBX al dword de oxígeno. Un resultado negativo se limita a cero y marca `[0x5429]=-1`. |

**Algoritmo trasladado:** un tick de oxígeno por tick nominal activo; dos si
el modificador está presente. No hay comparación de posición alrededor de esta
resta: quedarse quieto no evita el consumo. El desplazamiento solo interviene
en otras operaciones, como las pisadas. La duración completa nominal es:

```text
288000 unidades / 160 ticks por segundo = 1800 s = 30 minutos
```

La entrada de teclado en `0x16db9..0x16dd7` activa `[0x5409]`, y
`0x16726..0x16759` lo relaciona con aumento de velocidad. No se implementa
carrera ni se conecta Shift u otra tecla a ese flag. `Vitals::update` admite
el modificador como entrada para conservar la operación demostrada; la partida
actual siempre utiliza el consumo normal. No se deduce un modificador por
moverse rápido, por colisiones o por una tecla distinta.

**Precisión y adaptación temporal:** el port acumula fracciones de tick y consume
solo ticks enteros a 160 Hz nominales, independientemente de FPS. No agrega un
tick por fotograma por el valor inicial 1 del contador DOS. Tampoco reproduce
el desbordamiento del word después de una suspensión larga. El divisor físico
del PIT y la división entera del encadenamiento de audio introducen pequeñas
diferencias con cada configuración DOS; no se afirma equivalencia de reloj
físico. La constante 160 procede del ejecutable, no de una tasa elegida para
equilibrar el juego. Los 30 minutos son nominales, no una medición de hardware.

En el port, simulación activa significa vista 3D, cámara disponible y foco de
teclado en la ventana. En mapa 2D, carga, pérdida de foco o muerte no se consume
oxígeno. Se descarta el intervalo de entrada/reanudación, para no recuperar de
golpe el tiempo suspendido. Los eventos de pérdida y ganancia de foco también
invalidan ese intervalo si ambos llegan en el mismo bombeo de eventos.
Esta es una política explícita para las herramientas actuales del port, no una
afirmación sobre el mapa original. Los futuros menús/pausa deberán suministrar
`simulationActive=false` al mismo adaptador; no se ha añadido una tecla de pausa.

## Alertas, agotamiento y recuperación de oxígeno

`0x16b05..0x16b15` compara el oxígeno con **19200**; por debajo solicita el
mensaje original `ATTENTION: OXYGEN ABOUT TO FAIL!!!!`. Son menos de 120 segundos
nominales de reserva normal. `0x16b16..0x16b59` usa **4800** como segundo umbral
y calcula un estado de efecto visual a partir de `(4800-oxígeno)*75/4800`,
escribiendo los campos de paleta/alerta. Corresponde a menos de 30 segundos
nominales. No se ha recuperado toda la presentación y temporización de esas
alertas: no se añade un parpadeo ni una cola de mensajes provisional.

No hay llamada a una petición de sonido en ese tramo de alerta baja. Esto no
demuestra ausencia de sonido en todas las rutas del juego. En particular,
**no se activa respiración permanente o al caminar**.

La recuperación demostrada es un **intercambio**: `0x1bd59..0x1bd6e` carga el
oxígeno del traje y la reserva `[0x544f]`, escribe la reserva en el traje y guarda
el valor anterior del traje en la botella. Las cadenas `Utiliser bouteille` y
`Ne pas utiliser`, en los datos de esa ruta, respaldan su contexto. La reserva
vacía no equivale a una suma o a rellenar siempre al máximo.

`exchangeOxygen(int& reserve)` reproduce ese intercambio. Rechaza un valor
negativo (reserva ausente) y limita una reserva externa al máximo conocido.
Si el jugador ya ha muerto, no intercambia nada ni revive. La mecánica que
posee la botella, sus condiciones de acceso y la recogida no están implementadas.
No hay fuentes de oxígeno colocadas arbitrariamente en el mapa.

## Salud, daño y recuperación

La barra usa directamente el word `[0x5491]`; 1000 es su valor completo.
`0x1663f..0x1665c` resta **2**, limita un resultado negativo a cero y marca
`[0x5429]=-1`. La rama depende de estados de movimiento no trasladados por
completo; no se aplica esa pérdida automáticamente a caminar o chocar contra
paredes. `0x10420..0x1043e` contiene otra ruta que resta un resultado escalado
por 500/100; no se convierte en un daño genérico de enemigo sin rastrear el
evento y sus parámetros. Las cantidades de ataques futuros siguen pendientes.

`applyDamage(unidadesInternas)` ofrece la operación de resta y saturación,
aceptando la cantidad que determine el futuro evento original. No hay enemigos,
ataques inventados, daño periódico ni conversión a un rango 0..100. Una cantidad
negativa o cero se ignora; una cantidad superior a la salud la deja a cero,
sin reproducir subdesbordamientos de aritmética word. Un jugador muerto no
recibe más modificaciones. No se calcula daño desde el HUD.

La ruta de inyectores `0x1be19..0x1bf37` restaura repetidamente AX=**1000** y
lo escribe en salud; sus distintas ramas actualizan los slots `[0x5459]` y
`[0x545b]`. Las cadenas incluyen `Utiliser injecteur` e `injecteur-1`.
`restoreHealth()` implementa esa restauración completa. No se inventan curas
de +25/+50 ni regeneración. Inventory comprueba un inyector lleno y deja su
slot vacío al invocar la operación. Se impide usarla para revivir después de una muerte.

Las seis zonas de heridas del cuerpo y sus parpadeos se conocen visualmente,
pero no se ha reconstruido el evento que determina cada herida ni su reloj
completo. No se generan heridas a partir de una simple cantidad de daño.

## Muerte y audio

`0x76a9..0x76d9` distingue dos condiciones:

1. Salud igual a cero: pone **AX=0** y llama a `0x1723a`.
2. Si la salud no es cero, oxígeno igual a cero: pone **AX=1** y llama a la misma
   función despachadora. El oxígeno cero no necesita reducir también la salud.

`0x17240..0x17253` separa AX=1 y AX=2 de la rama normal AX=0. En la rama de
salud, `0x17300..0x1730c` pone AX=25, BX=0 y llama al reproductor directo
`0x13123`; aparecen los recursos MORT0.VGA/MORT0.SPR. En la rama de oxígeno,
`0x1750f..0x1751b` pone AX=26, BX=0 y llama a la misma ruta; posteriormente
selecciona `DATA\\MORT1.ANM`. Las llamadas en `0x17486` y `0x1759e` llegan a
`0x13090`, que detiene las voces al terminar la presentación.

La asociación anterior de BANK[26] con muerte por oxígeno queda por tanto
**confirmada por el llamador**, no solo deducida del nombre OXYGENE.DSS.
`GameAudio::beginDeath` solicita BANK[25] o BANK[26] una sola vez por transición,
conservando sus formatos originales: BANK[25] es puntual (33916 muestras,
6,7832 s); BANK[26] tiene bucle DSS (11125 muestras en la primera pasada,
vueltas posteriores [0,11124)). No se fuerza repetición del efecto sin bucle.

**Límite de la presentación actual:** no se han integrado la decodificación y
secuenciación visual de MORT0/MORT1 ni su vuelta al menú. Se conserva una fase
de muerte pendiente: se congela la capacidad de actuar, se mantienen las barras
y se informa en el título. El sonido de oxígeno continúa con su bucle nativo
durante esa espera; F10 de diagnóstico termina el evento y llama a `endDeath`,
o cerrar la aplicación libera el reproductor. Esta duración de espera es una
adaptación explícita: no se presenta como la duración original de MORT1 y no
se inventa un temporizador de corte. La futura presentación deberá llamar a
`endDeath` en el mismo final de secuencia que el original.

BANK[14] sigue asociado a `POWER IS SURGING` y a otros estados, no a respiración.
Existe una tercera causa AX=2 relacionada con otros mecanismos; no se implementa
sin esos sistemas. Las pisadas y puertas mantienen su código de cadencia,
ganancia y disparadores. Al morir no hay desplazamiento nuevo, así que no se
generan nuevos pasos; los efectos ya iniciados pueden terminar naturalmente.

## Arquitectura y conexiones pendientes

- `player_vitals.h/.cpp`: estado, reloj nominal, operaciones y causa de muerte;
  sin SDL, HUD, puertas, audio, cámara ni recursos gráficos.
- `survival_session.h/.cpp`: adaptador de partida. Aplica la política de
  actividad, pasa valores a `hud::State`, anuncia transiciones a `GameAudio`
  y atiende exclusivamente el diagnóstico optativo. No dibuja las barras.
- `main.cpp`: pasa tiempo y actividad al adaptador antes de procesar movimiento;
  usa `alive()` para la captura de movimiento/E y el control O. No incorpora
  fórmulas de daño o consumo ni dibujo adicional.
- `GameAudio`: adapta únicamente la causa a los IDs demostrados; no calcula
  salud, consumo o duración ficticia de cinemáticas. Ausencia de banco o de
  dispositivo no impide la muerte lógica.

Enemigos/combate deberán proporcionar cantidades originales de daño; Inventory
ya posee las reservas y consume los inyectores del controlador de suministros;
menús deberán señalar pausa; la presentación de muerte deberá gestionar MORT0,
MORT1 y su final. Ninguno de esos sistemas se ha sustituido por un framework
genérico o por reglas provisionales.

## Diagnóstico y comprobación manual

Partida normal, sin teclas de modificación de valores:

```sh
./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB
```

Entrar con TAB a 3D. El oxígeno baja incluso quieto; la primera columna vacía
de la barra de 43 píxeles aparece tras unos **41,86 s nominales**. La salud no
disminuye por movimiento ni por ataques inexistentes. En mapa 2D o perdiendo el
foco no debe avanzar. Los valores de salud/oxígeno dejan de ser demostrativos;
arma/munición siguen siendo la representación estática inicial, sin disparos.

Para una prueba breve, activar explícitamente la herramienta:

```sh
./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB --vitals-debug
```

La ventana indica **VITALS DEBUG**. Sin ese argumento estas teclas no modifican
los valores. No sustituyen controles previos y no existe aceleración oculta.

| Tecla de diagnóstico | Operación |
| --- | --- |
| F5 | Daño de prueba de 250 unidades. **250 es una cantidad diagnóstica, no un ataque original identificado.** |
| F6 | Restauración completa de salud, solo si sigue vivo. |
| F7 | Avanzar únicamente el reloj de oxígeno 60 segundos nominales (9600 unidades). No adelanta puertas, sprites ni el resto del mundo. |
| F8 | Avanzar el oxígeno 1800 segundos para provocar su agotamiento inmediato. |
| F9 | Intercambiar con una reserva diagnóstica llena. No crea inventario ni permite resucitar. |
| F10 | Reiniciar solo valores vitales y terminar el evento de muerte. No reinicia mapa, posición ni puertas. |

Comprobar F5 una/dos veces: solo cambia la barra inferior; F6 la repone.
F7 cambia la barra superior; F9 la repone. Para muerte por salud, F10 y cuatro
pulsaciones F5: barra inferior vacía, BANK[25], sin movimiento, E ni O.
F10 y F8 prueban la otra causa: barra superior vacía, salud conservada y
BANK[26] con su bucle. F6/F9 no deben revivir; F10 debe detener el bucle.
ESC sale normalmente. Repetir sin `--vitals-debug` para confirmar que F5..F10
no activan las herramientas. No se ha sustituido la animación original pendiente
por una nueva pantalla o texto de muerte dibujado en el HUD.

## Pruebas y reproducción

```sh
python3 tools/inspect_vitals.py ~/Desktop/In_Extremis/EXTREMIS.EXE
cmake -S . -B build
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
./build/hud_tests ~/Desktop/In_Extremis/DATA
./build/audio_tests build/audio-test-data ~/Desktop/In_Extremis/DATA
```

Las pruebas cubren plantilla inicial, 160 Hz a 1/18/30/60/144/240 FPS, fracciones
de tick y pausa, modificador doble, tiempos inválidos/largos, resta de daño,
recuperación completa, intercambio frente a suma, límites, causas independientes,
ausencia de resurrección y cambios tras morir, reinicio, diagnóstico desactivado,
ventanas/eventos repetidos, foco perdido/ganado en un mismo bombeo y estado real
recibido por el HUD. Se verifica la cuantización visual de las barras sin cambiar
sus recursos y se conservan los tests de límites de framebuffer.

La integración de audio usa exclusivamente SDL dummy: una solicitud por muerte,
bucles sin reinicios, parada al reiniciar, finalización puntual de BANK[25] y
muerte lógica sin dispositivo. El test de entrada usa vídeo dummy y verifica
desplazamiento cero con control deshabilitado. Las suites existentes verifican
puertas, pasos, colisiones, sprites y renderizado. La escucha y las transiciones
interactivas deberán comprobarse manualmente; los tests no equivalen a una
comparación de la cinemática original en DOS.

Resultado de la entrega: compilación correcta, **16/16 suites CTest** y
**14/14 pruebas Python** superadas. Las suites `player_vitals`,
`survival_session` y `original_hud` pasan también con AddressSanitizer y
UndefinedBehaviorSanitizer. La prueba opcional del HUD con recursos reales y
la decodificación de los 44 registros originales de audio, incluida la regresión
de la cola de puertas, también pasan. No se ha realizado una escucha nueva
del dispositivo físico ni una comprobación interactiva de la cinemática DOS.

Archivos añadidos: `include/player_vitals.h`, `src/player_vitals.cpp`,
`include/survival_session.h`, `src/survival_session.cpp`, `tests/vitals_tests.cpp`,
`tests/survival_session_tests.cpp`, `tools/inspect_vitals.py`,
`tests/test_inspect_vitals.py` y este documento. Modificados en esta tarea:
`CMakeLists.txt`, `src/main.cpp`, `include/original_hud.h`,
`include/game_audio.h`, `src/game_audio.cpp`, `tests/hud_tests.cpp`,
`docs/original-hud.md` y `docs/audio-system.md`. El resto de cambios locales
anteriores se conserva; no hay commits, pushes ni modificaciones de originales.

## Conexión con suministros del nivel 1

`player::Inventory` llama a `Vitals::restoreHealth` y `exchangeOxygen`, sin
almacenar una segunda salud u oxígeno del traje. El inyector del panel 8/8 se
recoge con G y se usa con H: restaura 1000 y conserva envase vacío. No cura
al recogerlo ni revive muertos. Una botella llena se usaría con J, intercambiando
su reserva y la del traje; ausente o vacía no puede usarse (guardia demostrada
por `0x1bd21..0x1bd36`). No hay botella identificada en los cuatro paneles del
nivel 1, por lo que no se coloca ninguna para demostrar esta función.

Los nuevos controles son adaptaciones del port mientras faltan las pantallas
modales originales. Vitals sigue consumiendo tiempo solo en simulación activa;
G/H/J se descartan fuera de esa simulación y tras morir. F10 mantiene su alcance
diagnóstico: reinicia Vitals, no inventario ni armarios. Véase
[items-and-inventory.md](items-and-inventory.md) para evidencias, límites,
pruebas y posiciones manuales. Combate y objetos de otros tipos siguen pendientes.

## Pausa durante interfaces originales

Las interfaces de ItemSession detienen la simulación vital mediante la política
existente de actividad. No se modifica la tasa de consumo. Al cerrar se descarta
el intervalo de reanudación, sin descontar el tiempo que el jugador pasó en un
armario/lector. El original bloquea el bucle principal durante estas interfaces
y restablece [0x1428]=1 al volver por 0xa636. Tests cubren pausa y ausencia de
catch-up. La recogida confirmada almacena suministros; no cura ni rellena el traje
antes de usar su recurso real. [original-interactions.md](original-interactions.md)
detalla los controles, el estado del inyector y las limitaciones de presentación.
