# Tâche 1/6 — lot 1A DJ LOOP / GLOBAL

Base : `7ec5887fcebbe222ab55e4b4e51bb429298187ae`, branche `feature/native-e3-bar-beat-reprise`.
CI de base PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40550179321.
Commit fonctionnel : `1c9ba37df440dbc31a3fbdae94abc065c0033cb1`.
CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40576338105.
GCC, Clang, Apple Clang, ASan/UBSan, vrai wrapper AU macOS et build iOS non signé : PASS. Validation locale : 48 suites C++, contrôles SOURCE/profils/simulateur et 10 tests C PASS. Aucun PASS AUM ou matériel.

Le resolver S3 conserve GLOBAL/LOOP lors de BOT3 + rotation commencée strictement avant 200 ms. Un PUSH solo court ou tenu avec boucle observée ON demande STOP sans changer de page ; OFF autorise la navigation existante. UNKNOWN conserve la page sans prétendre avoir observé OFF. Le release qualifié ne déclenche ni navigation solo ni second STOP.

La décision spécialisée `DECISION_DJ_LOOP_GLOBAL_P3_V1.txt` impose de conserver boucle et longueur lors de GLOBAL ↔ LOOP. Une navigation vers une autre page, dont l'overlay MUTE natif, demande STOP/RESET. STOP transport invalide les confirmations anciennes. Les LOOP1/2/3 permanentes et b1/b2/b3/m restent distincts du DJ LOOP. Aucun routage audio n'est ajouté.

LENGTH utilise toujours S7 type16/adresse948 → CC52 relatif. Le vrai dispatcher AU appelle `Result::packet(0)` avant `sent` : cette copie conserve maintenant qualification DJ2 et horodatage DJ1. Les régressions ont échoué avant correction sur la sortie GLOBAL et sur la perte de `dj2Qualified` dans la copie AU.

`pendingDjLoopService` expose START, STOP ou STOP/RESET avec sa génération ; `acknowledgeDjLoopService` confirme uniquement la remise à un exécutant, jamais la lecture audio. Un nouveau START/STOP invalide une remise ou observation ancienne. Aucun consommateur audio n'est installé dans le wrapper AU. S7 kind19 est un service sans binding MIDI (`Core/OutputS4.hpp`), pas une commande Loopy vérifiée.

Les frontières LOAD et TOP sont détaillées dans `LOT_1B_1C_FRONTIERES_2026-10-27.md`. Tâche 1/6 en cours, 0/6 clôturée. Dépendances externes isolées, sans les présenter comme acceptées ni résolues. Les modifications préexistantes des inventaires et du test FX sont préservées et exclues des commits fonctionnels.
