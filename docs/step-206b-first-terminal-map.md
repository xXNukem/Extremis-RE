# Paso 206B — panel inicial y permiso del mapa

El desbloqueo es intencionado: el panel inicial es un **terminal de transferencia
de planos**, no una taquilla de suministros vacía. No se modifica código ni
ninguna mecánica. Revisión acotada de los datos locales y del EXE, apoyada en
[Paso 205](step-205-original-functionality-audit.md) y
[Paso 206](step-206-original-map.md); no se repite la auditoría completa.

## Identificación original

Índices desde cero: nivel 1 (zona 0, C200), **sector 1 / pared 6**.
GAME.LAB: nivel en `0x7e`, sector en `0x370`, origen `(3200,400)`;
registro de pared en **`0x402`**, bytes `05 00 06 00 06 36 00 00`:
vértices 5→6, tipo **`0x3606`**, extra **`0x0000`**.
Segmento global `(3200,1000)→(3200,1200)`; punto de interacción probado:
`(3250,1100)`, mirando −X. La familia `0x3600` es el terminal de plano;
`0x3200` identifica suministros. El extra cero no es un objeto que recoger
ni el sentinel `0xffff` de un compartimento vacío.

El código de zona está en `0x2ca` (cabecera +0x0e → offset relativo `0x024c`):
words `90,2,0,1`; IJEKAH satisface el checksum 90. El lector de códigos
8/7 (`0x3508`, C001) es otro terminal y no concede por sí mismo el plano.

## Causa y contraste con DOS

El despacho EXE `0xa608..0xa613` comprueba el tipo lógico 6 y llama a
`0x19a4f`. Esta ruta valida el código (`0x19f5e..0x19f67`) y presenta la
transferencia B_PLAN3/TRANSFER. Inicializa el contador a 100 en `0x1a1bd`;
el bucle `0x1a1f4..0x1a210` espera a cero. Después, `0x1a217..0x1a225`
escribe **0xff en byte[0x601b + word[0x51f0]]**, el permiso de esa zona.
No es una operación de inventario.

En el port, `items::World` clasifica `0x3600` como `codePanel`.
`ItemSession::open` abre el código si la zona no está autorizada;
`confirmCode → showContents` selecciona `Screen::Plan`.
La única asignación positiva a `plans_` es `ItemSession::update`:
reduce `transfer` en pasos de 4 y, al observar cero, ejecuta
`plans_[zone_]=true` y cierra el modal. `planTransferred()` consulta ese mismo
permiso. Abrir el panel, aceptar el código o recoger un suministro no lo
concede; cancelar antes de completar tampoco. No hay un efecto secundario
de «taquilla vacía». En una sesión nueva no basta con abrir y cerrar el panel:
hay que validar el código y completar TRANSFER. La rapidez de la presentación
puede diferir de DOS: el ritmo por iteración ya está documentado en Paso 206.

El acceso DOS `0x6fbc..0x6ff1` comprueba primero el bloqueo `[0x5200]`
(mensaje en archivo `0x21972`: `ATTENTION: AN ALIEN IS NEAR!!`) y después
el byte de plano de la zona. Si este es cero, la cadena exacta en `0x21990` es:

```text
ATTENTION:\r\n  ACCESS NOT POSSIBLE WITH CARD!!
```

Aquí `\r\n` representa los bytes de retorno de carro y salto de línea.
El texto dice literalmente **WITH CARD**, no «Impossible without card».
La condición no consulta tarjetas ni batería. Con byte distinto de cero y sin
bloqueo llama al mapa en `0x6ffc`. El port conserva esta condición de permiso.
Esto confirma la causa del desbloqueo, sin afirmar equivalencia completa del
bloqueo por enemigos: la aproximación de sectores visibles ya consta en Paso 206.

## Verificación

Lectura directa de GAME.LAB y desensamblado acotado del EXE local; sin nueva
sesión interactiva DOS. SHA256 EXE:
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
SHA256 GAME.LAB:
`acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`.

Compilación correcta de `InExtremisPort`, `interaction_tests` y
`original_map_tests`. CTest limitado a `original_interactions` y `original_map`:
**2/2 correctas**. También pasó:

```sh
./build/original_map_tests build/map-test-data /Users/kurlos/Desktop/In_Extremis/DATA
```

Esta prueba usa GAME.LAB real, introduce IJEKAH mediante eventos, comprueba
que validar el código todavía no concede el plano, completa la transferencia
y abre el mapa. Las suites comprueban además inicio sin permiso, ausencia de
objeto entregado, permisos separados por zona y reinicio de partida.
Logs: `/tmp/step206b-build.log`, `/tmp/step206b-originals.log`.
Único cambio de esta investigación: este documento; sin commits ni pushes.
