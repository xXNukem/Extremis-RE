# Enemigos originales del nivel 1 — Paso 204

## Estado vigente

El Paso204 añade explosión mundial, fragmentos, GOUTTE y detector de presencia
con batería. Véase la sección específica del Paso204 al final.

Los nueve enemigos se crean desde GAME.LAB, se animan, persiguen por enlaces
originales, reciben impactos y daño, reaccionan y mueren. El ataque de banco1
ya reduce la salud real del jugador. Mapa, modales, falta de foco y muerte del
jugador suspenden la simulación sin recuperar tiempo acumulado al reanudar.

Se conserva MONS1, la paleta, el renderer y su depth buffer, los proyectiles
existentes, Inventory, Vitals y GameAudio. No hay un segundo sistema de armas,
selección numérica, enemigos inventados, IA aleatoria, NavMesh ni nuevos niveles.
Los límites de fidelidad se enumeran abajo: no se presenta como emulación exacta.

## Evidencia y datos originales

Offsets absolutos de EXTREMIS.EXE, SHA256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Dirección de datos +0x200 corresponde al archivo.

0x11335..0x113a8 carga los bancos del encabezado y asigna la sección +0x0c a
[0x519e]. Nivel1 comienza en GAME.LAB0x7e: relativo0x14e, absoluto0x1cc,
count9 seguido por nueve registros de28bytes, hasta relativo0x24c.
0xe067..0xe07d y0xf839..0xf8bd recorren estos registros. No son entitiesA ni
la sección de enlaces +0x08. La lectura valida límites, cantidad y sectores. La tabla +0x10, relativo0x254,
tiene count0 en nivel1.0x10584..0x10681 puede reutilizar registros de conducta4
retirados desde generadores, pero no hay generadores en este nivel: no se añade
reaparición a sus nueve registros iniciales.

| Índice | Sector (base0) | XY local | XY global | Conducta |
| --- | --- | --- | --- | --- |
| 0 | 3 | 300,100 | 4100,2100 | 2 |
| 1 | 5 | 700,500 | 2500,1100 | 2 |
| 2 | 6 | 300,300 | 1900,1500 | 2 |
| 3 | 7 | 100,700 | 1500,2900 | 2 |
| 4 | 14 | 500,500 | 1500,1900 | 2 |
| 5 | 12 | 700,700 | 700,1500 | 2 |
| 6 | 11 | 900,700 | 1100,1100 | 2 |
| 7 | 10 | 300,200 | 1500,800 | 2 |
| 8 | 12 | 700,500 | 700,1300 | 4 |

Todos: salud4, estado6, fase0, ángulo0, polígono0xffff, sprite0x8000,
elevación0. Destino de origen coincide con XY inicial. Conducta2 frente a4
no demuestra dos especies: comparten banco y salud.

Los catorce words de EnemyRecord preservan el orden original:
+0 salud; +2 conducta; +4 estado; +6 fase; +8 sector; +10/+12 XY;
+14 polígono; +16 ángulo (720 por vuelta); +18 reservado sin interpretar;
+20 imagen/flags; +22 elevación/contador superior; +24/+26 origen.
Los sectores conservan además sus rutas del encabezado +0x08, sección0x6e:
15 offsets relativos, y en cada destino count/lista de(sector,globalX,globalY).
No se inventan conexiones cuando no existe ruta.

Existe un campo de banco MONS por nivel. Eso confirma un banco principal por
nivel, no una única IA para todos: el EXE tiene ramas específicas por banco5/6.
El runtime de este módulo solo admite MONS1, el banco auditado del nivel1.

## Recursos y representación

0x7dcc..0x7e47 carga MONS*.SPR y MONS*.SPT y copia cabeceras.
MONS1.SPR contiene20 imágenes del formato SPR compartido. Se mantienen PAL1,
índice0 transparente, negros opacos, dimensiones y anclajes originales.
0xfa30..0xfa91: altura mundo =270×alturaFrame/120, con constante banco1 en
archivo0x59bd. Frame0 es78×120, ancla38,119, Z inicial0.
El flag0x2000 se traduce al flip horizontal existente.

sprites::render utiliza el mismo depth buffer que decoraciones, proyectiles,
paredes y puertas. El cambio de imagen conserva la escala proporcional y los
anclajes; no se redibuja arte ni se repiten entidades al actualizar la escena.
La perspectiva conserva el port: viewport320×150, horizonte75 y FOV70°.
No se replica el sesgo de profundidad DOS+50 ni su horizonte50.

MONS1.SPT tiene160bytes =20 máscaras de8×8. En0x17ee8..0x17f59 se normaliza
el punto proyectado, se consulta la fila y el bit `1<<columna`:
**bit1 rechaza, bit0 admite impacto**. No son secuencias de animación.
Se usa esa polaridad y la máscara del frame visible, también con el rectángulo
proyectado espejado. Como en el EXE, no se invierte de nuevo el byte de máscara.

## Estados y animaciones

- Movimiento:0xe696..0xe703, imágenes0..4, espejo al completar el ciclo.
- Espera, estado6:0xe718..0xe74d, contador módulo32 y espejo al volver a0.
  Tabla archivo0xf1ff:
  `4,4,4,4,5,5,5,5,4,4,4,4,5,5,5,5,4,4,4,4,5,5,5,5,5,6,8,8,7,6,5,4`.
- Ataque, estado2:0xe2ae..0xe300, tabla archivo0xf1f1:
  `5,6,8,8,7,6,5`. Incrementa antes de consultar; termina al alcanzar7.
  Al iniciar ataque,0xe580..0xe59f salta a esta rutina: la primera fase visible
  es1/imagen6, no una fase0 añadida por el port.
- Reacción, estado4:0xe10d..0xe16c, fase1 muestra9; vuelve a estado0.
- Muerte, estado5:0xe22b..0xe29b, alterna5/9 durante ocho actualizaciones;
  al finalizar usa salud0xffff para retirar la entidad. No queda cadáver fijo.

Cada enemigo mantiene su salud, estado, contador, posición, orientación y flags.
Una muerte no vuelve a ejecutarse; load reinicia desde los datos del nivel.
Mapa, redibujado e inventario no reconstruyen los registros iniciales.
Las imágenes10..19 no se insertan en ciclos por ser consecutivas.

## Temporización: diferencia deliberada

0x7026 llama0xe018 una vez por iteración DOS. La animación aumenta su contador
por iteración. El movimiento se ajusta por[0x1422] (estimador de FPS):
0xe018..0xe058 usa, para banco1,
`max(5, 10*floor(floor(30*18/[0x1422])/10))`.

**18 es una referencia de movimiento, no un reloj fijo original demostrado.**
Para hacer funcional la simulación e independiente del renderizado se normaliza
el port a18 pasos/s y30 unidades/paso. Las poses y sus transiciones son las
recuperadas, pero su duración fija es una adaptación explícita. No se afirma
que reproduzca las duraciones de cualquier máquina DOS. El disparo conserva
su reloj anterior; no se ha cambiado su cadencia para seguir este paso de IA.

Se descarta el intervalo de entrada/reanudación y se limita a0.25s cada intervalo
activo para evitar recuperaciones largas tras un bloqueo del proceso. Durante
pausas se conserva la reacción pendiente sin descontar salud. Las pruebas
comparan movimiento, fases y salud a30 y120 FPS.

## Activación, navegación y movimiento

0xe08a..0xe0b9 activa conducta2→3 al aparecer el sector en la lista visible
[0x5128]. En el port se activa en el sector del jugador, también mirando hacia
otro lado; para otros sectores se usa visibilidad del monstruo en la proyección
actual y línea despejada. Es una aproximación más restrictiva que recuperar
la lista DOS completa de sectores visibles por portales. Conducta4 persigue
desde el inicio cuando existe ruta; no necesita esa activación visual.

0xe38b..0xe4c5 elige origen o jugador y consulta rutas.0xf627..0xf723 y
0xf723..0xf838 recorren conexiones de polígonos/sectores evitando ciclos,
retienen el primer coste mínimo, y acumulan distancia de Chebyshev por puntos
de paso (sin coste adicional al encontrar el enlace directo al destino).
EnemyNavigation reproduce esa selección sobre las tablas ya decodificadas.
La geometría compartida localiza sector/polígono y convierte coordenadas.
Se adelanta el destino de navegación al alcanzar un punto de paso, sin detener
permanentemente al actor en el borde entre polígonos o sectores.

0xf2cf..0xf4da: en el mismo polígono, ambos deltas inferiores a100 solicitan
acción de ataque.100 es proximidad por ejes, **no un radio físico**.
El giro usa la mitad de la diferencia angular, mínimo4 unidades de720, y
avanza cuando la diferencia es menor que180. El port mantiene esa regla.

El movimiento reutiliza collision::move con el mundo dinámico de DoorControl;
no atraviesa paredes ni puertas cerradas y solo acepta posiciones localizables
en polígonos. La corrección es la del port: cuerpo circular común de radio30,
no una traducción exacta de0x1074b/0x10db7. El EXE tiene tolerancias80/30 según
la arista (0x10849..0x10877), que no se presentan como un radio circular80.
0x11029..0x11173 confirma el contacto con el jugador: caja de ±100 por eje,
solo en su sector y para registros no retirados. Se corrige el destino primero
en Y y después en X según el lado anterior, manteniendo el skin numérico común
para no filtrar por el borde. Se vuelve a comprobar la geometría si hubo corrección;
las pisadas y el consumo de carrera observan el desplazamiento final. Un cuerpo
moribundo bloquea hasta0xffff. El contacto entre enemigos y la apertura automática
de puertas no se añaden sin resolver sus reglas.

## Impactos, daño y muerte

Se reutilizan los20 proyectiles de WeaponFire, el arma y su munición actual.
Se barre cada tramo contra paredes/puertas y contra el plano del sprite visible.
El candidato más cercano cuya máscara SPT admite el punto gana; una superficie
bloquea al enemigo que está detrás. La interpolación visual desde el fogonazo
no modifica esta trayectoria lógica ni permite alcanzar a través de geometría.

0x17f5f..0x17fdc: el proyectil pasa a impacto2/fase1 y solicita BANK16.
Para un enemigo vivo, fase0, estado4, orientación inversa al tiro y resta de
arma+30. Daños por arma: **1,1,1,2,5,4**. Salud<=0 establece0/estado5.
Estado5 puede detener un proyectil pero no vuelve a recibir daño;0xffff se omite.
La munición solo se descuenta al aceptar el disparo, no al aplicar cada fase.

La prueba original0x17ecb..0x17ee2 usa una ventana de profundidad[0,200) después
del plano del enemigo. El port usa intersección continua en el plano coherente
con su proyección y su retícula; no reproduce esa tolerancia dependiente del
paso y de la perspectiva DOS. Las máscaras y cantidades de daño sí se conservan.

Al terminar las ocho fases se reproduce BANK17 una vez y se retira la entidad.
El Paso204 conecta a continuación las imágenes10..19 y GOUTTE, descritas abajo.

## Ataque al jugador: nueva evidencia del Paso 203

0xe300..0xe386 emite en fase2, exige el sector del jugador y solicita BANK9/10
alternando[0x57b7]. Escribe estado1/valor50 en[0x5493]+10×índice.
0x13f5b..0x13faa demuestra que **50 es duración de un indicador HUD**, no daño.

La misma emisión pone[0x51d6]=1 y[0x51d8]=0.0x165db..0x166b5 consume esa
reacción:0x16645 resta **2 de salud**;0x166a6 compara el contador con **8** y
al completarlo desactiva el efecto. Esto permite implementar el ataque sin
inventar daño50 ni restar por cada frame de renderizado.

Se mantiene una reacción global: ocho pasos de2 unidades. Ataques simultáneos
reinician el contador, no crean ocho daños independientes por monstruo.
El alcance se comprueba al iniciar la animación; en fase2 se comprueba sector,
con una comprobación adicional de línea despejada para impedir daño a través
de paredes/puertas. No se inventan proyectiles enemigos para banco1.
El indicador de miembro del HUD, giro, retroceso y flash de paleta de la reacción
DOS no están reconstruidos: se conecta salud, HUD existente y muerte existentes.

## Sonidos y recursos

GameAudio conserva puertas27, pisadas7 y muerte del jugador25/26.
EnemySystem emite IDs de eventos; no abre un gestor de audio distinto:

| Evento | BANK | Momento |
| --- | --- | --- |
| Ataque válido | 9/10 | Fase2, alternancia global |
| Impacto admitido por SPT | 16 | Una vez por colisión de proyectil |
| Retirada por muerte | 17 | Final de la secuencia, una vez |

Se reproducen una sola vez, con el mezclador existente. No se reinician al
redibujar poses, descontar los2 puntos de reacción, abrir/cerrar un modal o
cambiar de vista. No se silencian indiscriminadamente otros efectos.

## Retícula y origen visual del disparo

El EXE0x1810f..0x18254 dibuja ARME[3] solo al disparar, con centro(160,58).
El port conserva la decisión del Paso202: mira estable(160,75), sobre el eje
óptico de su viewport320×150. El disparo lógico se mantiene en XY de la cámara
y Z128. El DOS emite proyectiles físicos en XY del jugador y Z=cámara−30,
con offsets laterales alternos para arma2 (0x17b43..0x17bdb).

El defecto del Paso202 era dibujar también el proyectil de vuelo en el eje
óptico. Ahora se separan posición lógica, efecto de vuelo, fogonazo e impacto.
ARME[2] sigue compuesto por sus offsets originales y recoil+5. Su centro se
proyecta inversamente al primer paso físico de cada arma, usando el FOV común.
Las posiciones del efecto de vuelo interpolan en mundo desde ese emisor hacia
el próximo impacto real. El impacto vuelve al eje lógico; no se desplaza la mira.
No se elige una distancia de convergencia a ojo: se obtiene de geometría/enemigo.
Sin objetivo finito, el offset mundial permanece y su proyección converge
asintóticamente al eje. La pose del emisor queda capturada para cada disparo.

Es una adaptación visual deliberada del port, no la trayectoria inferior DOS
copiada literalmente. Las seis armas usan sus ARME/BALLE originales. Paredes,
puertas y SPT deciden el impacto lógico, no la posición cosmética del efecto.

## Verificación y comprobación manual

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/enemy_tests build/enemy-test-data ~/Desktop/In_Extremis/GAME.LAB
./build/weapon_fire_tests ~/Desktop/In_Extremis/DATA
./build/hud_tests ~/Desktop/In_Extremis/DATA
python3 -B tools/inspect_enemies.py \
  --lab ~/Desktop/In_Extremis/GAME.LAB \
  --exe ~/Desktop/In_Extremis/EXTREMIS.EXE \
  --data-dir ~/Desktop/In_Extremis/DATA
```

El inspector verifica el EXE, las instrucciones de daño y contador; exporta
posiciones, rutas, anclajes, máscaras y tablas a build/texture-debug/enemies.
Los gráficos originales no se modifican. hud_tests con DATA genera capturas
step203-enemy-0..4.bmp y step203-flight-0..5.bmp en build/hud-debug, utilizando
el compositor real. El catálogo visual permite contrastar reposo, movimiento,
reacción9, muerte y retirada, y la salida del proyectil desde el arma.

1. Reiniciar el programa con el binario recompilado y GAME.LAB original; Tab a3D.
2. Mirar de frente a una pared, disparar una vez: el efecto sale del fogonazo,
   se aproxima a la mira y el impacto coincide con su eje. Repetir cerca/lejos.
   Pistol0 requiere pulsaciones; mantener arma1 del armario repite como antes.
3. Conservar C001, su lector y puerta7 mediante E. El primer enemigo está en
   (4100,2100), sector3. Al acceder al sector debe animarse y perseguir.
4. Disparar con la mira sobre él: un impacto válido causa reacción; cuatro
   impactos de arma0 matan al enemigo de salud4. La secuencia parpadea y se
   retira una sola vez. Debe aparecer la explosión, fragmentos y GOUTTE si está cerca; no un cadáver fijo.
5. Acercarse a menos de100 por ambos ejes en su polígono: inicia ataque;
   su fase2 emite sonido y baja salud en pasos de2. Retirarse permite comprobar
   que la reacción termina; no se aplica daño nuevo por cada frame de pantalla.
6. Interponer pared/puerta cerrada: debe bloquear movimiento, impactos y nuevas
   emisiones de ataque. No esperar que el enemigo abra puertas automáticamente.
7. I/ESC, Tab y pérdida de foco suspenden combate. Cerrar manteniendo el ratón
   no dispara: soltar y pulsar de nuevo. Z/X y Shift conservan sus controles.
8. Cambiar de vista no recupera muertos. Reiniciar el programa restaura los9.
   El diagnóstico F10 de vitals, si se activa, no reinicia el mundo.

## Archivos del Paso 203 y límites restantes

- include/src weapon_fire: origen visual, convergencia e impactos con enemigos.
- include/src enemy_system: recursos, estados, movimiento, combate y sonidos.
- include/enemy_navigation.h, src/enemy_navigation.cpp: rutas originales.
- include/src game_lab: tabla de rutas; include/src player_collision: geometría
  de polígonos compartida; first_person_view reutiliza esa comprobación.
- include/camera.h: FOV compartido para render, vuelo y activación.
- include/src game_audio, src/main.cpp: coordinación con simulación, salud y audio.
- CMakeLists.txt; tests/enemy_tests.cpp, weapon_fire_tests.cpp, hud_tests.cpp y
  audio_tests.cpp; tools/inspect_enemies.py y documentación de arma/HUD.

Pendientes de fidelidad: reloj DOS variable frente a la normalización18Hz;
lista completa de sectores visibles; redondeos/trigonometría de words;
colisiones80/30 exactas y contacto entre enemigos; apertura/cierre sobre enemigos;
indicadores, giro/retroceso/paleta de daño. No se añaden
mecánicas de esos puntos sin evidencia completa. Los enemigos ya son funcionales;
estos límites no se ocultan bajo una afirmación de equivalencia total con DOS.

## Resultado de las comprobaciones del Paso 203

Compilación normal y ASan+UBSan completadas. CTest20/20 en ambos builds;
Python unittest16/16. Sin diagnósticos de memoria/UB ni errores de diff whitespace.

Pruebas nuevas: emisión desde boca e impacto centrado para seis armas/cinco
orientaciones; máscaras0/1; enemigo más próximo; bloqueo por geometría; descarte
DOS fuera de vista antes de aplicar daño; consumo único; daño y retirada sin
regeneración; reacción global sin apilar ataques simultáneos; navegación por
polígonos/sectores sin bloqueo en el portal; colisión de jugador y retirada del
cuerpo; pausa/reanudación; equivalencia de estado a30/120 FPS; BANK9/10/16/17.

Con recursos reales: nueve posiciones dentro de sus sectores, rutas originales,
SPT nativo, activación/ataque que reduce salud, HUD y vuelo compuestos; también
bajo sanitizers para enemigos/disparo. inventory_tests, door_control_tests e
interaction_tests mantienen C001/puerta7, intercambio de armario y modales.
Se inspeccionaron las capturas del compositor con MONS1/ARME/BALLE originales;
la comprobación interactiva completa por el usuario sigue siendo necesaria.

Sin commits, pushes, descarte de cambios locales ni modificación de originales.

## Paso 204 — explosión, fragmentos y baba

**Evidencia directa:**0xe257..0xe297 retira el enemigo, solicita BANK17 y guarda
un único contador[0x5777]=0 y puntero[0x5779].0xfc16..0x100ee produce el efecto.
No son ANM: MONS1.SPR contiene las imágenes mundiales, y GOUTTE.SPR el primer
plano. MONS1.SPT continúa siendo máscara de impacto, no animación.

Secuencia de banco1:

1. Fases0 y1: imágenes10 y11, sobre posición/elevación del enemigo retirado.
2. Fases2..8: siete explosiones escalonadas. Descriptor de33words en archivo
   0x5ad5 (66bytes/banco): triples `(desfase,lateral,altura)`:
   `(3,50,200),(3,-20,170),(2,40,140),(1,-30,110),(0,30,80),(0,-40,50),(2,20,20)`.
   Índice de tabla=`fase-2+desfase`; solo0..6. La tabla **de bytes**0x100ef es
   `15,14,13,12,13,14,15`, no una secuencia arbitraria12→13→14→15.
3. En fase0 se inicializan cuatro fragmentos16/17/18/19, que se actualizan
   y dibujan desde fase1 **simultáneamente** con el cuerpo y las siete explosiones.
   No se encadenan después de fase9. Triples `(imagen,lateral,altura)` del
   descriptor: `(16,-60,210),(17,60,210),(18,-60,40),(19,60,40)`.
   La dirección apunta desde la muerte al jugador; el desplazamiento inicial
   lateral es perpendicular a esa dirección (0xfd63..0xfed3).
4.0xff06..0x100b6 intenta avance `100*18/[0x1422]` por iteración. La comparación
   0xff9d..0x10048 cambia a imagen12 cuando cambia XY, no cuando se recibe un
   nuevo impacto de arma. Después aparecen13,14,15 y se retira el fragmento.

DeathEffects conserva un slot independiente de los enemigos, no nuevas entidades
con salud. La última muerte procesada sustituye al efecto mundial anterior,
como el puntero global DOS. GOUTTE mantiene su contador independiente: una
muerte lejana puede reemplazar el efecto mundial sin borrar una salpicadura
cercana aún visible. Load reinicia ambos estados. Ninguna de estas imágenes
entra en las máscaras de tiro o en las cajas de colisión del enemigo retirado.
Los sprites comparten el banco MONS1 ya cargado, sin copiar ni redibujar recursos.

La altura mantiene270×alturaFrame/120 y sus anclajes. Los desplazamientos
laterales de las siete explosiones son relativos a la proyección de cámara;
los fragmentos tienen posiciones mundiales. Se conserva la oclusión por píxel.
Los colores originales incluyen explosiones naranja y fragmentos verdes; no se
recolorean para convertir toda la secuencia en un efecto verde.

### GOUTTE y composición

0x692b carga `data\\goutte.spr` (nombre en archivo0x4c18),16 imágenes en dos
familias de8. Se mantienen PAL1 e índice0 transparente. Las imágenes cambian
anchura/altura(5×20…1×39); la segunda familia tiene tonos oscuros/grises auténticos.
No se sustituye por un rectángulo verde ni se escala a toda la pantalla.

0xfdd4..0xfdfd y0x10013..0x1003c ponen[0x57ad]=0 si
`max(abs(dx),abs(dy))<400`: al iniciar el efecto y cuando cambia XY de un
fragmento cercano. No hay prueba de arma, movimiento del jugador o línea de
visión adicional en esas rutas. No todas las muertes lejanas salpican la cámara.

0x13aa2..0x13b93 compone el contador0..29. Hay43 posiciones `(x,y,retraso)`
en0x13bb6, precedidas por count43 en0x13bb4. Para cada gota:
`local=floor(contador/2)-retraso`; si0..7, imagen=`local+((43-índice)&1)*8`.
Se añade a X el word de0x13b94 con stride4: `0,0,0,1,2,2,2,2`.
No se confunden los words intercalados con ocho desplazamientos DWORD.
Al alcanzar30 se desactiva. Se reconstruye cada framebuffer desde la escena,
por lo que las gotas que terminan no dejan píxeles de la fase anterior.

Orden recuperado en el bucle: efecto mundial0x712a; arma/retícula0x727a;
GOUTTE0x7284; casco/panel/mensajes0x72c9. El port mantiene ese orden de capas:
GOUTTE encima del arma y retícula, debajo de esquinas/mensajes y modales,
**recortado a x0..319,y0..149**. El panel inferior y detector no se manchan.

### Tiempo, finalización y límites

Se usa el paso normalizado18Hz del Paso203: fases mundiales, avance100/paso y
contador de gotas30/pasos (1.667s desde su último reinicio). Esa frecuencia fija
es adaptación del port; el EXE aumenta contadores por iteración, no demuestra
18Hz de animación. Las pruebas comparan el estado a30 y120 FPS de renderizado.
El render no modifica contadores; mapa, modales, foco y muerte del jugador
conservan la pausa del sistema existente.

Los fragmentos usan el corrector geométrico compartido del port, no una copia
completa de0x1074b/0x10db7. Si no cambia XY conservan la imagen de fragmento,
como la comparación del EXE; no se inventa un TTL para ocultar ese caso. Después de fase8 y al
terminar los cuatro se limpia el slot aunque hayan terminado en pasos distintos:
el port cuenta todos los inactivos, evitando depender del contador temporal BP
de0x10050. Las tolerancias geométricas DOS exactas y los casos de fragmento
bloqueado siguen siendo una limitación de fidelidad. Los casos abiertos y la
escena original comprobada terminan sin residuos.

Solo se implementa el descriptor de MONS1. Otros bancos tienen descriptores
propios; banco9 tiene la rama especial de fin de juego en0xe25e..0xe27a.
No se generaliza esta secuencia a los29 niveles pendientes.

### Audio demostrado

BANK17 contiene «explo monstre»,4678 muestras a5000Hz, sin flag de bucle ni
continuous. Se solicita una vez al comenzar la explosión, no al acabar cada
fragmento o cada gota. El audio conserva su cola natural de0.9356s.
`EXPLO.DSS` coincide en PCM con **BANK29**, no con BANK17: no se reasocia por nombre.
0x13103 marca un byte pendiente por ID; solicitudes iguales de la misma tanda
se agrupan. Se conserva BANK16 para impactos y el resto del audio validado.
Las rutinas0x13aa2..0x13b93,0x100f6..0x101e4 y0x13e00..0x13ef1 no solicitan
sonidos de baba, barrido ni detección. No se añaden pitidos. El sonido genérico
de confirmar un menú sigue perteneciendo a la interfaz existente.

## Paso 204 — detector conectado y batería

El detalle de cálculo/render está en [original-hud.md](original-hud.md).
El detector lee EnemyRecord actual, incluidos inmóviles y estado5/salud0, y
omite solo0xffff. No detecta decoraciones, proyectiles ni los efectos de muerte.
El contador usa los pasos activos del sistema, incluso cuando está apagado;
activar no reinicia una animación nueva. La lectura es acotada a diez contactos,
sin asignaciones dinámicas ni raycasts o navegación extra.

Original nuevo juego: bateríaDWORD−1 en archivo0x1d8be, cargada en0x1deea;
detector0 en0x6abd. Los paneles reales del nivel1 tienen tarjetas/arma/inyector,
**ninguna batería**. Por ello el juego normal no recibe una batería inventada
ni un radar gratuito. Inventory y la ruta de recogida existente admiten la clase0 demostrada, sin
añadir ubicaciones al mapa, y la sección0 del
inventario existente puede encender/apagar el detector con carga.

Para comprobarlo sin alterar la partida normal se amplía exclusivamente el modo
ya existente `--vitals-debug`: **F11 equipa una batería de prueba900s cuando no
hay una cargada y activa el radar**. No es una tecla DOS ni una recogida del nivel.
F11 no hace nada en modo normal. La óptica/linterna que comparte batería en DOS
no se implementa ni se presenta como una opción funcional.

Comprobación manual adicional:

```sh
./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB --vitals-debug
```

1. Tab a3D; F11 (o Fn+F11 según teclado). El detector debe barrer y mostrar
   contactos reales, también tras paredes/puertas. Girar/desplazarse cambia XY.
2. I, sección de batería (primera fila), E para acciones y E para apagar;
   I/ESC cierra. Repetir para encender. La carga no se gasta dentro del modal.
3. Acercarse a un enemigo y matarlo: comprobar10/11, explosiones escalonadas,
   fragmentos intercalados y gotas sobre el viewport; HUD inferior legible.
4. El punto puede continuar durante estado5 hasta que se retira el enemigo;
   después desaparece, sin convertir fragmentos en nuevos contactos.
5. I/Tab/foco pausa también los efectos; al reanudar no se recuperan segundos
   inactivos. Varias muertes sustituyen el slot mundial, no apilan explosiones
   ilimitadas ni reproducen BANK17 por frame. Reiniciar restaura la ausencia
   de batería y limpia efectos, lectura y contador.

Archivos del Paso204: include/src enemy_effects (nuevos), motion_detector y su
tabla privada (nuevos); enemy_system, original_hud, sprite_renderer,
first_person_view, player_inventory, survival_session, original_interactions,
item_session, level_items, main y CMake; tests/motion_detector_tests (nuevo), enemy_tests,
hud_tests, inventory_tests, interaction_tests y survival_session_tests; tools/inspect_enemies.py
amplía el manifiesto verificable; estas dos páginas actualizan la evidencia.
