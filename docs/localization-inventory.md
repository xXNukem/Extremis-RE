# Localización: inventario y preparación (Fase 1)

Fecha: 2026-11-23. Investigación local del árbol actual. Esta página cataloga superficies y puntos de origen; no añade un sistema de traducción ni modifica recursos. Para el original, se reutilizan las notas indexadas de EXE y formatos, sin volver a desensamblar rangos ya estudiados.

## Alcance y recuento

Los recuentos distinguen **cadenas dinámicas del port** (literales/casos visibles), **familias de contenido original** y **superficies rasterizadas**. No se cuentan variables, códigos, nombres de archivos, mensajes de log, diagnósticos de desarrollo ni texto de depuración como localización.

| Categoría visible | Port actual verificable | Original / recursos | Localización inicial en catálogo |
| --- | ---: | ---: | ---: |
| Menú principal, pausa, opciones, etiquetas de vídeo/audio/controles/idioma y estado | 26 filas de menú/opciones en el catálogo, sin contar variantes dinámicas | Menú DOS nativo documentado con recursos gráficos; el port usa sus propios literales | 26 |
| HUD y mensajes de gameplay | 14 IDs de `messages::Id` | Hay mensajes adicionales del EXE y recursos; ver nota de muerte/auto-destrucción | 14 |
| Inventario, lectores, tarjetas, paneles, elevadores y lanzaderas | Textos C++ localizados como familias, pero falta completar un conteo de literales visible con inspección helper-aware; no se declara total cerrado | Cadenas originales rasterizadas en sprites, más etiquetas/códigos del EXE; el port contiene inglés adaptado | 0 entradas de catálogo de esta familia en esta entrega |
| Contraseña / LOAD GAME | 5 filas de pantalla/estado en catálogo (más códigos originales, que se preservan literalmente) | Etiqueta original francesa en EXE `0x238d2`; códigos en tabla documentada por `password-continuation.md` | 5 |
| Comunicación narrativa nivel 22 | 10 entradas originales: pregunta, YES, NO y siete páginas | EXE `0x21e04–0x22d70`, siete cadenas narrativas completas; página 1 mezcla inglés/francés y 2–7 francés | 2 etiquetas breves traducidas; pregunta y siete páginas pendientes de cotejo/exportación |
| Introducción / créditos del port | 7 textos editoriales visibles identificados en `credits_view.cpp` (títulos, atribuciones, crédito del port); conteo de filas traducidas no cerrado | CONCEPT/TITRE ANM y PRESENTE/WHO1–4 VGA; los créditos están pintados en las imágenes, no son cadenas EXE separadas | 0 entradas de catálogo en esta entrega; no se traducen nombres propios |
| Introducción PSEQ original | Dos buffers de texto embebidos en EXE (`0x23906`, 350 bytes; `0x23a68`, 316 bytes) | PSEQ3/PSEQ6 muestran texto sobre ANM/sprites con `CON_VUET.SPR`; no es una pista de subtítulos independiente | 0 traducidas todavía; 2 cadenas fuente preservadas por offset/longitud |
| Muerte / GAME OVER / victoria | El port dibuja textos de ocho líneas de GAME OVER desde `TYPOOVER.SPR`; el ending FSEQ no llama helper de texto | GAMEOVER.VGA + TYPOOVER.SPR; ocho líneas fuente en EXE `0x21b96`; FSEQ6 tiene FIN rasterizado en fotograma | 0 traducidas; 8 líneas y 1 FIN quedan para extracción por evidencia |
| Ascensor / shuttle cinematográfico | Prompt, nombres de destino y cuatro guiones de texto de shuttle en EXE; el ascensor ordinario es mayoritariamente gráfico | `A_LEVEL.VGA/.SPR`; shuttle: NAV_*.VGA, NAV_SEQ*.ANM, VAIS_EXP.ANM, ARR_BASE.ANM y sprites; cuatro buffers EXE `0x22e2c`, `0x22edf`, `0x22fa5`, `0x2306b` | 0 traducidas; texto de cuatro escenas pendiente de cotejo |
| Otros paneles / terminales | Cadenas inglesas de estado en sesiones C++; muchos rótulos se pintan como sprites/códigos | `GAME.LAB`, EXE y DATA/SRP: fondos, manos, teclados, indicadores y glifos; no todo texto se conserva como cadena | Pendiente de extracción sistemática de cadenas restantes |

Los conteos del port son de cadenas/plantillas fuente, no de cada combinación dinámica (por ejemplo porcentaje de batería, número de sector o segundos restantes). El listado reproducible de fuentes está en «Mapa de extracción». No se cuenta inglés como traducción; el catálogo registra filas inglesas de referencia y las traducciones ES/FR completadas.

## Procedencia y superficies

| Superficie | Procedencia | Forma de representación | Referencias locales |
| --- | --- | --- | --- |
| Menús del port y pausa | Literales C++ | `original_font::Font` sobre SDL, salvo fallback de depuración | `src/app/startup/port_menu_state.cpp`, `port_menu_view.cpp`, `port_menu.cpp` |
| Opciones LANGUAGE | C++ + `settings.ini` | Integración de 2026-11-24: EN/ES/FR se seleccionan, aplican a la interfaz y persisten; texto original de cinemáticas y sprites queda pendiente | `src/app/startup/user_settings.cpp`, `port_menu_state.cpp` |
| HUD y mensajes | C++ enum `messages::Id`; números interpolados | Texto sobre `CASQUE.SPR` | `include/extremis/ui/game_messages.h`, `src/extremis/ui/game_messages.cpp`, `src/extremis/ui/original_hud.cpp` |
| Inventario/paneles | C++ de interacción; algunas etiquetas/códigos DATA | `menuText`/`smallText` usa glifos del recurso `INVENT_A.SPR` y fuentes originales del panel | `src/extremis/ui/original_interactions.cpp`, `src/app/session/item_session.cpp`, `src/app/session/item_animation.cpp` |
| Cinemática PSEQ | EXE + composición ANM/VGA/SPR | Dos buffers de texto EXE, dibujados sobre canvas ANM; sprites de texto en PSEQ6 | [Nota PSEQ](reference/exe/introduction.md#text-players-transitions-and-input) |
| Comunicación nivel 22 | EXE `0x21e04–0x22d70` | Cadenas ASCII/bytes con CR sobre VGA, usando glifos `CON_VUET.SPR`/`CON_VISA.SPR`; texto se compone sobre área central, alternando fondos | [Nota de comunicación](reference/exe/level22-communication.md#text-layout-and-original-wording) |
| Shuttle | EXE `0x22e2c–0x2306b` y ANM/VGA/SPR | Texto por glifos que se dibuja sobre cada composición; cuatro buffers según escena | [Nota de elevador/shuttle](reference/exe/elevator-cinematics.md) |
| Intro de créditos original | Texto pintado dentro de `PRESENTE.VGA`, `WHO1–4.VGA`; `TITRE.ANM` es animación | No hay cadena reemplazable separada establecida | [Nota de introducción](reference/exe/introduction.md#pre-pseq-block-implemented-2026-11-15-after-the-audit) |
| Muerte / GAME OVER | EXE tiene ocho líneas terminadas en NUL; tipografía y fondo en recursos | Líneas rasterizadas por glifos sobre GAMEOVER; no son parte de la imagen de fondo | [Nota de muerte](reference/exe/player-death.md#common-ending-and-restart) |
| Final FSEQ | Recursos FSEQ ANM/VGA/SPR | FIN está dibujado en el último fotograma FSEQ6; no se invoca el callback PSEQ | [Nota FSEQ](reference/exe/ending.md) |

## Mapa de extracción local

Fuentes del port que contienen texto visible (inspeccionadas selectivamente; excluye `src/app/diagnostics/`, logs, mensajes SDL, nombres internos, y cadenas de herramientas):

- Menú/opciones: `src/app/startup/port_menu_state.cpp`, `src/app/startup/port_menu_view.cpp`.
- Inicio/créditos: `src/app/startup/credits_view.cpp`; strings editoriales y composiciones.
- Gameplay/HUD: `src/extremis/ui/game_messages.cpp`, `src/extremis/ui/original_hud.cpp`.
- Paneles/inventario: `src/extremis/ui/original_interactions.cpp`, `src/app/session/item_session.cpp`, `src/app/session/item_animation.cpp`.
- Cinemáticas y texto de escenas: `src/extremis/cinematics/level22_communication.cpp`, `original_intro.cpp`, `original_shuttle.cpp`, `src/app/session/shuttle_prompt.cpp`.
- Password: `src/app/session/shuttle_session.cpp`, `src/app/session/session_frame.cpp`; el código de continuación se trata como identificador, no texto traducible.

La extracción restante debe recorrer estos archivos con un extractor consciente de los helpers de dibujo (`menuText`, `smallText`, `Font::draw`, blit de glifos), no contar cualquier literal C++ como texto visible. También debe volcar los buffers originales acotados fuera del repositorio, revisar CR/terminadores/encoding y registrar hashes; no copiar diálogos originales al repositorio hasta tener claro el permiso/licencia de redistribución. La documentación existente resume temas de páginas narrativas, pero no sustituye la transcripción cotejada.

## Cinemáticas: viabilidad de traducción

| Secuencia | Texto identificado | ¿Traducible conservando animación/audio? | Método futuro mínimo |
| --- | --- | --- | --- |
| CONCEPT / TITRE | Logo/título en sprite/ANM; no se verificó prosa | No requiere traducción salvo marcas/logos | Conservar imagen y audio |
| PRESENTE / WHO1–4 | Rótulos y créditos rasterizados en VGA | Sí, solo si se acepta cubrir zonas concretas; requiere inspección de píxel/legibilidad | Crear overlays localizados transparentes a coordenadas 320×200 por tarjeta y componer entre decode y presentación; no alterar ANM/audio |
| PSEQ3 | 350 bytes de texto en EXE, dibujado sobre PSEQ3.ANM | Sí, es la mejor candidata: texto separado del video | Seleccionar buffer traducido y usar el mismo callback/CR; respetar duración, número de líneas y ancho disponible |
| PSEQ6 | Segundo buffer de 316 bytes, dibujado sobre ANM/sprites | Sí, misma arquitectura de callback; validar sincronía y límites de línea | Overlay por glifos o canvas con el mismo paso de animación |
| GAME OVER | Ocho líneas EXE sobre fondo VGA con sprites de fuente | Sí, reemplazo de líneas manteniendo fondo/tiempo | Traducir tabla y ajustar centrado por anchura, comprobando cada línea |
| FSEQ | `FIN` está pintado en FSEQ6.ANM; no hay helper/cadena externa identificada | Solo mediante parche visual localizado del fotograma o overlay temporal | Preferir overlay estrecho sincronizado al fotograma final, si el texto no se mueve; validar todos los frames y alpha indexado |
| Ascensor ordinario | Sin texto narrativo identificado; composición A_LEVEL | No aplica según evidencia actual | No tocar |
| Shuttle | Cuatro cadenas/guiones EXE dibujados sobre composites y ANM | Sí, por la ruta de glifos; espacio estrecho y timing por byte | Traducir buffers conservando CR, byte/step y revisar sprite-font/anchura por escena |
| Comunicación nivel 22 | Pregunta y siete páginas EXE dibujadas sobre fondos, retratos y animación | Sí, texto ya es independiente de fondos/ANM; es una candidata clara | Sustituir buffers localizados, mapear glifos necesarios y conservar CR, fases y número de páginas |
| Muerte MORT0/MORT1 | No se identificó prosa; sprites visuales | No aplica con evidencia actual | No tocar |

No se ha comprobado visualmente cada píxel de cada fotograma en esta fase. Por ello, «texto pintado» en créditos/FIN es evidencia de recurso descrita en las notas existentes, no una prueba de que cualquier overlay sea imperceptible. Hace falta generar previews locales y un registro de rectángulos antes de implementar. Ninguna cinemática completa se reconstruirá ni se reemplazará.

## Tipografía y caracteres

### Port (`CASQUE.SPR`)

`src/extremis/ui/original_font.cpp` exige 86 sprites y usa sprites 42–85: A–Z, 0–9, `? ! ' . , : -`. Las minúsculas ASCII se convierten a mayúsculas. Espacio tiene avance fijo. Cualquier byte no mapeado se omite visualmente con avance de espacio. Este renderer **no admite** acentos, `Ñ/ñ`, `Ç/ç`, `¿` ni `¡` como texto UTF-8; cada byte UTF-8 se vería como glifo desconocido. El sprite count por sí solo no prueba que los sprites restantes sean glifos: el código los valida únicamente como letras/dígitos/puntuación.

### Fuentes originales de cinemática/panel

PSEQ usa glifos de `CON_VUET.SPR`; comunicación y shuttle usan tablas con 76 glifos en `CON_VUET.SPR`/`CON_VISA.SPR` y códigos extendidos. La tabla PSEQ enumera letras, cifras y puntuación más bytes DOS acentuados; comunicación/shuttle tienen sus propios índices y conjuntos de bytes (`level22_communication.cpp`, `original_shuttle.cpp`). Los bytes extendidos observados en el código (`0x82`, `0x85`, `0x88`, `0x8a`, `0x93`, `0x96`, `0x97`) tienen glifos en esos mapas, pero no se confirma que cubran todos los caracteres requeridos de ES/FR ni que cada uno equivalga a una letra acentuada concreta. Faltan pruebas de glifo y revisión visual sobre recursos originales.

**Solución mínima futura:** ampliar el mapa lógico de `CASQUE.SPR` usando sprites actualmente no asignados solo después de inspeccionar su contenido real; si faltan, añadir una fuente pequeña de soporte y un mapeo Unicode→glifo compartido. En cinemáticas indexadas, añadir equivalencias explícitas de bytes a glifos disponibles o componer los glifos nuevos en overlays. No emitir UTF-8 directamente al mapa ASCII actual. Requeridos: `ÁÉÍÓÚÜÑáéíóúüñ`, `ÀÂÆÇÈÉÊËÎÏÔŒÙÛÜŸ` y minúsculas francesas, además de `¿¡`; definir si la visualización original convierte todo a mayúsculas.

## Integración futura mínima (no implementada)

1. Cambiar el enum actual de idioma (`UserLanguage::English` únicamente) a EN/ES/FR y aceptar/persistir esos valores en `settings.ini`; mantener EN por defecto.
2. Crear un catálogo con IDs estables y formatos tipados, separado por área; toda cadena visible debe solicitarse por ID, incluyendo opciones, mensajes, paneles, cinematics y créditos. Mantener códigos/contraseñas/nombres de archivo sin traducir.
3. Pasar una referencia inmutable al idioma/catálogo desde el estado de sesión compartido a menús, HUD, paneles y reproductores; no añadir un segundo estado de preferencia.
4. Mantener las cadenas de animación originales como datos de escena y localizar solo texto, no secuencias, sonidos ni orden de escenas. Retener CR/índices de tipowriter salvo que un test de ancho obligue a una plantilla de líneas específica.
5. Resolver fuente y layout antes de activar ES/FR; medir cada traducción a escala lógica, revisar clips, centrado, texto que se dibuja sobre VGA, paneles estrechos y prompts. Dar fallback EN por ID ausente.
6. Añadir verificación de cobertura (ningún ID visible sin EN/ES/FR o estado explícito pendiente), pruebas de longitud/layout/font y revisión manual por pantallas y secuencias originales.

## Pendiente de verificar

- Transcripción exacta y export segura de las dos cadenas PSEQ, ocho líneas de GAME OVER, cuatro buffers de shuttle, y todos los literales de interacción que no están en el catálogo inicial.
- Cobertura completa de objetos/tarjetas/nombres de panel y texto que solo exista como sprite/imagen en DATA/EXE.
- Glifo real representado por cada byte extendido y cobertura exacta de francés/español en cada fuente original.
- Rectángulos por fotograma para créditos, logotipo, FIN y cualquier texto rasterizado; overlays visuales no se han probado.
- Si la opción LANGUAGE debe cambiar créditos/nombres editoriales además de interfaz y gameplay; nombres propios requieren conservarse.
- Accesibilidad/licencia de incluir traducciones de diálogos originales en repositorio; por ahora no se copia prosa completa del juego.

## Cierre de extracción del 24-11-2026

El catálogo existente se amplió con transcripción acotada del EXE local y 51 cadenas/plantillas de interacción, inventario y paneles del C++ actual. Las cinco superficies de LOAD GAME que constaban en el inventario anterior ya tenían traducción en el catálogo: título, indicación de confirmación, aceptación, rechazo y etiqueta original «Code d'accès rapide». Se añaden ahora dos textos realmente ausentes del aviso de código rápido (`QUICK CODE`, `PRESS ANY KEY`). La cuenta inicial de «42» no correspondía al número de filas ya traducidas: el catálogo previo contenía 31 filas de menú/contraseña, 14 de HUD y tres etiquetas breves originales completas (48). Tras esta ampliación hay **125 IDs con propuesta ES y FR**. `comm.question` era una reserva sin original exacto y ahora tiene fuente cotejada y traducciones; las reservas narrativas de la tabla anterior se resuelven en la nueva sección de extracción, sin contar ambas veces.

| Secuencia | Archivo y fuente textual exacta | Área / momento del texto | Presentación / traducción sin alterar animación |
| --- | --- | --- | --- |
| PSEQ3 | `PSEQ3.ANM`, EXE `0x23906`, 350 bytes | Empieza con la película; callback antes de copiar cada fotograma; origen (10,10), CR suma 11 a y; película repetida hasta NUL | Texto independiente sobre canvas; buffer localizado con mismo callback y reflujo de líneas |
| PSEQ6 | `PSEQ6_1.ANM`, `PSEQ6_2.SPR`, EXE `0x23a68`, 316 bytes | Película y cola de sprites; origen (10,10), progreso por callback | Texto independiente; buffer localizado y pruebas de tiempo/anchura |
| Comunicación nivel 22 | `CON_VUE.VGA`, `CON_ECR1/2.VGA`, `CON_VUET.SPR`, `CON_VISA.SPR`; pregunta EXE `0x21e04`, siete páginas `0x21e4d–0x22d70` | Pregunta centrada y opciones en y=20/30/40; páginas desde (96,19), CR suma 6 a y; retrato y barra animados | Texto independiente en zona x=96..255, y=19..150. Localizar buffers y comprobar reflujo por página; conservar animación y audio ausente |
| GAME OVER | `GAMEOVER.VGA`, `TYPOOVER.SPR`; ocho líneas EXE `0x21b96` | Texto centrado, primera línea y=60, paso 10; efecto de aparición por carácter | Sustituir líneas, conservar fondo, centrado y tiempos; una línea vacía es deliberada |
| Shuttle | `NAV_*.VGA`, `NAV_SEQ*.ANM`, `VAIS_EXP.ANM`, `ARR_BASE.ANM`; cuatro buffers EXE `0x22e2c–0x23141` | Texto en panel superior (x=125, y=5), CR suma 5; callback durante escenas según `original_shuttle.cpp` | Texto independiente con sprites `NAV_INST.SPR`; mantener secuencia y audio; comprobar ancho/tiempo por escena |
| Embarque | `NAV_HANG.VGA`, `NAV_INTE.VGA`, `CON_VUET.SPR`; seis buffers EXE `0x22d71–0x22e2b` | Preguntas y motivos centrados por `shuttle_prompt.cpp`; momento depende de estado de embarque | Sustituir texto centrado, no tocar fondo ni estados |
| Créditos originales | `PRESENTE.VGA`, `WHO1–WHO4.VGA`; título `TITRE.ANM` | Texto pintado en las tarjetas estáticas; título pintado en fotogramas | Overlay de rectángulos específicos sobre las tarjetas decodificadas antes de presentar; medir píxeles y paleta. No se identificó texto dinámico que sustituir. Nombres propios se conservan |
| Final FSEQ | `FSEQ1/3/5/6.ANM`, `FSEQ2/4*.VGA/.SPR` | Solo `FIN` identificado, pintado en último fotograma de FSEQ6 | «FIN» funciona en ES/FR; no hace falta overlay. Para otra palabra, overlay del área final por fotograma con inspección visual |
| MORT0/MORT1 | `MORT0.VGA/.SPR`, `MORT1.ANM` | Sin texto narrativo separado identificado | No requiere localización salvo GAME OVER |

Los originales exactos y traducciones están en `localization-catalog.md`. `FADDING` y `ARTERIAL USAGE` en shuttle son lecturas exactas, pero su intención es incierta; las traducciones marcadas para esos rótulos son provisionales. La primera página de comunicación contiene inglés fragmentado y francés, y requiere revisión editorial antes de declarar equivalencia literal. Los rótulos pintados en `PRESENTE/WHO` se conocen por la nota de introducción, pero faltan rectángulos medidos y un OCR/cotejo de todos sus píxeles. Los sprites de interfaces también pueden contener texto no recogido por el inventario C++.

**Bloqueos de integración:** (1) mapa de glifos Unicode y comprobación visual de acentos/Ñ/Ç/¿/¡ en CASQUE, CON_VUET, CON_VISA y fuentes de panel; (2) reflujo y medición de todas las líneas de comunicación, GAME OVER y cine en 320×200, incluidas las velocidades del efecto de escritura; (3) transcripción visual de rótulos embebidos en sprites de panel y tarjetas de créditos, seguida de overlays con coordenadas verificadas; (4) revisión editorial de los dos rótulos ambiguos del shuttle y la primera página narrativa. El catálogo textual dinámico identificado está preparado para una integración experimental, pero no se puede afirmar cobertura gráfica completa ni activar ES/FR todavía.

La fase 2 añade tres avisos de aspecto y dos rótulos de transferencia omitidos en el cierre anterior: el total actualizado es **130 IDs**, de los que **101** son texto interactivo conectado y **29** pertenecen a reservas originales/cinemáticas. El cambio de idioma desde menú y pausa usa la misma referencia de `UserSettings`; la cinemática y el texto pintado en los fondos quedan fuera de esta integración.

## Actualización de Fase 3 (2026-11-24)

Los textos independientes de PSEQ3/PSEQ6, pregunta y páginas 2–7 del nivel 22, guiones 1 y 3 del shuttle, sus cuatro avisos de embarque y las líneas no vacías de GAME OVER se dibujan ahora en ES/FR sobre las imágenes originales. YES/NO se traducen con el mismo idioma activo. Los textos ambiguos `comm.page.01` y `shuttle.script.02/.04` conservan el EXE en todos los idiomas. El FIN de FSEQ6 es la misma palabra en EN/ES/FR y queda intacto.

Inspección de los VGA originales: `PRESENTE.VGA` contiene «Présente» en x108..208, y83..102; `WHO1.VGA` «PROGRAMMATION» en x86..237, y77..90; `WHO2.VGA` «GRAPHISME» en x113..209, y77..90; `WHO3.VGA` «TRAVAIL ADDITIONNEL» en x60..259, y78..91; `WHO4.VGA` «BANDE SONORE» en x94..228, y77..90. Las líneas rojas y los nombres de autores están fuera de la máscara blanca de las letras. La composición ES sustituye únicamente los píxeles blancos del rótulo dentro de la franja y dibuja con `CASQUE.SPR`; EN/FR muestran el VGA original. `TITRE.ANM` muestra el título propio «IN EXTREMIS» y no se traduce. Las tarjetas y el fotograma FIN no requieren reconstrucción gráfica. Los créditos editoriales adicionales del port en `credits_view.cpp` son distintos de estas tarjetas originales y quedan por inventariar como una tarea editorial separada.

La cuenta original queda en **127/130 IDs resueltos**. Cinco IDs de tarjetas, descubiertos al cerrar el raster, amplían el catálogo a **132/135 resueltos**. Esta actualización reemplaza los estados «pendiente» de las tablas históricas anteriores; las tres ambigüedades siguen siendo el bloqueo concreto. No se afirma una comparación visual exhaustiva DOS contra port en todos los aspectos y resoluciones; sí se comprobaron los límites de texto con los sprites DOS originales y las pruebas de orden, reloj y audio existentes.

## Corrección posterior de modales (2026-11-24)

La afirmación histórica de que los rótulos pintados en los sprites de inventario y
taquillas seguían en inglés queda supersedida para `INVENT_A.SPR`, `CH_OBJ4A.SPR`
y `CHARME4A.SPR`: el port cubre sus regiones de texto y compone EN/ES/FR con
los 16 IDs nuevos indicados en `localization-catalog.md`. La composición fue
inspeccionada visualmente a 320×200 con los recursos originales, incluidos
inventario, tarjetas, suministros, armas e intercambio de baterías. La
verificación en una partida interactiva sigue pendiente.

## Sustitución gráfica del inventario (2026-11-24, Port-tested)

La composición anterior de `Renderer::inventory` y `Renderer::cards` dibujaba
primero `INVENT_A.SPR`, los sprites naranjas de selección y el panel de sección,
y después añadía texto de `TYPO_3X5.BIN` sobre franjas del framebuffer. El
texto inglés estaba rasterizado en los tres lugares; las franjas no lo
eliminaban por completo y el nuevo texto usaba otros orígenes y otra fuente.
La afirmación anterior de que esa composición estaba visualmente resuelta
queda supersedida.

`INVENT_A.SPR` (SHA-256
`589b825f7928be5dfd140fdd12123d5a2c54f3cdeb5a9324f7e899c37c60db74`)
contiene el fondo de 184×180. Sus títulos de la columna izquierda ocupan las
bandas locales y=2..21, 29..38, 48..67, 74..93, 98..117 y 122..132;
las letras quedan en x=70..109, después de los conectores que terminan en
x=69. Los sprites 1..5 son los rótulos naranjas de selección, el 26 es
`Exit`, y los sprites 6..10 son paneles de 71×25 con títulos verdes sobre
fondo de índice 223. El borde del panel y sus objetos están fuera de las
regiones restauradas. Estas coordenadas son **locales a los sprites**, no
offsets del EXE; la base se dibuja en pantalla en (68,10), la selección en
x=139 y los paneles en x=180.

El port crea copias ES/FR del banco en memoria. En la base restaura solo los
píxeles de las bandas de texto al negro original (índice 254), conserva los
conectores y compone con los glifos sombreados de `INVENT_T.SPR` (SHA-256
`6e5157e2aa0b3cc053a84a251ddf35f9b265810ebad95b0e1a905b25d55d5215`).
En los paneles restaura su interior uniforme al índice 223 antes de escribir
el título traducido. Los rótulos de selección se rehacen como sprites
naranjas dentro de la columna original; el ancho máximo de 40 píxeles evita
invadir el panel derecho, y las palabras largas se comprimen horizontalmente.
La ruta EN usa intactos los sprites cargados. Los archivos DOS no se escriben.

Capturas de las seis selecciones en los tres idiomas:
`build/panel-debug/inventory-all-sections-en-es-fr.png`; composición completa:
`build/panel-debug/inventory-en-es-fr.png`. Se inspeccionaron restos ingleses,
bordes, objetos, selección, acentos y palabras. La captura de prueba procede
del renderizador del port con recursos originales; no constituye una
observación dinámica del ejecutable DOS.

Se generaron además previsualizaciones con escalado de vecino más próximo y
presentación 4:3 dentro de 640×480, 1280×720 y 1680×720 para cada idioma
(`build/panel-debug/inventory-language-*-{640x480,1280x720,1680x720}.png`).
No aparecieron recortes ni invasión del borde al cambiar tamaño o relación de
aspecto. Estas son pruebas de presentación sobre los fotogramas 320×200, no
capturas de una partida ejecutada en esas ventanas.
