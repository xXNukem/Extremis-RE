# Control del jugador

Actualización: [disparo del arma equipada — fase 1](equipped-weapon-fire.md).
Las menciones anteriores a ausencia de disparo quedan sustituidas por ese informe.

## Controles vigentes (Paso 201)

WASD mueve y navega; E interactúa/confirma; Z cura; X usa O2; mantener Shift corre.
I abre/cierra inventario, ESC cierra modal, Tab alterna mapa/3D. G/H/J ya no
controlan interacción/consumibles. Flechas/Espacio siguen como alternativas modales.
La carrera usa factor2 sobre marcha300, diagonales normalizadas y O2 doble al
intentar desplazamiento con Shift. No cambia giro ni sensibilidad. La aceleración
DOS no se ha reproducido: véase [evidencia y limitaciones del Paso201](step-201-interfaces-and-controls.md).

La entrada discreta vive en ItemSession y el movimiento continuo en PlayerInput;
las referencias históricas de este documento a E en PlayerInput han quedado
sustituidas. Un cambio de contexto exige liberar las teclas de movimiento mantenidas.

La posición inicial del nivel 1 procede de la plantilla DOS 0x1d8a0:
sector 0, local (300,320), ángulo180/720. GAME.LAB aporta el origen(3200,0):
**X=3500, Y=320, ángulo=1.5708 rad**. Sustituye la búsqueda provisional de
(3500,900). No cambia altura(128), FOV ni proyección. Véase la
[auditoría de interfaces](interface-audit-2026-11-02.md) para evidencia y pruebas.
I abre el inventario original; G/E/Tab conservan sus funciones.

## Organización

- `camera.h` contiene la estructura compartida, extraída de `first_person_view.h`
  sin cambiar sus campos ni valores iniciales.
- `player_control.h/.cpp`: entrada abstracta y cálculo plano sin SDL ni conocimiento del cargador. Ahora puede entregar el desplazamiento
  al módulo de colisiones estáticas independiente. Normaliza el vector local antes de convertirlo a coordenadas globales.
  Avance = `(cos(a), sin(a))`; derecha = `(sin(a), -cos(a))`, coherente con el
  eje Y invertido del mapa y la proyección existente. Ratón hacia la derecha
  reduce el ángulo; flecha izquierda lo aumenta.
- `player_input.h/.cpp`: consulta el teclado continuo y el desplazamiento relativo
  SDL3 una vez por fotograma. La clase posee la captura de una ventana y la libera
  antes de destruirla. Entrega la geometría estática al control; no calcula contactos ni dibuja.
- `player_debug.h/.cpp`: muestra X, Y y ángulo en radianes con el texto de
  depuración de SDL3. El mapa admite un Camera opcional y dibuja una flecha blanca
  con la misma transformación que el resto de la geometría. No cambia el encuadre
  para seguir al jugador; fuera del mapa el marcador puede salir de pantalla.
- `main.cpp` solo entrega eventos y tiempo al módulo de entrada y sigue actualizando
  las puertas con su tiempo real. El renderizador no consulta teclas ni ratón.

## Parámetros provisionales

Centralizados en `player::Settings`, en `include/player_control.h`:

| Parámetro | Valor |
| --- | --- |
| Desplazamiento | 300 unidades del mapa / segundo |
| Giro con flechas | 1.5 radianes / segundo, mismo valor anterior |
| Ratón horizontal | 0.003 radianes / unidad relativa SDL |

No son velocidades recuperadas de EXTREMIS.EXE. Esta tarea no investiga nuevas
rutinas originales de movimiento. No se añade movimiento vertical ni inclinación.
El teclado usa segundos transcurridos, sin limitar el tiempo según FPS; el ratón
usa desplazamiento acumulado y **no** se multiplica por delta time. El movimiento
se calcula con el ángulo actualizado en ese fotograma. No hay aceleración ni inercia.

## Captura y controles

La captura relativa se activa únicamente con cámara disponible, vista 3D y foco
de teclado en la ventana. Se libera en 2D o al perder foco. Los cambios de foco o
TAB descartan el desplazamiento acumulado y el fotograma de transición para
impedir saltos de orientación o aplicar tiempo pasado sin foco. También se drena
el movimiento mientras el control está inactivo. Se comprueba el resultado de
`SDL_SetWindowRelativeMouseMode`; un error se registra y termina limpiamente.
Se usaron las declaraciones y documentación de SDL3 instaladas (`SDL_mouse.h`)
para comprobar el vaciado de deltas y la API relativa por ventana.

- TAB: alternar mapa 2D / primera persona.
- W/S: avanzar / retroceder.
- A/D: desplazamiento lateral izquierdo / derecho.
- Ratón: giro horizontal en 3D con foco.
- Flechas izquierda/derecha: giro existente.
- E física: interactuar con una puerta frontal cercana. La acción original configurable
  SHOOT or OPEN se conserva como evidencia histórica, no como asignación del port.
- O: abrir/cerrar todas las puertas como depuración, omitiendo acceso pero respetando el cuerpo.
- F: activar/desactivar conjuntamente texturas de suelo y techo; véase
  [la investigación de planos](floor-ceiling-rendering.md).
- P: contornos de polígonos.
- N/M: previsualización de materiales existente.
- Escape: salir.

El movimiento solo está activo en 3D. Las paredes estáticas bloquean y permiten
deslizamiento; las puertas móviles también bloquean según su apertura actual.
Véase [la investigación e implementación de colisiones](player-collision.md).

## Verificación

Compilación C++17/SDL3 correcta. Pasan cinco suites CTest, incluida la nueva
`player_movement`, y siete pruebas Python de extracción. Las pruebas de movimiento
se ejecutan sin iniciar ni enlazar SDL: verifican cuatro direcciones con varias
orientaciones, reposo, diagonales normalizadas, pasos de tiempo uniformes y
variables, giro con flechas y ratón independiente de delta time. Las pruebas
existentes de puertas, renderizado, materiales y carga continúan pasando.

Se ejecutó la aplicación con GAME.LAB y DALLE/PAL originales del nivel 1,
conservando las 15 puertas y la cámara inicial.

Pendiente de validación manual en la ventana: fluidez y sensibilidad del ratón,
WASD y diagonales, captura/liberación al alternar TAB, pérdida/recuperación de
foco sin saltos, flecha del mapa y texto de posición, y O durante el movimiento.

Las pulsaciones de E muestran temporalmente trazas de detección, candidata y
resultado. El cambio de captura a 3D conserva la petición hasta el siguiente
fotograma estable; 2D o falta de foco la rechazan explícitamente. Las restricciones
de acceso y O como depuración global permanecen separadas de la interacción normal.
