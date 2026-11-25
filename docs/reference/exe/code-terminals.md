# Terminales de códigos y progresión de los 30 niveles

Auditoría de 2026-11-24. **El nivel 2 sí contiene una terminal de consulta.** Está en S9 W16 y requiere C002. La conexión 13 impide reabrir la puerta desde S13 una vez cerrada. Con la tarjeta de ascensor E001 se puede continuar al nivel 3, donde hay otra copia del código que se podía consultar antes de cruzar.

## Inventario

**DATA-confirmed.** Lectura de las 30 zonas de `GAME.LAB` (SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`) y de las tablas en `EXTREMIS.EXE` (SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`). Los niveles se numeran desde 1. Los sectores, paredes, conexiones y ascensores conservan sus índices desde 0. Las coordenadas son el centro de la cara en el mundo. Los offsets son absolutos en el archivo indicado.

Cada fila da la contraseña de **zona** que aceptan los teclados del nivel. La columna LAB señala la palabra de control apuntada por la cabecera de zona `+0x0e`. Los destinos enumeran las páginas que se pueden consultar, en orden. Un guion en consulta significa que no existe una pared lectora de esa clase. Las dos columnas numéricas cuentan, en el orden indicado, los dispositivos que piden código y los otros dispositivos de tarjeta. La última columna identifica conexiones inicialmente bloqueadas por tarjeta y los cortes de retorno explicados más abajo.

El inventario adjunto identifica las **306 paredes interactivas** individualmente, con coordenadas, sector, pared, offset, tipo, parámetro, credencial y, para los lectores, destino. Incluye los armarios que entregan tarjetas. [Inventario completo por dispositivo (CSV)](campaign-panels.csv)

| Nivel | Zona y código de acceso | LAB | Consulta, posición y tarjeta | Nivel y código mostrados | Armas / suministros / mapa | Ascensores / puertas / seguridades | Conexiones |
|---:|---|---|---|---|---|---|---|
| 1 | C200 `IJEKAH` | `0x2ca` | S8 W7 (3000,2500) C001 | 1 `IJEKAH`, 2 `JBHGAD` | 1 / 3 / 1 | 2 / 1 / 0 | 7 |
| 2 | B101 `JBHGAD` | `0x1992` | S9 W16 (2100,400) C002 | 2 `JBHGAD`, 3 `CJBAIK` | 1 / 2 / 1 | 2 / 1 / 0 | 3 · corte 13 |
| 3 | A002 `CJBAIK` | `0x37f0` | S18 W14 (1400,900) C002 | 3 `CJBAIK`, 4 `FIJGAD` | 1 / 2 / 1 | 2 / 0 / 0 | — |
| 4 | B103 `FIJGAD` | `0x5a06` | S7 W31 (1500,1600) C002 | 4 `FIJGAD`, 5 `KADBEI` | 0 / 2 / 1 | 2 / 0 / 0 | — |
| 5 | C204 `KADBEI` | `0x6986` | S3 W11 (1400,3300) C002 | 5 `KADBEI`, 6 `JDKCHB` | 1 / 2 / 1 | 1 / 1 / 0 | 1 |
| 6 | D305 `JDKCHB` | `0x848a` | S18 W3 (1300,1400) C003 | 6 `JDKCHB`, 13 `ABFEKI` | 0 / 0 / 1 | 3 / 0 / 0 | — |
| 7 | E406 `JDEKBA` | `0xace8` | S2 W7 (2900,3000) C003 | 7 `JDEKBA`, 14 `AIDCJF` | 1 / 2 / 1 | 1 / 0 / 0 | — |
| 8 | C207 `CHIGAK` | `0xcc0c` | S4 W26 (800,3500) C003 | 8 `CHIGAK`, 15 `EFGHJC` | 1 / 2 / 1 | 1 / 0 / 0 | — |
| 9 | B108 `HDGJAE` | `0xe4c2` | S4 W15 (1700,3000) C003 | 9 `HDGJAE`, 10 `IBGJAF` | 0 / 1 / 1 | 2 / 0 / 0 | — |
| 10 | A009 `IBGJAF` | `0x104ae` | S3 W6 (700,2800) C003 | 10 `IBGJAF`, 12 `JEAGHI`, 14 `AIDCJF`, 2 `JBHGAD` | 1 / 2 / 1 | 2 / 0 / 0 | — |
| 11 | C210 `AFBJDH` | `0x13040` | S2 W19 (3100,600) C003 | 21 `KCFGIB`, 22 `HEAJDF`, 11 `AFBJDH` | 1 / 2 / 1 | 1 / 0 / 0 | — |
| 12 | B111 `JEAGHI` | `0x145dc` | S3 W10 (3000,300) C003 | 12 `JEAGHI`, 19 `AEDKGB`, 15 `EFGHJC` | 1 / 1 / 1 | 2 / 0 / 0 | — |
| 13 | A012 `ABFEKI` | `0x18734` | S3 W15 (5300,3600) C003 | 13 `ABFEKI`, 20 `KFGCJD`, 17 `EIABDC` | 2 / 5 / 1 | 2 / 1 / 0 | 30 |
| 14 | B113 `AIDCJF` | `0x1b7a6` | S4 W16 (500,1200) C004 | 14 `AIDCJF`, 19 `AEDKGB`, 13 `ABFEKI` | 1 / 1 / 1 | 3 / 0 / 0 | — |
| 15 | C214 `EFGHJC` | `0x1d67e` | S4 W7 (2000,2900) C004 | 15 `EFGHJC`, 20 `KFGCJD`, 3 `CJBAIK` | 1 / 3 / 1 | 3 / 0 / 0 | — |
| 16 | D315 `IJHGBA` | `0x1f81c` | S13 W4 (1700,3400) C005 | 16 `IJHGBA`, 18 `JFBADG` | 3 / 4 / 1 | 2 / 1 / 0 | 12 |
| 17 | E416 `EIABDC` | `0x225e2` | S4 W14 (3800,1100) C005 | 17 `EIABDC`, 21 `KCFGIB` | 1 / 3 / 1 | 3 / 0 / 0 | — |
| 18 | D317 `JFBADG` | `0x23f04` | S3 W18 (1100,600) C006 | 18 `JFBADG`, 19 `AEDKGB`, 22 `HEAJDF` | 1 / 4 / 1 | 2 / 0 / 0 | — |
| 19 | F518 `AEDKGB` | `0x25dd0` | S15 W14 (1900,1200) C006 | 19 `AEDKGB`, 20 `KFGCJD` | 1 / 5 / 1 | 2 / 1 / 0 | 11 |
| 20 | H719 `KFGCJD` | `0x2776a` | — | — | 2 / 4 / 0 | 1 / 0 / 0 | — |
| 21 | G620 `KCFGIB` | `0x27dde` | S9 W2 (4300,800) C006 | 21 `KCFGIB`, 22 `HEAJDF`, 23 `GDEAJF` | 3 / 10 / 1 | 2 / 2 / 0 | 6, 19 |
| 22 | H721 `HEAJDF` | `0x2a334` | — | — | 3 / 18 / 1 | 1 / 3 / 16 | 10, 17, 43 · corte 1 |
| 23 | I822 `GDEAJF` | `0x2ef9c` | S5 W10 (1900,1800) C011 | 23 `GDEAJF`, 24 `HEFDKB`, 26 `CDAJKE` | 1 / 5 / 1 | 3 / 1 / 0 | 18 |
| 24 | G623 `HEFDKB` | `0x33cc6` | S7 W4 (2800,2700) C012 | 25 `JGCDIB` | 0 / 2 / 1 | 1 / 0 / 0 | — · sin pareja 9 |
| 25 | H724 `JGCDIB` | `0x34e3c` | S9 W2 (1800,2500) C013 | 23 `GDEAJF` | 1 / 1 / 1 | 1 / 1 / 0 | 11 |
| 26 | H725 `CDAJKE` | `0x360f6` | S10 W6 (400,900) C013 | 27 `EFIBAG` | 0 / 3 / 1 | 2 / 0 / 0 | — |
| 27 | F526 `EFIBAG` | `0x37dd6` | S25 W7 (4500,3200) C014 | 28 `BHAJKG` | 2 / 6 / 1 | 1 / 0 / 0 | — |
| 28 | I827 `BHAJKG` | `0x3a9ea` | S3 W14 (2900,3800) C014 | 29 `HJKDCF`, 30 `FKIEJG` | 0 / 2 / 1 | 2 / 1 / 0 | 21 |
| 29 | F528 `HJKDCF` | `0x3d74c` | — | — | 1 / 1 / 1 | 2 / 0 / 0 | — |
| 30 | G629 `FKIEJG` | `0x3e638` | — | — | 12 / 24 / 1 | 1 / 0 / 0 | — |

## Qué activa cada dispositivo

- `0x3104` (44 armeros), `0x3205` (122 armarios) y `0x3606` (29 mapas) usan el teclado de la zona hasta su autorización. El panel de mapa transfiere el plano. En el port, el campo interno `codePanel` designa ese panel de mapa.
- `0x3508` (26 consultas) usa una tarjeta C cuyo número figura en el byte bajo del parámetro. Cada lector presenta la lista de su zona. Los niveles 20, 22, 29 y 30 tienen lista en el LAB y carecen de lector. Entre los otros 26 lectores se mencionan los 30 códigos. La tabla describe disponibilidad física y requisitos locales. La obtención ordenada de todas las tarjetas con tres huecos de inventario requiere validación en partida.
- `0x3308` (14 lectores de puerta) requiere la C del byte alto y desbloquea la conexión del byte bajo. `0x3422` (16 seguridades del nivel 22) requiere la N del byte alto y marca la seguridad numerada por el byte bajo. `0x3002` (55 mandos de ascensor) usa tarjeta E y permisos por destino. El parámetro identifica el descriptor de ascensor. En el CSV, `E / route` remite a ese control de permisos. [Ascensores y rutas](elevator-cinematics.md)
- Las contraseñas de **continuación** tienen ocho caracteres. El DOS divulga 16 en llegadas elegibles y cada una selecciona una plantilla fija al continuar. Los lectores de la tabla muestran códigos de **seis letras** A–L. Para el teclado, DOS compara la suma ponderada de los valores 0–11 con pesos 1, 2, 4, 8, 16 y 32, reducida a un byte. Los 30 valores calculados a partir de las letras del EXE coinciden con sus palabras en el LAB. [Continuación y contraseñas](password-continuation.md)

**EXE-confirmed, comprobación estática.** El despacho de clase 5 en `0xa5fb–0xa601` llama a `0x9ace`. El índice arranca a cero en `0x9b66`. La comparación de tarjeta se sigue por `0x9b7e–0x9b8c`, `0x9c66–0x9c6e` y `0x9d85–0x9d8d`. El lector toma la base de `[0x519a]` y obtiene el destino de `base+4+2*índice` en `0x9e34–0x9e48`. En `0x9e4a–0x9e79` usa ese destino para buscar el nombre de cuatro bytes (archivo `0x60ef+4*destino`) y las seis letras (archivo `0x6167+6*destino`, valores desde A=0). En `0x9f7a–0x9f9e` una opción sale, la otra incrementa el índice y vuelve a cero al alcanzar el contador de `base+2`. `ItemSession::showCode` y `confirm` siguen ese orden con los datos originales.

Dos listas merecen atención. En el nivel 10, `0x104ae` contiene suma 10 y cuenta 4. Su cuarto índice, 1 en `0x104b8`, también es la cuenta de generadores. La cuarta página que resulta es B101. En el nivel 11, `0x13040` contiene suma 102 y cuenta 3. Los índices son 20, 21 y 10. La palabra 15 de `0x1304a` queda fuera de la lista. Se conserva en la tabla lo que leería el DOS. Se desconoce la intención del autor de esos dos registros.

## Recorrido del nivel 2

1. En el nivel 1, el lector S8 W7, con C001, permite pasar de C200 a **B101 `JBHGAD`**. Tras llegar al nivel 2 por el ascensor 1 en S0, se puede caminar S0 → S1 → S2. El armario S2 W25, (2200,3100), offset LAB `0x1e3e`, entrega **C002** tras introducir el código B101.
2. En S1 W32, (2700,2000), offset `0x1c26`, el parámetro `0x0203` exige C002 y desbloquea la conexión 3 hacia S4. La tarjeta se conserva. Desde S4 se llega a S9. La consulta S9 W16, (2100,400), offset `0x2a9c`, tipo `0x3508`, parámetro 2, acepta la misma tarjeta. La primera página repite B101 y la siguiente muestra **A002 `CJBAIK`** para el nivel 3. La lista LAB en `0x1992` contiene las palabras 183, 2, 1, 2.
3. Dos caras explican síntomas diferentes. En la conexión 8, S8 W0 (`0x2764`, tipo `0x1809`) impide solicitar apertura hacia S9. S9 W2 (`0x2a2c`, `0x102b`) sí la solicita. Existe el rodeo S8 → S7 → S6 → S5 → S4 → S9.
4. El camino al ascensor sigue S9 → S10 → S12 → S13 → S14. En la conexión 13, S12 W1 (`0x30d2`, `0x1035`) abre hacia S13, pero S13 W0 (`0x325c`, `0x1809`) no puede reabrir hacia S12. El umbral está entre (400,1700) y (600,1700). **Después del cierre, S13 y S14 quedan aislados de los sectores anteriores por los caminos internos del nivel.** Mientras el estado todavía permite cruzar, se puede volver. Falta la ubicación de la incidencia observada para identificar cuál de las dos puertas era.
5. El ascensor 2, S14 W6, (500,3400), offset `0x33c0`, admite E001 hacia S0 del nivel 3. En ese nivel las puertas parten sin bloqueo de tarjeta. Con C002, la consulta de S18 W14 vuelve a dar A002 y luego B103. Una ruta es S0 → S1 → S4 → S5 → S6 → S9 → S10 → S11 → S12 → S13 → S15 → S16 → S18. Por tanto, con las tarjetas del recorrido se puede avanzar aunque se haya omitido la consulta de S9. La llegada con otro inventario, un intercambio de tarjetas o una partida continuada requieren considerar su estado propio.

**EXE-confirmed / Port-tested.** El bit de cara `0x0800` en `0x109a8–0x109b1` salta la solicitud hasta `0x10a4a`. El umbral de paso sigue evaluándose en `0x10a53–0x10a64`. En macOS Debug se probó `DoorControl::constrainPlayer` en el centro de las seis caras de las conexiones 3, 8 y 13. En aproximación de normal 183 a 171, la 3 conservó `0x0400` hasta aceptar C002 y después solicitó apertura desde ambos lados. La 8 sólo escribió `0x0100` desde S9 y la 13 sólo desde S12. Partiendo de `0x013d`, el cruce de normal +20 a −10 entró en el vecino correcto y dejó `0x0140` en los seis casos. Pendiente observación directa en DOS. [Rutina de puertas y orientación](doors.md)

## Otros límites de progresión y correspondencia con el port

- **Grafo de sectores.** Para cada zona se enlazó cada cara de conexión con su vecino, se comprobó su geometría inversa y se buscó un camino de retorno admitiendo apertura desde las caras sin `0x0800`. Con las cerraduras de tarjeta ya desbloqueadas y las puertas inicialmente cerradas, los cortes sin rodeo interno son la conexión 13 del nivel 2 y la conexión 1 del nivel 22 (apertura S1 → S2). La tabla identifica también los bloqueos iniciales por tarjeta. El análisis no simula temporización, enemigos, geometría caminable dentro del sector, ascensores ni los eventos especiales de la nave. [Secuencia del nivel 22](level22-communication.md)
- **Nivel 24.** Conexión 9, S1 W1 en `0x33e3e`, tipo `0x1809`, une los vértices (2100,2200) y (2100,2000). La lista nombra S6. En S6 los dos vecinos son S7 y S3. Falta la cara inversa. `doors::closedDoors` exige dos caras opuestas con el mismo índice y omite este par incompleto. El grafo conserva rutas alternativas. Su efecto sobre el paso en DOS necesita una prueba dinámica.
- **Autorización al volver (EXE-confirmed).** Los teclados consultan y escriben `[0x6039+zona]` en `0x18642–0x18653` y `0x18991–0x189a1`. La inicialización común de nueva partida y continuación borra todos los indicadores en `0x6b3d–0x6b4b`. C200 empieza sin autorizar. El origen de la primera pista antes de obtener C001 sigue abierto.
- **Port-tested.** Antes, `ItemSession::load` borraba la autorización al cambiar de zona. Ahora `ItemSession` conserva un indicador por zona durante la partida y lo activa al terminar un `CodeResult` aceptado. La regresión de `interaction_tests` reproduce el antiguo fallo al regresar a C200 y comprueba la independencia de las zonas. Con el `GAME.LAB` de referencia se comprobó en Debug el viaje de ida y vuelta por el ascensor entre los niveles 4 y 5, la llegada de la lanzadera del 22 al 23 y el reinicio de las autorizaciones al continuar con `ZENDOR56` y al elegir nueva partida.
- **Clasificación y pruebas.** Las 306 paredes `0x3xxx` del archivo de referencia encajan en las siete familias del port. Se ejecutó `interaction_tests --lab GAME.LAB` en macOS con los datos originales. Recorrió 251 dispositivos distintos de ascensores, abordándolos desde posiciones locales de prueba, y transfirió 28 tarjetas. La prueba comprueba el acceso a los lectores con su tarjeta y el checksum de los teclados. La evidencia de destinos y páginas procede del cruce de datos y del bucle del EXE. La auditoría no es una partida completa en DOS ni en el port.
