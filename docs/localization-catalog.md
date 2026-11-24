# Catálogo de localización inicial (Fase 1)

IDs estables propuestos con prefijo por superficie. EN reproduce el literal visible actual o el texto breve original indicado; ES/FR son propuestas editoriales. «Verificado» significa procedencia/texto cotejados en fuente local, no revisión por traductor nativo. No se altera ningún recurso ni código. No se incluyen diálogos largos del EXE sin cotejar sus bytes completos.

## Menús y opciones

| ID | EN exacto | Español | Français | Procedencia / contexto | Restricción | Estado |
| --- | --- | --- | --- | --- | --- | --- |
| menu.new_game | NEW GAME | NUEVA PARTIDA | NOUVELLE PARTIE | C++ menú principal | Ancho de renglón | Traducido; fuente verificada |
| menu.load_game | LOAD GAME | CARGAR PARTIDA | CHARGER UNE PARTIE | C++ menú principal | Ancho de renglón | Traducido; fuente verificada |
| menu.options | OPTIONS | OPCIONES | OPTIONS | C++ menú principal | — | Traducido; fuente verificada |
| menu.exit | EXIT | SALIR | QUITTER | C++ menú principal | — | Traducido; fuente verificada |
| pause.back_to_game | BACK TO GAME | VOLVER AL JUEGO | REPRENDRE LA PARTIE | C++ menú pausa | — | Traducido; fuente verificada |
| options.video | VIDEO | VÍDEO | VIDÉO | C++ opciones | Acentos | Traducido; fuente verificada |
| options.audio | AUDIO | AUDIO | AUDIO | C++ opciones | — | Traducido; fuente verificada |
| options.controls | CONTROLS | CONTROLES | COMMANDES | C++ opciones | El francés debe distinguir controles de mando si conviene | Traducido; fuente verificada |
| options.language | LANGUAGE | IDIOMA | LANGUE | C++ opciones | — | Traducido; fuente verificada |
| options.back | BACK | VOLVER | RETOUR | C++ opciones | — | Traducido; fuente verificada |
| options.aspect_ratio | ASPECT RATIO | RELACIÓN DE ASPECTO | FORMAT D’IMAGE | C++ vídeo | «AUTO» queda igual | Traducido; fuente verificada |
| options.resolution | RESOLUTION | RESOLUCIÓN | RÉSOLUTION | C++ vídeo | Valores numéricos invariables | Traducido; fuente verificada |
| options.fisheye | FISHEYE EFFECT | EFECTO OJO DE PEZ | EFFET FISHEYE | C++ vídeo | Comprobar ancho | Traducido; fuente verificada |
| options.floor_ceiling | FLOOR AND CEILING | SUELO Y TECHO | SOL ET PLAFOND | C++ vídeo | Etiqueta larga | Traducido; fuente verificada |
| options.colored_lights | COLORED LIGHTS | LUCES DE COLORES | ÉCLAIRAGE COLORÉ | C++ vídeo | Etiqueta larga | Traducido; fuente verificada |
| options.vsync | VSYNC | SINCRONIZACIÓN VERTICAL | SYNCHRONISATION VERTICALE | C++ vídeo | Puede requerir abreviar | Traducido; fuente verificada |
| options.master_volume | MASTER VOLUME | VOLUMEN GENERAL | VOLUME GÉNÉRAL | C++ audio | Número 0–100 invariable | Traducido; fuente verificada |
| options.control_scheme | CONTROL SCHEME | ESQUEMA DE CONTROL | PROFIL DE COMMANDES | C++ controles | Valores MODERN/CLASSIC | Traducido; fuente verificada |
| options.modern | MODERN | MODERNO | MODERNE | C++ controles | Nombre de perfil | Traducido; fuente verificada |
| options.classic | CLASSIC | CLÁSICO | CLASSIQUE | C++ controles | Nombre de perfil | Traducido; fuente verificada |
| options.on | ON | ACTIVADO | ACTIVÉ | C++ estado | Debe caber junto a la etiqueta | Traducido; fuente verificada |
| options.off | OFF | DESACTIVADO | DÉSACTIVÉ | C++ estado | Debe caber junto a la etiqueta | Traducido; fuente verificada |
| options.unavailable | UNAVAILABLE | NO DISPONIBLE | INDISPONIBLE | C++ estado actual | Estado, no opción | Traducido; fuente verificada |
| options.selected | SELECTED | SELECCIONADO | SÉLECTIONNÉ | C++ estado actual | Concordancia puede requerir frase neutra | Traducido; fuente verificada |
| options.auto_aspect | AUTO: MATCH MONITOR ASPECT | AUTO: USAR ASPECTO DEL MONITOR | AUTO : FORMAT DE L’ÉCRAN | C++ aviso de vídeo | Etiqueta larga | Traducido; fuente verificada |
| options.vsync_failed | VSYNC SWITCH FAILED | NO SE PUDO CAMBIAR VSYNC | ÉCHEC DU CHANGEMENT DE VSYNC | C++ aviso | Transitorio | Traducido; fuente verificada |
| options.display_failed | DISPLAY MODE SWITCH FAILED | NO SE PUDO CAMBIAR EL MODO DE PANTALLA | ÉCHEC DU CHANGEMENT DE MODE D’AFFICHAGE | C++ aviso | Transitorio, largo | Traducido; fuente verificada |
| options.aspect_original | 4:3 ORIGINAL DOS VIEW | 4:3 VISTA DOS ORIGINAL | 4:3 VUE DOS ORIGINALE | C++ aviso de vídeo | Ancho | Traducido; fuente verificada |
| options.aspect_wide | 16:9 EXPANDED GAME VIEW | 16:9 VISTA AMPLIADA | 16:9 VUE ÉLARGIE | C++ aviso de vídeo | Ancho | Traducido; fuente verificada |
| options.aspect_ultrawide | 21:9 EXPANDED GAME VIEW | 21:9 VISTA AMPLIADA | 21:9 VUE ÉLARGIE | C++ aviso de vídeo | Ancho | Traducido; fuente verificada |
| password.heading | CONTINUATION CODE | CÓDIGO DE CONTINUACIÓN | CODE DE CONTINUATION | C++ port; original dice «Code d'accès rapide» en EXE 0x238d2 | Mantener código intacto | EN port verificado; ES/FR propuesta |
| password.enter_confirm | ENTER TO CONFIRM | INTRO: CONFIRMAR | ENTRÉE : VALIDER | C++ LOAD GAME | Tecla depende de plataforma | Traducido; fuente verificada |
| password.accepted | CODE ACCEPTED | CÓDIGO ACEPTADO | CODE ACCEPTÉ | C++ LOAD GAME | — | Traducido; fuente verificada |
| password.refused | CODE REFUSED | CÓDIGO RECHAZADO | CODE REFUSÉ | C++ LOAD GAME | — | Traducido; fuente verificada |

## HUD y gameplay

| ID | EN exacto | Español | Français | Procedencia / contexto | Restricción | Estado |
| --- | --- | --- | --- | --- | --- | --- |
| hud.portal_locked | ATTENTION:\r\n  IMPOSSIBLE TO OPEN PORTAL!\r\n  DISABLE THE SECURITY | ATENCIÓN:\r\n  ¡IMPOSIBLE ABRIR LA COMPUERTA!\r\n  DESACTIVE LA SEGURIDAD | ALARME:\r\n  IMPOSSIBLE D’OUVRIR LE SAS !\r\n  DÉSACTIVEZ LA SÉCURITÉ | `game_messages.cpp`; mensaje de puerta | Ancho/fuente; renglones preservados | ES/FR propuestos; texto port verificado |
| hud.door_unlocked | VALID CARD..... DOOR UNLOCKING! | TARJETA VÁLIDA..... ¡ABRIENDO PUERTA! | CARTE VALIDE..... OUVERTURE DU SAS ! | C++ mensaje puerta | Punto de elipsis como efecto | Traducido; fuente verificada |
| hud.card_rejected | IMPOSSIBLE TO UNLOCK THIS DOOR... | IMPOSIBLE ABRIR ESTA PUERTA... | IMPOSSIBLE DE DÉVERROUILLER CETTE PORTE... | C++ mensaje puerta | Ancho | Traducido; fuente verificada |
| hud.security_disabled | THE SECURITY IS ALREADY DISABLE | LA SEGURIDAD YA ESTÁ DESACTIVADA | LA SÉCURITÉ EST DÉJÀ DÉSACTIVÉE | C++ mensaje; inglés original del port tiene gramática no nativa | Ancho | Corregido editorialmente; fuente verificada |
| hud.map_blocked | ATTENTION: AN ALIEN IS NEAR!! | ATENCIÓN: ¡HAY UN ALIENÍGENA CERCA! | ATTENTION : UN EXTRATERRESTRE EST PROCHE !! | `game_messages.cpp`; original citado en nota EXE | Ancho HUD | Traducido; texto port verificado |
| hud.map_unavailable | ATTENTION:\r\n  ACCESS NOT POSSIBLE WITH CARD!! | ATENCIÓN:\r\n  ¡LA TARJETA NO PERMITE EL ACCESO! | ATTENTION :\r\n  CARTE NON AUTORISÉE POUR CET ACCÈS !! | C++ mensaje terminal | Dos líneas | Traducido; fuente verificada |
| hud.self_destruction | 0M 00S UNTIL EXPLOSION! | ¡EXPLOSIÓN EN 0M 00S! | EXPLOSION DANS 0M 00S ! | C++; dígitos interpolados | Preservar posiciones de sustitución y unidades | Traducido; dinámica requiere test |
| hud.bomb_warning | ATTENTION:\r\n  5 SECONDS BEFORE EXPLOSION! | ATENCIÓN:\r\n  ¡EXPLOSIÓN EN 5 SEGUNDOS! | ATTENTION :\r\n  EXPLOSION DANS 5 SECONDES ! | C++ advertencia de bomba | Número dinámico/fijo | Traducido; fuente verificada |
| hud.shock | ATTENTION: SHOCKS | ATENCIÓN: DESCARGA ELÉCTRICA | ATTENTION : DÉCHARGE ÉLECTRIQUE | C++ mensaje daño | Longitud | Traducido; fuente verificada |
| hud.power_surge | ATTENTION\r\n POWER IS SURGING!!! | ATENCIÓN\r\n ¡SOBRECARGA ELÉCTRICA! | ATTENTION\r\n SURTENSION ÉLECTRIQUE !!! | C++ evento de energía | Mantener concepto | Traducido; fuente verificada |
| hud.battery_failed | ATTENTION:\r\n  NEW BATTERY HAS FAILED!!!!!! | ATENCIÓN:\r\n  ¡LA BATERÍA NUEVA HA FALLADO! | ATTENTION :\r\n  LA NOUVELLE BATTERIE EST HORS SERVICE ! | C++ batería | Ancho | Traducido; fuente verificada |
| hud.radar_activated | ATTENTION: RADAR SENSOR WAS ACTIVATED | ATENCIÓN: RADAR ACTIVADO | ATTENTION : RADAR ACTIVÉ | C++ mensaje | Original suena a estado de activación | Traducido; fuente verificada |
| hud.sensors_activated | ATTENTION: SENSORS ACTIVATED | ATENCIÓN: SENSORES ACTIVADOS | ATTENTION : CAPTEURS ACTIVÉS | C++ mensaje | — | Traducido; fuente verificada |
| hud.empty_compartment | THIS COMPARTMENT IS EMPTY! | ¡ESTE COMPARTIMENTO ESTÁ VACÍO! | CE COMPARTIMENT EST VIDE ! | C++ panel | Ancho | Traducido; fuente verificada |

## Etiquetas originales breves verificables

| ID | Original exacto | Español | Français | Procedencia / contexto | Restricción | Estado |
| --- | --- | --- | --- | --- | --- | --- |
| exe.quick_code | Code d'accès rapide | Código de acceso rápido | Code d'accès rapide | EXE `0x238d2`, etiqueta del código de continuación | Glifos/acentos | Original verificado por nota; ES traducido |
| comm.yes | YES | SÍ | OUI | Tabla de comunicación EXE `0x21e27` | Glifo Í y ancho | EN/FR exactos documentados; ES traducido |
| comm.no | NO | NO | NON | Tabla de comunicación EXE `0x21e2b` | — | Verificado/traducido |
| comm.question | (texto de 34 bytes, ver EXE `0x21e04`) | Pendiente | Pendiente | Pregunta narrativa del terminal | Se debe transcribir/cotejar antes de traducir | No incluida hasta exportar fuente exacta |

## Cadenas narrativas y recursos pendientes

**Estado histórico:** las reservas de PSEQ, comunicación, shuttle y GAME OVER de esta tabla están resueltas por las secciones «Extracción exacta» y «Traducciones» añadidas abajo. No se cuentan por segunda vez. Los textos rasterizados `WHO1–4` siguen pendientes de cotejo visual.

| ID estable reservado | Texto fuente | ES | FR | Procedencia/contexto | Estado |
| --- | --- | --- | --- | --- | --- |
| intro.pseq3.body | 350 bytes + NUL | Pendiente | Pendiente | EXE `0x23906`; texto sobre PSEQ3.ANM | Offset/longitud documentados; no cotejado en esta fase |
| intro.pseq6.body | 316 bytes + NUL | Pendiente | Pendiente | EXE `0x23a68`; texto con sprites en PSEQ6 | Offset/longitud documentados; no cotejado |
| ending.game_over.line.01..08 | Ocho líneas NUL | Pendiente | Pendiente | EXE `0x21b96`; GAMEOVER.VGA + TYPOOVER.SPR | Pendiente exportar exactamente; nombres/códigos se conservan |
| shuttle.script.01..04 | 178, 197, 197, 214 bytes + NUL | Pendiente | Pendiente | EXE `0x22e2c`, `0x22edf`, `0x22fa5`, `0x2306b` | Pendiente exportar, preservar CR y sincronía |
| comm.page.01..07 | 241, 726, 496, 656, 681, 819, 250 bytes | Pendiente | Pendiente | EXE `0x21e4d`, `0x21f3f`, `0x22216`, `0x22407`, `0x22698`, `0x22942`, `0x22c76` | Referencias exactas en `level22-communication.md`; no duplicar transcripción aquí |
| intro.who.01..04 | Texto rasterizado | Pendiente | Pendiente | `WHO1–WHO4.VGA` | Requiere revisar originales; mantener nombres propios |
| ending.fin | FIN | FIN | FIN | Rasterizado en fotograma final FSEQ6 | Traducible solo mediante overlay localizado; validar imagen |

## Reglas editoriales iniciales

- Mantener `IN EXTREMIS`, Blue Sphere, nombres de autores, códigos de continuación, códigos de acceso, identificadores de sector, teclas, `VSync`, `AUTO`, y nombres de archivo sin traducir salvo una etiqueta explicativa separada.
- Preferir «compuerta» para *portal* cuando sea la puerta física; mantener «puerta» en HUD si así se entiende mejor. Mantener «sobrecarga» para *POWER IS SURGING* como evento eléctrico.
- En francés, usar «sas» para compuerta de nave y «porte» para puerta general; elegir con contexto del recurso. Revisar acentos y apóstrofos tipográficos según capacidad de glifos.
- Respetar CR, líneas, placeholders numéricos y pasos del efecto de escritura. El catálogo no certifica ajuste visual hasta una fase de diseño/layout.

## Extracción exacta de textos originales (EXE local)

Archivo SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`. Los bloques siguientes son transcripciones del EXE local con `CR` representado como salto de línea; se preservan espacios, errores ortográficos y líneas vacías. Los offsets son absolutos de archivo. Estas transcripciones se añaden por petición expresa de catálogo, no son nuevos hallazgos de gameplay.

### `intro.pseq3.body` — EXE `0x23906` (350 bytes)

```text
DETECTION SYSTEMS 

Analysis...................

        Class:  Laboratory station   

        Type : Command Ship  

Scanner running............

        Immatriculation : ZWZ 46-30-E4

        Name: Explorer 4

Observations...............

Human lifeforms have been detected.

                                                                      
```

### `intro.pseq6.body` — EXE `0x23a68` (316 bytes)

```text
!!! ATTENTION !!!

Malfunctions detected       :

        Gravity force regulators..

        Reactor surges present.....

Analyse danger.............

        Explosion imminent....

Recommendations............

        IMMEDIATE EVACUATION...

                                                                      
```

### `comm.question` — EXE `0x21e04` (34 bytes)

```text
Would yo like to turn on system ??
```

### `comm.page.01` — EXE `0x21e4d` (241 bytes)

```text
This is without doubt my last message...

If you had arrived here, then possibly  
I might have thought about a rescue ....
The man posing a threat, at which he    
would have a chance.

Peut-etre  seriez vous capable vous meme
de reussir...
```

### `comm.page.02` — EXE `0x21f3f` (726 bytes)

```text
Voici ce qui nous est arrive:

Suite  a la decouverte  d'un asteroide a
la trajectoire hasardeuse, la communaute
scientifique avait decide d'envoyer  une
sonde  de reconnaissance  vers  cet amas
rocheux.
Munis  d'appareillages  sophistiques, la
sonde   avait   donne  des  informations
plutot   curieuses  sur  l'asteroide. Il
etait constitue de  matieres inconnues a
ce  jour, et   fait  plus  etrange,  des
mouvements etaient  captes a l'interieur
meme de l'asteroide.

Il   fut   donc   decide   d'envoyer  un
vaisseau laboratoire  pour etudier cette
etrange decouverte.La taille gigantesque
du vaisseau laboratoire  permit de recu-
perer l'amas  rocheux  en entier. Ce qui
allait  en   faciliter   d'autant   plus
l'etude.
```

### `comm.page.03` — EXE `0x22216` (496 bytes)

```text
On commenca tout d'abord par analyser la
surface. La pierre  etait bien  composee
de  matieres  inconnues   comme  l'avait
decrit la sonde, mais surtout ce n'etait
pas un rocher naturel. Il semblait avoir
ete faconne artificiellement. Des signes
semblables a des hieroglyphes couvraient
certains  endroits  et  la surface etait
beaucoup trop lisse.

Ce pouvait'il que se soit l'oeuvre d'une
population extra-terrestre?  Et dans  ce
cas  pourquoi  retrouver  cette   pierre
flottant dans l'espace?
```

### `comm.page.04` — EXE `0x22407` (656 bytes)

```text
Les  ordinateurs  furent  utilises  pour
dechiffrer  ces  etranges  signes,  mais
malgre  leur puissance de calcul elevee,
aucun   resultat   ne  fut  revele  dans
l'immediat. Cela etait peut-etre du a un
autre  mode  de pensee  de  la  part des
auteurs du message...

Pendant ce temps, la deuxieme analyse de
la sonde avait ete confirmee :
quelque chose bougeait  a l'interieur du
mysterieux   asteroide.   La   curiosite
poussee au plus haut point par la possi-
bilite d'etre  pour  la premiere fois en
contact avec  un  extra-terrestre  nous 
poussa  a  commencer  au  plus  vite  la
perforation de la pierre afin de degager
ce qu'elle pouvait contenir.
```

### `comm.page.05` — EXE `0x22698` (681 bytes)

```text
C'est alors que tout se passa tres vite:

Au moment ou l'on allait atteindre notre
but les ordinateurs donnerent la  signi-
fication  des  signes  qui  recouvraient
l'asteroide:
C'etait une mise en garde...

Mais il etait  trop tard,  le mal  avait
ete fait,une creature gigantesque surgit
et se mis immediatement  a  attaquer les
personnes se trouvant a proximite, avant
de s'enfuir dans les  couloirs  du vais-
seau. Le systeme de securite  fut  imme-
diatement enclenche  mais il  etait trop
tard, nous venions de faire penetrer une
pondeuse dans notre vaisseau.

Tres vite des dizaines de creatures a la
faculte  de  croissance  monstrueusement
rapide envahirent le vaisseau...
```

### `comm.page.06` — EXE `0x22942` (819 bytes)

```text
Nous  n'avons  pu  les  etudier et  sans
information,  nous  restons   impuissant
devant leur flot.Nos appareils d'analyse
atmospherique du vaisseau  ont neanmoins
pu nous signaler la presence de virus et
bacteries etrangeres dans l'air ambiant.
La plupart  d'entre  nous  n'ont  pu  se
proteger a  temps  et sont mort dans une
periode tres breve. Les rares survivants
sont  contraints  de porter  des  combi-
naisons spatiales pour pouvoir  respirer
sans danger.

Desormais, nous savons que  notre  temps
est compte. La seule mesure valable dans
un tel cas  est  de detruire le vaisseau
et tout  ses  intrus  mais  l'ordinateur
central possede  un systeme  de securite
l'empechant de s'autodetruire. J'ai donc
decider de diriger  le vaisseau  dans la
direction  de  l'etoile  la  plus proche
afin que tout disparaisse.
```

### `comm.page.07` — EXE `0x22c76` (250 bytes)

```text
Comme je vous l'ai dit  au debut de  mon
message, si vous etes arrive  jusqu'ici,
j'aurais echoue dans ma tentative.

S'il vous  est possible  de realiser  ce
que je n'ai pu accomplir, n'hesitez pas!

Bonne chance...













Fin du message......
```

### `shuttle.script.01` — EXE `0x22e2c` (178 bytes)

```text
-------------------
  ORDINATOR  : OK  

  MOTOR      : OK  

  ENERGY     : OK  
-------------------
                                                                            
```

### `shuttle.script.02` — EXE `0x22edf` (197 bytes)

```text
-------------------
     EVACUATION    
-------------------

-------------------
     FADDING       
-------------------
                                                                            
```

### `shuttle.script.03` — EXE `0x22fa5` (197 bytes)

```text
-------------------
 DIRECTION : AXIA  
-------------------

-------------------
                   
-------------------
                                                                            
```

### `shuttle.script.04` — EXE `0x2306b` (214 bytes)

```text
-------------------
 DIRECTION : AXIA
-------------------
-------------------
        MODE       
    ARTERIAL USAGE 
-------------------
                                                                            
```

### `shuttle.prompt.01` — EXE `0x22d71` (38 bytes)

```text
Voulez vous pénétrer dans la navette ?
```

### `shuttle.prompt.02` — EXE `0x22da0` (33 bytes)

```text
Voulez vous démarrer la navette ?
```

### `shuttle.prompt.03` — EXE `0x22dc2` (56 bytes)

```text
Impossible de démarrer la navette sans carte de sécurité
```

### `shuttle.prompt.04` — EXE `0x22dfb` (48 bytes)

```text
Impossible de décoller. Autodestruction éteinte.
```

- `ending.game_over.line.01` — EXE `0x21b97`: `Unfortunately you have failed in your current mission!!!`

- `ending.game_over.line.02` — EXE `0x21bd0`: ``

- `ending.game_over.line.03` — EXE `0x21bd1`: `Perhaps you lack the experience??`

- `ending.game_over.line.04` — EXE `0x21bf3`: `Perhaps you couldn't be relied upon??`

- `ending.game_over.line.05` — EXE `0x21c19`: `Perhaps the enemy was too numerous??`

- `ending.game_over.line.06` — EXE `0x21c3e`: `Shit happens when humanity is forced to live under`

- `ending.game_over.line.07` — EXE `0x21c71`: `the oppresion of vile creatures that`

- `ending.game_over.line.08` — EXE `0x21c96`: `terrify and cause destruction...`

## Traducciones de la extracción original

Las traducciones siguientes conservan significado y nombres; el ajuste visual a la fuente indexada sigue pendiente. Cada ID remite al texto fuente exacto de la sección anterior. Las páginas de comunicación se entregan en prosa continua; los saltos de línea deberán rehacerse para el ancho 160 píxeles de su área de texto.

| ID | Español | Français | Estado |
| --- | --- | --- | --- |
| intro.pseq3.body | SISTEMAS DE DETECCIÓN / Análisis... / Clase: estación de laboratorio / Tipo: nave de mando / Escáner en marcha... / Matrícula: ZWZ 46-30-E4 / Nombre: Explorer 4 / Observaciones... / Se han detectado formas de vida humanas. | SYSTÈMES DE DÉTECTION / Analyse... / Classe : station laboratoire / Type : vaisseau de commandement / Analyse en cours... / Immatriculation : ZWZ 46-30-E4 / Nom : Explorer 4 / Observations... / Des formes de vie humaines ont été détectées. | Traducido; separar en 10 líneas del original antes de integrar |
| intro.pseq6.body | ¡ATENCIÓN! / Fallos detectados: / Reguladores gravitatorios... / Sobrecargas del reactor... / Análisis de riesgos... / Explosión inminente... / Recomendaciones... / EVACUACIÓN INMEDIATA. | ATTENTION ! / Défaillances détectées : / Régulateurs de gravité... / Surtensions du réacteur... / Analyse des risques... / Explosion imminente... / Recommandations... / ÉVACUATION IMMÉDIATE. | Traducido; ancho/tiempo por carácter pendientes |
| comm.question | ¿Desea activar el sistema? | Voulez-vous activer le système ? | Traducido; original tiene errata «yo» |
| comm.page.01 | Este es, sin duda, mi último mensaje. Si ha llegado hasta aquí, tal vez aún quede una posibilidad de salvarnos. Quizá usted pueda lograr lo que yo no conseguí... | C'est sans doute mon dernier message. Si vous êtes arrivé jusqu'ici, il reste peut-être une chance de nous sauver. Peut-être réussirez-vous là où j'ai échoué... | Traducción interpretativa: pasaje inglés original está gramaticalmente roto; revisión editorial obligatoria |
| comm.page.02 | Esto fue lo que ocurrió: tras descubrir un asteroide de trayectoria errática, la comunidad científica decidió enviar una sonda de reconocimiento. Sus sofisticados instrumentos transmitieron datos extraños: el asteroide estaba formado por materiales desconocidos y se detectaba movimiento en su interior. Decidimos enviar una nave laboratorio. Su enorme tamaño permitió recoger el asteroide entero y facilitar su estudio. | Voici ce qui nous est arrivé : après la découverte d'un astéroïde à la trajectoire erratique, la communauté scientifique a envoyé une sonde de reconnaissance. Ses instruments sophistiqués ont révélé des matériaux inconnus et des mouvements à l'intérieur de l'astéroïde. Nous avons donc envoyé un vaisseau laboratoire. Sa taille immense a permis de récupérer l'astéroïde entier pour l'étudier. | Traducido; remaquetación pendiente |
| comm.page.03 | Primero analizamos la superficie. Como indicó la sonda, la roca contenía materiales desconocidos. Pero, sobre todo, no parecía natural: tenía forma artificial, signos semejantes a jeroglíficos y una superficie demasiado lisa. ¿Era obra de una civilización extraterrestre? Y, si era así, ¿por qué flotaba en el espacio? | Nous avons d'abord analysé la surface. Comme l'avait indiqué la sonde, la roche contenait des matériaux inconnus. Mais surtout, elle n'était pas naturelle : elle semblait façonnée artificiellement, portait des signes semblables à des hiéroglyphes et sa surface était trop lisse. Était-ce l'œuvre d'une civilisation extraterrestre ? Alors pourquoi flottait-elle dans l'espace ? | Traducido; remaquetación pendiente |
| comm.page.04 | Los ordenadores intentaron descifrar los signos, pero ni su potencia de cálculo produjo resultados inmediatos. Quizá sus autores pensaban de otra manera. Entretanto, confirmamos el segundo hallazgo de la sonda: algo se movía dentro del asteroide. La posibilidad de un primer contacto nos llevó a perforar la roca cuanto antes para descubrir qué contenía. | Les ordinateurs ont tenté de déchiffrer les signes, mais même leur puissance de calcul n'a donné aucun résultat immédiat. Leurs auteurs pensaient peut-être autrement. Entre-temps, la seconde découverte de la sonde a été confirmée : quelque chose bougeait dans l'astéroïde. La perspective d'un premier contact nous a poussés à percer la roche au plus vite pour découvrir ce qu'elle contenait. | Traducido; remaquetación pendiente |
| comm.page.05 | Entonces todo sucedió muy deprisa. Justo cuando íbamos a alcanzar el interior, los ordenadores descifraron los signos: eran una advertencia. Ya era demasiado tarde. Una criatura gigantesca surgió, atacó a quienes estaban cerca y huyó por los pasillos. Activamos la seguridad, pero habíamos dejado entrar en la nave a una ponedora. Pronto la invadieron decenas de criaturas de crecimiento monstruosamente rápido. | Alors tout s'est passé très vite. Au moment où nous allions atteindre l'intérieur, les ordinateurs ont déchiffré les signes : c'était une mise en garde. Trop tard. Une créature gigantesque a surgi, attaqué les personnes proches puis s'est enfuie dans les couloirs. Nous avons activé la sécurité, mais nous avions laissé entrer une pondeuse à bord. Bientôt, des dizaines de créatures à la croissance monstrueusement rapide ont envahi le vaisseau. | Traducido; «pondeuse» se conserva semánticamente como «ponedora» |
| comm.page.06 | No hemos podido estudiarlas y, sin información, no podemos detenerlas. Los analizadores atmosféricos han detectado virus y bacterias ajenos en el aire. La mayoría murió antes de poder protegerse; los supervivientes debemos llevar trajes espaciales para respirar sin peligro. Sabemos que nos queda poco tiempo. Debemos destruir la nave y a sus intrusos, pero la seguridad del ordenador central impide la autodestrucción. He puesto rumbo a la estrella más cercana para que todo desaparezca. | Nous n'avons pas pu les étudier et, sans information, nous ne pouvons arrêter leur flot. Les analyseurs atmosphériques ont détecté des virus et des bactéries étrangers dans l'air. La plupart sont morts avant de pouvoir se protéger ; les survivants doivent porter des combinaisons spatiales pour respirer sans danger. Notre temps est compté. Il faut détruire le vaisseau et ses intrus, mais la sécurité de l'ordinateur central empêche l'autodestruction. J'ai mis le cap sur l'étoile la plus proche pour que tout disparaisse. | Traducido; remaquetación pendiente |
| comm.page.07 | Como dije al comienzo, si ha llegado hasta aquí, mi intento ha fracasado. Si puede terminar lo que yo no logré, no lo dude. Buena suerte... Fin del mensaje. | Comme je l'ai dit au début, si vous êtes arrivé jusqu'ici, ma tentative a échoué. Si vous pouvez accomplir ce que je n'ai pas pu faire, n'hésitez pas. Bonne chance... Fin du message. | Traducido; remaquetación pendiente |
| shuttle.script.01 | ORDENADOR: OK / MOTOR: OK / ENERGÍA: OK | ORDINATEUR : OK / MOTEUR : OK / ÉNERGIE : OK | Traducido; conservar guiones de marco |
| shuttle.script.02 | EVACUACIÓN / DESVANECIMIENTO | ÉVACUATION / FONDU | «FADDING» parece errata del original; confirmar intención visual |
| shuttle.script.03 | DESTINO: AXIA | DESTINATION : AXIA | Traducido; AXIA invariable |
| shuttle.script.04 | DESTINO: AXIA / MODO / USO ARTERIAL | DESTINATION : AXIA / MODE / USAGE ARTÉRIEL | Traducción literal provisional: sentido de «ARTERIAL USAGE» desconocido; revisión visual/contextual |
| shuttle.prompt.01 | ¿Desea entrar en la lanzadera? | Voulez-vous entrer dans la navette ? | Traducido |
| shuttle.prompt.02 | ¿Desea poner en marcha la lanzadera? | Voulez-vous démarrer la navette ? | Traducido |
| shuttle.prompt.03 | No se puede arrancar la lanzadera sin tarjeta de seguridad. | Impossible de démarrer la navette sans carte de sécurité. | Traducido |
| shuttle.prompt.04 | No se puede despegar. La autodestrucción está desactivada. | Décollage impossible. L'autodestruction est désactivée. | Traducido |
| ending.game_over.line.01 | Por desgracia, ha fracasado en su misión. | Malheureusement, vous avez échoué dans votre mission. | Traducido |
| ending.game_over.line.02 | [línea vacía] | [ligne vide] | Mantener línea vacía |
| ending.game_over.line.03 | ¿Tal vez le faltaba experiencia? | Peut-être manquiez-vous d'expérience ? | Traducido |
| ending.game_over.line.04 | ¿Quizá no se podía confiar en usted? | Peut-être ne pouvait-on pas compter sur vous ? | Traducido |
| ending.game_over.line.05 | ¿O eran demasiados enemigos? | Ou les ennemis étaient-ils trop nombreux ? | Traducido |
| ending.game_over.line.06 | A veces todo sale mal cuando la humanidad vive | Parfois, tout tourne mal quand l'humanité vit | Traducido; continúa en línea siguiente |
| ending.game_over.line.07 | bajo la opresión de criaturas infames | sous l'oppression de créatures infâmes | Traducido; continúa en línea siguiente |
| ending.game_over.line.08 | que siembran el terror y la destrucción... | qui sèment la terreur et la destruction... | Traducido |

## Inventario, paneles y textos visibles del port pendientes del catálogo inicial

Fuente exacta: `original_interactions.cpp`, `item_session.cpp`, `item_animation.cpp`, `session_frame.cpp` y `port_menu_state.cpp`. `{zona}`, `{tarjeta}`, `{n}`, `{porcentaje}` y `{código}` representan fragmentos dinámicos; no son texto original. Las opciones condicionales de un mismo literal comparten ID. Duplicados como `Exit`, `Use injector`, `No elevator card`, `No security card`, `CODE DATA UNAVAILABLE` y los mensajes `messages::Id` ya catalogados se registran una sola vez. Las frases impresas dentro de sprites de panel aún necesitan transcripción gráfica.

| ID | EN exacto / plantilla | Español | Français | Contexto / tratamiento |
| --- | --- | --- | --- | --- |
| password.quick_code | QUICK CODE | CÓDIGO RÁPIDO | CODE RAPIDE | Aviso al llegar, `session_frame.cpp` |
| password.press_any_key | PRESS ANY KEY | PULSE CUALQUIER TECLA | APPUYEZ SUR UNE TOUCHE | Aviso al llegar |
| panel.type_zone | TYPE IN CODE ZONE {zona} | INTRODUZCA EL CÓDIGO DE LA ZONA {zona} | SAISISSEZ LE CODE DE LA ZONE {zona} | `original_interactions.cpp`; zona invariable |
| panel.transfer | TRANSFER | TRANSFERENCIA | TRANSFERT | `interaction_animation.cpp`, animación de plano |
| panel.zone | ZONE {zona} | ZONA {zona} | ZONE {zona} | `interaction_animation.cpp`, zona invariable |
| panel.zone_seized | CODE ZONE {zona} ALREADY SEIZED | CÓDIGO DE LA ZONA {zona} YA OBTENIDO | CODE DE ZONE {zona} DÉJÀ OBTENU | `item_session.cpp` |
| panel.code_unavailable | CODE DATA UNAVAILABLE | DATOS DEL CÓDIGO NO DISPONIBLES | DONNÉES DU CODE INDISPONIBLES | Repetido tres veces |
| panel.code_ok | OK... | CORRECTO... | CORRECT... | Respuesta del teclado |
| panel.code_wrong | WRONG CODE!!... | ¡CÓDIGO INCORRECTO!... | CODE ERRONÉ !... | Respuesta del teclado |
| panel.code_zone | CODE ZONE {zona}  {letras} | CÓDIGO ZONA {zona}  {letras} | CODE ZONE {zona}  {letras} | `{letras}` es dato original invariable |
| panel.no_elevator_card | No elevator card | Sin tarjeta de ascensor | Pas de carte d'ascenseur | También menú de tarjetas |
| panel.no_security_card | No security card | Sin tarjeta de seguridad | Pas de carte de sécurité | También menú de tarjetas |
| panel.incompatible_card | Incompatible card type | Tarjeta incompatible | Carte incompatible | Ascensor/seguridad |
| panel.card_refused | CARD REFUSED... | TARJETA RECHAZADA... | CARTE REFUSÉE... | Lector |
| panel.card_accepted | CARD ACCEPTED.... | TARJETA ACEPTADA... | CARTE ACCEPTÉE... | Lector |
| panel.security_off | SECURITY {n} DEACTIVATED.. | SEGURIDAD {n} DESACTIVADA... | SÉCURITÉ {n} DÉSACTIVÉE... | Número dinámico |
| panel.security_failed | IMPOSSIBLE TO DEACTIVATE THE SECURITY | IMPOSIBLE DESACTIVAR LA SEGURIDAD | IMPOSSIBLE DE DÉSACTIVER LA SÉCURITÉ | Panel |
| panel.mine_owned | ALREADY HAVE A MINE | YA TIENE UNA MINA | VOUS AVEZ DÉJÀ UNE MINE | Armario |
| panel.oxygen_owned | ALREADY HAVE AN OXYGEN BOTTLE | YA TIENE UNA BOTELLA DE OXÍGENO | VOUS AVEZ DÉJÀ UNE BOUTEILLE D'OXYGÈNE | Armario |
| panel.injectors_owned | ALREADY HAVE TWO INJECTORS | YA TIENE DOS INYECTORES | VOUS AVEZ DÉJÀ DEUX INJECTEURS | Armario |
| panel.control_panel | CONTROL PANEL.... | PANEL DE CONTROL... | PANNEAU DE COMMANDE... | Animación |
| panel.controller_card | CONTROLLER CARD.. | TARJETA DE CONTROL... | CARTE DE COMMANDE... | Animación |
| panel.battery_owned | ALREADY HAVE A BATTERY | YA TIENE UNA BATERÍA | VOUS AVEZ DÉJÀ UNE BATTERIE | Armario |
| panel.take_battery | TAKE BATTERY {porcentaje}% | TOMAR BATERÍA {porcentaje}% | PRENDRE BATTERIE {porcentaje} % | Armario, ancho muy limitado |
| panel.keep_battery | KEEP BATTERY {porcentaje}% | CONSERVAR BATERÍA {porcentaje}% | GARDER BATTERIE {porcentaje} % | Armario |
| panel.use_card | Use card {tarjeta} | Usar tarjeta {tarjeta} | Utiliser carte {tarjeta} | Tarjeta invariable |
| panel.exchange_card | Exchange with card {tarjeta} | Cambiar por tarjeta {tarjeta} | Échanger contre carte {tarjeta} | Tarjeta invariable |
| panel.card | Card {tarjeta} | Tarjeta {tarjeta} | Carte {tarjeta} | Inventario |
| inventory.exit | Exit | Salir | Quitter | Compartido por paneles/inventario |
| inventory.battery_level | Battery level {n} | Carga de batería {n} | Niveau de batterie {n} | Unidad derivada de ticks, verificar rótulo |
| inventory.radar_on | Turn on radar | Activar radar | Activer le radar | Selector batería |
| inventory.radar_off | Turn off radar | Desactivar radar | Désactiver le radar | Selector batería |
| inventory.optics_on | Turn on optics | Activar óptica | Activer l'optique | Selector batería |
| inventory.optics_off | Turn off optics | Desactivar óptica | Désactiver l'optique | Selector batería |
| inventory.no_change | No change | Sin cambios | Aucun changement | Selector batería |
| inventory.injector_1 | Use injector-1 | Usar inyector 1 | Utiliser injecteur 1 | Distinguir ranura |
| inventory.injector_2 | Use injector-2 | Usar inyector 2 | Utiliser injecteur 2 | Distinguir ranura |
| inventory.use_bomb | Use bomb | Usar bomba | Utiliser la bombe | Inventario |
| inventory.use_oxygen | Use oxygen bottle | Usar botella de oxígeno | Utiliser la bouteille d'oxygène | Inventario |
| inventory.use_injector | Use injector | Usar inyector | Utiliser l'injecteur | Inventario |
| inventory.do_not_use | Do not use | No usar | Ne pas utiliser | Inventario |
| inventory.no_battery | No battery | Sin batería | Pas de batterie | Inventario |
| inventory.empty_battery | Empty battery | Batería agotada | Batterie vide | Inventario |
| inventory.battery_radar | Battery utilized for the radar | Batería usada por el radar | Batterie utilisée par le radar | Inventario |
| inventory.battery_optics | Battery utilized for optics system | Batería usada por la óptica | Batterie utilisée par l'optique | Inventario |
| inventory.battery_available | Battery available | Batería disponible | Batterie disponible | Inventario |
| inventory.no_oxygen | No oxygen bottle | Sin botella de oxígeno | Pas de bouteille d'oxygène | Inventario |
| inventory.empty_oxygen | Empty oxygen bottle | Botella de oxígeno vacía | Bouteille d'oxygène vide | Inventario |
| inventory.no_injector | No injector | Sin inyector | Pas d'injecteur | Inventario |
| inventory.empty_injector | Empty injector | Inyector vacío | Injecteur vide | Inventario |
| inventory.no_cards | No cards | Sin tarjetas | Pas de cartes | Inventario |
| inventory.no_mine | No mine | Sin mina | Pas de mine | Inventario |
| inventory.mine | Mine | Mina | Mine | Inventario |

## Estado de integración de interfaz (Fase 2, 2026-11-24)

La tabla C++ `include/extremis/ui/localization.h` deriva las 101 entradas de menús, HUD e interacción de este documento: las 96 de la fase anterior, tres avisos de aspecto detectados en `port_menu_view.cpp` y dos rótulos de transferencia detectados en `interaction_animation.cpp`. El total del catálogo es ahora **130 IDs con columnas ES y FR**; 29 corresponden a guiones originales/cinemáticas o etiquetas originales reservadas, que esta fase no activa. La traducción de componentes usa el mismo idioma de `UserSettings`, persistido en `settings.ini` como 0=francés, 1=inglés, 2=español. Las traducciones dinámicas mantienen sus valores de tarjeta, zona, porcentaje y tiempo.

No se promueven a texto verificado las propuestas de `comm.page.01`, `shuttle.script.02` («FADDING») ni `shuttle.script.04` («ARTERIAL USAGE»). Los tres pertenecen a cinemáticas de la fase posterior; el reproductor conserva sus buffers originales del EXE. La duda editorial no cambia ningún texto activo de gameplay.

La cobertura tipográfica de la interfaz usa las letras originales de CASQUE/INVENT y TYPO como base, con marcas de acento/tilde/cedilla y signos invertidos añadidos al dibujo; no cambia archivos DOS. El mapa UTF-8 cubre los caracteres presentes en las 101 traducciones, incluidos Á/É/Í/Ó, À/È, é/è/í/ó, Ñ/ñ, Ç/ç, ¿/¡ y el apóstrofo tipográfico. Las traducciones largas se ajustan horizontalmente en menús y filas de inventario, sin omitir palabras. **Pendiente de revisión visual:** en paneles de 3×8 y filas de 176 píxeles la compresión puede reducir legibilidad. Los rótulos pintados dentro de sprites originales permanecen en su idioma original y requieren overlays medidos en otra fase; no se atribuye cobertura total a esas superficies gráficas.

## Estado de cinemáticas (Fase 3, 2026-11-24)

Esta sección actualiza los estados históricos anteriores; el texto inglés continúa leyéndose de los recursos originales. De los **130 IDs** contabilizados al cierre de Fase 2, **127** tienen ahora comportamiento EN/ES/FR: 101 de interfaz y 26 de las 29 reservas cinematográficas. Las tres excepciones son `comm.page.01`, `shuttle.script.02` y `shuttle.script.04`: la equivalencia de los pasajes es dudosa y las propuestas ES/FR de la tabla anterior **no están activas ni verificadas**. Se conserva el texto EXE en los tres idiomas. `ending.fin` está integrado por identidad: «FIN» ya funciona en los tres idiomas y permanece rasterizado.

La inspección visual de los originales reveló **cinco rótulos de crédito adicionales** que no formaban parte del recuento de 130 IDs de la Fase 2. Tienen IDs estables en el mismo catálogo; el total ampliado es **135 IDs, 132 resueltos**. Los nombres propios de los autores permanecen en los VGA. El francés original se conserva en EN y FR; el overlay ES se compone en ejecución, sin alterar DATA.

| ID | Original exacto visible | ES | FR | Recurso / área 320×200 | Estado |
|---|---|---|---|---|---|
| intro.presente | Présente | PRESENTA | Présente | `PRESENTE.VGA`, letras blancas x108..208, y83..102 | Overlay ES activo; EN/FR original |
| intro.who.01 | PROGRAMMATION | PROGRAMACIÓN | PROGRAMMATION | `WHO1.VGA`, letras x86..237, y77..90 | Overlay ES activo |
| intro.who.02 | GRAPHISME | GRÁFICOS | GRAPHISME | `WHO2.VGA`, letras x113..209, y77..90 | Overlay ES activo |
| intro.who.03 | TRAVAIL ADDITIONNEL | TRABAJO ADICIONAL | TRAVAIL ADDITIONNEL | `WHO3.VGA`, letras x60..259, y78..91 | Overlay ES activo |
| intro.who.04 | BANDE SONORE | BANDA SONORA | BANDE SONORE | `WHO4.VGA`, letras x94..228, y77..90 | Overlay ES activo |

En `include/extremis/ui/localization.h`, `cinematicById` aporta las traducciones ES/FR ya catalogadas; devuelve vacío para inglés y para los tres casos dudosos. PSEQ3/PSEQ6, comunicación, shuttle, embarque y GAME OVER conservan su fuente EXE original, número de páginas, reloj, señales de sonido y transiciones. El dibujo localizado usa los sprites originales y escala el progreso visible al número de caracteres originales; sólo recompone las líneas dentro del área existente. `œ/Œ` se compone con dos glifos originales, además de los acentos y signos invertidos ya admitidos. El test de límites usa los `.SPR` originales y comprueba que no se sale de las áreas asignadas.

## Corrección de modales y glifos (2026-11-24, port-tested)

El catálogo activo pasa de 101 a 117 entradas. Se añaden `inventory.category_battery`,
`inventory.category_oxygen`, `inventory.category_injectors`, `inventory.category_cards`,
`inventory.category_bomb`; `modal.take_question`, `modal.yes`, `modal.no`,
`modal.condition`, `modal.ammo`, `modal.number`; y `modal.supply_battery`,
`modal.supply_oxygen`, `modal.supply_energy`, `modal.supply_card`,
`modal.supply_bomb`. Las tres columnas exactas están en `localization.h`.

Las etiquetas inglesas del inventario y las preguntas de recogida proceden de
`INVENT_A.SPR`, `CH_OBJ4A.SPR` y `CHARME4A.SPR`. El port compone las traducciones
sólo sobre las regiones de letras en ES/FR; EN conserva los sprites originales.
Las tarjetas abiertas sobre la base del inventario reciben el mismo tratamiento.
Los nombres propios de armas (`Bolter`, `LG`, etc.) se conservan.

La ruta UTF-8 del port ya decodificaba por carácter, pero `á/Á` no tenía caso y
`ô` se forzaba a `O` mayúscula. Además, el mapa de `TYPO_3X5.BIN` no tiene
minúsculas ni signos `?`/`!`, por lo que las letras acentuadas minúsculas y los
signos invertidos desaparecían en paneles. El port usa la forma mayúscula del
mismo glifo de 3×5 cuando falta la minúscula y compone la puntuación ausente con
píxeles del mismo tamaño. Las pruebas cubren las cadenas UTF-8 reales del
catálogo y la composición con los recursos originales. La inspección visual se
ha hecho en capturas 320×200 de inventario, suministros y armas en EN/ES/FR;
queda pendiente confirmar esas mismas pantallas durante una partida interactiva.
