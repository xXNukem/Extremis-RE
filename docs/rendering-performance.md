# Rendimiento actual y propuesta de migración a SDL_GPU

## Conclusión y alcance

La caída con F tiene un cuello principal medido en CPU: generar y muestrear los
planos para todos los píxeles. La proyección por filas es barata; no domina la
subida de textura. SDL_Renderer ya utiliza **Metal** para mostrar el framebuffer
en este equipo, pero sigue recibiendo una imagen calculada por CPU. Cambiar solo
la API de presentación no elimina ese trabajo.

Esta tarea añade únicamente una herramienta de diagnóstico, su target opcional
y documentación/resultados. No cambia el renderizador, los controles, las puertas,
las colisiones ni el formato GAME.LAB. No se implementa un backend GPU.

## Método reproducible

Equipo: Apple M5, arm64, 10 núcleos lógicos, 16 GiB RAM. AppleClang
21.0.0.21000401; SDL compilado/ejecutado 3.4.18; Cocoa/Metal. SDL informa modo de
pantalla de 60 Hz y `SDL_GetRenderVSync=0`. No se ha medido Windows ni Linux.

Compilación **Release**, `-O3 -DNDEBUG -std=c++17`, sin LTO. El build anterior en
`build/` tiene `CMAKE_BUILD_TYPE` vacío y no activa optimización en sus flags; no
se usa para atribuir los tiempos de esta tabla. No se cuantifica aquí una mejora
Debug→Release mediante una comparación directa.

Se carga el nivel 1 y sus recursos originales fuera del repositorio. Cámara fija
X=3500, Y=900, ángulo=1.57079632679; 250 muros y 15 puertas. Se prueban por separado
puertas cerradas/abiertas y F off/on, en cuatro resoluciones **de píxeles reales**,
con ocho fotogramas de calentamiento y 60 muestras por caso. Se incluye el texto
de depuración que muestra la aplicación. No se miden carga de archivos, movimiento,
colisiones, elección de puerta ni animación: los estados son fijos para repetir
exactamente la escena.

Tres ejecuciones seriales:

1. `software`: SDL_CreateSoftwareRenderer, sin ventana ni pausa; aísla el coste CPU.
2. `window`: ventana SDL_Renderer normal, backend Metal, sin pausa artificial.
3. `window ... paced`: mismo backend y `SDL_Delay(16)` real al final de cada
   fotograma de producción, igual que main. No es una suma estimada de 16 ms.

La herramienta incluye el archivo de implementación original en una **unidad de
traducción exclusiva del benchmark** para llamar a sus funciones internas sin
copiarlas ni ampliar la API pública. No vuelve a enlazar ese .cpp por separado.
La ruta por etapas separa todos los rayos de todos los muestreos verticales para
medir lotes completos, sin temporizadores por píxel/columna. Compara byte a byte
su salida con `FirstPersonRenderer::draw` en el destino software antes de medir:
las 16 combinaciones coinciden, incluidos planos y texto.

Los FPS se calculan con la ruta pública de producción completa, no con la suma de
microbenchmarks. Las etapas tienen su propio calentamiento y se miden sin pausa,
incluso en la ejecución `paced`; la columna `production_ms` de esa ejecución sí
incluye la pausa. Se conservan media, p50 y p95. No son tiempos de ejecución GPU:
las llamadas SDL pueden encolar o esperar trabajo de forma asíncrona. FPS de envío
no equivale a fotogramas visibles en el monitor.

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j4
cmake --build build/release --target render_benchmark -j4
mkdir -p build/render-profiling
./build/release/render_benchmark "$HOME/Desktop/In_Extremis/GAME.LAB" software 60 > build/render-profiling/software.csv 2> build/render-profiling/software.log
./build/release/render_benchmark "$HOME/Desktop/In_Extremis/GAME.LAB" window 60 > build/render-profiling/metal.csv 2> build/render-profiling/metal.log
./build/release/render_benchmark "$HOME/Desktop/In_Extremis/GAME.LAB" window 60 paced > build/render-profiling/metal-paced.csv 2> build/render-profiling/metal-paced.log
ctest --test-dir build/release --output-on-failure
python3 -m unittest discover -s tests -p test_extract_dalle.py
```

La ventana del benchmark no acepta controles de juego; cerrarla interrumpe la
medición. Recursos y CFG solo se leen. F se fija en cada caso, independientemente
del CFG, para comparar on/off. Se vuelve al programa normal para la validación de
controles. No hay shaders nuevos ni inicialización SDL_GPU en la herramienta.

## Mediciones

Valores de puertas cerradas en esta tabla; las variantes abiertas están completas
en los [CSV y logs conservados](rendering-performance-data/metal.csv). Cada celda
muestra **ms/fotograma / FPS de envío**. Tiempo de pared real, no utilización de
CPU del proceso ni FPS garantizados de pantalla.

| Resolución | Software F off | Software F on | Metal F off | Metal F on | Metal + Delay16 F off | Metal + Delay16 F on |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 640×400 | 0.55 / 1824.1 | 2.86 / 350.1 | 8.28 / 120.8 | 8.31 / 120.3 | 20.84 / 48.0 | 23.32 / 42.9 |
| 960×600 | 0.99 / 1008.1 | 6.16 / 162.4 | 8.33 / 120.1 | 8.33 / 120.1 | 23.19 / 43.1 | 23.34 / 42.9 |
| 1280×800 | 1.58 / 632.8 | 10.69 / 93.6 | 8.33 / 120.1 | 10.94 / 91.4 | 23.69 / 42.2 | 30.59 / 32.7 |
| 1920×1080 | 3.10 / 322.8 | 21.81 / 45.8 | 8.33 / 120.0 | 22.02 / 45.4 | 23.33 / 42.9 | 40.27 / 24.8 |

El coste de planos crece aproximadamente con el número de píxeles. Desglose de la
ruta por etapas, **Metal, F on**, sin pausa, puertas cerradas (ms):

| Resolución | Fondo plano | Planos | Rayos paredes+puertas | Raster vertical | Subida | Present CPU | p95 ruta pública |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 640×400 | 0.010 | 2.362 | 0.323 | 0.177 | 0.089 | 5.366 | 8.617 |
| 960×600 | 0.019 | 5.125 | 0.482 | 0.387 | 0.173 | 2.132 | 8.580 |
| 1280×800 | 0.032 | 9.223 | 0.643 | 0.722 | 0.304 | 0.022 | 10.996 |
| 1920×1080 | 0.067 | 18.533 | 0.954 | 1.576 | 0.593 | 0.032 | 22.141 |

Clear, encolado de SDL_RenderTexture y texto suelen quedar por debajo de 0,01 ms
cada uno en F on; véase el CSV para cada caso. No se deduce de ello su coste GPU.
A 1920×1080/F off, Present consume unos 4,48 ms en el lote sin pausa, frente a
0,032 ms con F on. La espera disminuye cuando el trabajo CPU anterior es mayor.
No se atribuye esa espera a VSync activado: SDL devuelve 0. Puede intervenir la
cola/driver/compositor; no se midieron fences ni un timeline GPU para distinguirlos.

El modo de pantalla reportado es 60 Hz, aunque el bucle sin pausa devuelve unas
120 presentaciones/s en casos ligeros. Esto **no prueba** 120 imágenes visibles/s.
La diferencia entre `paced` y sin pausa debe medirse, como se ha hecho: las esperas,
el planificador y las frecuencias de CPU cambian; sumar 16 ms sería insuficiente.

### Perspectiva, coordenadas, paredes y puertas

Los microbenchmarks del [log software](rendering-performance-data/software.log)
llaman a las funciones reales y consumen sus resultados. En la cámara fija:

| Operación aislada | 960×600 (ms) | 1920×1080 (ms) |
| --- | ---: | ---: |
| Proyectar todas las filas | 0.0030 | 0.0053 |
| Coordenadas periódicas de todos los píxeles, sin leer RGB | 4.6528 | 16.7090 |
| Rayos contra geometría estática | 0.6292 | 1.2691 |
| Rayos contra solo las 15 puertas cerradas | 0.0494 | 0.0969 |
| Rasterizar columnas clasificadas como pared | 0.3255 | 1.3210 |
| Rasterizar columnas clasificadas como puerta | 0.0548 | 0.2144 |

A 960 hay 788 columnas de pared y 172 de puerta; a 1920, 1578 y 342.
Para aislar puertas se llama al mismo castSceneRay con un Level vacío y las
puertas reales; no se implementa otro intersector. Para separar raster se clasifica
el impacto una vez y se llama al muestreador original sobre cada conjunto.

Estas pruebas no se suman como un presupuesto exacto: cambian el patrón de caché,
el contexto de optimización y la actividad sostenida del procesador; el cálculo
de coordenadas también incluye acumulación de checksum. En particular, no se resta
el microbenchmark estático del tiempo agrupado para inventar un coste de puertas.
Sirven para mostrar órdenes de magnitud: proyectar filas es mucho más barato que
calcular coordenadas por píxel; las puertas no explican el coste de los planos.
Las medias CPU de distintos lotes pueden cambiar incluso con VSync off.

### Trabajo y asignaciones por fotograma

Hallazgos del código, contrastados con las mediciones:

- `planes::render` recorre todo el framebuffer, incluidos píxeles que luego tapan
  paredes/puertas. `plane_material::coordinates` valida parámetros, envuelve,
  escala y limita X e Y en cada píxel. Hay **dos fmod por píxel**: 1.152.000 llamadas
  a 960×600 y 4.147.200 a 1920×1080. Se midió el conjunto de coordenadas, no el
  tiempo exclusivo dentro de libm/fmod; no se atribuye todo ese coste a fmod solo.
- La perspectiva se calcula por fila, no por píxel. Repite trigonometría de cámara
  por fila, pero su coste medido es pequeño. El muestreo de color y empaquetado
  son adicionales al cálculo de coordenadas.
- Los rayos recorren muros de todos los sectores por columna y reconstruyen las
  caras de puertas por columna. No hay aceleración espacial. La complejidad se
  aproxima a ancho × segmentos; los planos, a ancho × alto.
- Se llena primero un fondo que F on vuelve a sobrescribir; luego las columnas
  pisan parte de los planos. A 1080p un framebuffer ocupa 8.294.400 bytes (~7,91 MiB).
  Solo subirlo a 60 FPS implica ~498 MB/s lógicos; en este equipo no es el mayor
  coste medido. No equivale necesariamente a tráfico PCIe (Apple Silicon es UMA).
- `FirstPersonRenderer::pixels_` y la textura streaming se recrean/ajustan únicamente
  al cambiar de tamaño. Hits y caras temporales en producción son valores de pila,
  no vectores nuevos por píxel. Las reservas del benchmark se hacen antes de medir.
- Los hooks de SDL_malloc/calloc/realloc registraron **0 llamadas durante los lotes
  calientes** en todos los casos. No cubren malloc directo de frameworks Metal,
  Objective-C ni todas las reservas internas del driver. La ausencia de churn en
  los vectores propios se constata por inspección del código; no es un perfil de
  todas las asignaciones del proceso. Redimensionado/carga quedan fuera de esta cifra.
- `main.cpp` no configura VSync y ejecuta SDL_Delay(16) **después** de presentar,
  sin descontar el tiempo que ya costó el fotograma. Hay una limitación adicional
  medida, no un objetivo ajustado de 60 FPS. No se modifica en esta tarea.

## Optimizaciones inmediatas recomendadas, aún no aplicadas

Orden propuesto y criterio de aceptación: medir cada cambio con la misma batería
y exigir comparación de píxeles contra el software actual.

1. Usar el build Release para pruebas de rendimiento. Separar explícitamente
   política de pacing/presentación de los costes CPU. En una tarea posterior,
   sustituir la suma fija de Delay por una única política configurable de límite
   o VSync; no combinar dos esperas a ciegas. Cambia la cadencia y requiere validación.
2. Calcular rayos e intervalos opacos antes de los planos y omitir los píxeles que
   se van a sobrescribir. Se puede conservar projectWall/raster actuales y comparar
   exactamente los límites de recorte. El porcentaje ahorrado depende de la vista;
   no se promete una mejora numérica sin implementar/medir.
3. Probar coordenadas incrementales por fila y wrapping especializado para 128×128,
   amortizando fmod y validaciones. Mantener negativos, bordes, horizonte y el
   ajuste numérico de costuras: una sustitución ingenua de doble por float/entero
   podría cambiar texeles. El microbenchmark da prioridad a este experimento,
   pero no demuestra su futura aceleración exacta.
4. Preparar caras de puertas y datos de cámara una vez por frame. Mantener la
   misma selección de cara/progreso/UV. Es menor prioridad que los planos.
5. Evitar el relleno plano redundante donde se garantiza cobertura texturizada;
   preservar horizonte y fallback de materiales inválidos. Su ahorro será pequeño
   frente a los ms de coordenadas, según el tiempo de fondo medido.

Renderizar internamente a menor resolución es otra opción de calidad/rendimiento,
pero cambia la imagen: no se propone como corrección transparente ni como nuevo
valor por defecto. No se han aplicado fast-math, filtros, aproximaciones de
paleta ni cambios de escala para acelerar estas mediciones.

## Propuesta concreta de SDL_GPU

Lo siguiente es **diseño propuesto**, no código implementado ni rendimiento medido
de un renderer GPU. La API está disponible en SDL3; se debe fijar una versión SDK
mínima y comprobarla en CI. La documentación oficial describe los backends y sus
formatos; SDL no traduce automáticamente un shader de un formato a todos los demás.

| Plataforma objetivo | Backend preferido | Shader distribuido |
| --- | --- | --- |
| macOS | Metal | MSL; opcional metallib generado en CI macOS |
| Windows | Direct3D 12; Vulkan como alternativa comprobada | DXIL / SPIR-V |
| Linux | Vulkan | SPIR-V |

La selección debe comprobar creación de dispositivo y disponibilidad real y dar
fallback explícito al renderer software. No prometer soporte en cualquier GPU
por el solo hecho de arrancar SDL. Referencia fijada al SDK instalado:
[SDL_gpu.h 3.4.18](https://raw.githubusercontent.com/libsdl-org/SDL/release-3.4.18/include/SDL3/SDL_gpu.h).

### Separación mínima de datos

Conservar GAME.LAB y todas las decisiones de juego como autoridad. Proponer una
lista C++17 de superficies de dibujo, derivada en memoria, sin serializar otro
formato de nivel ni sustituir sus IDs:

- Estáticas: extremos globales, altura 256 actual, ID material, intervalo U,
  modo de cara y orden estable. Construcción una vez desde paredes.
- Dinámicas: caras actuales devueltas por `doors::visibleFace`, con material y
  `textureSpan`; se actualizan después del sistema de puertas, sin otro estado
  de animación o colisión en GPU.
- Frame: Camera, tamaño de salida en píxeles, F, override N/M y referencias a los
  recursos existentes. En C++17 usar referencias a vectores o puntero+cantidad,
  no depender de std::span de C++20.

`scene_surfaces.h/.cpp` sería un módulo pequeño para esa extracción, con funciones
compartidas de materiales. `gpu_renderer.h/.cpp` poseería dispositivo, buffers,
texturas, pipelines y presentación. Un selector simple software/GPU en el borde
aplicación es suficiente; no una jerarquía de entidades/renderers. Mantener el
software disponible para generar referencia antes de adaptar su interfaz a los
descriptores compartidos. No hacer depender la física de los buffers GPU.

### Representación por tipo de superficie

**Paredes y marcos:** convertir cada segmento opaco en dos triángulos verticales,
con UV de su A→B original y altura existente. Reutilizar wallTextureIndex y el
intervalo de 30 columnas de los marcos. Depth test selecciona la superficie más
cercana, sustituyendo el raycast y el muestreo por columnas del backend GPU.
Muros normales conservan el comportamiento de ambas caras; no activar culling
indiscriminadamente. Conservar orden/desempate de superficies coincidentes.

**Portales:** las conexiones 0x1000 no generan una pared de relleno. Al principio
se dibujan las superficies de todos los sectores y se deja actuar al depth buffer,
igualando la visibilidad global actual. No introducir un nuevo sistema de portales
ni culling por sector que cambie qué se ve. Se podrá optimizar solo tras comparar.

**Puertas:** subir un pequeño lote de vértices/instancias dinámicas a partir de
visibleFace, una vez por frame, con su recorte UV y cara orientada a cámara. No
modelarlas como un sólido nuevo: las dos caras se retraen de forma opuesta y la
retención física de aproximación pertenece a DoorControl. E/O y protección al
cierre siguen en CPU sin ninguna duplicación en shader.

**Suelo/techo:** primera implementación propuesta: un triángulo de pantalla completa
que evalúa en fragment shader la misma intersección horizontal y las mismas
coordenadas mundo→SOL. Dibujarlo como fondo, sin escribir depth, antes de muros.
No necesita teselar polígonos del nivel ni calcular millones de píxeles en CPU.
Mantener horizonte, alturas, periodo 400, selección global SOL1 y F/color plano.
La perspectiva sigue en shader; no trasladar la tabla de píxeles CPU ya calculada.

**Sprites/animaciones futuras:** buffers de instancias para billboards y selección
de frame/capa por datos de animación compartidos. UV/ID de frame en lugar de subir
una imagen completa cada frame. Solo aplicar máscaras/alpha cuando se haya
identificado el comportamiento de cada recurso; negro opaco de DALLE/SOL no se
convierte en transparencia. Aún no se implementan reglas de sprites no investigadas.

### Materiales, paletas y shaders

Propuesta inicial: subir una vez los RGB ya decodificados a texturas RGBA8 UNORM,
con muestreo nearest, sin mipmaps, MSAA, corrección gamma o filtrado añadidos.
Array de capas para DALLE 60×74; dos capas de SOL 128×128. Evitar un atlas que
introduzca bleeding de bordes. El shader debe calcular el texel activo explícitamente
para respetar los intervalos de marco/puerta y la cuantización del software.

Conservar los índices originales en una fase posterior permitiría R8 + una paleta
256×1; comprobar formato/uso soportado antes de elegir UINT o UNORM y convertir
índices de forma exacta. Para ello, ampliar el decoder existente para conservar
índices/paleta junto al RGB de referencia, sin crear otro lector de DALLE/SOL.
Esto facilita futuras paletas animadas sin subir todos los texeles. No reinterpretar los recursos binarios ni cambiar ahora sus loaders.
El sombreado de muros debe usar profundidad perpendicular (view Z), no distancia
radial, y conservar la fórmula y truncado de canal;
los planos conservan su ausencia de sombreado adicional.

Mantener una fuente HLSL y una compilación offline reproducible que emita DXIL,
SPIR-V y MSL según plataforma (metallib opcional compilado con las herramientas
de Apple en CI macOS); usar SDL_shadercross como herramienta del
build/CI, con versiones fijadas. Distribuir los binarios/reflexión necesarios y
evitar depender de compiladores externos instalados en el equipo del jugador.
La CLI y las dependencias de compilación están descritas en el
[README oficial de SDL_shadercross](https://raw.githubusercontent.com/libsdl-org/SDL_shadercross/main/README.txt).
No se ha instalado ni probado ese toolchain en esta tarea.

Generar/verificar bindings, entrypoint y número de texturas, buffers y uniforms
en el paquete de shaders. No mantener tres versiones de la lógica de materiales.
El empaquetado debe respetar las convenciones de recursos por backend de
[SDL_CreateGPUShader](https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader).

### Recursos, presentación y visor 2D

Plan de vida: dispositivo y texturas una vez; buffers estáticos al cargar el nivel;
buffers dinámicos/transferencia reutilizables para puertas e instancias, con cycling
correcto para no pisar datos en vuelo. Uniform pequeño para cámara/viewport/opciones.
Un render pass principal y pocos cambios de pipeline/material; no crear recursos
por triángulo ni por fotograma. Redimensionado recrea únicamente lo necesario,
por ejemplo depth y targets de tamaño de ventana.

Una ventana debe tener un propietario claro de presentación: no mantener el
SDL_Renderer de ventana actual y un swapchain SDL_GPU competiendo por ella. El
backend GPU reclamaría la ventana y establecería explícitamente composición y
política de presentación. Al reclamarla, SDL usa SDR y VSync por defecto; no asumir
los valores del SDL_Renderer actual. Comprobar los modos soportados antes de elegir
uno. Véase [SDL_ClaimWindowForGPUDevice](https://wiki.libsdl.org/SDL3/SDL_ClaimWindowForGPUDevice).
El formato real del swapchain se consulta y puede cambiar con sus parámetros:
[SDL_GetGPUSwapchainTextureFormat](https://wiki.libsdl.org/SDL3/SDL_GetGPUSwapchainTextureFormat).

Para preservar TAB y el mapa al introducir el backend: usar inicialmente drawMap
sobre un SDL_CreateSoftwareRenderer de superficie fuera de pantalla y subir ese
resultado en modo 2D al swapchain GPU. Ese renderer no posee la ventana. Más tarde,
extraer su transformación cenital y dibujar líneas/marcadores en un lote GPU.
Conservar texto de depuración con una capa equivalente; no perder UI porque
SDL_RenderDebugTextFormat pertenecía al renderer anterior.

Centralizar la conversión de coordenadas de cámara a clip. SDL_GPU publica NDC
con Z en [0,1] y viewport/UV con origen superior izquierdo; evitar invertir Y una
segunda vez por Vulkan. El shader de plano puede usar directamente píxeles y la
base frente/derecha actual. Probar escalado HiDPI con dimensiones reales del target,
no las dimensiones lógicas de la ventana.

## Migración gradual y comparación de fidelidad

1. **Ahora:** conservar benchmark y resultados como baseline. Sin cambios gráficos.
2. **Siguiente tarea acotada:** target GPU experimental opcional, fuera de main,
   crea dispositivo/offscreen, prueba shaders/materiales y descarga imágenes para
   comparación. Sin sustituir el ejecutable funcional ni inventar prestaciones.
3. **Plano GPU aislado:** reproducir referencias de planos con el mismo Frame y
   recursos, probar F y negativos/horizonte/resize. Medir coste CPU/GPU por separado.
4. **Muros y puertas GPU:** quads, depth y materiales compartidos; comparar estados
   cerrada/25/50/75/100 %, ambas aproximaciones, clipping cercano y superposiciones.
   Evitar una fase que solo suba el framebuffer completo y se presente como solución
   del coste de CPU. Una carga de framebuffer sirve para comprobar presentación,
   no para demostrar que se aceleró el rasterizado.
5. **Integración opcional:** elección explícita de backend con fallback, soporte
   completo TAB/2D/debug, E/F/O y misma lógica de movimiento/colisión; luego CI y
   pruebas manuales en Metal, Vulkan y D3D12 antes de cambiar el valor predeterminado.

Matrices de comparación: cuatro resoluciones, tamaños impares, cámara fija en
varios sectores, giros y posiciones junto a paredes, F off/on, N/M, puertas en
ambas caras y progreso intermedio, falta de recursos y resize/HiDPI. Guardar color
y capturas deterministas. Exigir coincidencia de IDs/UV y colores interiores; medir
por separado diferencias de silueta/convención de rasterizado de un píxel, sin
esconder una rotación o un cambio de paleta detrás de un RMSE global aceptable.

Riesgos que requieren pruebas explícitas: float GPU frente a double CPU en costuras
y horizonte, near clip y cobertura de triángulos, empates depth entre superficies,
cuantización de sombreado y sRGB, offsets de texel nearest, Y/orientación, el caso
especial de las dos caras de puerta y la presentación 2D. No modificar reglas de
juego para disimular divergencias. Establecer tolerancias documentadas solo tras
identificar cada diferencia.

Medir GPU con timestamps/capturas de herramientas de plataforma y esperas solo
para diagnósticos/readback; evitar readback sincrónico de cada frame normal.
Mantener una política explícita de frames en vuelo y pacing. Comparar latencia,
memoria y tiempo CPU además de FPS: una cola más larga puede aparentar mejor FPS
sin mejorar la respuesta. Aún no hay cifras de un backend SDL_GPU del port.

## Archivos y validación de esta tarea

Añadidos `tools/render_benchmark.h/.cpp`, este documento y snapshots de CSV/logs en
`docs/rendering-performance-data/`. CMake incorpora solamente el target diagnóstico
`render_benchmark`, **EXCLUDE_FROM_ALL**. El ejecutable normal y sus interfaces no
se instrumentan ni se reescriben. Los hooks de memoria viven solo en la herramienta.

Validación: compilación Release correcta, nueve suites CTest y siete pruebas Python
superadas. Verificación exacta de imágenes entre ruta por etapas y producción en
los 16 casos software, y 16 casos por cada ejecución de ventana. Las medidas son
una muestra de un único M5 en este entorno, sin afinidad de CPU ni frecuencias
fijadas; no extrapolar FPS a otras plataformas. No hay commits ni pushes.
