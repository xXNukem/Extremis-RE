# Disparo del arma equipada — fase 1 (2026-11-03)

## Resultado y alcance

Botón izquierdo dispara el arma y munición actuales de Inventory. Armas0/4
requieren nueva pulsación;1/2/3/5 repiten mientras se mantiene. El consumo es una
unidad por emisión aceptada, después de comprobar munición y ranura libre entre20.
Se conservan el intercambio de armario, WASD/E/Z/X/Shift/I/Tab, HUD320×200,
viewport320×150 y presentación4:3. No hay selección numérica, arsenal, enemigos,
IA, daño ni cambios a MONS/SPT o recursos originales. Sin commits ni pushes.

## Evidencia y reloj

EXE SHA256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Offsets absolutos; dirección de datos+0x200 corresponde al archivo.
`tools/inspect_interactions.py` verifica el hash y exporta los rangos ampliados y
los56 words de cada descriptor de arma a build/panel-debug.

[0x1422] **no es el reloj de160ticks/s ni una cadencia de arma**. Es un estimador
de FPS utilizado para convertir la espera a iteraciones del juego:

- 0x184d..0x1857 programa PIT con divisor7457 (1193182Hz de referencia).
- 0x18d1..0x190f cuenta interrupciones; cada80IRQ toma[0x1424]×2 y lo guarda
  en[0x1422], reiniciando[0x1424] a **1**, no0.
- 0x738e..0x7395 incrementa[0x1424] una vez por iteración normal del juego.
- 0x6c25..0x6c39 impone mínimo9 al valor utilizado por el juego.
- 0x17b08..0x17b1f calcula `N=floor(k×[0x1422]/18)+1`.
- 0x17a4b..0x17a5f descuenta N sin emitir; la siguiente iteración puede disparar.

El port conserva ese algoritmo **por iteración activa**, con la ventana
`80×7457/1193182` segundos medida mediante elapsed SDL. No se ha inventado un
valor constante de disparos/segundo ni una simulación a18Hz. Se conservan el
contador, su reset a1, redondeos y mínimo9. Por eso la espera en segundos tiene
la dependencia de FPS/redondeos del original. La pose de disparo dura una
iteración, como[0x5213]=0→−1 en0x17a2d..0x17a45.

| Arma | k, descriptor+28 | Nueva pulsación | Sonido |
| --- | --- | --- | --- |
| 0 | 4 | Sí | BANK1 |
| 1 | 3 | No | BANK2 |
| 2 | 2 | No | BANK3 |
| 3 | 3 | No | BANK4 |
| 4 | 6 | Sí | BANK5 |
| 5 | 1 | No | BANK6 |

La inicialización y fase del estimador se adaptan al reloj SDL y a la pausa del
port: no se emula una IRQ concurrente. El intervalo de reanudación se descarta,
sin reproducir disparos acumulados. Falta comparación DOS interactiva de fases
de temporizador; no se presenta como emulación ciclo a ciclo.

## Munición y ciclo de vida real

0x17a73 comprueba munición.0x17ab3..0x17ad3 busca primero un estado0 entre20
registros de12bytes. Solo después0x17b25..0x17b40 descuenta una unidad y escribe
estado1. Sin munición o sin espacio se conserva el inventario; el EXE solicita
BANK0 también si la tabla está llena. La lógica del port devuelve resultado
None/Fired/Empty/Full para que GameAudio solicite el efecto correspondiente.
No depende del éxito del dispositivo de sonido para crear un proyectil válido.

WeaponFire mantiene20 registros con posición, dirección, arma de origen y fase:

- Movimiento en estado1:0x17c4d..0x17c6a, desplazamiento del descriptor+32
  por iteración. Velocidades originales80/80/80/40/80/100; no son unidades/s.
- 0x17d1d..0x17d5c libera si la profundidad no es válida o el centro sale de
  x0..319, usando la proyección DOS recuperada `(160+side×200/(forward+50))`.
  Girar y dejar el proyectil detrás también libera su ranura.
- 0x17e4e..0x17e59 cambia una colisión de superficie a estado2/fase1.
- 0x180e9..0x18100 recorre fases1..5 y pone estado0 al llegar a6.
- Descriptor+100 contiene el mapa de imágenes. Impactos0..4 usan1,2,1,2,3;
  arma5 usa1,2,3,4,5. No se supone una secuencia de imágenes consecutivas.

El port reutiliza collision::rayDistance sobre paredes sólidas y puertas dinámicas.
El barrido entre posición anterior y nueva evita atravesarlas. Al tocar superficie
se detiene, presenta las fases originales y libera el registro. Se usa el skin
numérico ya existente para evitar z-fighting, no una distancia de impacto nueva.
Los sprites salen de BALLE0..5 y se dibujan con sprites::render y el depth buffer
existente; no son una tabla ficticia que nunca se libera. No hay TTL arbitrario.

Diferencias explícitas: DOS decide colisiones con registros proyectados; aquí se
usa la geometría sólida de colisión ya disponible, no sus máscaras de enemigos
ni colisiones con sprites decorativos. El port protege el dominio relativo de
coordenadas signed16 al alcanzar32768, pero no reproduce la cuantización/wrap
exactos del EXE. Tampoco se iguala la proyección de partículas a la cámara DOS:
se conserva el FOV y horizonte actuales. Cada proyectil conserva el arma que lo
emitió al intercambiar en un armario, evitando cambiar sus gráficos a mitad de
vuelo. Los efectos/sonidos de impacto adicionales del DOS quedan pendientes;
esta fase no aplica daño ni inventa reglas para actores.

## Composición original del arma

Se cargan las cuatro imágenes de ARME0..5. Índice0 permanece como icono del HUD;
índice1 es el cuerpo en reposo.0x1810f..0x18254 demuestra la composición de tiro:

1. Cuerpo índice1 con y original+5.
2. Capa índice2 con desplazamiento del descriptor+14/+16.
3. Para arma2, offsets dependen del contador módulo4, remapeando3→1, y la
   capa índice2 se dibuja **antes** del cuerpo. Las otras dibujan cuerpo antes
   de capa2. No es una secuencia artificial1→2→3.
4. Índice3 es la marca central de puntería original, en descriptor+4/+6,
   dibujada al final de la rama de disparo.

Posiciones de reposo y escala nativa se conservan. El casco se compone después,
sin invadir el HUD inferior. Los dos campos visuales firing/barrelPhase proceden
de WeaponFire; el renderer no consume munición ni decide si se dispara.
BALLE usa dimensiones, pivotes y alturas del descriptor. BALLE3[0] tiene un tamaño
raw distinto del descriptor: se adapta en memoria mediante nearest como la escala
original, sin parchear el archivo ni crear arte nuevo.

## Entrada, estados y audio

PlayerInput observa el botón izquierdo de su ventana, tanto Down como Up. Conserva
un flanco pressed para clics cortos, además del estado held. La captura inactiva,
los cambios de foco y de mapa descartan pulsaciones pendientes. En transiciones se
consulta el estado local/global del ratón para detectar un botón ya mantenido.
WeaponFire exige liberación/nueva pulsación tras un bloqueo y distingue semi/auto.

ItemSession ahora captura botones, movimiento y rueda mientras un modal o apertura
de inventario posee la entrada. Main deja que PlayerInput observe releases, pero
no arme esos clics. Los clics capturados quedan inhibidos hasta soltar incluso
si la apertura y el cierre se tramitan en un mismo lote de eventos. Tras resolver la interfaz, solo permite actualizar disparo
si hay3D, foco, jugador vivo, captura estable, gráficos cargados y ningún modal.
Así E de cierre, un clic en inventario o el botón mantenido al volver no disparan.
Los proyectiles y su reloj se congelan con los modales/mapa/foco, sin catch-up.

GameAudio::fireWeapon solicita BANK[arma+1] (0x17ad8..0x17ae0).
GameAudio::emptyWeapon solicita BANK0 (0x17a7e,0x17aca) sin apilar la misma muestra
vacía cada fotograma, coherente con solicitudes por ID del scheduler DOS. No hay
nuevo reproductor, bucles forzados, cambios en puertas/pasos/muerte ni sonidos
arbitrarios. Los fallos de carga de BALLE deshabilitan disparo con aviso en consola,
sin descontar munición silenciosamente.

## Pruebas y comprobación

Nueva suite equipped_weapon_fire:

- Disparo inicial y mantenido en las seis armas; nueva pulsación semi/auto.
- Consumo exacto y rechazo sin munición, durante espera o con20 ranuras ocupadas.
- Fórmula y número de iteraciones de espera a30/60/144FPS, sin imponer DPS fijo.
- Salida del campo de visión, paso por cinco fases de impacto y reutilización.
- Liberación en dominio de coordenadas extremo sin escenario ni TTL ficticio.
- Bloqueo y reanudación manteniendo botón; sin catch-up de tiempo.
- Intercambio real devuelve al armario el arma con su munición restante.
- Con recursos originales compara coeficientes, offsets, dimensiones, pivotes,
  mapas de fases y flags contra los56 words de cada registro EXE.

Resultado: compilación normal y con ASan/UBSan correctas; **19/19 suites CTest**
en ambas configuraciones y **16/16 pruebas Python**. Pasan las comprobaciones con
recursos originales de disparo, HUD/proyectiles, audio, inventario, lectores y
puerta7; las de disparo, HUD y audio también bajo sanitizadores. No se han
reportado errores de memoria/framebuffer. La interacción física en ventana y la
comparación DOS completa quedan para comprobación manual.

Se amplían HUD para capas/orden y composición de proyectil real mediante el
renderer existente, entrada para propiedad modal del ratón y audio para los siete
IDs correctos. Se conservan las regresiones previas de inventario y C001/puerta7.
Diagnósticos: build/hud-debug/firing-composition-sheet.png y fire-projectile.png.
Son composiciones del port con gráficos originales, no capturas de una ejecución DOS.

```sh
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/weapon_fire_tests ~/Desktop/In_Extremis/DATA
./build/hud_tests ~/Desktop/In_Extremis/DATA
./build/audio_tests build/audio-test-data ~/Desktop/In_Extremis/DATA
./build/inventory_tests ~/Desktop/In_Extremis/GAME.LAB
./build/door_control_tests ~/Desktop/In_Extremis/GAME.LAB
./build/interaction_tests ~/Desktop/In_Extremis/DATA
python3 -m unittest discover -s tests -p 'test_*.py'
ctest --test-dir build/interface-sanitized --output-on-failure
```

Revisión manual en una ejecución nueva:

1. Tab para3D. Arma0: un clic consume500→499 y presenta fogonazo/marca central;
   mantener no repite; soltar y pulsar vuelve a disparar cuando termina la espera.
2. Arma1 del armario8/6: intercambio con E, confirmar Yes. Mantener botón repite;
   soltar detiene. Reintercambiar conserva las municiones restantes de ambas.
3. Abrir I o un panel. Pulsar/mantener ratón, cerrar con I/ESC/E: no debe gastar
   munición. Soltar y volver a pulsar para disparar. Igual tras Tab y pérdida de foco.
4. Muerto no dispara. Z/X y carrera siguen funcionando sin cambiar de control.
5. Comprobar sonidos de arma y BANK0 sin munición. No usar selección numérica:
   no se ha implementado. Las otras armas se verifican en pruebas con su inventario
   preparado mediante el mismo exchangeWeapon, sin entregarlas al jugador real.

## Archivos de esta fase

- Nuevos: include/weapon_fire.h, src/weapon_fire.cpp, tests/weapon_fire_tests.cpp.
- Entrada: include/src player_input y src/item_session.cpp.
- Inventario: include/src player_inventory, solo consumeShot añadido;
  exchangeWeapon no cambia.
- Presentación: include/src original_hud, include/src first_person_view,
  src/main.cpp para coordinar el estado y prestar la Scene de proyectiles.
- Audio: include/game_audio.h y src/game_audio.cpp.
- CMakeLists.txt: biblioteca concreta de disparo y su prueba, enlaces necesarios.
- tests/hud_tests.cpp, tests/audio_tests.cpp, tests/player_input_tests.cpp.
- tools/inspect_interactions.py, este informe y referencias de documentación.

Los restantes cambios locales previos se han conservado.

## Paso 202: retícula estable y eje óptico

Esta sección sustituye las coordenadas de mira y lanzamiento descritas arriba.
ARME[3] se dibuja también en reposo, centrado en(160,75) del viewport320×150.
El disparo nace en XY de cámara y Z=Camera::eyeHeight(128), paralelo a su eje.
Se retiran del lanzamiento físico Z98 y los offsets laterales de cañones; se
conservan íntegros en la animación visual. Son diferencias deliberadas DOS:
0x1810f..0x18254 dibujaba la marca solo al disparar, con centro(160,58), mientras
el port tiene horizonte75. Una trayectoria inferior/paralela no puede alinearse
con una retícula fija para todas las distancias. No se elige una convergencia
arbitraria. La prueba opticalAxis verifica seis armas, cinco orientaciones y
varias distancias mediante sprites::project, además de proyección de impacto.
Véase [enemy-system.md](enemy-system.md) para evidencia, hitos y límites actuales.
Los enemigos visibles aún no reciben impactos: el combate con ellos sigue pendiente.

## Paso 203: emisor visual y combate

La descripción del Paso202 de todos los sprites de vuelo sobre el eje óptico
queda sustituida. El origen lógico sigue en XY de cámara/Z128 y los impactos
siguen centrados en la mira; el vuelo se representa desde el centro de ARME[2]
(fogonazo original) mediante proyección inversa con el FOV común. Se captura la
pose/cañón de cada emisión. La convergencia se obtiene del siguiente obstáculo
o enemigo real, sin distancia elegida a ojo; un vuelo sin objetivo finito mantiene
su offset mundial y converge visualmente por perspectiva. En impacto, offset0.

El DOS tiene proyectiles físicos (20 ranuras), no un hitscan instantáneo:
0x17b43..0x17bdb emite en XY del jugador, Z=cámara−30, y desplaza lateralmente
arma2. La solución visual del port es deliberadamente distinta para conservar
la mira central validada. No altera recursos ni la composición ARME1+2+3.

WeaponFire barre cada paso contra el mundo dinámico y las máscaras MONS1.SPT;
la colisión válida más próxima decide impacto. Consume una unidad al aceptar
el disparo, no por frame de animación o daño. Se conserva semiautomático0/4,
automático1/2/3/5, reloj, ranuras, BANK[arma+1], BANK0 y exchangeWeapon.
El daño1/1/1/2/5/4 se aplica a salud real; BANK16 suena por impacto y BANK17
al finalizar la muerte. Los bloqueos de mapa, modal, foco y jugador muerto siguen
controlando la entrada existente. La normalización18Hz afecta a enemigos, **no**
a la cadencia del arma. Véase [enemy-system.md](enemy-system.md) para límites.

muzzleAndImpactProjection prueba seis armas/cinco orientaciones: el primer sprite
proyecta sobre el centro del fogonazo y el impacto en(160,75). enemy_tests prueba
SPT, prioridad, paredes/puertas, daño único y munición. hud_tests con DATA genera
step203-flight-0..5.bmp para inspeccionar la composición con gráficos auténticos.
