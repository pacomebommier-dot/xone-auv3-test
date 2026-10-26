# Intégration CC V3 — tranche suivante, 26/10/2026

Base : 08790d02f83d04b61c62dd3ea770b3191ceaa2fa. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40364783510 (GCC, Clang, Apple Clang, sanitizers et build AUv3 iOS unsigned). Première intégration e2dcfec également PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40364331092.

## Nouveau raccordement réel
HostEngine reçoit MIDI15 CC0..19 via la même surface/gestuelle/NAV, résout la route S3 en deux identités distinctes (cellule LIVE et adresse API), applique le pickup source, écrit dans LiveState existant et projette via OutputS4 existant. Le callback AUv3 existant émet le résultat musical sur MIDI7..14. Aucun remapping CC4/8 L02 n'est utilisé dans ce trajet.

Source : S7_S3_CLOTURE_V22_NONCURRENT.txt, @z0/@wu/@zp/@zr/@zc/@x1, sélection LOT_PATCH_BASE_08_CLOTURE_V1/SOURCES_COMPILATION_NATIVE. Le nouveau Shared/XoneContinuousInput.hpp porte seulement la résolution et le pickup manquants ; les 12 headers historiques restent inchangés.

Fixture indépendante dans Tests/test_host_continuous.cpp : les 20 contrôles du contexte NAV initial sont associés explicitement aux cellules source et API attendues. Exemples : CC16 -> cellule1328 -> API560 (MIDI11 CC48), CC0 -> cellule1546 -> API709 (MIDI12 CC69), CC4 -> cellule1744 -> API907 (MIDI14 CC11). Ce sont des commandes neutres ; aucun instrument Roland n'est joué.

## Condition d'entrée et limites
Le LIVE source inconnu bloque toute émission, conformément à @z0 : aucune valeur initiale0 inventée. Le harness fournit une SessionImage validée avant rendu ; restoreSessionBeforeRendering réutilise la validation LiveState existante, n'émet rien et devient indisponible après démarrage. Ce n'est PAS encore une restauration depuis fullState AUM : le raccordement de cette sérialisation reste L13. Donc l'AUv3 neuve, sans session connue, reste silencieuse sur ce trajet musical ; aucune tranche FX réellement contrôlée sur iPhone n'est revendiquée.

LOOP API569..573 reste exclu de ce trajet simple : il exige un batch cohérent r1/r2/r3 à partir de b1/b2/b3/m. SPOT/TOUCH/FLASH, DJ relatif, actions quantifiées, banques S8, alternatives et profils matériel restent à raccorder. La NAV source S7 ne vaut pas spécialisation S8 complète.

## Exactitude des sorties
Le LIVE exprime la valeur désirée ; un TX réussi ne devient jamais observation backend. Le cache de déduplication S4 est confirmé seulement lorsque le callback de sortie retourne succès. Un échec ou une sortie absente laisse possible l'émission sur le prochain événement physique identique ; aucun retry/ACK générique ou timer supplémentaire ajouté.

## Tests et progression
Test préalable échoué : API de session/lecture LIVE host absentes. Après raccordement : mapping des 20 CC, UNKNOWN, pickup lent/rapide avec retournement, rattrapage à la 100e tentative, déduplication après succès, nouvel essai après échec/sortie absente, STOP conservant LIVE et aucune observation fabriquée. Les 4 suites host et contrôles source sont rejoués ; la CI du commit de cette tranche doit encore être confirmée séparément.

Statut : AUTOMATED_INTEGRATION_PASS sur harness avec session connue ; AUM_FULL_DEFERRED ; K2_PHYSICAL_PENDING ; SH01A_PHYSICAL_PENDING. Aucun lot L05/L06/L13 clôturé. Les 8 étapes et 16 lots du plan40 restent applicables. Prochain raccordement : publication/lecture de session AUM sûre entre threads, puis compléments musicaux et propriétaires S8.
