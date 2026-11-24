#pragma once
#include <array>
#include <string>
#include <string_view>
namespace localization {
enum class Language : unsigned { French=0, English=1, Spanish=2 };
struct Entry { std::string_view id,en,es,fr; };
inline constexpr std::array<Entry,117> entries{{
    {"menu.new_game","NEW GAME","NUEVA PARTIDA","NOUVELLE PARTIE"},
    {"menu.load_game","LOAD GAME","CARGAR PARTIDA","CHARGER UNE PARTIE"},
    {"menu.options","OPTIONS","OPCIONES","OPTIONS"},
    {"menu.exit","EXIT","SALIR","QUITTER"},
    {"pause.back_to_game","BACK TO GAME","VOLVER AL JUEGO","REPRENDRE LA PARTIE"},
    {"options.video","VIDEO","VÍDEO","VIDÉO"},
    {"options.audio","AUDIO","AUDIO","AUDIO"},
    {"options.controls","CONTROLS","CONTROLES","COMMANDES"},
    {"options.language","LANGUAGE","IDIOMA","LANGUE"},
    {"options.back","BACK","VOLVER","RETOUR"},
    {"options.aspect_ratio","ASPECT RATIO","RELACIÓN DE ASPECTO","FORMAT D’IMAGE"},
    {"options.resolution","RESOLUTION","RESOLUCIÓN","RÉSOLUTION"},
    {"options.fisheye","FISHEYE EFFECT","EFECTO OJO DE PEZ","EFFET FISHEYE"},
    {"options.floor_ceiling","FLOOR AND CEILING","SUELO Y TECHO","SOL ET PLAFOND"},
    {"options.colored_lights","COLORED LIGHTS","LUCES DE COLORES","ÉCLAIRAGE COLORÉ"},
    {"options.vsync","VSYNC","SINCRONIZACIÓN VERTICAL","SYNCHRONISATION VERTICALE"},
    {"options.master_volume","MASTER VOLUME","VOLUMEN GENERAL","VOLUME GÉNÉRAL"},
    {"options.control_scheme","CONTROL SCHEME","ESQUEMA DE CONTROL","PROFIL DE COMMANDES"},
    {"options.modern","MODERN","MODERNO","MODERNE"},
    {"options.classic","CLASSIC","CLÁSICO","CLASSIQUE"},
    {"options.on","ON","ACTIVADO","ACTIVÉ"},
    {"options.off","OFF","DESACTIVADO","DÉSACTIVÉ"},
    {"options.unavailable","UNAVAILABLE","NO DISPONIBLE","INDISPONIBLE"},
    {"options.selected","SELECTED","SELECCIONADO","SÉLECTIONNÉ"},
    {"options.auto_aspect","AUTO: MATCH MONITOR ASPECT","AUTO: USAR ASPECTO DEL MONITOR","AUTO : FORMAT DE L’ÉCRAN"},
    {"options.vsync_failed","VSYNC SWITCH FAILED","NO SE PUDO CAMBIAR VSYNC","ÉCHEC DU CHANGEMENT DE VSYNC"},
    {"options.display_failed","DISPLAY MODE SWITCH FAILED","NO SE PUDO CAMBIAR EL MODO DE PANTALLA","ÉCHEC DU CHANGEMENT DE MODE D’AFFICHAGE"},
    {"options.aspect_original","4:3 ORIGINAL DOS VIEW","4:3 VISTA DOS ORIGINAL","4:3 VUE DOS ORIGINALE"},
    {"options.aspect_wide","16:9 EXPANDED GAME VIEW","16:9 VISTA AMPLIADA","16:9 VUE ÉLARGIE"},
    {"options.aspect_ultrawide","21:9 EXPANDED GAME VIEW","21:9 VISTA AMPLIADA","21:9 VUE ÉLARGIE"},
    {"password.heading","CONTINUATION CODE","CÓDIGO DE CONTINUACIÓN","CODE DE CONTINUATION"},
    {"password.enter_confirm","ENTER TO CONFIRM","INTRO: CONFIRMAR","ENTRÉE : VALIDER"},
    {"password.accepted","CODE ACCEPTED","CÓDIGO ACEPTADO","CODE ACCEPTÉ"},
    {"password.refused","CODE REFUSED","CÓDIGO RECHAZADO","CODE REFUSÉ"},
    {"hud.portal_locked","ATTENTION:\r\n  IMPOSSIBLE TO OPEN PORTAL!\r\n  DISABLE THE SECURITY","ATENCIÓN:\r\n  ¡IMPOSIBLE ABRIR LA COMPUERTA!\r\n  DESACTIVE LA SEGURIDAD","ALARME:\r\n  IMPOSSIBLE D’OUVRIR LE SAS !\r\n  DÉSACTIVEZ LA SÉCURITÉ"},
    {"hud.door_unlocked","VALID CARD..... DOOR UNLOCKING!","TARJETA VÁLIDA..... ¡ABRIENDO PUERTA!","CARTE VALIDE..... OUVERTURE DU SAS !"},
    {"hud.card_rejected","IMPOSSIBLE TO UNLOCK THIS DOOR...","IMPOSIBLE ABRIR ESTA PUERTA...","IMPOSSIBLE DE DÉVERROUILLER CETTE PORTE..."},
    {"hud.security_disabled","THE SECURITY IS ALREADY DISABLE","LA SEGURIDAD YA ESTÁ DESACTIVADA","LA SÉCURITÉ EST DÉJÀ DÉSACTIVÉE"},
    {"hud.map_blocked","ATTENTION: AN ALIEN IS NEAR!!","ATENCIÓN: ¡HAY UN ALIENÍGENA CERCA!","ATTENTION : UN EXTRATERRESTRE EST PROCHE !!"},
    {"hud.map_unavailable","ATTENTION:\r\n  ACCESS NOT POSSIBLE WITH CARD!!","ATENCIÓN:\r\n  ¡LA TARJETA NO PERMITE EL ACCESO!","ATTENTION :\r\n  CARTE NON AUTORISÉE POUR CET ACCÈS !!"},
    {"hud.self_destruction","0M 00S UNTIL EXPLOSION!","¡EXPLOSIÓN EN 0M 00S!","EXPLOSION DANS 0M 00S !"},
    {"hud.bomb_warning","ATTENTION:\r\n  5 SECONDS BEFORE EXPLOSION!","ATENCIÓN:\r\n  ¡EXPLOSIÓN EN 5 SEGUNDOS!","ATTENTION :\r\n  EXPLOSION DANS 5 SECONDES !"},
    {"hud.shock","ATTENTION: SHOCKS","ATENCIÓN: DESCARGA ELÉCTRICA","ATTENTION : DÉCHARGE ÉLECTRIQUE"},
    {"hud.power_surge","ATTENTION\r\n POWER IS SURGING!!!","ATENCIÓN\r\n ¡SOBRECARGA ELÉCTRICA!","ATTENTION\r\n SURTENSION ÉLECTRIQUE !!!"},
    {"hud.battery_failed","ATTENTION:\r\n  NEW BATTERY HAS FAILED!!!!!!","ATENCIÓN:\r\n  ¡LA BATERÍA NUEVA HA FALLADO!","ATTENTION :\r\n  LA NOUVELLE BATTERIE EST HORS SERVICE !"},
    {"hud.radar_activated","ATTENTION: RADAR SENSOR WAS ACTIVATED","ATENCIÓN: RADAR ACTIVADO","ATTENTION : RADAR ACTIVÉ"},
    {"hud.sensors_activated","ATTENTION: SENSORS ACTIVATED","ATENCIÓN: SENSORES ACTIVADOS","ATTENTION : CAPTEURS ACTIVÉS"},
    {"hud.empty_compartment","THIS COMPARTMENT IS EMPTY!","¡ESTE COMPARTIMENTO ESTÁ VACÍO!","CE COMPARTIMENT EST VIDE !"},
    {"password.quick_code","QUICK CODE","CÓDIGO RÁPIDO","CODE RAPIDE"},
    {"password.press_any_key","PRESS ANY KEY","PULSE CUALQUIER TECLA","APPUYEZ SUR UNE TOUCHE"},
    {"panel.transfer","TRANSFER","TRANSFERENCIA","TRANSFERT"},
    {"panel.zone","ZONE {zona}","ZONA {zona}","ZONE {zona}"},
    {"panel.type_zone","TYPE IN CODE ZONE {zona}","INTRODUZCA EL CÓDIGO DE LA ZONA {zona}","SAISISSEZ LE CODE DE LA ZONE {zona}"},
    {"panel.zone_seized","CODE ZONE {zona} ALREADY SEIZED","CÓDIGO DE LA ZONA {zona} YA OBTENIDO","CODE DE ZONE {zona} DÉJÀ OBTENU"},
    {"panel.code_unavailable","CODE DATA UNAVAILABLE","DATOS DEL CÓDIGO NO DISPONIBLES","DONNÉES DU CODE INDISPONIBLES"},
    {"panel.code_ok","OK...","CORRECTO...","CORRECT..."},
    {"panel.code_wrong","WRONG CODE!!...","¡CÓDIGO INCORRECTO!...","CODE ERRONÉ !..."},
    {"panel.code_zone","CODE ZONE {zona}  {letras}","CÓDIGO ZONA {zona}  {letras}","CODE ZONE {zona}  {letras}"},
    {"panel.no_elevator_card","No elevator card","Sin tarjeta de ascensor","Pas de carte d'ascenseur"},
    {"panel.no_security_card","No security card","Sin tarjeta de seguridad","Pas de carte de sécurité"},
    {"panel.incompatible_card","Incompatible card type","Tarjeta incompatible","Carte incompatible"},
    {"panel.card_refused","CARD REFUSED...","TARJETA RECHAZADA...","CARTE REFUSÉE..."},
    {"panel.card_accepted","CARD ACCEPTED....","TARJETA ACEPTADA...","CARTE ACCEPTÉE..."},
    {"panel.security_off","SECURITY {n} DEACTIVATED..","SEGURIDAD {n} DESACTIVADA...","SÉCURITÉ {n} DÉSACTIVÉE..."},
    {"panel.security_failed","IMPOSSIBLE TO DEACTIVATE THE SECURITY","IMPOSIBLE DESACTIVAR LA SEGURIDAD","IMPOSSIBLE DE DÉSACTIVER LA SÉCURITÉ"},
    {"panel.mine_owned","ALREADY HAVE A MINE","YA TIENE UNA MINA","VOUS AVEZ DÉJÀ UNE MINE"},
    {"panel.oxygen_owned","ALREADY HAVE AN OXYGEN BOTTLE","YA TIENE UNA BOTELLA DE OXÍGENO","VOUS AVEZ DÉJÀ UNE BOUTEILLE D'OXYGÈNE"},
    {"panel.injectors_owned","ALREADY HAVE TWO INJECTORS","YA TIENE DOS INYECTORES","VOUS AVEZ DÉJÀ DEUX INJECTEURS"},
    {"panel.control_panel","CONTROL PANEL....","PANEL DE CONTROL...","PANNEAU DE COMMANDE..."},
    {"panel.controller_card","CONTROLLER CARD..","TARJETA DE CONTROL...","CARTE DE COMMANDE..."},
    {"panel.battery_owned","ALREADY HAVE A BATTERY","YA TIENE UNA BATERÍA","VOUS AVEZ DÉJÀ UNE BATTERIE"},
    {"panel.take_battery","TAKE BATTERY {porcentaje}%","TOMAR BATERÍA {porcentaje}%","PRENDRE BATTERIE {porcentaje} %"},
    {"panel.keep_battery","KEEP BATTERY {porcentaje}%","CONSERVAR BATERÍA {porcentaje}%","GARDER BATTERIE {porcentaje} %"},
    {"panel.use_card","Use card {tarjeta}","Usar tarjeta {tarjeta}","Utiliser carte {tarjeta}"},
    {"panel.exchange_card","Exchange with card {tarjeta}","Cambiar por tarjeta {tarjeta}","Échanger contre carte {tarjeta}"},
    {"panel.card","Card {tarjeta}","Tarjeta {tarjeta}","Carte {tarjeta}"},
    {"inventory.exit","Exit","Salir","Quitter"},
    {"inventory.battery_level","Battery level {n}","Carga de batería {n}","Niveau de batterie {n}"},
    {"inventory.radar_on","Turn on radar","Activar radar","Activer le radar"},
    {"inventory.radar_off","Turn off radar","Desactivar radar","Désactiver le radar"},
    {"inventory.optics_on","Turn on optics","Activar óptica","Activer l'optique"},
    {"inventory.optics_off","Turn off optics","Desactivar óptica","Désactiver l'optique"},
    {"inventory.no_change","No change","Sin cambios","Aucun changement"},
    {"inventory.injector_1","Use injector-1","Usar inyector 1","Utiliser injecteur 1"},
    {"inventory.injector_2","Use injector-2","Usar inyector 2","Utiliser injecteur 2"},
    {"inventory.use_bomb","Use bomb","Usar bomba","Utiliser la bombe"},
    {"inventory.use_oxygen","Use oxygen bottle","Usar botella de oxígeno","Utiliser la bouteille d'oxygène"},
    {"inventory.use_injector","Use injector","Usar inyector","Utiliser l'injecteur"},
    {"inventory.do_not_use","Do not use","No usar","Ne pas utiliser"},
    {"inventory.no_battery","No battery","Sin batería","Pas de batterie"},
    {"inventory.empty_battery","Empty battery","Batería agotada","Batterie vide"},
    {"inventory.battery_radar","Battery utilized for the radar","Batería usada por el radar","Batterie utilisée par le radar"},
    {"inventory.battery_optics","Battery utilized for optics system","Batería usada por la óptica","Batterie utilisée par l'optique"},
    {"inventory.battery_available","Battery available","Batería disponible","Batterie disponible"},
    {"inventory.no_oxygen","No oxygen bottle","Sin botella de oxígeno","Pas de bouteille d'oxygène"},
    {"inventory.empty_oxygen","Empty oxygen bottle","Botella de oxígeno vacía","Bouteille d'oxygène vide"},
    {"inventory.no_injector","No injector","Sin inyector","Pas d'injecteur"},
    {"inventory.empty_injector","Empty injector","Inyector vacío","Injecteur vide"},
    {"inventory.no_cards","No cards","Sin tarjetas","Pas de cartes"},
    {"inventory.no_mine","No mine","Sin mina","Pas de mine"},
    {"inventory.mine","Mine","Mina","Mine"},
    {"inventory.category_battery","Battery Charger","Batería","Batterie"},
    {"inventory.category_oxygen","Air Bottles","Oxígeno","Oxygène"},
    {"inventory.category_injectors","Injectors","Inyectores","Injecteurs"},
    {"inventory.category_cards","Cards","Tarjetas","Cartes"},
    {"inventory.category_bomb","Bomb","Bomba","Bombe"},
    {"modal.take_question","Do you want to take it?","¿Desea recogerlo?","Voulez-vous le prendre ?"},
    {"modal.yes","Yes","Sí","Oui"},
    {"modal.no","No","No","Non"},
    {"modal.condition","Working order","Estado","État"},
    {"modal.ammo","Munitions","Munición","Munitions"},
    {"modal.number","Number","Número","Numéro"},
    {"modal.supply_battery","Battery SE","Batería SE","Batterie SE"},
    {"modal.supply_oxygen","Reserve Air bottles","Reserva de\noxígeno","Réserve\nd'oxygène"},
    {"modal.supply_energy","Reserve Energy-giving","Reserva\nenergética","Réserve\nd'énergie"},
    {"modal.supply_card","Electronic Card","Tarjeta\nelectrónica","Carte\nélectronique"},
    {"modal.supply_bomb","Bomb DAM10","Bomba DAM10","Bombe DAM10"},
}};
inline constexpr std::array<Entry,31> cinematicEntries{{
    {"intro.presente","","PRESENTA",""},
    {"intro.who.01","","PROGRAMACIÓN",""},
    {"intro.who.02","","GRÁFICOS",""},
    {"intro.who.03","","TRABAJO ADICIONAL",""},
    {"intro.who.04","","BANDA SONORA",""},
    {"intro.pseq3.body","","SISTEMAS DE DETECCIÓN / Análisis... / Clase: estación de laboratorio / Tipo: nave de mando / Escáner en marcha... / Matrícula: ZWZ 46-30-E4 / Nombre: Explorer 4 / Observaciones... / Se han detectado formas de vida humanas.","SYSTÈMES DE DÉTECTION / Analyse... / Classe : station laboratoire / Type : vaisseau de commandement / Analyse en cours... / Immatriculation : ZWZ 46-30-E4 / Nom : Explorer 4 / Observations... / Des formes de vie humaines ont été détectées."},
    {"intro.pseq6.body","","¡ATENCIÓN! / Fallos detectados: / Reguladores gravitatorios... / Sobrecargas del reactor... / Análisis de riesgos... / Explosión inminente... / Recomendaciones... / EVACUACIÓN INMEDIATA.","ATTENTION ! / Défaillances détectées : / Régulateurs de gravité... / Surtensions du réacteur... / Analyse des risques... / Explosion imminente... / Recommandations... / ÉVACUATION IMMÉDIATE."},
    {"comm.question","","¿Desea activar el sistema?","Voulez-vous activer le système ?"},
    {"comm.page.01","","",""},
    {"comm.page.02","","Esto fue lo que ocurrió: tras descubrir un asteroide de trayectoria errática, la comunidad científica decidió enviar una sonda de reconocimiento. Sus sofisticados instrumentos transmitieron datos extraños: el asteroide estaba formado por materiales desconocidos y se detectaba movimiento en su interior. Decidimos enviar una nave laboratorio. Su enorme tamaño permitió recoger el asteroide entero y facilitar su estudio.","Voici ce qui nous est arrivé : après la découverte d'un astéroïde à la trajectoire erratique, la communauté scientifique a envoyé une sonde de reconnaissance. Ses instruments sophistiqués ont révélé des matériaux inconnus et des mouvements à l'intérieur de l'astéroïde. Nous avons donc envoyé un vaisseau laboratoire. Sa taille immense a permis de récupérer l'astéroïde entier pour l'étudier."},
    {"comm.page.03","","Primero analizamos la superficie. Como indicó la sonda, la roca contenía materiales desconocidos. Pero, sobre todo, no parecía natural: tenía forma artificial, signos semejantes a jeroglíficos y una superficie demasiado lisa. ¿Era obra de una civilización extraterrestre? Y, si era así, ¿por qué flotaba en el espacio?","Nous avons d'abord analysé la surface. Comme l'avait indiqué la sonde, la roche contenait des matériaux inconnus. Mais surtout, elle n'était pas naturelle : elle semblait façonnée artificiellement, portait des signes semblables à des hiéroglyphes et sa surface était trop lisse. Était-ce l'œuvre d'une civilisation extraterrestre ? Alors pourquoi flottait-elle dans l'espace ?"},
    {"comm.page.04","","Los ordenadores intentaron descifrar los signos, pero ni su potencia de cálculo produjo resultados inmediatos. Quizá sus autores pensaban de otra manera. Entretanto, confirmamos el segundo hallazgo de la sonda: algo se movía dentro del asteroide. La posibilidad de un primer contacto nos llevó a perforar la roca cuanto antes para descubrir qué contenía.","Les ordinateurs ont tenté de déchiffrer les signes, mais même leur puissance de calcul n'a donné aucun résultat immédiat. Leurs auteurs pensaient peut-être autrement. Entre-temps, la seconde découverte de la sonde a été confirmée : quelque chose bougeait dans l'astéroïde. La perspective d'un premier contact nous a poussés à percer la roche au plus vite pour découvrir ce qu'elle contenait."},
    {"comm.page.05","","Entonces todo sucedió muy deprisa. Justo cuando íbamos a alcanzar el interior, los ordenadores descifraron los signos: eran una advertencia. Ya era demasiado tarde. Una criatura gigantesca surgió, atacó a quienes estaban cerca y huyó por los pasillos. Activamos la seguridad, pero habíamos dejado entrar en la nave a una ponedora. Pronto la invadieron decenas de criaturas de crecimiento monstruosamente rápido.","Alors tout s'est passé très vite. Au moment où nous allions atteindre l'intérieur, les ordinateurs ont déchiffré les signes : c'était une mise en garde. Trop tard. Une créature gigantesque a surgi, attaqué les personnes proches puis s'est enfuie dans les couloirs. Nous avons activé la sécurité, mais nous avions laissé entrer une pondeuse à bord. Bientôt, des dizaines de créatures à la croissance monstrueusement rapide ont envahi le vaisseau."},
    {"comm.page.06","","No hemos podido estudiarlas y, sin información, no podemos detenerlas. Los analizadores atmosféricos han detectado virus y bacterias ajenos en el aire. La mayoría murió antes de poder protegerse; los supervivientes debemos llevar trajes espaciales para respirar sin peligro. Sabemos que nos queda poco tiempo. Debemos destruir la nave y a sus intrusos, pero la seguridad del ordenador central impide la autodestrucción. He puesto rumbo a la estrella más cercana para que todo desaparezca.","Nous n'avons pas pu les étudier et, sans information, nous ne pouvons arrêter leur flot. Les analyseurs atmosphériques ont détecté des virus et des bactéries étrangers dans l'air. La plupart sont morts avant de pouvoir se protéger ; les survivants doivent porter des combinaisons spatiales pour respirer sans danger. Notre temps est compté. Il faut détruire le vaisseau et ses intrus, mais la sécurité de l'ordinateur central empêche l'autodestruction. J'ai mis le cap sur l'étoile la plus proche pour que tout disparaisse."},
    {"comm.page.07","","Como dije al comienzo, si ha llegado hasta aquí, mi intento ha fracasado. Si puede terminar lo que yo no logré, no lo dude. Buena suerte... Fin del mensaje.","Comme je l'ai dit au début, si vous êtes arrivé jusqu'ici, ma tentative a échoué. Si vous pouvez accomplir ce que je n'ai pas pu faire, n'hésitez pas. Bonne chance... Fin du message."},
    {"shuttle.script.01","","ORDENADOR: OK / MOTOR: OK / ENERGÍA: OK","ORDINATEUR : OK / MOTEUR : OK / ÉNERGIE : OK"},
    {"shuttle.script.02","","",""},
    {"shuttle.script.03","","DESTINO: AXIA","DESTINATION : AXIA"},
    {"shuttle.script.04","","",""},
    {"shuttle.prompt.01","","¿Desea entrar en la lanzadera?","Voulez-vous entrer dans la navette ?"},
    {"shuttle.prompt.02","","¿Desea poner en marcha la lanzadera?","Voulez-vous démarrer la navette ?"},
    {"shuttle.prompt.03","","No se puede arrancar la lanzadera sin tarjeta de seguridad.","Impossible de démarrer la navette sans carte de sécurité."},
    {"shuttle.prompt.04","","No se puede despegar. La autodestrucción está desactivada.","Décollage impossible. L'autodestruction est désactivée."},
    {"ending.game_over.line.01","","Por desgracia, ha fracasado en su misión.","Malheureusement, vous avez échoué dans votre mission."},
    {"ending.game_over.line.02","","",""},
    {"ending.game_over.line.03","","¿Tal vez le faltaba experiencia?","Peut-être manquiez-vous d'expérience ?"},
    {"ending.game_over.line.04","","¿Quizá no se podía confiar en usted?","Peut-être ne pouvait-on pas compter sur vous ?"},
    {"ending.game_over.line.05","","¿O eran demasiados enemigos?","Ou les ennemis étaient-ils trop nombreux ?"},
    {"ending.game_over.line.06","","A veces todo sale mal cuando la humanidad vive","Parfois, tout tourne mal quand l'humanité vit"},
    {"ending.game_over.line.07","","bajo la opresión de criaturas infames","sous l'oppression de créatures infâmes"},
    {"ending.game_over.line.08","","que siembran el terror y la destrucción...","qui sèment la terreur et la destruction..."},
}};
inline std::string_view cinematicById(std::string_view id,Language language) {
    if(language==Language::English) return {};
    for(const auto& e:cinematicEntries) if(e.id==id) return language==Language::Spanish?e.es:e.fr;
    if(id=="comm.yes") return language==Language::Spanish?"SÍ":"OUI";
    if(id=="comm.no") return language==Language::Spanish?"NO":"NON";
    return {};
}
inline const Language* activeLanguage=nullptr;
inline void bind(const Language* language) { activeLanguage=language; }
inline Language current() { return activeLanguage?*activeLanguage:Language::English; }
inline constexpr std::string_view byId(std::string_view id,Language language) {
    for (const auto& e:entries) if(e.id==id) return language==Language::Spanish?e.es:language==Language::French?e.fr:e.en;
    return {};
}
inline std::string translate(std::string_view source,Language language=current()) {
    if(language==Language::English) return std::string(source);
    if(source.rfind("ATTENTION:\r\n",0)==0) {
        const auto end=source.find(" UNTIL EXPLOSION!");
        if(end!=std::string_view::npos) {
            const auto time=source.substr(12,end-12);
            return language==Language::Spanish ? "ATENCIÓN:\r\n¡EXPLOSIÓN EN "+std::string(time)+"!"
                : "ATTENTION :\r\nEXPLOSION DANS "+std::string(time)+" !";
        }
    }
    auto trimmed=source;
    while(!trimmed.empty() && trimmed.back()==' ') trimmed.remove_suffix(1);
    for(const auto& e:entries) {
        if(e.en==source || e.en==trimmed) return std::string(language==Language::Spanish?e.es:e.fr);
        const auto mark=e.en.find('{');
        if(mark==std::string_view::npos) continue;
        const auto end=e.en.find('}',mark);
        if(end==std::string_view::npos) continue;
        const auto before=e.en.substr(0,mark),after=e.en.substr(end+1);
        if(source.size()<before.size()+after.size() || source.substr(0,before.size())!=before ||
           source.substr(source.size()-after.size())!=after) continue;
        const auto value=source.substr(before.size(),source.size()-before.size()-after.size());
        std::string output(language==Language::Spanish?e.es:e.fr);
        const auto start=output.find('{'),finish=output.find('}',start);
        if(start!=std::string::npos && finish!=std::string::npos)
            output.replace(start,finish-start+1,value);
        return output;
    }
    return std::string(source);
}
inline constexpr std::string_view byEnglish(std::string_view value,Language language) {
    if(language==Language::English) return value;
    for (const auto& e:entries) if(e.en==value) return language==Language::Spanish?e.es:e.fr;
    return value;
}
}
