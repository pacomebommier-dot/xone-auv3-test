# MUTE / SOLO / BAR et LED — reprise du 27 octobre 2026

Branche `feature/native-e3-bar-beat-reprise`, sans merge main.
Checkpoint e8347165 : CI complète PASS https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40512963300.
Correction f34be24 : CI complète PASS https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40514977400.

## Raccordements

- Un callback MUTE en échec conserve une incertitude : même un retour à la valeur précédemment envoyée exige un nouvel envoi. Le succès du callback supprime cette dette, sans confirmer un effet AUM.
- Le wrapper refuse les événements hors buffer ou désordonnés. Le timestamp AU immédiat conserve son sens. À égalité, BAR précède MIDI, donc un UP au BAR arme le BAR suivant ; BAR précède aussi STOP à cet instant. STOP avant BAR annule les gestes en attente.
- L'overlay qualifie BOT2 LONG avant le traitement d'un événement physique franchissant les 200 ms.
- `XoneMuteLedView` alimente `XoneMuteLedOutput`, puis le véritable callback AUv3. MIDI15/index14, aucune modification de NOTE528..543 sur MIDI7..14.
- Référence physique : `L10_S7_NONCURRENT.txt` gelé, handlers `LedInitBindings`, `LedRenderAll`, `LedRenderPosition` ; binding natif historique `l01::ledBinding`, inchangé. BASE par rangées : 36,37,38,39 / 32,33,34,35 / 28,29,30,31 / 24,25,26,27 ; jaune +36, vert +72. OFF 0x8E/0 ; ON 0x9E/127. Phase S7 de 100 ms, pilotée par le temps de rendu indépendant du transport. L'ancien A2 utilisait une table par colonnes et 250 ms : ne pas mélanger ces indexations.
- Première entrée/réentrée : purge des trois couleurs de chaque BASE, puis couleur voulue. Changement : OFF avant ON. STOP supprime le pending et son clignotement, conserve l'applied. Sortie de page : extinction de l'image MUTE ; aucune couleur du contexte sous-jacent n'est inventée. Son renderer complet reste à raccorder.
- Échec LED : cible inconnue, purge/retry au buffer suivant ; les autres BASE continuent. Le cache ne confirme que les callbacks réussis. Aucun TX LED avant la première visite MUTE.

## Validation et limites

Tests portables : couleurs et octets des 16 BASE, phases, pending/applied, SOLO/FX, STOP, page, retry et déduplication ; gestes concurrents et LONG dans le même événement. Harness réel AU macOS : sorties MIDI musicales et LED inspectées séparément, échecs ON/OFF, refus des événements tardifs/futurs et collisions BAR/MIDI/STOP. Les empreintes des sources et tests historiques restent protégées.

Raccordement 235e82c : CI complète PASS https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40516070266. Complément wrapper 7e8618c : CI complète PASS https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40516886629. Preuve : evidence/v3/CI_MUTE_20261027.json. GCC/Clang/Apple Clang, ASan/UBSan, wrapper AU macOS et build iOS PASS ; 33 suites C++ et 10 cas C. Aucun PASS AUM/iPhone/K2 déclaré. L15 (recette intégrée) et L16 (matériel) restent différés. Prochaine tâche dans ce périmètre : renderer du contexte sous-jacent pour restituer une image connue lors de la sortie MUTE, avec les seuls contrats LED concernés.