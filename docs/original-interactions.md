# Interfaces originales de interacción del nivel 1

Actualización vigente: [Paso 201 — controles e interfaces](step-201-interfaces-and-controls.md).
Los controles y el alcance descritos abajo corresponden a revisiones anteriores.

Estado actualizado: [auditoría de interfaces de 2026-11-02](interface-audit-2026-11-02.md).
Ese informe corrige los diagnósticos históricos que siguen en este documento.

## Resultado y alcance

G ya no retira objetos ni elige automáticamente la primera C. Abre la interfaz
correspondiente. Se han integrado las vistas estables originales de teclado de
código, suministros, armas, selección/sustitución de tarjetas, lector de puerta,
terminal de consulta de códigos y transferencia de plano. Inventario, Vitals,
contenido mutable de paredes y puerta 7 siguen siendo los propietarios reales
de sus datos; no hay un segundo inventario o sistema de seguridad.

Las posiciones, gráficos, tipografías y algoritmos enumerados como confirmados
proceden del ejecutable y de los bancos originales. **No se ha completado la
presentación cinematográfica DOS:** faltan zoom, animaciones de apertura/cierre,
algunos estados intermedios, LEDs, transiciones globales de paleta y relojes de
reproducción. No se describe esta entrega como equivalencia visual completa de
esas secuencias. Las diferencias concretas aparecen más abajo.

Referencia EXE SHA256:
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Direcciones de este documento: offsets absolutos de archivo. Punteros de datos
se convierten a archivo sumando 0x200. No se modifica ningún recurso original.

## Interacciones presentes y detección

| Sector/pared | Tipo/extra | Interfaz y efecto |
|---|---|---|
| 1/6 | 0x3606 / 0 | Código de zona y transferencia del plano; ningún inyector |
| 3/6 | 0x3205 / 0x3001 | Armario con E001 |
| 7/10 | 0x3205 / 0x4001 | Armario con C001 |
| 7/17 | 0x3308 / 0x0107 | Seleccionar C001 y desbloquear puerta 7 |
| 8/6 | 0x3104 / 0x11f4 | Armario de arma 1 y munición 500; intercambio |
| 8/7 | 0x3508 / 1 | Seleccionar C001 y consultar códigos de zona |
| 8/8 | 0x3205 / 0x2064 | Armario con inyector de carga 100 |

Son nueve paredes: las siete de la tabla y los dos ascensores **0/6** y **9/6**
(0x3002, registros0/1). Cuatro contienen objetos transferibles. Los paneles
0x3002 no son narrativos: `0xa515 → 0xce1a` carga CH_LEVEL.SPR y selector E.
Se ha recuperado su interfaz; el viaje queda pendiente. Véase la auditoría actual.
No se reclasifican los 27 sprites Table A.

Se conserva el alcance recuperado de `0x10bc1..0x10bfc`: distancia normal
[0,90), proyección dentro del segmento y orientación aproximadamente ±30°.
El port conserva la prueba conservadora de oclusión y sus diferencias de
cuantización ya documentadas en `items-and-inventory.md`. Una vez abierto el
modal, el jugador no puede moverse para cambiar el objetivo.

## Descubrimiento del código previo de los armarios

La implementación anterior empezaba demasiado tarde en las rutinas de recogida.
Antes de mostrar el objeto, tanto `0x8042` (armas) como `0x183ce` (suministros)
pueden pedir el código de la zona. `0x19a4f` utiliza el mismo teclado para el
panel inicial. Ese paso queda integrado y deja de omitirse.

`0x11388..0x11392` fija [0x519a] desde el offset del word situado en cabecera
de nivel +0x0e. En el primer registro de GAME.LAB:

- Inicio de nivel: archivo 0x7e.
- Offset relativo de tabla: 0x024c = 588.
- Tabla en archivo 0x2ca: words **90, 2, 0, 1**.
- 90: comprobación de acceso de la zona.
- 2: cantidad de referencias para el terminal de códigos.
- 0 y 1: entradas del catálogo EXE que consulta ese terminal.

`0x188d0..0x18901` y `0x19f36..0x19f67` suman seis valores en BL:

`sum((letra[i] − 'A') << i) & 0xff`, con i=0..5.

El teclado ofrece A..L, borrado y validación. La comparación es con el word de
la tabla, no con un código de seis bytes almacenado en GAME.LAB. Se conserva
el overflow de BL. No se añade un algoritmo criptográfico ni una combinación
arbitraria. `0x18991..0x1899f` / `0x19ff7..0x1a005` marcan la zona autorizada
cuando se acepta. Los siguientes armarios de esa zona no vuelven a pedir entrada.

La lectura `0x9e34..0x9e79` demuestra los catálogos EXE:

| Entrada | Nombre, desde archivo 0x60ef + 4×entrada | Seis valores, desde 0x6167 + 6×entrada | Texto |
|---|---|---|---|
| 0 | C200 | 8,9,4,10,0,7 | **IJEKAH** |
| 1 | B101 | 9,1,7,6,0,3 | **JBHGAD** |

IJEKAH produce 90 y corresponde a la zona del registro cargado. Otros textos
pueden producir el mismo checksum débil: el port no los rechaza si el original
los aceptaría. El catálogo de consulta integrado se limita a las dos entradas
referenciadas en este nivel; no se afirma implementar todos los códigos del juego.

## Recursos, resolución y representación

Se reutiliza `spr_format`, sin nuevo formato gráfico ni edición de imágenes.
Las interfaces se componen sobre el framebuffer 320×200 ya existente y usan
PAL_DI.PAL. Se conserva presentación 4:3 y filtrado nearest.

| Recurso inglés | Contenido | Uso estable recuperado |
|---|---|---|
| CHARME.SPR | 10 imágenes; base 196×100 | Teclado en (62,25), mano y dedo originales |
| CH_OBJ4A.SPR | 31 imágenes; base índice 12, 196×100 | Suministros y tarjetas |
| CHARME4A.SPR | 26 imágenes; base índice 12, 196×100 | Armas, descripción y munición |
| INVENT_A.SPR | 27 imágenes; base 184×180 | Selector de tarjetas en (68,10) |
| INVENT_T.SPR | 76 glifos | Texto original del menú, incluidas minúsculas |
| B_PORTE.SPR | 5 imágenes; base 196×100 | Lector de puerta en (62,25) |
| B_CODE.SPR | 10 imágenes; base 196×100 | Terminal de consulta en (62,25) |
| B_PLAN3.SPR | 5 imágenes; base 196×100 | Transferencia en (62,25) |
| TYPO_3X8.BIN | 634 bytes | Mensajes del display a color de paleta 102 |

Las variantes A contienen inglés (Yes, Working order, Battery Charger, etc.).
Se han contrastado con las F francesas; no se redibujan ni traducen sus píxeles.
CH_OBJ2/3, CHARME2/3 y B_PLAN2 contienen etapas previas de apertura; no se emplean
como sustitutos de la vista estable.

TYPO_3X8: bytes 0/1 = 3/8, 96 bytes de correspondencia para ASCII 32..127,
bitmaps desde byte 98, ocho filas por glifo, tres bits altos por fila y avance
horizontal cuatro. `0xa759..0xa932` respalda el decodificado. No se usa fuente
SDL ni del sistema. INVENT_T sigue el mapa de `0x1cf04..0x1d021`: dígitos,
mayúsculas, minúsculas y puntuación, con avance ancho+1 y espacio de cuatro.

### Coordenadas adicionales confirmadas

- Teclado: teclas pulsadas en x=88+15×columna, y=58/75; reposo de la mano
  x=70+15×columna, y=70+17×fila y dedo en +42,+9. Ventana de recorte
  (62,25)..(257,124), según `0x19587..0x19707`.
- Suministros: descriptor de ocho int16 por clase en archivo 0x1835e.
  Posiciones finales: botella (146,61), inyector (188,54), tarjetas (181,75).
  Nombre en (75,32); pregunta en (75,60); Yes en (90,96), No en (117,96).
- Número de tarjeta: letra en (214,36), dígitos en (221,36), (227,36), (233,36).
  Oxygen muestra parámetro/18 como porcentaje; inyector muestra su carga 100.
  Ese 100 no se convierte en una cura de 100 puntos.
- Armas: descriptor en archivo 0x7fe2. Posiciones finales para tipos 0..5:
  (160,54), (139,61), (146,54), (139,47), (139,54), (146,54).
  Munición en x220/226/232, y36. Son coordenadas propias, no las de suministros.
- Menú: texto centrado dentro de x72..247; y146 y +11 por entrada; fondo
  índice 254. `0x1ce21..0x1ce70` y `0x1d05a..0x1d0c5`. Tarjetas en
  x203, y50/81/112, imágenes INVENT_A[19+familia], `0x1c729..0x1c80d`.
- Lector: tarjeta en (85,52), recortada a y65..124, `0x952b..0x9579` y
  `0x9883..0x98af`. Texto del display en (77,36).
- Terminal de códigos: texto en (76,37), mano en x134,y68/86; dedo en +39,+8.
  `0x9def..0x9e2f`, `0xa304..0xa39f`.
- Plano: TRANSFER en (142,51), número en (154,81), zona en (140,112),
  `0x1a348..0x1a478`.

Salvo INVENT_A, las vistas estables caben en el viewport y conservan el HUD
inferior. El selector 184×180 cubre parte del HUD: corresponde a su extensión
original y no a un cambio del viewport 3D.

## Controles, entrada y simulación

El archivo CFG suministrado contiene scancodes PC 4b/4d/48/50 (flechas) y 39
(Espacio). `0x16cd9..0x16dce` obtiene ejes y acción configurados. No se afirma
que esas teclas sean universales: DOS permite configurarlas.

- **I** abre el inventario general (scancode original PC0x17); arriba/abajo y Espacio.
- **G** abre el panel cercano: binding conservado del port, no afirmado como tecla DOS.
- Teclado de código: flechas para mover la mano, Espacio pulsa la tecla marcada.
  Fila superior A..F y borrar; inferior G..L y validar. Se necesitan seis letras.
- Suministros/armas: izquierda/derecha eligen Yes/No, Espacio confirma.
- Selector: arriba/abajo eligen tarjeta o Exit, Espacio confirma.
- Terminal de códigos: arriba/abajo eligen el botón de otro código o salida;
  Espacio consulta el siguiente o cierra.
- **Escape** cancela: extensión explícita del port. Fuera del modal conserva
  la salida existente; una repetición de la misma pulsación no cierra el juego.
- Tras el resultado del lector, Espacio/Escape devuelve al mundo. Es provisional:
  DOS presenta el resultado temporalmente y ejecuta su cierre animado.

La tecla que abre no confirma. Se ignoran repeticiones y eventos de otras ventanas.
Mientras el modal posee la entrada no se procesan E, H/J, WASD, Tab, ratón ni
los controles de depuración del mundo. La pérdida de foco descarta acciones
pendientes. El cierre restaura la captura mediante PlayerInput, sin reenviar la
tecla de confirmación a puertas o a otras acciones.

**Evidencia de pausa:** las rutinas modales bloquean el retorno a la actualización
del juego; el dispatcher `0xa636..0xa63e` restablece [0x1428]=1 al volver.
La transferencia también escribe cero en `0x1a1c3`. No se acumula después todo
el tiempo del modal como consumo vital. El port congela O2, mensajes, puertas,
sprites y texturas de pared durante la interfaz y descarta el intervalo de
reanudación. El audio ya solicitado continúa; no se para el dispositivo SDL.

El selector conserva el contador original módulo 64 y la mitad de 32 llamadas
para el parpadeo. El plano conserva 100→0 en pasos de 4 por iteración modal.
Son **cantidades por iteración confirmadas**, no una duración en segundos
recuperada. Su velocidad observada puede diferir de DOS: falta reconstruir el
ritmo exacto de esas iteraciones. No se inventa una frecuencia histórica.

## Efectos y conservación del estado

### Armarios y suministros

La apertura solo observa. No o Escape no cambian nada. Yes usa `World::take` y
las operaciones ya existentes de Inventory, una sola vez. Vacío muestra
`THIS COMPARTMENT IS EMPTY!` sin abrir: así lo decide el dispatcher original,
con traducción autorizada de la cadena francesa.

El inyector se guarda, no cura al recogerlo. H conserva la operación existente
fuera de interfaces: usa uno lleno, restaura 1000 y deja envase vacío. J conserva
el intercambio de reserva de una botella llena; no hay botella colocada en los
siete paneles de este nivel. Estas teclas de uso siguen siendo adaptaciones:
no se han recuperado todavía sus menús completos ni INJECT/OXYGEN.ANM.

Con tres tarjetas, Yes abre el selector de sustitución. Elegir un slot intercambia
ambas tarjetas; Exit/Escape conserva ambas. `0x193ca..0x19419` respalda la
conversión entre packed de inventario y extra de pared (+0x3000). No se duplica
ni elimina la tarjeta depositada. Con dos inyectores llenos o botella llena,
se conserva el contenido y se anuncia capacidad; la selección original de
sustitución de esos recipientes sigue pendiente. Esto no afecta a los suministros
presentes en el nivel 1, pero es una limitación explícita de los casos generales.

Armas: Yes intercambia arma y munición; el armario recibe el arma anterior.
Cancelar no equipa ni consume nada. No se implementan disparos.

### C001 y puerta 7

G en 7/17 abre el selector real. La familia C se verifica al confirmar; E001 no
se convierte en un permiso. Una C de identificador incorrecto es rechazada.
C001 limpia exclusivamente 0x0400 de puerta 7 y permanece en el inventario.
Se presenta B_PORTE con su resultado; al cerrar, E abre la puerta normalmente.
Repetir G con seguridad ya desactivada muestra el aviso sin re-bloquearla.

`0x9434..0x948d`, `0x1ac94..0x1acbc`, `0x1afbc..0x1aff1` y
`0x95a1..0x95df` respaldan filtro, byte de identificador y modificación de estado.
Sin C se presenta la traducción del aviso original de falta de tarjeta y Exit.
No se altera ninguna otra puerta. No se usan tarjetas después de morir.

### Terminal 8/7

La revisión de `0x9c2b` confirma **modo 3, familia C**, no E. `0x9b84..0x9b8c`
y `0x9d85..0x9d8d` comparan el byte bajo con extra&255: aquí requiere C001.
Se conserva la tarjeta. `0x9e34..0x9e79` lee las referencias tras el código del
nivel y recupera nombre/seis letras de las tablas EXE. `0x9f81..0x9f9a` recorre
las referencias. Este terminal consulta C200/IJEKAH y B101/JBHGAD; **no modifica
la puerta 7**. El rechazo se muestra como CARD REFUSED..., traducción inglesa
de la cadena CARTE REFUSED... del ejecutable.

### Panel inicial

Tras autorizar C200 presenta B_PLAN3 y TRANSFER. Al terminar marca el estado
local de plano transferido y vuelve a 3D, sin entregar inyector, oxígeno o arma.
`0x1a1f4..0x1a225` respalda contador y byte [0x601b+zona]=0xff.
`0x6fd3..0x7001` consulta ese byte antes de abrir el mapa, confirmando su función.
El mapa Tab del port sigue disponible como herramienta preexistente: no se
introduce ahora esa restricción DOS ni se rompe el control validado. El flag
queda preparado para separar mapa jugable y depuración posteriormente.

### Audio

Se reutiliza GameAudio y su mezclador:

- BANK[12]: confirmación Yes/No, antes de ambas ramas (`0x18f97`, `0x8c0b`).
- BANK[28]: tecla de código y selección de inventario (`0x19e60..0x19e68`,
  `0x1af21..0x1af29`, `0x1af97..0x1af9f`).
- BANK[19]: tarjeta aceptada (`0x95ab..0x95b3`, `0x9db1..0x9db9`).

No se reinician por renderizar. La reproducción no es condición para transferir
objetos o desbloquear. Motores, sonido de transferencia y el resto del audio de
las animaciones quedan pendientes de integrar junto con esas secuencias. Las
puertas, pasos y muerte conservan sus disparadores.

## Arquitectura

- game_lab añade solo accessCode y codeTargets, leídos del formato original.
- level_items separa consulta de objetivo y transferencia. Abrir no llama a take.
- ItemSession posee objetivo/modal, selección, entrada, autorización y flag del
  plano; coordina Inventory, DoorControl, Vitals, mensajes y audio.
- original_interactions carga recursos y dibuja State sobre el framebuffer
  prestado, sin reglas de inventario o acceso. Font/sprites se validan al cargar.
- FirstPersonRenderer compone el modal después del HUD sobre el mismo buffer,
  conservando textura, escalado y memoria. No se asignan imágenes por fotograma.
- main solo deriva actividad/captura, enruta eventos y pasa el estado al renderer.

Si faltan gráficos/HUD/paleta/fuente, no se permite abrir un modal invisible ni
se vuelve silenciosamente a la recogida instantánea. El error queda en consola.
Código de nivel ausente no concede autorización ficticia. No hay ECS ni framework
genérico de menús. La única reserva O2 y salud siguen en sus propietarios previos.

## Diferencias y trabajo pendiente

1. Vistas estables y flujo lógico recuperados; faltan animaciones ANM y SPR
   intermedias, zoom y giro/inserción animada de tarjeta. El dedo ya usa sus pivotes y frames; su ritmo DOS sigue pendiente.
2. Los gráficos modales usan PAL_DI auténtica, pero la escena/HUD subyacentes
   mantienen su RGBA/PAL de juego. No se emula el cambio global de paleta VGA
   ni el fundido completo observado en las rutinas DOS.
3. Ritmo por iteración de parpadeo/transferencia no equivale a Hz históricos.
4. Resultados del lector cierran con acción/Escape; pendiente el temporizador
   original de 60 unidades y la secuencia de cierre. Código erróneo permite
   corregir/reintentar en la misma pantalla; DOS sale de esa rama mediante su
   presentación de error/cierre. No modifica autorización ni inventario.
5. Se omiten avisos intermedios de código ya introducido/OK y algunas pausas
   mientras se cambian bancos. Los estados finales están recuperados.
6. Sustitución de botellas/inyectores llenos y pantallas completas de uso de
   consumibles siguen pendientes. No se fabrican objetos para poder probarlas.
7. Catálogo de códigos integrado solo para las dos referencias reales del nivel.
8. Paneles narrativos 0x3002, botones de ratón y lectura de controles CFG aún
   pendientes; se conservan G/E/Tab y flechas/Espacio de la configuración conocida.
9. Mantener Tab como mapa de diagnóstico, Escape como cancelación y las
   traducciones inglesas se documenta como decisión del port, no evidencia DOS.

## Pruebas y reproducción

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/inventory_tests ~/Desktop/In_Extremis/GAME.LAB
./build/door_control_tests ~/Desktop/In_Extremis/GAME.LAB
./build/interaction_tests ~/Desktop/In_Extremis/DATA
./build/hud_tests ~/Desktop/In_Extremis/DATA
python3 tools/inspect_interactions.py ~/Desktop/In_Extremis
```

El inspector comprueba SHA256 del EXE y genera desensamblado, descriptores,
parámetros GAME.LAB y hashes/dimensiones de recursos en build/panel-debug.
Las pruebas generan siete composiciones BMP y una composición sobre la escena
3D real, comprobadas también visualmente como diagnóstico, no como ejecución
interactiva DOS comparada fotograma a fotograma.

Se cubren apertura sin transferencia, código incorrecto/correcto, límites y
checksum BL, No, Escape, Yes, persistencia, duplicados, intercambio de arma,
selector C, rechazo E/ID incorrecto, ausencia de C, puerta 7, lectura de códigos,
selección/salida de sustitución de tarjetas, plano sin inyector, muerte, pausa,
reanudación, eventos ajenos/repetidos, recursos ausentes, font corrupta,
coordenadas y framebuffer inválido. Los tests SDL usan dummy o superficies de
software; no necesitan dispositivo físico de audio.

## Validación manual del nivel 1

```sh
./build/InExtremisPort ~/Desktop/In_Extremis/GAME.LAB --vitals-debug
```

Tab entra en 3D. En Mac, G, flechas, Espacio y Escape no necesitan Fn.
Los F5/F10 de diagnóstico pueden requerir Fn/🌐.

1. Acercarse a un armario o al panel inicial y pulsar G. No debe dar ningún
   objeto al abrir. Si aparece el teclado, seleccionar **IJEKAH** con flechas
   y pulsar Espacio en cada letra. Elegir la tecla de validación, abajo a la
   derecha, y pulsar Espacio. Borrar es la tecla de arriba a la derecha.
2. Panel inicial: desde (3250,1100), mirando −X, G. Tras el código aparece
   TRANSFER y regresa a 3D. No debe aparecer un inyector en el HUD.
3. Tarjeta E001: (4350,2300), mirando +X. G abre la pregunta; No/Escape no
   cambia inventario, Yes la guarda. G después muestra compartimento vacío.
4. Tarjeta C001: (1500,2950), mirando +Y. Repetir comprobación Yes/No.
5. Lector de puerta: (2500,2550), mirando +Y. G abre el selector. Arriba/abajo
   eligen tarjeta; E001 produce incompatibilidad, C001 desbloquea. Cerrar con
   Espacio; E frente a puerta 7 debe abrir. La tarjeta permanece; repetir lector
   anuncia seguridad ya desactivada. No usar O para validar este resultado.
6. Arma: (2950,2700), mirando +X. G muestra el arma, nombre y munición.
   Confirmar No y luego Yes para contrastar: solo Yes cambia el arma. Una nueva
   visita muestra el arma anterior depositada; no hay disparos.
7. Terminal de códigos: (2950,2500), mirando +X. G, seleccionar C001 y confirmar.
   Espacio sobre el botón superior consulta la siguiente referencia; abajo y
   Espacio sale. Deben verse C200/IJEKAH y B101/JBHGAD, sin cambiar puertas.
8. Inyector: (2900,2450), mirando −Y. G muestra su interfaz y carga 100.
   Yes lo almacena sin curación automática. Fuera del modal, F5 baja salud y H
   usa el inyector mediante Vitals: salud 1000, envase vacío conservado.
9. Durante un modal, WASD, ratón, E, H/J y Tab no deben actuar sobre el mundo.
   O2 no debe bajar. Tras cerrar, no debe descontarse todo el tiempo transcurrido.
   Escape cierra primero el panel; una pulsación nueva fuera conserva la salida.
10. Reiniciar aplicación restaura nivel/inventario/autorización/plano. F10 sigue
    reiniciando únicamente Vitals, no se convierte en un reinicio de partida.

### Resultado de verificación

Compilación completa correcta; 18/18 suites CTest y 16/16 tests Python.
ASan/UBSan: 7/7 suites relevantes correctas. También pasan las ejecuciones con
GAME.LAB y DATA originales, incluido el desbloqueo de puerta 7, el código 90,
las referencias 0/1, la carga de todos los bancos/font y la composición 3D/HUD.
No se ha realizado una nueva sesión interactiva de juego ni una comparación
contra DOS ejecutándose; la validación manual anterior queda para el usuario.

Archivos nuevos: include/original_interactions.h, src/original_interactions.cpp,
tests/interaction_tests.cpp, tools/inspect_interactions.py y este documento.
Integraciones: game_lab, level_items, player_inventory, item_session,
survival_session, game_audio y first_person_view (pares .h/.cpp), src/main.cpp,
CMakeLists.txt; tests inventory, survival_session y hud; tools/inspect_items.py;
documentos items-and-inventory, original-hud, door-rendering, audio-system y
player-vitals. Se conservan los cambios locales previos. Sin commits ni pushes.

La cola de avisos anteriores se limpia al abrir una interfaz para no conservar
un aviso de bloqueo congelado detrás de la nueva presentación. DOS limpia la
cola al regresar por 0xa62d; el port anticipa esa limpieza y conserva después el
nuevo resultado en el HUD como ya se solicitó para la seguridad. Es una diferencia
de presentación explícita. Se modifican también game_messages.h/.cpp para esa
operación de limpieza, sin cambiar duración ni deduplicación.
