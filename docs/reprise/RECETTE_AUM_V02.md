# XONE V0 — candidate AUM : recette courte
28 octobre 2026. Build interne conservée : V0.2, aumi/XnLb/Xn02.
Statut : NON_TESTE_AUM. Le SHA candidat est le COMMIT.txt de l’artefact CI.

## Installation et signature iPhone / iPad
1. Télécharger l’artefact `XoneEngineNative-V0.2-<SHA>` du job iOS **PASS**. Vérifier COMMIT.txt et IPA_SHA256.txt. L’IPA est non signé : il ne s’installe pas directement.
2. Chemin reproductible : sur Mac avec Xcode, extraire `XONE_V0_SOURCES.zip` du même artefact, puis `brew install xcodegen` et `xcodegen generate`. Ouvrir XoneEngineTest.xcodeproj.
3. Dans Signing & Capabilities, choisir sa propre équipe Apple pour **XoneEngineTest et XoneMidiAU**. Les IDs source sont `org.xonelab.XoneEngineNativeV02` et `.XoneMidiAU`. Si nécessaire, choisir ses IDs uniques dans project.yml, l’extension gardant le préfixe de l’app, puis régénérer. Ne pas modifier aumi/XnLb/Xn02.
4. Connecter/déverrouiller l’iPhone ou l’iPad, accepter la confiance et activer le mode développeur si Xcode le demande. Choisir l’app XoneEngineTest et cet appareil, puis Run. Conserver le journal de signature/installation et le SHA source. Ouvrir l’app conteneur une fois.
5. Ouvrir AUM, ajouter une tranche MIDI et le MIDI Processor **XONE Lab: XONE ENGINE V0.2**. Noter modèle, iOS, version AUM, équipe/profil et date de signature. Aucun certificat, UDID ni profil de signature n’est fourni par la CI.

## Session minimale à créer et sauvegarder dans AUM
Créer `XONE_V0_<SHA>_RECETTE` avec une entrée (K2 **ou** Mozaic K2_SIM_V3.txt), XONE et un moniteur POST. Matrice : entrée→XONE ; XONE V0.2 TEST OUT→POST. Un moniteur PRE parallèle peut vérifier l’entrée. Pas de route entrée→POST, sortie→XONE, MIDI Thru ou instrument.

Le script Mozaic existant simule les notes/CC physiques sur CH15. Pads0..3 = BOT1..4 ; page boutons0/pad4 = note36 ; page1/pad8 = note48. SHIFT GUI deux fois sélectionne CC12..17, knob4 = CC16. Voir Mozaic/README.md. Compilation Mozaic et session .aum réelle à effectuer sur l’appareil : aucun fichier .aum fabriqué ici.

**Session isolée, moniteur seul.** Le bouton « Recette simulée : charger l’état test » remplace le LIVE via la restauration existante, après vrais UP et règlement des OFF dus. Il reprend exactement la fixture du harnais AU (CC16 cible64, SPOT FX1 OFF, trois champs TOUCH/FLASH simulés). Il ne configure aucun instrument, preset externe, BASE, PATCH, backend ou observation. Les autres cellules restent UNKNOWN. Ce bouton n’est jamais exécuté au démarrage.

## Sept essais
| Essai | Action | Observation attendue / verdict à relever |
|---|---|---|
| Ouverture / refus sûr | Instance neuve, sans fixture : envoyer `BE 10 40` (CC16=64 CH15) | RX puis UNKNOWN_LIVE, aucun CC de sortie ; pas de crash. Les gestes restent disponibles. |
| CC / PICKUP | Sur instance neuve charger explicitement la fixture. Envoyer CC16=0 ; attendre >200 ms ; CC16=64 puis100 | Aucun TX pour0 ; `BA 30 40` puis `BA 30 64` ; TX_OUT SENT résultat0 et mêmes octets dans POST. Aucun retour instrument supposé. |
| NAV / SPOT | Instance neuve + fixture : note36 DOWN/UP court ; note48 DOWN, tenir >200 ms, vrai UP | Sélection FX1 puis `99 00 7F`, puis `89 00 00`. Un tap court de48 latche selon SMART ; le second tap éteint. |
| STOP | Même SPOT actif, démarrer puis arrêter le transport AUM ; relâcher ensuite réellement48 | OFF dû sur POST, aucun nouvel ON au vrai UP. Vérifier également STOP MIDI `FC` avec un émetteur capable. STOP ne fabrique aucun UP. |
| MUTE / SOLO et LED | Nouvelle instance, transport120 BPM/4:4 ; BOT2 court (note13), BASE1 court (36) ; puis BASE2 long≥200 ms (37) | MUTE appliqué au prochain BAR : `9A 10 7F`. SOLO au prochain BAR selon projection existante ; relâchements et sortie SOLO à vérifier. LED CH15 visibles sur POST, matérielles seulement si retour configuré. Aucun effet audio sans MIDI Learn AUM. |
| Sauvegarde / réouverture | Après CC16=100 et tout OFF, sauvegarder la session, retirer/recharger la session puis CC16=100 | LIVE pris en charge restauré ; état observé toujours UNKNOWN. NAV complète, banques et persistance MUTE/SOLO ne sont pas promises. |
| Fermeture | STOP et vrais UP, vérifier OFF sur POST, fermer/réouvrir l’interface puis recharger le node | Pas de crash ; l’interface seule ne détruit pas le moteur. Réallocation conserve les dettes OFF et les réessaie au rendu suivant. |

**Limite de fermeture :** aucun callback n’est envoyé hors rendu. Une suppression définitive sans STOP et sans prochain rendu ne peut garantir la livraison externe d’un OFF. Envoyer STOP et constater les OFF avant retrait ; relever tout comportement AUM contraire, sans annoncer de validation matérielle.

Contrôles négatifs : débrancher entrée→XONE puis sortie→POST ; POST doit se taire dans chaque cas. Un TX demandé n’est prouvé que si callback SENT résultat0 **et** POST l’observe. `dropped>0` rend la trace incomplète : ralentir et refaire.

## Tableau de portée
| État | Portée |
|---|---|
| OPERATIONNEL (preuve logicielle à confirmer sur SHA candidat par CI) | Chaîne K2/S1/S2/NAV/S3/S4→callback AU, CC/pickup, sélection/LOOP, SPOT, STOP et retries, MUTE/SOLO, LED de page MUTE, document LIVE partiel. |
| A_CONFIGURER | Instruments, MIDI Learn/routage AUM, sources audio, profils/ports authentifiés, PATCH/BASE/RESET/FILTER. Pas de conversion UNKNOWN→0. |
| DIFFERE | MACHINE local non publié préservé hors candidate ; PATCH/MEMORY/CUE/LOAD/BASE complets, PRESET/RED exhaustifs, LED secondaires, persistance étendue, services DJ LOOP/Browser LOAD. Six tâches inchangées. |
| NON_TESTE_AUM | Tous les essais ci-dessus, installation/signature sur appareil, compilation du simulateur Mozaic, K2 et LED matériels. |

Pour chaque essai relever PASS/FAIL/NON TESTABLE, SHA, configuration, octets POST et trace. Références : [guide AUM](https://www.kymatica.com/aum/help), [cycle AUAudioUnit](https://developer.apple.com/documentation/audiotoolbox/auaudiounit).
