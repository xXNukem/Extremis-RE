Extremis RE — instalación de los archivos originales
====================================================

Extremis RE requiere una copia propia y legítima de In Extremis (1993).
El paquete no incluye GAME.LAB, EXTREMIS.EXE ni la carpeta DATA.

macOS: extrae el ZIP. Copia GAME.LAB, EXTREMIS.EXE y DATA junto a
EXTREMIS-RE.app, o dentro de una subcarpeta (por ejemplo,
"InExtremis original/GAME.LAB", "InExtremis original/EXTREMIS.EXE" y
"InExtremis original/DATA/..."). Abre EXTREMIS-RE.app desde Finder.

Windows: extrae el ZIP. Copia esos mismos archivos junto a
EXTREMIS-RE.exe, o dentro de una subcarpeta del paquete. Abre el .exe
con doble clic. Mantén SDL3.dll junto al .exe.

También se admite una instalación anidada hasta cuatro subcarpetas bajo
el ejecutable, por ejemplo "Juegos/DOS/InExtremis/GAME.LAB". En macOS
se busca asimismo bajo la carpeta que contiene el paquete .app.

La búsqueda no depende del directorio de trabajo. Primero se comprueba
la carpeta del ejecutable y después cada nivel de subcarpetas hasta
cuatro niveles. Dentro de un nivel, el orden es alfabético. En macOS
se repite el criterio desde la carpeta que contiene el .app. Solo se
elige una instalación que supere la validación completa; las carpetas
incompletas no tienen prioridad. No se siguen enlaces simbólicos.

Para desarrollo se puede pasar una ruta explícita a GAME.LAB. Una ruta
explícita inválida muestra su propio error y no activa la búsqueda.

Si aparece un error de datos, comprueba que DATA esté junto a GAME.LAB,
que EXTREMIS.EXE esté en la misma carpeta y que los archivos originales
no estén dañados. No cambies los nombres de los archivos.

Validación de Windows: el workflow `.github/workflows/windows-package.yml`
compila en `windows-latest`, ejecuta CTest y publica
`Extremis-RE-Windows.zip` como artefacto. La prueba nativa y la
comprobación visual del icono siguen pendientes hasta ejecutar el workflow.

Construcción de paquetes desde el código fuente
-----------------------------------------------

macOS (con SDL3 y CMake instalados):

    cmake -S . -B build/package-macos -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DEXTREMIS_MACOS_BUNDLE=ON
    cmake --build build/package-macos --target InExtremisPort
    cpack --config build/package-macos/CPackConfig.cmake -B build/package-macos

El resultado es build/package-macos/Extremis-RE-macOS.zip. Incluye
EXTREMIS-RE.app, SDL3 dentro del paquete y README.txt. La firma es
local (ad hoc); falta la firma de distribución y notarización de Apple.

Windows, desde una consola de desarrollo con SDL3 y CMake instalados:

    cmake -S . -B build/package-windows -DBUILD_TESTING=OFF
    cmake --build build/package-windows --config Release --target InExtremisPort
    cpack --config build/package-windows/CPackConfig.cmake -C Release -B build/package-windows

El ZIP previsto incluye EXTREMIS-RE.exe, SDL3.dll y README.txt.
Esta ruta de compilación y su arranque todavía requieren una prueba
nativa en Windows.
