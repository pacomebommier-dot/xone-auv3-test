# Reprise active — intégration XONE native V3
27 octobre 2026. Branche de reprise ciblée : feature/native-e3-bar-beat-reprise. Aucun merge main.

Reprise MUTE/SOLO/BAR et LED : voir [INTEGRATION_V3_MUTE.md](INTEGRATION_V3_MUTE.md), qui complète les checkpoints historiques ci-dessous. Les LED MUTE sont raccordées au callback AU ; le renderer du contexte sous-jacent reste ouvert.

## Code et preuves
- e2dcfec : surface K2, gestes S2 et NAV S3 raccordés au callback AUv3 ; DEF-L02-TRACE-01 corrigé. K2_SIM unique préparé en source Mozaic.
- 08790d0 : sample rate lu après allocation des ressources de rendu.
- 03be998 : CC physiques -> résolution source -> pickup -> LIVE -> S4/API neutre, sur session connue. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40366951514
- d30bb1d : dernier LIVE dans fullStateForDocument, CRC et snapshots cohérents entre host/rendu ; aucune émission au chargement. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40397065753
- 3e901da : sélections physiques de source raccordées aux routes CC et gains LOOP dérivés cohérents ; résultats MIDI de0 à3 paquets, confirmation séparée de chaque callback. CI complète PASS après relance du seul job iOS affecté par des timeouts réseau : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40397913253
- e7aefbce369ddbdcc95547be3c82b3baf8c60e2e : état commun fullState/fullStateForDocument, harness de la vraie classe AU et profil SH-01A SOURCE préparé. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40400931566
- Les commits de tests RED 15a7a243 et fc7fcd65 échouent volontairement avant les implémentations correspondantes. Ne pas les prendre pour des releases.

- 23e75ce puis525bb80e60bfaac8dccfd886fc23e7a22c52abcb : SPOT SMART/MIX physiques, destination capturée, dettes OFF confirmées et fermeture STOP réelle, y compris borne de révision LIVE. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40474469116 ; preuve evidence/v3/CI_525BB80.json. Ce checkpoint SMART/MIX est complété par le raccordement TOUCH/FLASH décrit ci-dessous ; actions encore ouvertes.

- 85b081e puis812b5b3/9e2866911331bf70e1aa439dbc62ef9f415aadf5 : TOUCH/FLASH raccordés, ownership physique, pulse conditionnel et capture après qualification LONG du même événement. CI complète PASS : https://github.com/pacomebommier-dot/xone-auv3-test/actions/runs/40478692311 ; preuve evidence/v3/CI_9E28669.json. Le test AU intra-buffer reste actif ; son ancien échec provenait du callback relu sur une AU désallouée. Aucun lot matériel clôturé.

## État technique des 8 étapes / 16 lots
| Étape | Lots | Raccordements et réserves |
|---|---|---|
| E1 | L01/L02 | L01 clos documentairement ; correctif de traces et CI L02 acquis ; parcours iPhone nominal historique conservé, compléments manuels différés |
| E2 | L03/L04 | Surface et gestes BOT/combos/NAV raccordés ; K2_SIM source préparée, pas de compilation Mozaic ni essai physique nouveau ; SPOT SMART/MIX/TOUCH/FLASH raccordés sur référence LIVE connue, validation AUM/matériel différée |
| E3 | L05/L06 | NAV, sélection physique, pickup/CC/LIVE/S4, LOOP, SPOT SMART/MIX/TOUCH/FLASH et OFF au STOP raccordés ; BAR/BEAT, actions et releases musicales complètes encore ouverts |
| E4 | L07/L08 | PATCH/MEMORY et CUE/LOAD/BASE : modules et contrats conservés, pas encore une chaîne musicale intégrée |
| E5 | L09/L10 | PRESET/RED et propriétaires/alternatives S8 : non raccordés ; les modes SPOT MACHINE ne doivent pas être capturés/restaurés par PATCH/PRESET/RED |
| E6 | L11/L12 | UNKNOWN conservé ; LED MUTE MIDI15 raccordées, feedback authentifié et autres vues LED ouverts |
| E7 | L13/L14 | Persistance du seul dernier LIVE raccordée aux API d'état AU, vérifiée sur une vraie classe AU sous macOS ; autres domaines/NAV/banques et replay vers profil validé ouverts ; table SH-01A source préparée, sans binding/canal/port ; aucune session AUM FX réelle déclarée prête |
| E8 | L15/L16 | AUM_FULL_DEFERRED ; K2_PHYSICAL_PENDING ; SH01A_PHYSICAL_PENDING. JU-06A après SH. Aucun PASS inventé |

## Limites à respecter
La session native connue du harness ne vaut pas une image instrument observée. Une AUv3 neuve sans référence connue reste UNKNOWN ; pas de zéros ou de PC Roland inventés. La réouverture restaure actuellement l'image logicielle, sans prétendre avoir restitué le matériel ni toutes les sorties MIDI appliquées.
Les 12 headers Core, 14 suites historiques, GOLDEN/S7/S8 et preuves V0.1/V0.2 restent préservés. Vérification distante du27/10 : main=5874ae2f29a2543d9df7cda4814803d04dd36257 ; feature/native-l02-v02=8fff66b4ed97acaf1b65f37028700319ed2e935e.

## Reprise pratique
Lire INTEGRATION_V3.md, INTEGRATION_V3_CC.md, INTEGRATION_V3_SESSION.md et INTEGRATION_V3_SELECTION_LOOP.md et INTEGRATION_V3_SPOTS.md et INTEGRATION_V3_TOUCH_FLASH.md avec les documents actifs Drive00/40..44/37 et C-NAT-05/06.
Le checkout local E:/Mon Drive/MOZAIC SCRIPTS/XONE K2/XONE_ENGINE_NATIVE a été resynchronisé proprement le27/10 (fast-forward03be998 ->9fbc48d, checkout initial propre). Exécution locale de nouveau disponible. La branche principale reste préservée.
Prochain raccordement : actions avec propriétaires/dettes OFF et horloge BAR/BEAT réelle, puis banques et propriétaires S8. Ne pas réduire cette suite aux remappings CH1 du banc L02. Les essais AUM restent groupés au jalon L15.

## Preuves du code e7aefbce (checkpoint précédent)
GCC, Clang, Apple Clang, ASan/UBSan, contrôle des12 headers et14 suites historiques,20 suites C++ (14 historiques +6 host), régression C V0.1 (10 cas), contrôles source K2_SIM/SH-01A et harness de la vraie classe AU sous macOS : PASS sur e7aefbce. Le build iphoneos unsigned et l'archivage IPA/logs sont PASS sur ce même SHA. Le harness ne constitue pas une exécution AUM/iOS.
L'artefact porte encore le nom historique V0.2 ; son nom GitHub complet inclut le SHA e7aefbce. Ne pas lui attribuer les essais iPhone de8fff66b, ne pas demander une installation intermédiaire. La recette groupée reste L15.
Les commits543ea961/9316f550 documentent les corrections du harness (buffer nul interdit puis format non standard refusé). Aucun test ignoré pour obtenir le PASS final.

## Preuves du code525bb80 (checkpoint SMART/MIX)
21 suites C++ (14 historiques +7 host) et10 cas C V0.1 PASS sous GCC/Clang/Apple Clang, ASan/UBSan PASS, vrai wrapper AU macOS PASS, build iOS et archivage PASS. Le harness exerce les ON/OFF réels du wrapper pour STOP MIDI et transport host. Aucun essai AUM/iPhone/K2/SH-01A exécuté. La branche locale est synchronisée avec GitHub ; aucun merge main.

## Preuves du dernier code9e28669
22 suites C++ (14 historiques +8 host),10 cas C V0.1, GCC/Clang/Apple Clang, ASan/UBSan : PASS. Vraie classe AU macOS : TOUCH/FLASH, échecs de callback, STOP et qualification LONG à offset intra-buffer PASS. Build iOS unsigned et archivage PASS sur le même SHA. Aucun essai natif Mozaic/AUM/iPhone/K2/SH-01A exécuté ; recette L15 et matériel L16 restent ouverts. Source et tests synchronisés local/GitHub, main et preuve L02 inchangés.
