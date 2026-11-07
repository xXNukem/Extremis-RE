# Paso 205 — auditoría de funcionalidades DOS pendientes

Fecha: 2026-11-06. Base: `31effb0`. Solo investigación y documentación;
sin cambios de mecánicas, commits, pushes ni recursos originales añadidos.

## Evidencia y alcance

- **EXE**: confirmado mediante instrucciones del ejecutable local.
- **DOS**: confirmado ejecutando el original. **No hay resultados nuevos de esta
  clase en esta entrega**: no se encontró DOSBox, DOSBox-X, DOSBox Staging ni Boxer
  en PATH o las ubicaciones de aplicaciones/binarios examinadas. No se instalaron
  emuladores ni se modificaron EXE, CFG, partidas o GAME.LAB.
- **REC**: deducido de bytes o inspección visual de recursos; no demuestra uso.
- **EXT**: documentación externa, no sustituto de EXE/DOS.
- **PEND**: hipótesis o detalle que todavía requiere comprobación.

Los offsets de código son **absolutos de archivo**, hexadecimales. Las variables
entre corchetes son direcciones de datos originales; para los datos estáticos
de esta imagen se suma `0x200` para localizar sus bytes en archivo. No convertir
así los punteros a memoria asignada. Índices de sectores, entidades y sprites
desde cero; niveles del informe desde uno. API de sprites DOS desde uno.

Referencias exactas:

```
EXTREMIS.EXE 149303 bytes
SHA256 7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397
GAME.LAB
SHA256 acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c
```

Se añadió `tools/audit_step205.py`: censo de los 30 niveles, metadatos/hashes
de recursos, radios originales y desensamblado de rangos acotados. Reutiliza
el lector SPR y LLVM existentes. Salida ignorada `build/audit205/manifest.json`
y `evidence.txt`; no copia imágenes ni archivos originales al repositorio.
El volcado exploratorio `code.txt` es un barrido lineal: contiene datos embebidos
y **no constituye por sí solo un catálogo de funciones**. Las conclusiones se
apoyan en entradas y ramas concretas. Los extremos de algunos rangos pueden
mostrar una instrucción cortada; no se interpreta como código válido.

## 0. Estado real del trabajo interrumpido

Al comenzar, `git status --short` estaba vacío, HEAD era `31effb0`; no había
ediciones locales que rescatar. Los archivos atribuidos al Paso 204 están
incluidos en ese commit. No puede reconstruirse el momento histórico del corte
de cuota a partir de un árbol limpio, pero sí comprobar qué contiene hoy.

| Área | Implementado y conectado | Límite pendiente |
|---|---|---|
| Muerte | `enemy_effects.h/.cpp`, `enemy_system`, proyección/oclusión en `sprite_renderer`, composición en `first_person_view` | Solo descriptor MONS1; banco 9 y otros bancos no generalizados |
| Fragmentos | Cuatro piezas simultáneas y siete explosiones escalonadas, único slot mundial reemplazable | Corrector geométrico del port, no reproducción exacta de tolerancias DOS |
| Visor | 43 gotas con 16 imágenes GOUTTE, recorte 320×150, encima del arma y debajo del casco/HUD | Cadencia fija adaptada de 18 Hz; no demostrada como cadencia de animación DOS |
| Audio muerte | BANK17 una petición al iniciar; EXPLO.DSS no se confunde con él | No se ha comparado una ejecución DOS nueva |
| Detector | `motion_detector.cpp/.h`, tablas Q14/atan originales, contactos de registros vivos, HUD y `main` conectados | Barrido vinculado a pasos normalizados de enemigos; control temporal original por iteración |
| Batería | `player_inventory`, `level_items`, `item_session`, `survival_session`, menú y HUD | Sustitución con carga rechazada por Capacity; porcentaje y avisos incompletos; óptica ausente |

No son módulos huérfanos: `main.cpp` entrega la lectura al HUD, el inventario
actualiza carga y `survival_session` propaga estado. F11 solo instala batería
de diagnóstico con `--vitals-debug`; no es un control DOS. No se añadió batería
al nivel 1. Los detalles de fases y tablas siguen en
[enemy-system.md](enemy-system.md#paso-204--explosión-fragmentos-y-baba) y
[original-hud.md](original-hud.md#paso-204--detector-original-funcional).

Verificación de esta auditoría: compilación completa correcta, **21/21 CTest**,
**16/16 tests Python**. También pasan `motion_detector_tests` contra EXE,
`hud_tests` contra DATA y `enemy_tests` contra GAME.LAB usando un directorio
de fixtures dentro de build. Estos checks prueban integración y recursos;
no equivalencia visual DOS ni una sesión manual. Un primer intento de invocar
enemy_tests pasó GAME.LAB en el argumento de fixtures: falló al intentar crear
un directorio sobre un archivo existente; se corrigió la invocación y pasó.
No se escribió en los originales. No se ha repetido ASan/UBSan en esta entrega.

Los documentos anteriores mezclan estados históricos y actuales (por ejemplo,
«no hay arma funcional» junto a secciones posteriores de combate). Para los
temas auditados prevalece este informe; no se borró documentación histórica.

## 1. Detector, óptica y batería

### Estados y activación

**EXE:** carga `[0x544b]` DWORD; radar `[0x5461]` word; óptica `[0x545f]` word.
Nueva partida: batería −1 en archivo `0x1d8be`; dispositivos apagados en
`0x6ab4..0x6ac6`. No hay detector gratuito al comenzar.

El acceso demostrado es inventario (I, scancode `0x17`, dato `0x561f`), sección
batería. `0x1c309..0x1c434` distingue ausente, vacía, disponible, usada por radar
y usada por óptica. `0x1bc24..0x1bd1c` ejecuta la elección:

| Estado | Acción y efecto EXE |
|---|---|
| Disponible | `0x1bc72` radar=−1, o `0x1bc80` óptica=−1 |
| Radar activo | `0x1bcb7` apaga radar; la alternativa en `0x1bcca` enciende óptica |
| Óptica activa | `0x1bcfb` apaga óptica; la alternativa en `0x1bd0e` enciende radar |

**No son simultáneos por esta interfaz.** Comparten una sola carga, pero
seleccionar el otro dispositivo desactiva el anterior. El OR del consumidor
no demuestra que se puedan activar ambos en juego normal. No se encontró
una tecla independiente en el despacho de controles examinado; no asignarla.

### Recogida, duración y agotamiento

**EXE:** suministro clase 0; cantidad de 12 bits de `Wall.extra`.
`0x18fba..0x1900d` instala `cantidad*160` en batería. No escribe los indicadores
de activación: recogerla **no enciende** automáticamente el radar/óptica.
Si no había batería, vacía la taquilla con `0xffff`; si había envase agotado,
deposita carga cero. Con carga positiva pasa a selección de sustitución.
`0x19321..0x19362` confirma intercambio: deja `cargaAnterior/160` en la pared
(división entera) y equipa la nueva. Esta rama falta en el port.

**REC:** el censo de paneles clase 0 encuentra 12 baterías de 900 unidades
en niveles 2, 7, 11, 13, 15, 17, 20, 21, 22, 27 y dos en 30; ninguna en 1.
Detalle sector/pared en manifiesto. Son 144000 ticks, 900 segundos nominales
activos (15 minutos). `0x1c328..0x1c33b` calcula porcentaje usando denominador
144000, no la capacidad inicial de cada batería individual.

**Corrección a la redacción del Paso 204:** `0x16b5a..0x16bb8` no resta
literalmente uno por invocación: si radar OR óptica está activo, resta
`zero_extend(word[0x1428])` a la carga. Es tiempo acumulado del reloj nominal
de 160 Hz ya estudiado, no coste por contacto ni por fotograma dibujado.
Al llegar a cero apaga ambos, reconstruye HUD (`0x13984`) y pone carga cero.
`0x16bb9..0x16bc5` solicita aviso por debajo de 4800 ticks (30 s nominales).
La cola gestiona duplicados; no implica un sonido por tick. En los bucles
modales revisados no se llama al consumidor y al regresar se reajusta el reloj;
confirmar pausa efectiva con DOS antes de declarar equivalencia temporal total.

### Contactos, animación, sonido y HUD

**EXE recontrastado con pruebas:** `0x100f6..0x101e4` no consulta velocidad.
Detecta enemigos inmóviles y moribundos; omite salud `0xffff`, no bloquea por
paredes ni puertas. No incluye decoración, balas o fragmentos retirados.
Orden original, máximo diez antes del filtro de barrido. Rotación con tablas
Q14, división efectiva por 64 unidades de mundo por unidad de detector.
Ventana rectangular `−27≤x<27`, `−3≤y<32`: lateral aprox. ±1728,
profundidad −192..2048, **no radio circular**.

CASQUE[4] apagado; [5..12] barrido según contador global `&7`;
[13..15] contactos 5×5/3×3/1×1 con filtrado angular sin wrap añadido.
`0x13e8f..0x13ee7`: origen `(24+x,198−y)`, recorte del radar 46×29.
CASQUE[40/41] batería/dispositivo en `(222,156)` (`0x14336..0x14372`).
Ni cálculo ni dibujo del detector solicitan audio; RADAR.DSS no demuestra
un pitido de detección. No añadirlo por el nombre del recurso.

### Óptica

**EXE:** `0x7ae8..0x7b12` genera otra paleta de 256 entradas:
`(R,G,B) -> (0,min(2*G,63),0)` en componentes VGA de seis bits.
`0x73e7..0x73fb` selecciona base `[0x4af4]` o verde `[0x4df4]`.
`0x74ed..0x7504` evita el tinte de lámpara cuando óptica=−1;
los efectos transitorios tienen prioridad anterior. No es linterna con cono,
ni mejora de exposición por superficie. Se estudia su alcance global abajo.

**EXT:** [MobyGames](https://www.mobygames.com/game/3292/in-extremis/) menciona
la batería compartida; [Just Games Retro](https://www.justgamesretro.com/dos/in-extremis)
describe batería para radar u óptica. Corroboran la temática; no fijan algoritmo,
duración o controles. No se aplicaron los parches de trucos de páginas externas.

## 2. Mapa auténtico

Actualización: [Paso 206](step-206-original-map.md) implementa el mapa y amplía
la evidencia del fondo gris, mensajes y bloqueo por enemigos.

**EXE:** scancode `0x27` en archivo `0x5621`, consultado por `0x16e16..0x16e2c`;
tecla física semicolon en teclado PC US. No equiparar el carácter impreso en
cualquier distribución con un scancode universal. La pista `;` es consistente.

`0x6f9d..0x7001` rechaza un estado de bloqueo `[0x5200]`, después exige
`byte[0x601b + zona] != 0` y llama a `0x1a5ab`. **No consulta tarjeta ni batería
en ese acceso.** El byte lo activa la transferencia del plano
(`0x1a1f4..0x1a225`). En nivel 1 pasa por terminal de zona y código C200/IJEKAH;
la tarjeta C001 del lector no sustituye ese flag. No hay evidencia de una
«tarjeta del mapa» obligatoria en esta ruta.

**EXT:** [My Abandonware](https://www.myabandonware.com/game/in-extremis-1zf)
contiene la pista de `;` y tarjeta. La parte de tarjeta no está corroborada
por el despacho EXE: puede confundir acceso a información/códigos con permiso
de apertura. Mantener esta discrepancia explícita.

### Recursos y representación

**REC + EXE:** `DATA/MAP.SPR`, 263298 bytes, 30 registros, un bitmap por nivel.
Formato SPR común: contador +2, offsets relativos a base +4, cabecera por imagen
de ocho bytes (W,H,anclajes), píxeles indexados sin paleta interna. Nivel 1:
89×69; mínimo del conjunto 33×21 (nivel 20); máximo 233×157 (nivel 22).
Dimensiones de todos los registros y hashes en manifiesto.

`0x1a6cb..0x1a73e` abre MAP, busca entrada `4+4*zona`, lee su offset,
busca `offset+4` y carga el registro elegido. Esto prueba uso efectivo, no solo
presencia. `0x1a606` carga PAL_DI mediante nombre de archivo `0x18326`.
`0x1a6b6` carga BOUSSOLE.SPR (10 imágenes: base 32×32, ocho orientaciones,
marcador 3×3). Fuente TYPO_3X8 y etiquetas de tabla `0x60ef`.

`0x1a784..0x1a7a5` centra el mapa en 320×200 y alinea origen a coordenadas pares.
`0x1a8d7..0x1a91e` copia sus píxeles omitiendo índice cero sobre el fondo
preparado. No genera segmentos de pared a partir de geometría en esta ruta.

Posición (`0x1a7a9..0x1a7f8`), con coordenadas globales enteras positivas:

```
markerX = originX + 4*floor(globalX/200) + 1
markerY = originY + mapHeight - 4*floor(globalY/200) - 4
direction = floor(normalize720(playerAngle+45)/90)
```

Ocho orientaciones de brújula; marcador intermitente BOUSSOLE[9].
`0x1aa60..0x1aa73` alterna flag tras contador >20; no fijar una frecuencia
de vídeo como sustituto de ese reloj. `0x1a84d..0x1a875` mantiene un bucle
modal de dibujo/entrada que sale por acción `[0x5407]` (Espacio por defecto),
no actualiza enemigos/movimiento. Al volver restaura HUD/paleta y temporizador.
No hay pan, zoom o recorrido por otros pisos en ese bucle. Escape y repetición
de `;` deben contrastarse en DOS; no anunciarlos como controles confirmados.

El bitmap contiene contornos y zonas rellenas ya dibujadas. No hay máscara
de exploración ni actualización de puertas/enemigos/objetos en el bucle
analizado. Las marcas que formen parte del bitmap son estáticas: no etiquetar
un color como puerta o ascensor sin cotejarlo con geometría. Las diferencias
entre niveles están en las 30 imágenes y sus etiquetas, no en un automapa
moderno que revele progresivamente. No añadir enemigos ni objetos dinámicos.

**Estrategia:** la primera opción es viable y es la demostrada: reutilizar
MAP.SPR + BOUSSOLE + PAL_DI. No hace falta sustituirlo por geometría SDL.
Preservar `map_view`/Tab como diagnóstico separado; `ItemSession::planTransferred`
ya ofrece un punto de integración, pendiente de persistencia por nivel.

## 3–4. Terminales, taquillas y recogida

No todas las animaciones son ANM. Hay escalados, desplazamientos y composición
de imágenes SPR, además de películas cortas. Los bancos son piezas de una
secuencia; **cantidad de imágenes del banco no equivale a fotogramas emitidos**.

| Familia | Entrada/ruta EXE | Recursos y evidencia |
|---|---|---|
| Taquilla arma | `0x8042` | CHARME, CHARME2 (1×196×128), CHARME3 (11), CHARME4A/F (26), A_ARME.ANM; nombres `0x7f44..0x7faa` |
| Taquilla suministros/tarjetas | `0x183ce` | CHARME, CH_OBJ2 (1×196×128), CH_OBJ3 (12), CH_OBJ4A/F (31), A_ARME.ANM; nombres `0x182c0..0x18326` |
| Transferencia plano | `0x19a4f` | CHARME para código, B_PLAN2 (1×196×128), B_PLAN3 (5); transferencia marca flag, no entrega objeto |
| Lector puerta | `0x937c` | B_PORTE (5), selector INVENT y gráficos de tarjeta; aceptación `0x95a1..0x9634` |
| Consulta códigos | `0x9aca` / selección `0x9c2b` | B_CODE (10), tarjeta compatible, referencias de zona |
| Ascensor | `0xce1a` | CH_LEVEL (10), selector E, ruta/destino; no extrapolar secuencia de taquilla |

Bases de interfaces normalmente 196×100 en `(62,25)`, paleta PAL_DI.
En inspección de recursos, CH_OBJ3 contiene paneles/objetos y CHARME3 contiene
armas; OBJET1[2/3] son lámparas roja/verde. La apariencia sola no establece
orden ni duración. Véanse metadatos de todas las imágenes en el manifiesto.

### Secuencia de suministros trazada

1. **EXE** `0x183d9..0x183f7`: separa clase y cantidad de pared; aún no transfiere.
2. `0x18460..0x184f6`: prepara fondo, conversión/paleta de interfaz y buffers;
   no es simplemente dibujar el panel final sobre RGBA sin transición.
3. `0x1858b`: BANK11 en voz 0; zoom mediante escala porcentual desde 4
   (`0x18598`), centrado alrededor de (160,75). Código/teclado precede
   apertura cuando la zona aún no está autorizada.
4. `0x189c1` carga CH_OBJ2; `0x189d6` vuelve a solicitar BANK11.
   `0x189e3..0x18a9b`: ventana que desciende y se reduce en pasos de cuatro,
   gráfico ampliado a 392×256, coordenada vertical inicial −231, límite −45.
   Es movimiento de una imagen, no 47 imágenes diferentes en un archivo.
5. Las etapas CH_OBJ3/4 preparan contenido y pregunta. El selector usa
   descriptores por clase (`0x1835e`, siete registros de 16 bytes), no una
   animación de mano universal deducida del icono del objeto.
6. `0x18f97` solicita BANK12 **antes** de distinguir Yes/No; `0x18fb4`
   salta sin transferencia para No. Batería/oxígeno/inyectores/tarjetas tienen
   ramas propias y capacidades propias. En la rama simple de batería,
   inventario y pared cambian antes de la película final.
7. `0x191dd..0x192c9`: retorno con escala 100→4 en decrementos de cuatro,
   restauración del fondo. Con inventario ocupado puede abrir selector adicional;
   `0x19321..0x19429` resuelve sustitución antes de continuar.
8. `0x19429..0x19497`: ruta de recogida resuelta solicita BANK26 y llama
   reproductor `0x1470f` con nombre `0x18304` = **A_ARME.ANM**.
   Se usa también desde suministros: el nombre no limita el recurso a armas.
   Después restaura pantalla, audio/HUD y paleta (`0x1949a..0x194f2`).

**ANM confirmado:** `0x1470f..0x14959` valida `ANM1`, lee word +4 como número
de fotogramas y paleta de 768 bytes desde +6; primer bloque y siguientes se
decodifican por rutas distintas (`0x14b5a`, `0x14bee`). No tratar como SPR.
A_ARME: 5 fotogramas, 145680 bytes; INJECT: 10, 195443; OXYGEN: 8, 126635.
INJECT/OXYGEN aparecen en las rutas de uso de inventario (`0x1bd1c` en adelante),
no demuestran una película distinta para recoger cada consumible.

La instrucción que entrega 32 al reproductor está en `0x19474`. Ese parámetro se compara con
`[0x1428]` en `0x14917..0x14926` con condición estricta >. Son nominalmente
33 ticks mínimos por espera (0,20625 s a 160 Hz), **no 32 FPS**. La duración
total real depende también de decodificación/presentación; medirla en DOS.
El reproductor consulta Escape solo si `[0x14958] != 0` (`0x14904..0x14915`).
Falta cerrar la procedencia de ese flag en cada llamador: no afirmar que todas
las recogidas son cancelables ni inventar rollback por Escape.

### Encaje y límites pendientes

El port ya separa abrir y transferir (`level_items::take`, `ItemSession`,
`Inventory`) y conserva autorización/códigos/tarjetas/consumibles. Presenta
estados estables y parte del dedo/selección; faltan zoom, panel móvil, película
de recogida y sincronización completa. El inventario lleno no tiene una regla
única: arma se intercambia; suministros tienen capacidades; batería cargada
requiere el selector de sustitución todavía ausente. No resolverlo consumiendo
el objeto automáticamente al abrir o al acabar una animación.

Taquilla vacía: conservar el aviso y estado sin objeto actuales; el dispatcher
de interacción y los centinelas `0xffff` deben comprobarse en DOS para saber
si reproduce alguna apertura antes del aviso. No deducirlo de CH_OBJ3 vacío.
El cierre tampoco queda demostrado como simple inversión de apertura.

**PEND explícito:** tabla completa de frames/posiciones/esperas por dispositivo,
entrada y salida de lector/código/ascensor, instante de cada sonido, pausas de
consumo e interrupciones, apariencia del objeto agarrado para cada clase y
ramas de rechazo/vacío/capacidad. Se han localizado rutas y recursos, pero no
se presenta como recuperada cada secuencia completa. Rastrear el llamador
desde `0xa515` y cada bucle, capturar DOS y comparar antes de implementarlos.

## 5. Iluminación primitiva: algoritmo recuperado

### Clasificación de las tres posibilidades

**EXE:** hay un cambio **global de paleta según proximidad del jugador a ciertos
objetos**, dentro del procesamiento de objetos del sector actual. No es luz
independiente de cada sector/polígono renderizado ni sombreado exclusivo del
sprite de una lámpara. La condición usa datos originales y un banco de objetos,
no una excepción para el nivel 1. Esto explica la asociación espacial con lámparas.

No se ha demostrado que sea el único efecto de iluminación de todo el programa.
Sí se recuperó la cadena completa de esta luz local aparente, desde objeto hasta
DAC. No hay base para atribuir `Polygon.extra` a brillo: su distribución se
exporta, pero no participa en esta cadena. Tampoco se usa material SOL, pared
o techo para seleccionar el tinte encontrado.

### Datos, condición y orden

`0x11350..0x11354`: el tercer word del encabezado del nivel pasa a `[0x5190]`,
**banco OBJET**, no índice del nivel ni paleta. `0x7d00..0x7dba` elige tablas
de alturas/radios por banco. Para OBJET1, radio en archivo `0x5d4f`, words
de tipos 2 y 3 = **200**.

`0x16848` limpia `[0x520c]` antes de consultar objetos para el jugador.
Llamada `0x16888 -> 0x10db7`, último argumento cero, sector `[0x51ee]` y
posición candidata `[0x51e6/0x51e8]`. A continuación se resuelve colisión de
paredes con `0x1074b`. **Importa el orden**: el tinte se calcula con posición
candidata antes de esa corrección, no simplemente con la posición final de cámara.

`0x10db7` selecciona sector y recorre registros Table A de ocho bytes:
`vertex,type,param1,param2`. El vértice da centro local. Reglas:

1. `type & 0x2000` omite el objeto (`0x10e00..0x10e09`).
2. Caja local usando radio por tipo: mínimo incluido, máximo excluido en X/Y
   (`0x10e21..0x10e8f`). Con radio 200: `[cx−200,cx+200)` y equivalente Y.
3. `type & 0x1000` entra en ruta de colisión, no tinte (`0x10e95..0x10e9d`).
4. Argumento modo `[ebp+20]` debe ser cero; ya debe estar apagado el flag de
   luz `[0x520c]`; OBJET debe ser 1; byte bajo del tipo debe ser 2 o 3.
5. `d=max(abs(centerX−candidateX),abs(centerY−candidateY))`.
   `p=80−(d>>1)`. Si `p<=0`, no activa; limita a 80 (`0x10ed5..0x10f08`).
6. `[0x520c]=1`, `[0x520e]=p`, RGB VGA en `[0x5210..0x5212]`:
   tipo 2 `(63,0,0)`; tipo 3 `(0,63,0)` (`0x10f0c..0x10f33`).

Efecto positivo únicamente con `d<160`, pese a la caja preliminar de radio 200.
Distancia Chebyshev: contornos cuadrados, no círculo ni caída inversa cuadrática.
El flag hace ganar al **primer objeto elegible del sector en orden de registros**,
no al más cercano ni a una suma de luces. La rutina no consulta ángulo, normal,
texturas ni visibilidad. No busca lámparas en todos los sectores vecinos.
Cruzar sector puede cortar el efecto aunque la distancia siga siendo pequeña.

La intensidad **sí cambia discretamente con la distancia** (un punto porcentual
por cada dos unidades), pero no hay interpolación temporal. No sería fiel
convertirla en tinte constante por sector ni añadir suavizado al entrar/salir.
El giro puro no modifica el cálculo espacial; daños, movimiento corregido y
animación de paleta pueden producir otros cambios que hay que aislar.

### Paleta y prioridades

`0x73e7..0x73fb`: base normal o verde según óptica. Orden posterior:

1. Transiciones globales especiales (`0x747b` en adelante).
2. Efecto transitorio `[0x53f9]`, intensidad `[0x53fb]`, RGB `[0x53fd]`.
3. Si no hay transitorio y óptica no está activa, luz `[0x520c]`.
4. Base/restauración o actualización parcial por flag `[0x5425]`.

`0x7574..0x759c` entrega RGB/p a `0x14611`. Esta función construye tabla de
64 entradas de **componentes**, no una tabla de remapeo de 256 índices. Para
cada componente original `v` y componente destino `t`:

```
target = floor(t*p/100)
step = floor(((100-p)*65536)/100)       # p>0; p=0 usa tabla identidad
table[v] = floor(v*step/65536)
result = table[v] + target
```

Conservar truncamientos independientes; no sustituir por un blend float
redondeado al final. `0x1468e..0x146bb` escribe índice en puerto **0x3c8**,
los tres componentes en **0x3c9**, y repite **256 colores**. `xlatb` usa la
tabla de componentes temporal. No hay máscara de viewport en la escritura DAC.
El paletizado de la escena no cambia de índices para esta luz.

**Consecuencia EXE:** la paleta afecta paredes, planos, decoraciones, enemigos,
arma, gotas y HUD que estén presentes con esos índices en pantalla. No existe
en esta ruta una exclusión del HUD. No reservar sus colores ni recolorear solo
el viewport por una suposición moderna. La repercusión visual exacta de cada
índice y las transiciones modales deben cotejarse con DOS, pero la escritura
global está demostrada. La óptica también es global; no solo sprites verdes.

Al salir, `[0x520c]` se recalcula y `[0x5425]` permite restaurar base; el tinte
no se acumula sobre el RGB ya teñido. Un daño/flash usa la otra rama y puede
ocultar la luz momentáneamente. `0x10344`, `0x104b7`, `0x16669`, `0x16b51`
son otros productores de efectos: no confundir sus flashes con lámparas.

### Animaciones y diferencias entre niveles

`0x7400..0x747b` tiene una ruta para **banco DALLE=5**, cada iteración par del
contador `[0x5423]`, que modifica componentes de paleta alrededor de entradas
73..82 y pide actualización parcial. No es la misma ruta que la lámpara.
Hay escrituras a `+3` que deben preservarse y auditarse byte a byte antes de
describirlo como una rotación RGB perfecta. La cadencia depende del bucle DOS;
no inventar un parpadeo moderno de frecuencia fija.

**REC:** lámparas elegibles en niveles 1 (4), 5 (3), 8 (3), 11 (7), 15 (13):
30 en total, todos OBJET1. Tipos `0x4002/0x4003`, parámetros `(256,0)` en esta
muestra. La rama de tinte no usa esos dos parámetros. El aspecto de lámpara
está comprobado extrayendo OBJET1[2/3] con PAL1. SOL1 también contiene dibujos
de luminarias, pero su aspecto no les concede esta propiedad espacial.

Puntos de nivel 1 para comparación, coordenadas globales:

| Sector / entidad | Tipo | Centro | Color |
|---|---|---|---|
| 5 / 1 | 0x4002 | 2900,1000 | Rojo |
| 6 / 4 | 0x4003 | 2100,1800 | Verde |
| 7 / 0 | 0x4003 | 2000,2500 | Verde |
| 13 / 2 | 0x4003 | 1100,2500 | Verde |

Solapamientos útiles: nivel 11 sector 10, centros (100,1500), (100,1900)
rojos y (100,1700), (100,2100) verdes en ese orden de entidades. No mezclar
colores: contrastar prioridad y discontinuidad al abandonar la primera caja
efectiva. Para geometría de prueba sintética conservar el orden de Table A.

### Integración futura con renderer software

El port conserva índices en algunos recursos pero convierte texturas a RGB
al cargar (`texture_bank.cpp`) y compone framebuffer RGBA; no tiene todavía
una única paleta DAC aplicada al framebuffer final. Además conserva sombreado
de distancia en partes de `first_person_view`. No basta con cambiar PAL1 en
memoria ni aplicar un rectángulo translúcido encima.

Bloque independiente para resolver estado de tinte con datos originales y
orden de colisión; otro para paleta VGA exacta y prioridades; después integrar
una representación que conserve identidad de índice hasta presentación o
regenere los caches desde índices sin pérdida. Evitar inversión RGB→índice,
que es ambigua con colores duplicados, y no aplicar el efecto dos veces.
El criterio para elegir la integración es equivalencia de índices/componentes
y coste medido, no añadir luces dinámicas. No cambiar ahora proyección,
sombreado de distancia o controles para resolverlo todo en una sola modificación.

## 6. Comprobación DOS y pruebas preparadas como especificación

No se ha ejecutado todavía esta matriz; **no son pruebas aprobadas**. No se
añadieron tests de una funcionalidad inexistente ni fixtures que inventen su
resultado. Para la siguiente sesión: preparar emulador con copia de trabajo
de los recursos locales y carpeta de capturas ignorada; registrar versión,
CPU/ciclos, configuración, hash de archivos y scancodes. No alterar originales.

| Caso | Procedimiento / resultado que debe contrastarse |
|---|---|
| Fuera/entrada/centro/salida | Misma lámpara, d=200,160,159,100,1,0 y vuelta: p=0,0,1,30,80,80; anotar sector/posición candidata y final |
| Permanencia | Mantener XY; separar cambios espaciales de daños, alertas y contador de animación |
| Giro | Cuatro orientaciones sin traslado; el resolver no usa ángulo |
| Límites | Ejes y diagonales, mínimos/máximos de caja; cruzar límite de sector cerca de luz |
| Solapamiento | Nivel 11 sector 10; primera elegible gana, sin suma ni elección de más cercana |
| Óptica | Activar con batería: base verde y supresión de tinte local; apagar restaura color |
| Capas | Capturar índice y DAC, además de RGB: pared, plano, enemigo, arma y HUD; comprobar los 256 colores |
| Luz animada | Banco DALLE5, sin moverse; comparar bytes 73..82 y cadencia sin confundirlo con lámpara |
| Cambio de nivel | Recalcular estado/tabla/paleta; no arrastrar tintes ni modificar los PAL de disco |
| Batería | Nueva partida, recogida, menú, cambio radar↔óptica, carga parcial, intercambio, 30 s y cero |
| Mapa | Antes/después de transferencia, sin tarjetas y sin batería, abrir con 0x27, salir por Espacio, probar Escape y tecla repetida |
| Mapa y tiempo | Medir O2, batería y enemigos durante 10 s de mapa, restablecimiento al cerrar |
| Terminales/taquillas | Cada familia, autorizado/no autorizado, Yes/No, vacío, capacidad, intercambio; capturar frames y audio con timestamps |
| Interrupción | Intentar Escape/acción/movimiento en cada etapa; anotar cuándo cambia inventario y si la película puede saltarse |

Tests automatizados posteriores, pequeños y sin recursos redistribuidos:
resolver de luz con bancos/tipos/flags/modo, distancia y sector; prioridad de
orden en solapamientos; tabla VGA para los 64 valores y porcentajes 0..100;
precedencia óptica/daño/lámpara/restauración; equivalencia a 30/60/120 FPS para
estado espacial y reloj elegido (no imponerla retroactivamente al bucle DOS);
reinicio de nivel; framebuffer sintético que confirme efectos **deseados** en
HUD y sprites y ausencia de dobles transformaciones/modificaciones fuera del
estado correspondiente. Para mapa: imagen por zona, fórmula de marcador,
orientaciones, permiso de transferencia y pausa. Para secuencias: orden de
eventos, transferencia única, rechazo sin consumo, capacidad e interrupción.

## 7. Plan de implementación revisable por bloques

1. Validación DOS de lámparas/mapa/batería y captura temporal de una taquilla.
   Es la incertidumbre principal; no empezar fijando duraciones arbitrarias.
2. Resolver puro de tinte y pruebas sintéticas; consume Table A, banco y posición
   candidata con orden original. Mantener nombres de campos desconocidos.
3. Transformación VGA de seis bits y prioridades, con tests de componentes;
   después integración paletizada software y comparación de capas.
4. Óptica y batería: estados mutuamente excluyentes, porcentaje, avisos e
   intercambio cargado. Reutilizar inventario; sin energía nueva ni tecla nueva.
5. Mapa jugable con MAP/BOUSSOLE/PAL_DI y flag por zona. Tab sigue diagnóstico.
6. Reproductor ANM1 mínimo verificado; primero una secuencia A_ARME con timing
   y cancelación comprobados. Sin framework general innecesario.
7. Una familia de interfaz por cambio: suministros, armas, plano, lector,
   consulta, ascensor. SPR/zoom/audio y transferencia tienen eventos explícitos.
8. Completar cadencia y otros bancos de muerte/radar por separado del trabajo
   visual anterior. Cada bloque compila y ejecuta suites pertinentes antes del siguiente.

No implementar aún. C++17/SDL3/CMake, módulos .h/.cpp pequeños, 320×200,
sin bloom, HDR, luces interpoladas, sombras nuevas ni recursos distribuidos.

## Reproducción local

```sh
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/audit_step205.py "$HOME/Desktop/In_Extremis"
./build/motion_detector_tests "$HOME/Desktop/In_Extremis/EXTREMIS.EXE"
./build/hud_tests "$HOME/Desktop/In_Extremis/DATA"
./build/enemy_tests build/audit205/enemy-fixture "$HOME/Desktop/In_Extremis/GAME.LAB"
```

Los logs de compilación/pruebas de esta ejecución están en
`/tmp/step205-build.log`, `/tmp/step205-ctest.log`, `/tmp/step205-python.log`.
La vista local `build/audit205/resource-preview.png` es una extracción de
recursos, **no captura DOS**, y permanece ignorada. Los resultados decisivos
y offsets se conservan en este documento aunque se borre build.
