# Audio original: DSS1, reproducción y auditoría ambiental

Actualización: [disparo del arma equipada — fase 1](equipped-weapon-fire.md).
Las menciones anteriores a ausencia de disparo quedan sustituidas por ese informe.

Actualización vigente: [Paso 201 — controles e interfaces](step-201-interfaces-and-controls.md).
Los controles y el alcance descritos abajo corresponden a revisiones anteriores.

Estado actualizado: [auditoría de interfaces de 2026-11-02](interface-audit-2026-11-02.md).
Ese informe corrige los diagnósticos históricos que siguen en este documento.

## Revisión A: pérdidas vitales, respiración y muerte

La comprobación manual del usuario (pérdidas silenciosas y sonido al agotarse
una barra) coincide con el adaptador actual. La revisión no encuentra un evento
universal «salud/oxígeno disminuyó → reproducir sonido» en las ramas examinadas.
`0x1663f..0x166bf` resta salud y prepara un efecto visual sin solicitar audio;
`0x16ad8..0x16b59` consume oxígeno y solicita un mensaje/efecto visual, sin
petición de muestra. `0x16bca..0x16bdf` muestra un aviso de energía baja sin
sonido en esa rama. Esto no demuestra que todos los tipos de daño del original
sean silenciosos: faltan eventos de combate y sus rutas.

Se han vuelto a rastrear los candidatos E8 hacia `0x13103` (17), `0x13123`
(91) y `0x45ab` (37), y contrastado las ramas vitales, avisos, disparadores de
BANK[14], scheduler y ramas de muerte. La búsqueda de bytes no descarta rutas
indirectas ni código modificado durante ejecución. No aparece una petición
demostrada de respiración normal/acelerada o alerta sonora por oxígeno bajo en
estas rutas. BANK[26] sigue siendo muerte por oxígeno; BANK[14] sigue ligado
a POWER IS SURGING/otros estados; BANK[11] no se convierte en sonido de traje.
No se afirma que el juego entero carezca de respiración: su existencia durante
el nivel permanece sin evidencia suficiente y no se inventa.

Las causas de muerte y muestras 25/26 se confirman nuevamente por el llamador
`0x76a9..0x76d9` y el despachador `0x1723a`. No se observa una concatenación
de varias peticiones en las dos ramas de muerte examinadas. El PCM 25 finaliza
una vez; el 26 usa su bucle DSS durante MORT1, con parada original al final de
la secuencia. El port mantiene su espera visual pendiente y por tanto aún no
reproduce la duración de esa cinemática. No se han añadido nuevos sonidos ni
cambiado puertas/pisadas como resultado de esta revisión.

Reproducción: `python3 tools/inspect_audio_code.py ~/Desktop/In_Extremis/EXTREMIS.EXE
--section vitals` (una sola línea). El test de integración añade pérdidas
no fatales hasta oxígeno bajo y comprueba cero peticiones a 14/25/26 antes de
morir. Los tests previos verifican petición única, bucle, parada al reiniciar,
finalización, simultaneidad y cierre del stream. **Fase A verificada antes de
comenzar la fase B**; siguen pendientes respiración, otros tipos de daño y la
presentación/temporización visual de MORT0/MORT1.

## Resultado

Se han localizado y decodificado **30 entradas de BANK.DSS y 14 archivos DSS
individuales**: 44 registros, con **40 contenidos PCM distintos**. No equivale
a 44 acontecimientos identificados. Están integrados **BANK[27] para puertas**
y **BANK[7] para pisadas por desplazamiento efectivo**. Ambos han sido escuchados
y validados por el usuario, incluida la corrección de la cola repetida de puertas.
La auditoría de continuación no añade respiración ni maquinaria automática:
no se ha demostrado un disparador ambiental trasladable al nivel 1 actual.

**Ampliación de supervivencia:** la investigación del llamador confirma muerte
por salud → BANK[25] y muerte por oxígeno → BANK[26]. Ambos están integrados
como eventos de muerte, no como ambientes de nivel o respiración constante.
Véase [player-vitals.md](player-vitals.md) para evidencias y límites de la
presentación pendiente.

No se han asociado sonidos por su parecido auditivo ni por el nombre del archivo.
Los demás efectos se pueden escuchar mediante la herramienta independiente.
No se modifican recursos ni se incluyen sonidos originales en archivos versionados.

## Recursos y contenedor

En DATA: `BANK.DSS` (197203 bytes), `ALARME1.DSS`, `ALARME3.DSS`, `ALARME4.DSS`,
`BARGE1.DSS`, `BARGE2.DSS`, `CABINE.DSS`, `DEPART.DSS`, `EXPLO.DSS`, `LABO.DSS`,
`MONSTRE.DSS`, `OXYGENE.DSS`, `RADAR.DSS`, `TOUCHE.DSS` y `VENT.DSS`.

Un archivo individual contiene una cabecera de **44 bytes** y las muestras:

| Offset relativo | Campo confirmado |
| --- | --- |
| 0..3 | Firma ASCII `DSS1` |
| 4..27 | Nombre de 24 bytes, con espacios o puntos como relleno |
| 28..29 | Frecuencia nominal uint16 LE: 5000 en los 44 registros |
| 30..33 | Cantidad de bytes de muestras uint32 LE |
| 34..35 | Indicador de bucle uint16; el driver prueba cero/no cero |
| 36..39 | Comienzo del bucle, desplazamiento desde las muestras |
| 40..43 | Fin del bucle, exclusivo según la comparación del driver |
| 44.. | PCM sin compresión: un byte unsigned por muestra, un canal |

Los límites de bucle se validan cuando el indicador está activo. Algunos registros
sin bucle contienen valores residuales, incluso comienzo igual a fin; no se
interpretan como bucles ni se rechazan por esos campos inactivos.

BANK.DSS empieza con el número de entradas uint16 LE (**30**). Para cada entrada:

```text
volumen:uint8, requiere_petición_continua:uint8, prioridad:uint8,
cabecera_DSS1[44], muestras[longitud]
```

No hay una tabla de offsets almacenada: el cargador original recorre los tamaños
de los registros y construye punteros en memoria. Los IDs son el orden desde 0.
El volumen observado está entre 0 y 64. El segundo byte vale 0 o 255 y controla
la conservación del efecto mientras se recibe una petición. No debe confundirse
con el indicador de bucle dentro de la cabecera DSS1. El tercer byte interviene
en la selección de peticiones para las cuatro voces.

Duplicados byte a byte del PCM: BANK[2] = BANK[3]; BANK[26] = OXYGENE;
BANK[28] = TOUCHE; BANK[29] = EXPLO. Se conservan sus IDs distintos.

## Evidencias del ejecutable

Las direcciones son **offsets absolutos de archivo**, x86 de 32 bits, EXE SHA-256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.

| Código | Evidencia |
| --- | --- |
| `0x6304` | Configura `[0x3dde] = 5000`, frecuencia global de salida. |
| `0x4417..0x4429` | Inicializa cuatro registros de voz de 26 bytes. |
| `0x4444..0x4456` | Construye tablas de volumen con `índice_unsigned * volumen / 64`. |
| `0x44a7..0x44c1` | Programa el PIT con `1193000 / frecuencia`, división entera. |
| `0x45e9..0x4613` | Salta firma, nombre y frecuencia; lee longitud, flag de bucle y sus dos desplazamientos; las muestras empiezan inmediatamente después. |
| `0x473c..0x4747` | LODSB, traducción de volumen y suma de las voces: no hay decodificación ADPCM ni canales estéreo intercalados. |
| `0x474a..0x4777` | Fin de muestra por comparación de punteros; sin bucle desactiva la voz; con bucle carga comienzo y final. La primera pasada usa toda la muestra. |
| `0x477a..0x4783` | Recorre cuatro voces y divide la mezcla entre cuatro. |
| `0x12e1e..0x12e92` | Lee el contador del banco, sus tres bytes de metadatos y salta `44 + longitud` para localizar el siguiente sonido. |
| `0x12f0d..0x12f79` | Control de voces activas: petición, conservación o parada según el segundo byte de metadatos. |
| `0x12f96..0x13076` | Selección de peticiones según prioridad, hasta cuatro voces. |
| `0x13103..0x13121` | Solicita un ID marcando su byte de actividad. |
| `0x13123..0x13158` | Ruta directa: selecciona el puntero del ID y su volumen, llama a `0x45ab`. |
| `0x1182a..0x11848` | Consulta el estado de la conexión; si `0 < AL < 60`, solicita AX=27. No distingue aquí entre apertura y cierre. |
| `0x169b7..0x16a37` | Compara posición actual/anterior y avanza un acumulador; al alcanzar 144 solicita ID=7, cuyo nombre original es `pas`. |

La frecuencia de 5000 Hz es la configuración nominal demostrada, no una medida
de un equipo DOS real. El divisor entero del PIT produce una pequeña diferencia
física (1193000/238 ≈ 5012,6 Hz). El port usa la frecuencia nominal del recurso;
no intenta emular los errores de reloj de cada dispositivo original.

## Correspondencias y cuestiones abiertas

**Puertas:** ID 27 está demostrado por la consulta de conexiones durante su
proyección. El port reproduce una muestra completa al empezar cada episodio de
movimiento (`0 < progress < 1`), sin superponer 15 copias al pulsar O. Mantiene
el bloqueo de nuevos disparos hasta que termina el movimiento, aunque el sonido
ya haya finalizado; alcanzar un extremo no dispara otro efecto ni corta el que
sigue activo. La misma muestra sirve para abrir y cerrar. No se ha inventado
un sonido independiente de cierre.

**Pasos:** ID 7 (`pas`), 1902 muestras, 0,3804 s, volumen original 16/64,
sin bucle. Integrado y validado auditivamente. `PlayerInput` mide la distancia
entre posiciones antes y después de resolver el movimiento y las colisiones.
`FootstepCadence` pide el primer paso a 90 unidades y los siguientes cada 80;
no acumula tiempo estando quieto, girando o empujando una pared sin desplazarse.
El adaptador descarta nuevos disparos mientras sigue sonando el mismo paso y
no reproduce ráfagas para recuperar todos los umbrales de un fotograma largo.

Es una adaptación por distancia, no una emulación completa del reloj original:
`0x169b7..0x16a3d` compara X/Y actual y anterior; calcula un incremento usando
`[0x51e0]`, `[0x51e2]`, factor 18 y `[0x1422]`; a partir de 144 pide AX=7.
Con banco cargado, `0x13103` devuelve EAX=7*4=28, que se conserva como fase.
La referencia de 10 unidades por actualización e incremento 16 produce 90/80.
Las pruebas comparan esa referencia y desplazamientos equivalentes a distintos FPS.
La distancia euclídea efectiva y la supresión de solapamiento son políticas del port;
no se cambian el ritmo ni el volumen ya aprobados por el usuario.

**Ambientes:** existen sonidos con bucle (por ejemplo ALARME, CABINE, DEPART,
MONSTRE, OXYGENE, VENT y algunas entradas del banco). Esto demuestra capacidad
de reproducción continua, no su asignación a sectores del nivel 1. Las cadenas
de archivos individuales aparecen, entre otros, en `0xc870..0xc8fc` y
`0x21044..0x210f3`; no basta para asociarlos a una zona. No se activan ambientes
por aproximación o por nombre.

**Volumen y espacialización:** el mezclador examinado produce una salida mono
y recibe un nivel por efecto. No se ha demostrado panoramización estéreo ni una
fórmula de atenuación por distancia en las llamadas investigadas. No se añade
audio posicional inventado. Sigue pendiente rastrear modificaciones de volumen
y todas las rutas de petición de ambientes. Mono no demuestra por sí solo que
el juego carezca de atenuación espacial.

**Música:** el inventario investigado no ha revelado un recurso o secuenciador
musical identificado inequívocamente. Las opciones `Sound ON/OFF` y `SoundFX
ON/OFF` en `0x237c5..0x23806` no prueban que una de ellas controle música. No se
afirma que el juego carezca de música; quedan pendientes menús, secuencias y
otras rutas de audio. No se añade ninguna pista.

También se inspeccionó DATA.LAB (264638 bytes): no aparecen firmas DSS1, RIFF,
MThd, XM, IT o S3M en esa muestra. Esta búsqueda de firmas no descarta datos
musicales propietarios y no se usa como prueba de ausencia de música.

## Arquitectura del port

- `audio_data.h/.cpp`: lector DSS1 y banco sin SDL, con tamaños, firma, contador,
  frecuencia y bucles validados. Convierte PCM8 unsigned a float mediante
  `(byte - 128) / 128`. Carga los 30 sonidos una vez.
- `audio_mixer.h/.cpp`: cuatro voces, finalización, bucles originales y volumen.
  Su salida es float mono a 5000 Hz. No depende de SDL, puertas ni renderizado.
- `audio_player.h/.cpp`: propietario RAII del subsistema y del stream/dispositivo
  SDL3. Un callback alimenta el stream según demanda; SDL convierte frecuencia,
  formato y canales al dispositivo. No depende del FPS ni bloquea el bucle del
  juego. El callback no lee archivos ni reserva buffers propios.
- Las órdenes del hilo principal se serializan con `SDL_LockAudioStream`; SDL
  ya mantiene ese bloqueo dentro del callback. La destrucción retira el callback
  y espera su finalización antes de liberar el stream y el subsistema.
- `footstep_cadence.h/.cpp`: acumula exclusivamente distancia; no depende de SDL.
- `game_audio.h/.cpp`: conecta puertas a BANK[27] y desplazamiento efectivo a
  BANK[7], además de eventos explícitos de muerte a BANK[25]/BANK[26]. No
  modifica puertas ni introduce audio en el renderizador.
- Main carga el audio después de inicializar SDL y llama al adaptador después
  de actualizar puertas. Ausencia de banco/dispositivo: mensaje y juego sin sonido.

Las decisiones de ciclo de vida y bloqueo se han contrastado con la cabecera
SDL3 instalada y la documentación oficial de
[SDL_OpenAudioDeviceStream](https://wiki.libsdl.org/SDL3/SDL_OpenAudioDeviceStream)
y la [migración de audio a SDL3](https://github.com/libsdl-org/SDL/blob/main/docs/README-migration.md).

Parámetros/adaptaciones explícitos:

- Volumen maestro inicial **0,6**, configurable con `Player::setVolume`. No es
  un valor original. Se conservan los niveles por efecto y la división por cuatro.
- Rampas de entrada/salida de **32 muestras (6,4 ms)** para reducir clics; son
  una adaptación del port, no parte del formato ni un comportamiento DOS probado.
- BANK[27] tiene flag DSS sin bucle, pero metadato de petición continua. La
  primera integración forzaba su repetición hasta el fin del movimiento; esto
  generaba una segunda pasada parcial y se ha corregido tras la validación
  auditiva. La política actual es una reproducción completa por episodio,
  solicitada para el port; no es un hallazgo nuevo sobre el scheduler DOS.
- El original solicita el efecto al proyectar conexiones; el adaptador actual
  observa todas las puertas en movimiento, incluso fuera de vista y en 2D. No
  se presenta esta política como una reproducción exacta de alcance audible.
- El mezclador admite cuatro voces; si están ocupadas rechaza otra orden. La
  política completa de prioridades del scheduler original sigue pendiente. En
  la integración de puertas/pasos admite ambos simultáneamente;
  los pasos se limitan a una voz y las puertas conservan su agrupación global.
- El port intenta activar audio al cargar el nivel. Todavía no interpreta las
  opciones de sonido del CFG original ni añade controles de audio nuevos.

## Extracción, reproducción y pruebas

```sh
python3 tools/extract_audio.py --data-dir ~/Desktop/In_Extremis/DATA
cmake --build build/release --target audio_probe -j 4
./build/release/audio_probe ~/Desktop/In_Extremis/DATA/BANK.DSS 27 3
./build/release/audio_probe ~/Desktop/In_Extremis/DATA/VENT.DSS
ctest --test-dir build/release --output-on-failure
./build/release/audio_tests build/release/audio-test-data ~/Desktop/In_Extremis/DATA
python3 -m unittest discover -s tests -p 'test_*.py'
```

`build/audio-debug/catalog.csv` contiene procedencia, ID, nombre, offset,
duración, frecuencia, formato, tamaño, bucles, metadatos y hash del PCM. Los
44 WAV se generan con el PCM8 original, **sin** rampas o mezcla. Los WAV de
diagnóstico no son dependencias del motor ni reproducen automáticamente sus
bucles. El extractor impide escribir fuera de `build/audio-debug/`.

`audio_probe` reproduce directamente un DSS individual o un ID del banco, por
defecto hasta tres segundos, sin nuevas teclas en el juego. No añade asociaciones
a acontecimientos. Los archivos individuales conservan su nombre como ID de
catálogo; el banco conserva índices 0..29.

Las pruebas sin hardware cubren decodificación exacta de 0/128/255, cabeceras,
longitudes, contador, metadatos y bucles inválidos, recursos ausentes, mezcla,
cuatro voces, finalización, continuidad, volumen y rampas. El dispositivo SDL
`dummy` verifica consumo real desde callback, apertura/cierre repetidos y caída
segura ante un driver inexistente. La prueba opcional decodifica los 44 registros
originales sin modificarlos. Python comprueba el extractor y su destino seguro.

Resultado histórico de la primera integración: **12/12 suites CTest y 10/10 pruebas Python superadas**, además de la
decodificación C++ opcional de los 44 registros originales. `audio_probe` se ha
ejecutado con BANK[27] y VENT.DSS usando backend `dummy`. La aplicación también
se ha iniciado con el dispositivo habitual y confirma su apertura junto a los
30 efectos cargados; no constituye validación auditiva real del resultado.

Validación auditiva del usuario: puertas y pasos satisfactorios, sin fragmento
adicional al finalizar las puertas. Tras cambios de ciclo de vida conviene repetir
E/O, pasos y efectos simultáneos. Sigue pendiente comparar
con el original la continuidad y el alcance audible. Pasar pruebas con `dummy`
no equivale a haber oído y validado los efectos en altavoces.

## Corrección: fragmento repetido al finalizar las puertas

### Diagnóstico y causa demostrada

Una sola solicitud de `GameAudio::update` llamaba a `Player::play(27,true)`. El
segundo argumento imponía repetición completa en `Mixer::next`, aunque el flag
DSS de BANK[27] vale **0**. La muestra contiene **3700 frames mono a 5000 Hz**
(0,74 s), mientras que el recorrido provisional completo de la puerta dura 1 s.
A los 0,74 s, el mezclador volvía a la muestra 0; al terminar el movimiento se
llamaba a `stop`, dejando oír el inicio de esa segunda pasada hasta la parada.
No eran dos eventos de interacción ni un disparo extra al alcanzar el extremo.

Se ha reproducido el defecto con el recurso original y el mezclador, **sin
dispositivo ni conversión SDL**, simulando 5000 frames de movimiento y después
los 32 frames de la rampa de parada que ya existía:

| Modelo | Solicitudes/inicios | Frames PCM leídos | Vueltas al inicio | Finalizaciones naturales | Paradas |
| --- | --- | --- | --- | --- | --- |
| Anterior: repetición forzada y parada | 1 / 1 | 5032 | 1 | 0 | 1 |
| Corregido: una reproducción completa | 1 / 1 | 3700 | 0 | 1 | 0 |

El WAV `build/audio-debug/BANK-27.wav` tiene exactamente los mismos 3700 bytes
PCM que BANK[27], comparados byte a byte. SHA-256 del PCM:
`33395ebac2fd105f9a837226b25057a79e6af9f45eb1e8b43ac96204a18ca1a8`.
El bloque inicial de 256 bytes solo aparece en el offset 0 del recurso; la
segunda lectura del inicio se produce en el mezclador, no está añadida al WAV.
No se ha modificado el decodificador ni el tamaño de las muestras.

### Cambio concreto

`GameAudio` solicita `play(27)` sin repetición. Conserva el latch hasta el final
del movimiento para que los fotogramas restantes no relancen la muestra. Al
alcanzar un extremo solo libera ese latch: no llama a `stop`, por lo que una
transición corta tampoco recorta el efecto. Las reproducciones siguientes usan
su propio cursor. Se mantiene la agrupación global existente de puertas en
movimiento, incluido O; no se cambia su geometría, temporización ni controles.

No se añaden silencios, recortes, nuevas rampas ni cambios de frecuencia o
ganancia. Las rampas del mezclador ya existentes permanecen iguales. Con una
puerta de recorrido completo, el efecto ahora termina a los 0,74 s aunque el
movimiento visual continúe hasta 1 s: no se alarga el recurso para cubrirlo.
Los bucles explícitos de otros efectos siguen funcionando.

### Instrumentación y regresiones

`Mixer::statistics(id)` registra frames decodificados, solicitudes, inicios,
lecturas PCM, vueltas al inicio, finalizaciones y paradas por ID. `Player`
obtiene la instantánea bajo el bloqueo del stream, sin imprimir desde el callback.
Los contadores se preparan al cargar el banco, sin reservas dentro del productor.
`GameAudio` registra inicio y finalización desde el hilo principal, con totales
acumulados; `doorStatistics()` permite comprobar las solicitudes reales del
adaptador. `audio_probe` imprime los mismos contadores al terminar.

`sourceFrames` cuenta lecturas de frames PCM mono de entrada: cada frame float
ocupa 4 bytes. No cuenta los frames de salida resampleada, ni los ceros que un
callback entrega cuando ya no hay voces activas. Estos ceros son el estado de
reposo del mezclador, no silencios insertados en el efecto.

Las nuevas pruebas verifican:

- Longitudes 1,2,31,32,33,127,128,129,511,512,513,3700 y 5001; orden de todas las
  muestras, límites exactos y ausencia de señal/lecturas después del final.
- Reproducciones consecutivas con salida idéntica y cursores independientes;
  efectos simultáneos de 33 y 513 frames finalizando cada uno en su límite.
- Conversión SDL de 5000 frames float mono a 48000 frames float estéreo, con
  entradas completas o por bloques de 127 frames: misma salida y cola sin audio
  repetido después de la finalización.
- El adaptador real con SDL dummy: 15 puertas abriéndose generan una sola
  solicitud; finalizar el audio durante el movimiento no lo relanza; alcanzar
  un extremo no dispara ni detiene; un cierre consecutivo reproduce otra muestra
  completa incluso si la transición visual termina antes.
- El recurso BANK[27] real reproduce el modelo anterior y el corregido. La prueba
  opcional escribe `build/audio-debug/door-tail-regression.csv` con los contadores.

Se ejecutan con los comandos de pruebas anteriores. Resultado: **12/12 suites
CTest y 10/10 pruebas Python superadas**, además del diagnóstico del banco real.
El usuario confirmó posteriormente las puertas y la ausencia del fragmento
adicional. Las métricas anteriores corresponden al mezclador y SDL dummy.

Archivos de esta corrección: `include/audio_mixer.h`, `src/audio_mixer.cpp`,
`include/audio_player.h`, `src/audio_player.cpp`, `include/game_audio.h`,
`src/game_audio.cpp`, `tools/audio_probe.cpp`, `tests/audio_tests.cpp` y este
documento. No hay cambios en `main.cpp`, puertas, recursos, renderizado ni
decodificación en esta corrección.

Archivos nuevos: `include/audio_data.h`, `src/audio_data.cpp`,
`include/audio_mixer.h`, `src/audio_mixer.cpp`, `include/audio_player.h`,
`src/audio_player.cpp`, `include/game_audio.h`, `src/game_audio.cpp`,
`tools/extract_audio.py`, `tools/audio_probe.cpp`, `tests/audio_tests.cpp`,
`tests/test_extract_audio.py` y este documento. Modificados: `CMakeLists.txt` y
`src/main.cpp`.


## Auditoría de continuación (2026-11-02)

### Estado conservado y alcance

Al retomar, Git contenía numerosos cambios locales de gráficos, movimiento,
puertas y audio, incluidos archivos sin seguimiento. No se han descartado ni
restaurado. La compilación y las 13 suites CTest pasaban antes de modificar código.
El audio de puertas y pasos, la corrección de la cola, la cadencia y sus pruebas
ya estaban implementados. La documentación seguía describiendo una integración
solo de puertas: ese era un trabajo incompleto comprobable.

No había implementación de respiración, selección de ambientes por sector,
maquinaria ni control de bucles ambientales desde `GameAudio`. No se ha encontrado
un stub de oxígeno que completar. El soporte genérico de bucles del mezclador sí
existía y se conserva. Esta entrega cierra la auditoría, la corrección de recursos
y la documentación del bloque verificable; **la recuperación ambiental queda
parcial, pendiente de investigación y de futuras mecánicas**. No debe marcarse
como implementación completa de los ambientes del original en el roadmap.

### Respiración y oxígeno: evidencia y límites

- **Confirmado en recursos:** BANK[26] y OXYGENE.DSS tienen el mismo PCM,
  11125 muestras a 5000 Hz (2,225 s), flag de bucle activo, comienzo 0 y final
  exclusivo 11124. Tras la primera pasada completa, cada vuelta usa 11124 muestras
  (2,2248 s). Esta identidad no identifica por sí sola una respiración.
- **Confirmado en código:** `0x1750f..0x17517` selecciona AX=26 y BX=0 y llama
  a la ruta directa `0x13123`. `0x1754a` entrega el puntero de datos `0x177db`
  a una llamada de animación; en archivo `0x179db` está `DATA\MORT1.ANM`.
  Se reservan dos buffers de 64800 bytes y otro de 4096 en esa misma rama.
  `0x17240..0x17248` selecciona esa rama cuando AX=1; no es una petición
  incondicional del bucle de desplazamiento. Después se espera a que el contador
  `[0x1428]` supere 160 (`0x17558..0x1756a`) y se liberan buffers.
- **Confirmado en código:** `0x16ad8..0x16b16` resta `[0x1428]` de `[0x548d]`,
  duplicando el decremento si `[0x5409] != 0`. Al agotarse, fija `[0x5429]=-1`;
  por debajo de 19200 envía al mostrador de mensajes el puntero `0x217e9`.
  Esa cadena, en archivo `0x219e9`, dice `OXYGEN ABOUT TO FAIL`.
  La rama inferior a 4800 modifica estado visual. **En ese tramo no hay llamada
  a la petición de sonido**. No se deduce de ello que ninguna otra ruta tenga audio.
- **Confirmado al ampliar supervivencia:** `0x76c4..0x76d9` consulta oxígeno
  cero y llama con AX=1 a `0x1723a`, que selecciona esa rama. La muerte por
  oxígeno y BANK[26] ya están unidos por el llamador, no por el nombre del recurso.
- **Pendiente:** identificar inequívocamente el PCM de respiración y demostrar
  sus disparadores, reloj y dependencias. No hay evidencia suficiente para
  configurar respiración permanente, periódica, por movimiento o por salud.
  Su frecuencia de repetición en juego se desconoce; no se inventa un intervalo.
  El contador y la muerte lógica ya existen; las secuencias visuales de muerte
  quedan pendientes. OXYGENE se activa al morir por oxígeno, conservando su bucle
  DSS, nunca como ambiente provisional. No se ha identificado otra respiración.

### BANK[14] tampoco demuestra respiración

En `0x16be0..0x16c52` se resta `[0x1428]` del contador `[0x608f]`.
Si el resultado es negativo, muestra el texto de datos `0x21890` (archivo
`0x21a90`: `POWER IS SURGING`) y solicita ID=14 en `0x16bfb..0x16bff`.
Con resultado inferior a -640, repone el contador a `(aleatorio & 65535)+9600`.
La inicialización de `[0x608f]` a 9600 está en `0x6aea`.
Otra ruta, `0x16c57..0x16c65`, solicita ID=14 cuando `[0x60b7]==16` y decrementa
`[0x60b9]`; al agotarse fija `[0x5429]=-1`. En `0x95f3` se incrementa `[0x60b7]`
durante una interacción. No se ha identificado completamente esa mecánica.

BANK[14] tiene 4608 muestras (0,9216 s), flag de bucle y metadato de petición
continua. El scheduler `0x12f2f..0x12f68` conserva una voz continua si vuelve a
recibir petición; si deja de recibirla, la detiene. No se puede convertir cada
petición por frame en otra copia del sonido. Estos hechos respaldan un efecto
de estado/alerta, **no una clasificación auditiva como respiración**. No se añade
el contador aleatorio ni ninguna mecánica de daño, energía o inventario.

### Maquinaria: qué está identificado

BANK[11] tiene el nombre de cabecera `moteur ascenseur`, 3853 muestras (0,7706 s),
volumen 64, flag de bucle activo y metadato continuo 255. Primera pasada: 3853;
vueltas: [0,3852), 0,7704 s. En `0x1858b..0x18593`, AX=11, BX=0 llama a
`0x13123`: reproducción directa en voz 0, en una rutina que configura buffers
visuales y estado local (`0x184fb..0x18585`). Hay otras llamadas directas del
mismo ID, localizables con la herramienta; esto no demuestra un motor asociado
a cada puerta o a un sector concreto. No se incorpora un ascensor inexistente.

Los registros sin nombre 13 y 18..22 tienen metadato continuo. Solo 22 tiene
además bucle DSS; en los restantes, el metadato continuo **no significa que el
PCM vuelva automáticamente al inicio**. No se ha demostrado qué instalación,
objeto, estado o vista los activa en el nivel 1. El catálogo conserva duración,
bucles y prioridad sin atribuirles una función inventada. VENT, CABINE, DEPART y
ALARME también se conservan como recursos identificados por archivo, no como
emisores colocados en el mapa. No se añaden coordenadas ni atenuación supuestas.

### Catálogo del banco

Los nombres son los de la cabecera, no una clasificación funcional validada.
`sin nombre` corresponde al relleno de puntos. En la columna de bucle se indica
el intervalo de las vueltas posteriores; la primera pasada utiliza todo el PCM.
Los IDs integrados son 7 y 27, más 25 y 26 como eventos de muerte confirmados
en la ampliación de supervivencia.

| ID | Nombre del recurso | Duración (s) | Volumen /64 | Continuo | Bucle DSS |
| --- | --- | --- | --- | --- | --- |
| 0 | arme vide | 0.2822 | 64 | 0 | no |
| 1 | pistolet simple | 0.5450 | 64 | 0 | no |
| 2 | mitrailleuse simple | 0.4378 | 64 | 0 | no |
| 3 | mitrailleuse simple | 0.4378 | 64 | 0 | no |
| 4 | lance flamme | 0.7664 | 64 | 0 | no |
| 5 | sin nombre | 0.7538 | 64 | 0 | no |
| 6 | tir laser | 0.2676 | 64 | 0 | no |
| 7 | pas | 0.3804 | 16 | 0 | no |
| 8 | echo metallique | 0.8874 | 16 | 0 | no |
| 9 | coup karate | 0.3754 | 64 | 0 | no |
| 10 | coup karate | 0.3490 | 64 | 0 | no |
| 11 | moteur ascenseur | 0.7706 | 64 | 255 | [0,3852) |
| 12 | touche clavier | 0.1092 | 64 | 0 | no |
| 13 | sin nombre | 1.1070 | 32 | 255 | no |
| 14 | sin nombre | 0.9216 | 64 | 255 | [0,4607) |
| 15 | crie monstre | 0.5450 | 64 | 0 | no |
| 16 | monstre touche | 0.9964 | 64 | 0 | no |
| 17 | explo monstre | 0.9356 | 64 | 0 | no |
| 18 | sin nombre | 1.8116 | 32 | 255 | no |
| 19 | sin nombre | 2.1100 | 32 | 255 | no |
| 20 | sin nombre | 2.7402 | 32 | 255 | no |
| 21 | sin nombre | 0.4624 | 32 | 255 | no |
| 22 | sin nombre | 2.9152 | 16 | 255 | [0,14575) |
| 23 | sin nombre | 0.5874 | 64 | 0 | no |
| 24 | sin nombre | 1.4596 | 64 | 0 | no |
| 25 | sin nombre | 6.7832 | 64 | 0 | no |
| 26 | sin nombre | 2.2250 | 64 | 0 | [0,11124) |
| 27 | sin nombre | 0.7400 | 64 | 255 | no |
| 28 | sin nombre | 0.3536 | 64 | 0 | no |
| 29 | sin nombre | 6.1018 | 64 | 0 | no |

### Calidad, ciclo de vida y regresiones

Se conservan las capas y sus interfaces: no hay razón concreta para reorganizar
el mezclador o introducir un sistema de emisores sin disparadores demostrados.
El mezclador toma una referencia a un banco estable: su propietario `Player`
no lo reemplaza con un callback activo. Las órdenes y estadísticas toman el
bloqueo del stream. Los buffers del callback están en la pila; el banco y los
contadores se preparan fuera de él. No existe una cola de efectos pendientes.

Problema corregido: `close()` conservaba el banco decodificado y `effectCount()`
seguía informando sonidos tras cerrar. Un fallo de inicialización del subsistema
también conservaba PCM. Ahora se retira y espera el callback, se destruye el
stream/dispositivo, se limpian voces, se libera el banco y se reinician métricas;
la ruta de error usa esa misma limpieza. El cierre repetido es seguro.
`GameAudio` se destruye antes del renderer/ventana y antes de `SDL_Quit()`.
No se ha modificado la lógica de pasos, puertas, movimiento, renderizado ni main.

Pruebas añadidas: límites de primera pasada y vueltas de un bucle nativo,
observaciones sin reinicio, parada y silencio posterior, reinicio con cursor y
envolvente nuevos, coexistencia con un efecto puntual y limpieza de una voz
continua. En SDL dummy: liberación del banco al cerrar/fallar, reapertura tras
fallos de dispositivo o archivo, cierre con bucle activo y dos propietarios
independientes del subsistema. Estos son tests de capacidad del reproductor;
no simulan un disparador ambiental del juego todavía desconocido.

### Reproducir la investigación

```sh
python3 tools/inspect_audio_code.py ~/Desktop/In_Extremis/EXTREMIS.EXE
python3 tools/inspect_audio_code.py ~/Desktop/In_Extremis/EXTREMIS.EXE --section state
python3 tools/extract_audio.py --data-dir ~/Desktop/In_Extremis/DATA
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/audio_tests build/audio-test-data ~/Desktop/In_Extremis/DATA
python3 -m unittest discover -s tests -p 'test_*.py'
```

`inspect_audio_code.py` comprueba el SHA-256 antes de aplicar offsets, reutiliza
el desensamblador LLVM existente y muestra código, bytes, cadenas y candidatos
a llamadas directas E8. Los candidatos son una búsqueda de bytes, **no un grafo
de llamadas completo**: pueden incluir datos, y no cubren llamadas indirectas,
saltos o código modificado en ejecución. Se desensamblan por separado las ramas
citadas para comprobar sus instrucciones. Los punteros de datos de esta imagen
se traducen a archivo sumando 0x200; no se mezclan con los offsets de código
indicados en las tablas. La herramienta no modifica EXE ni recursos.

Resultado de esta auditoría: compilación correcta en `build`, **13/13 suites
CTest** y **12/12 tests Python** superados; prueba C++ opcional de los **44
registros originales** superada, incluida la regresión de la cola real de puertas.
Se ha ejecutado también la inspección de todas las secciones del EXE verificado;
salida reproducible local en `build/audio-debug/verified-audio-code.txt`.
No se ha realizado una nueva escucha ni validación interactiva del nivel.

Archivos cambiados en esta continuación: `src/audio_player.cpp`,
`tests/audio_tests.cpp`, `docs/audio-system.md`; nuevos
`tools/inspect_audio_code.py` y `tests/test_inspect_audio_code.py`.
Los listados de archivos de las secciones históricas corresponden a trabajos
anteriores conservados, no a nuevas modificaciones de esta auditoría.

### Comprobación manual y pendientes

En el primer nivel: permanecer quieto y girar (sin pasos), caminar y deslizarse
contra paredes (pasos solo con desplazamiento), empujar una pared estando bloqueado,
abrir/cerrar con E y con O, caminar durante una puerta en movimiento y cerrar la
aplicación mientras suenan efectos. Comprobar que la puerta no repite su inicio
al terminar y que no cambió el ritmo ya validado de las pisadas. Repetir la
apertura de la aplicación después de cerrarla. Las pruebas dummy no sustituyen
esa escucha. No deben esperarse respiración o maquinaria nuevas en esta entrega.

Para continuar la fidelidad ambiental se necesita seguir las rutas de objetos,
interacciones y secuencias hasta sus condiciones en GAME.LAB, además de comparar
con el original en ejecución. Siguen pendientes la identificación auditiva de
respiración, el mapa de activación de maquinaria y el arbitraje completo de
prioridades. Los efectos de ascensores e inventario requieren todavía sus
mecánicas y rutas verificadas. La ampliación de supervivencia ya conecta los
dos efectos de muerte; no añade música, sonidos sustitutivos ni emisores arbitrarios.

## Eventos de muerte integrados

`SurvivalSession` transforma una sola transición de estado en `beginDeath`:
salud cero → BANK[25], puntual de 6,7832 s; oxígeno cero → BANK[26], con el bucle
nativo del DSS. La selección se demuestra en `0x76a9..0x76d9`,
`0x1723a..0x17253`, `0x17300..0x1730c` y `0x1750f..0x1751b`.
No hay peticiones nuevas por fotograma ni cambios de volumen/cadencia en puertas
o pasos. La lógica de muerte no depende de que haya dispositivo de audio.

El final visual de MORT0/MORT1 está pendiente: el port mantiene una transición
explícita sin acciones del jugador. BANK[26] continúa durante esa espera y se
detiene con `endDeath` al reiniciar el diagnóstico, o se libera al cerrar.
Esta espera no emula la duración original de MORT1. En DOS, `0x17486` y
`0x1759e` llaman a la parada de voces `0x13090` al finalizar sus secuencias;
esa misma responsabilidad corresponderá al futuro presentador de muerte.
No se inventa un tiempo de corte ni se convierte el bucle en una respiración
continua durante la partida.

## Fase B: confirmación de armarios

La recogida introduce una sola muestra adicional integrada: **BANK[12]**,
petición original AX=12, voz 1, llamada directa `0x13123` en
`0x18f97..0x18f9f` (suministros) y `0x8c0b..0x8c13` (armas).
`ItemSession` solicita `GameAudio::confirmItemSelection` una vez por transferencia
aceptada. Intentos repetidos sobre contenido vacío, teclas repetidas, simulación
inactiva y muerte no provocan otra confirmación. Un segundo intercambio real
de armas sí es una operación nueva. Se reutilizan mezcla y cierre existentes.

La presentación modal DOS sigue pendiente; el port no reproduce todavía la
posición temporal exacta del sonido dentro de la animación del armario. BANK[11]
no tiene un uso exclusivo de ascensor: `0x1858b..0x18593` lo solicita dentro de
esa animación, junto a otros efectos de presentación como BANK[18]. No se lanza
un motor ambiental permanente ni se añade audio arbitrario a H/J. Los eventos
de puertas y pasos permanecen intactos. Catálogo, evidencia y limitaciones en
[items-and-inventory.md](items-and-inventory.md).

## Interfaces originales: confirmación y tarjeta aceptada

La integración modal corrige el momento descrito en la recogida provisional:
BANK[12] se solicita al confirmar **Yes o No**, antes de ambas ramas, como en
0x18f97 y 0x8c0b. Abrir con G o cancelar con Escape no reproduce esa confirmación.
Los teclados de código y selectores solicitan BANK[28] en sus eventos verificados
(0x19e60, 0x1af21, 0x1af97). BANK[19] acompaña una tarjeta aceptada en el lector
(0x95ab) y en el terminal de códigos (0x9db1), sin peticiones por renderizado.
Su DSS tiene loop_flag=0; la petición directa no se convierte en un motor infinito.

El mezclador continúa funcionando mientras se pausa la simulación del mundo.
Motores y sonidos ligados a las animaciones de zoom/apertura/transferencia aún
no se integran: requieren recuperar esas secuencias. No cambian los eventos de
puertas, pisadas o muerte. Véase [original-interactions.md](original-interactions.md).
