# Reprise intégration native V3 — 26 octobre 2026

## Audit différentiel et delta effectué
Base GitHub vérifiée : feature/native-l02-v02 = 8fff66b4ed97acaf1b65f37028700319ed2e935e ; main = 5874ae2f29a2543d9df7cda4814803d04dd36257. Copie isolée et branche feature/native-integration-v3.
Les 12 headers Core, 14 suites NativeTests et fixture NAV sont identiques aux blobs Git et aux empreintes MANIFEST_L01_11.json. Aucun portage gratuit ni changement dans ces modules.
Avant : AUv3 -> HostEngine -> SurfaceInput diagnostic ou banc CH1 CC4/8 ; S2/S3 non appelés.
Après : AUv3 -> HostEngine -> CoreCoordinator existant -> SurfaceInput -> GestureS2 -> S3DecisionBoundary -> NAV SHORT/LONG et restauration. Initialise uniquement la NAV source ; aucun profil backend, banque ou LIVE inventé. Le chemin CC L02 reste une régression technique distincte.

DEF-L02-TRACE-01 : RX porte désormais RECEIVED ; REJECTED est réservé à la décision. Tests préalables : assertion RX échouée sur la base ; test du nouveau raccordement refusé à la compilation faute d'API host NAV. Les tests passent après le delta.
Horloge gestuelle : temps accumulé des frames rendues et décalage intra-buffer, au sample rate du host, indépendant des positions de transport. Un seek ne recule pas cette horloge. Timer après les événements de chaque buffer, même sans MIDI ; ne constitue ni BAR ni BEAT. Si le host suspend complètement le rendu, cette horloge est suspendue aussi : reprise idle/reallocation à qualifier dans les tests host/L15.
STOP : passe par le coordinateur ; aucune fabrication UP, aucune libération physique anticipée. Les journaux internes du coordinateur sont drainés après chaque appel ; une UI lente peut perdre des traces (compteur dropped), pas saturer la file S3.

## Simulateur unique
Mozaic/K2_SIM_V3.txt : boutons 0..3 fixes ; bouton-page via knob7, trois pages pour les contrôles 4..35 ; knobs0..5 pour une banque de six CC ; SHIFT GUI choisit la banque CC (0..3). CC20/21 sont les DJ bruts. Knob6 inutilisé. CH15/index14 uniquement.
Table de référence : XONE_K2_REBUILD_ISO/04_NEW_CHAIN/03_SCRIPTS/S7_NONCURRENT/LOT_PATCH_BASE_08_CLOTURE_V1/SOURCES_COMPILATION_NATIVE/S7_S1_SURFACE_ROUTE_V1_NONCURRENT.txt, @MapButton, héritée GOLDEN/A1 U1DMapButton.
UP utilise HeldNote capturée au DOWN même après changement de page. Pas de MIDI sur OnLoad, STOP, ni de Thru. Le script reste SOURCE_PREPARED : le contrôle statique Python ne prouve pas une compilation native Mozaic. Les callbacks GUI suivent le guide Mozaic du fabricant (https://ruismaker.com/mozaic/).

## Vérifications locales
GCC portable w64devkit 2.10.1 / GCC16.2, C++17, -O2 -UNDEBUG -Wall -Wextra -Wpedantic -Werror.
14 suites natives historiques PASS ; 3 suites host PASS ; régression C V0.1 : 10 tests PASS. Logs dans evidence/v3 (exécutables exclus du Git). verify_source.py PASS : 12 headers, 14 suites, 81 REQ, 382 handlers. verify_simulator.py PASS : table 36 notes, domaine 22 CC, releases capturées, aucune émission OnLoad/Thru ; contrôle SOURCE uniquement.
Les tests host couvrent valeurs CC0..127 L02, refus d'origine/canal/format, boucle de retour, UNKNOWN, vrai UP après STOP, débordement trace, SHORT/LONG NAV, timeline transport reculée et 1000 cycles sans consommation UI. test_host_surface couvre les 36 boutons physiques et doublons, notes hors mapping et domaine CC.
La CI de la branche exécute les suites GCC/Clang, Apple Clang/sanitizers puis le build AUv3 iOS. Son résultat exact est à lire sur GitHub, sans l'inférer de GCC Windows. Aucune signature ou installation iPhone nouvelle demandée.

## État des 8 étapes / 16 lots
Le plan actif 40 reste autorité : https://drive.google.com/file/d/1pGWcdyU4jxXAhKr_B6zzfaltvvoeGlXh/view
E1 L01 clos documentairement ; L02 correctif SOURCE/AUTOMATED_TEST_PASS, AUM nominal historique conservé, compléments AUM différés.
E2 L03 table et simulateur préparés, surface raccordée ; L04 S2 raccordé. Ni clôture Mozaic ni clôture AUM.
E3 L05 NAV raccordée seulement ; LIVE/horloge musicale/scheduler restent à intégrer. L06 projection musicale non raccordée.
E4 L07/L08 banques PATCH/MEMORY et CUE/LOAD non raccordées.
E5 L09/L10 PRESET/RED, alternatives et propriétaires S8 non raccordés.
E6 L11/L12 feedback authentifié et sorties LED non raccordés ; UNKNOWN conservé.
E7 L13/L14 persistance AUM/routes FX/profil SH-01A restent à réaliser.
E8 L15 recette globale AUM différée ; L16 matériel K2/SH-01A en attente ; JU-06A ensuite.

## Ordre de raccordement suivant
1. Relier le chemin CC physique et les routes NAV aux véritables décisions LIVE/S4 selon l'API S7 et les contrats S8 ; fixture entrée brute -> LIVE -> CC neutre, puis TOUCH/FLASH et dettes OFF. Ne pas confondre adresse NAV encodée et adresse MIDI virtuelle.
2. Horloge BAR/BEAT réelle fournie par le host et transitions de destinations ; aucun BAR inventé.
3. Banques S8 INVALID, PATCH/BASE/dernier LIVE distincts, CUE/LOAD/PRESET/RED et modes MACHINE exclus de leurs captures, puis feedback/LED/persistance.
4. Préparer routes FX AUM et profil SH-01A vérifié, puis recette globale L15. Aucun PC/CC Roland ajouté dans ce delta.

Références actives lues : 00,40,41,42,43,44,37,35,36 sur Drive ; méthode simple V2 et consignes S7 locales. Leur cadence différée prévaut. Aucune clôture fictive ni merge main ; V0.1/V0.2 et références GOLDEN/S7/S8 préservées.
